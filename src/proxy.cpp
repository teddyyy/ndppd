// ndppd - NDP Proxy Daemon
// Copyright (C) 2011-2017  Daniel Adolfsson <daniel@priv.nu>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "ndppd.hpp"
#include "proxy.hpp"
#include "route.h"
#include "interface.hpp"
#include "rule.h"
#include "session.hpp"
#include "logger.hpp"
#include "socket.hpp"

NDPPD_NS_BEGIN

std::shared_ptr<proxy> proxy::get_or_create(const std::string &name)
{
    auto it = _proxies.find(name);

    if (it != _proxies.end())
        return it->second;

    return _proxies[name] = create(name);
}

proxy::proxy(const std::string &ifname)
        : _router(true), _ttl(30000), _timeout(500),
          _interface(interface::get_or_create(ifname))
{
    logger::debug() << "proxy::proxy() ifname=" << _interface->name();
    _interface->packet_socket()->register_event_handler(
            POLLIN,
            [](const std::shared_ptr<socket> &socket, int events) -> {
                address src, dst, tgt;
                static_pointer_cast<packet_socket>(socket)->read_ns(src, dst, tgt);
                handle_solicit(src, dst, tgt);
            });
}

void proxy::handle_solicit(const address &src, const address &dst, const address &tgt)
{
    logger::debug() << "proxy::handle_solicit() source=" << src.to_string()
                    << ", target=" << tgt.to_string();

    // Let's check this proxy's list of sessions to see if we can find one with the same target address.

    for (auto it = _sessions.begin(); it != _sessions.end(); it++) {
        if ((*it)->tgt() == tgt) {
            switch ((*it)->status()) {
            case session_status::WAITING:
            case session_status::INVALID:
                break;

            case session_status::VALID:
                (*it)->send_na();
                break;
            }

            return;
        }
    }

    // Since we couldn't find a session that matched, we'll try to find a matching rule instead,
    // and then set up a new session.

    std::shared_ptr<session> session;

    for (auto rule : _rules) {
        logger::debug() << "checking " << rule->address() << " against " << tgt;

        if (!dst.is_multicast() && rule->address() != dst)
            continue;

        if (rule->address() == tgt) {
            if (!session) {
                session = session::create(_ptr, saddr, daddr, taddr);
            }

#ifdef ASDF
            if (rule->is_auto()) {


                ptr<route> rt = route::find(taddr);

                if (rt->ifname() == _ifa->name()) {
                    logger::debug() << "skipping route since it's using interface " << rt->ifname();
                } else {
                    ptr<iface> ifa = rt->ifa();

                    if (ifa && (ifa != rule->ifa())) {
                        se->add_iface(ifa);
                    }
                }
            } else if (!rule->interface()) {
                // This rule doesn't have an interface, and thus we'll consider
                // it "static" and immediately send the response.
                se->handle_advert();
                return;
            } else {
                se->add_iface((*it)->ifa());
                #ifdef WITH_ND_NETLINK
                if (if_addr_find((*it)->ifa()->name(), &taddr.const_addr())) {
                    logger::debug() << "Sending NA out " << (*it)->ifa()->name();
                    se->add_iface(_ifa);
                    se->handle_advert();
                }
                #endif
            }
#endif
        }
    }

    if (se) {
        _sessions.push_back(se);
        se->send_solicit();
    }
}

NDPPD_NS_END

