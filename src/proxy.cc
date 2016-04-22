// ndppd - NDP Proxy Daemon
// Copyright (C) 2011-2016  Daniel Adolfsson <daniel@priv.nu>
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
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ndppd.h"

#include "proxy.h"
#include "route.h"
#include "iface.h"
#include "rule.h"
#include "session.h"

NDPPD_NS_BEGIN

std::list<std::shared_ptr<proxy> > proxy::_list;

proxy::proxy() :
    _router(true), _ttl(30000), _timeout(500)
{
}

std::shared_ptr<proxy_s> proxy::create(
    const std::shared_ptr<iface_s> &iface)
{
    // http://stackoverflow.com/questions/8147027
    struct make_shared_class : public proxy_s {};
    auto proxy = std::make_shared<make_shared_class>();
    proxy->_iface = iface;
    _list.push_back(proxy);
    return proxy;
}

std::shared_ptr<proxy_s> proxy::create(const std::string &ifname)
{
    return create(iface::open(ifname));
}

void proxy::handle_solicit(const in6addr_s &saddr, const in6addr_s &daddr,
    const in6addr_s &taddr)
{
    logger::debug()
        << "proxy::handle_solicit() saddr=" << saddr.to_string()
        << ", taddr=" << taddr.to_string();

    // Let's check this proxy's list of sessions to see if we can
    // find one with the same target in6addr.

    for (auto sit = _sessions.begin(); sit != _sessions.end(); sit++) {
        if ((*sit)->taddr() == taddr) {
            switch ((*sit)->status()) {
            case session::status_enum::WAITING:
            case session::status_enum::INVALID:
                break;

            case session::status_enum::VALID:
                (*sit)->send_advert();
            }

            return;
        }
    }

    // Since we couldn't find a session that matched, we'll try to find
    // a matching rule instead, and then set up a new session.

    std::shared_ptr<session_s> session;

    for (auto it = _rules.begin(); it != _rules.end(); it++) {
        auto rule = *it;

        logger::debug() << "checking " << rule->cidr() << " against " << taddr;

        if (rule->cidr().contains(taddr)) {
            if (!session) {
                session = session::create(shared_from_this(), saddr, daddr, 
                                          taddr);
            }

            if (rule->is_auto()) {
                std::shared_ptr<route> route = route::find(taddr);

                if (route->ifname() == _iface->name()) {
                    logger::debug()
                        << "skipping route since it's using interface "
                        << route->ifname();
                } else {
                    auto iface = route->iface();

                    if (iface && (iface != rule->iface())) {
                        session->add_iface(iface);
                    }
                }
            } else if (!rule->iface()) {
                // This rule doesn't have an interface, and thus we'll consider
                // it "static" and immediately send the response.
                session->handle_advert();
                return;
            } else {
                session->add_iface((*it)->iface());
            }
        }
    }

    if (session) {
        _sessions.push_back(session);
        session->send_solicit();
    }
}

std::shared_ptr<rule_s> proxy::add_rule(const cidr_s &cidr,
    const std::shared_ptr<iface_s> &iface)
{
    std::shared_ptr<rule_s> rule(rule::create(shared_from_this(), cidr, iface));
    _rules.push_back(rule);
    return rule;
}

std::shared_ptr<rule_s> proxy::add_rule(const cidr_s &cidr, bool auto_)
{
    std::shared_ptr<rule_s> rule(
        rule::create(shared_from_this(), cidr, nullptr, auto_));
    _rules.push_back(rule);
    return rule;
}

void proxy::remove_session(const std::shared_ptr<session_s>& session)
{
    _sessions.remove(session);
}

std::shared_ptr<iface_s> proxy::iface() const
{
    return _iface;
}

bool proxy::router() const
{
    return _router;
}

void proxy::router(bool val)
{
    _router = val;
}

int proxy::ttl() const
{
    return _ttl;
}

void proxy::ttl(int val)
{
    _ttl = (val >= 0) ? val : 30000;
}

int proxy::timeout() const
{
    return _timeout;
}

void proxy::timeout(int val)
{
    _timeout = (val >= 0) ? val : 500;
}

NDPPD_NS_END

