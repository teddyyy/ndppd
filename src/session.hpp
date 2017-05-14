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

#ifndef NDPPD_SESSION_HPP
#define NDPPD_SESSION_HPP

#include <vector>
#include <memory>

#include "ndppd.hpp"

NDPPD_NS_BEGIN

class proxy;
class inet6_socket;

enum class session_status
{
    WAITING, // Waiting for an advert response.
    VALID,   // Valid;
    INVALID  // Invalid;
};

class session
{
public:
    static void update_all(int elapsed_time);

    NDPPD_SAFE_CONSTRUCTOR(session)

    // Destructor.
    ~session();

    void add_interface(const std::shared_ptr<interface> &interface);

    void handle_na();

    void send_na();

    void send_ns();

    void refesh();

    const cidr &tgt() const { return _tgt; }

    const cidr &dst() const { return _dst; }

    const cidr &src() const { return _src; }

    void status(session_status status) { _status = status; }

    session_status status() const { return _status; }

private:
    session(const std::shared_ptr<proxy> &proxy, const cidr &src, const cidr &dst, const cidr &tgt);

    std::weak_ptr<proxy> _proxy;

    cidr _src, _dst, _tgt;

    /*! The remaining time in milliseconds the object will stay in the interface's session array or cache. */
    int _ttl;

    /*! Current session status, */
    session_status _status;

    /*! An list of interfaces this session is monitoring for ND_NEIGHBOR_ADVERT on. */
    std::list<std::shared_ptr<interface>> _interfaces;

};

NDPPD_NS_END

#endif // NDPPD_SESSION_HPP
