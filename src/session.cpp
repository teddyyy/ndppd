// ndppd - NDP Proxy Daemon
// Copyright (C) 2011  Daniel Adolfsson <daniel@priv.nu>
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

#include <algorithm>

#include "ndppd.hpp"
#include "proxy.hpp"
#include "interface.hpp"
#include "session.hpp"
#include "logger.hpp"

NDPPD_NS_BEGIN

static cidr all_nodes = cidr("ff02::1");

#ifdef JUNK
void session::update_all(int elapsed_time)
{
    for (auto it = _sessions.begin(); it != _sessions.end(); ) {
        auto session = (*it).lock();

        if (!session) {
            _sessions.erase(it++);
            continue;
        }

        auto proxy = session->_proxy.lock();

        assert((bool) proxy);

        it++;

        if ((session->_ttl -= elapsed_time) >= 0) {
            continue;
        }

        switch (session->_status) {
        case session_status ::WAITING:
            logger::debug() << "session is now invalid";
            session->_status = session_status ::INVALID;
            session->_ttl    = proxy->ttl();
            break;

        default:
            proxy->remove_session(session);
        }
    }
}
#endif

session::session(const std::shared_ptr<proxy> &proxy, const cidr &src, const cidr &dst, const cidr &tgt)
        : _proxy(proxy), _ttl(proxy->timeout()), _src(src), _dst(dst), _tgt(tgt), _status(session_status::WAITING)
{
    logger::debug() << "session::session() [src = << " << src << ", dst =" << dst << ", tgt = " << tgt;
    // FIXME: se->_saddr = address("::") == saddr ? all_nodes : saddr;
}

session::~session()
{
    logger::debug() << "session::~session() this=" << logger::format("%x", this);
}

void session::add_interface(const std::shared_ptr<interface> &interface)
{
    _interfaces.push_back(interface);
}

void session::send_ns()
{
    logger::debug() << "session::send_ns() (interfaces.size() = " << _interfaces.size() << ")";

    for (auto interface : _interfaces) {
        logger::debug() << " - " << interface->name();
        interface->socket()->send_ns(_tgt);
    }
}

void session::send_na()
{
    auto proxy = _proxy.lock();
    if (!proxy) return;

    proxy->interface()->socket()->send_na(_src, _tgt, proxy->router());
}

void session::handle_na()
{
    auto proxy = _proxy.lock();
    if (!proxy) return;

    _status = session_status::VALID;
    _ttl    = proxy->ttl();
    proxy->interface()->socket()->send_na(_src, _tgt, proxy->router());
    send_na();
}

NDPPD_NS_END
