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
#include <cassert>

#include "ndppd.hpp"
#include "proxy.hpp"
#include "interface.hpp"
#include "session.hpp"
#include "logger.hpp"

NDPPD_NS_BEGIN

std::list<std::weak_ptr<session>> session::_sessions;

static address all_nodes = address("ff02::1");

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

session::session(const std::shared_ptr<proxy> &proxy, const address &src, const address &dst, const address &tgt)
        : _proxy(proxy), _ttl(proxy->timeout()), _src(src), _dst(dst), _tgt(tgt)
{
    logger::debug() << "session::session() [src = << " << src << ", dst =" << dst << ", tgt = " << tgt;
    // FIXME: se->_saddr = address("::") == saddr ? all_nodes : saddr;
}

session::~session()
{
    logger::debug() << "session::~session() this=" << logger::format("%x", this);
}

void session::add_interface(const std::string &ifname)
{
    _sockets.push_back(interface::get_or_create(ifname)->inet6_socket());
}

void session::send_ns()
{
    logger::debug() << "session::send_solicit() (interfaces.size() = " << _interfaces.size() << ")";

    for (auto interface : _interfaces) {
        logger::debug() << " - " << interface->name();

        //interface->(_target);
    }
}

void session::send_na()
{
    _pr->ifa()->write_advert(_saddr, _taddr, _pr->router());
}

void session::handle_na()
{
    _status = VALID;
    _ttl    = _pr->ttl();
    _proxy->write_na(_saddr, _taddr, _pr->router());
    send_na();
}

const address &session::tgt() const
{
    return _target;
}

const address &session::src() const
{
    return _source;
}

const address &session::dst() const
{
    return _destination;
}

session::status_enum session::status() const
{
    return _status;
}

void session::status(session::status_enum  val)
{
    _status = value;
}

NDPPD_NS_END
