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

    NDPPD_SAFE_CONSTRUCTOR(session, _sessions.push_back(this_ptr))

    // Destructor.
    ~session();

    void add_iface(const std::string &ifname);

    const address &tgt() const;

    const address &dst() const;

    const address &src() const;

    session_status status() const;

    void status(session_status value);

    void handle_na();

    void send_na();

    void send_ns();

    void refesh();

private:
    static std::list<std::weak_ptr<session>> _sessions;

    session(const std::shared_ptr<proxy> &proxy, const address &src, const address &dst, const address &tgt);

    std::weak_ptr<ndppd::proxy> _proxy;

    address _src, _dst, _tgt;

    /*! The remaining time in milliseconds the object will stay in the interface's session array or cache. */
    int _ttl;

    /*! Current session status, */
    session_status _status;

    /*! An list of sockets this session is monitoring for ND_NEIGHBOR_ADVERT on. */
    std::list<std::shared_ptr<ine6_socket>> _sockets;

};

NDPPD_NS_END

#endif
