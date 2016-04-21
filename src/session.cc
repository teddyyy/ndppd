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
#include <memory>

#include "ndppd.h"
#include "proxy.h"
#include "iface.h"
#include "session.h"

NDPPD_NS_BEGIN

std::list<std::weak_ptr<session> > session::_sessions;

void session::update_all(int elapsed_time)
{
    for (auto it = _sessions.begin(); it != _sessions.end(); ) {
        if (it->expired()) {
            _sessions.erase(it++);
            continue;
        }

        auto session = (it++)->lock();

        if ((session->_ttl -= elapsed_time) >= 0) {
            continue;
        }

        switch (session->_status) {
        case session::status_enum::WAITING:
            logger::debug() << "session is now invalid";
            session->_status = session::status_enum::INVALID;
            // TODO: _proxy could be expired()
            session->_ttl    = session->_proxy.lock()->ttl();
            break;

        default:
            session->_proxy.lock()->remove_session(session);
        }
    }
}

session::~session()
{
    logger::debug()
        << "session::~session() this=" << logger::format("%x", this);

    /*for (auto it = _ifaces.begin(); it != _ifaces.end(); it++)
        (*it)->remove_session(shared_from_this());*/
    
}

std::shared_ptr<session_s> session::create(
    const std::shared_ptr<proxy_s> &proxy, const address_s &saddr,
    const address_s &daddr, const address_s &taddr)
{
    // http://stackoverflow.com/questions/8147027
    struct make_shared_class : public session_s {};
    auto session = std::make_shared<make_shared_class>();
    session->_proxy = proxy;
    session->_saddr = saddr;
    session->_taddr = taddr;
    session->_daddr = daddr;
    session->_ttl   = proxy->timeout();
    _sessions.push_back(session);

    logger::debug()
        << "session::create() proxy=xx " 
        << ", saddr=" << saddr << ", daddr=" << daddr << ", taddr=" << taddr
        << " =" << logger::format("%x", session.get());

    return session;
}

void session::add_iface(const std::shared_ptr<iface_s> &iface)
{
    if (std::find(_ifaces.begin(), _ifaces.end(), iface) != _ifaces.end())
        return;

    iface->add_session(shared_from_this());
    _ifaces.push_back(iface);
}

void session::send_solicit()
{
    logger::debug()
        << "session::send_solicit() (_ifaces.size() = "
        << _ifaces.size() << ")";

    for (auto it = _ifaces.begin(); it != _ifaces.end(); it++) {
        logger::debug() << " - " << (*it)->name();
        //(*it)->write_solicit(_taddr);
    }
}

void session::send_advert()
{
    //_proxy->iface()->write_advert(_saddr, _taddr, _proxy->router());
}

void session::handle_advert()
{
    _status = status_enum::VALID;
    _ttl    = _proxy.lock()->ttl();

    send_advert();
}

const address& session::taddr() const
{
    return _taddr;
}

const address& session::saddr() const
{
    return _saddr;
}

const address& session::daddr() const
{
    return _daddr;
}

session::status_enum session::status() const
{
    return _status;
}

void session::status(session::status_enum val)
{
    _status = val;
}

NDPPD_NS_END
