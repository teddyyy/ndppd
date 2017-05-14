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

#ifndef NDPPD_SOCKET_HPP
#define NDPPD_SOCKET_HPP

#include <tuple>
#include <map>
#include <memory>
#include <list>
#include <functional>
#include <vector>

#include <poll.h>

#include "ndppd.hpp"
#include "cidr.hpp"
#include "hwaddress.hpp"

NDPPD_NS_BEGIN

//! Base implementation for sockets.
class socket
        : public std::enable_shared_from_this<socket>
{
public:
    static void poll_all();

    typedef std::function<void(const std::shared_ptr<socket> &, int)> event_handler;

    NDPPD_SAFE_CONSTRUCTOR(socket)

    virtual ~socket();

    operator int() const { return _fd; }

    hwaddress get_hwaddress(const std::string &ifname) const;

    //! Receives a message.
    ssize_t recv(sockaddr *addr, uint8_t *buf, size_t size) const;

    //! Sends a message.
    ssize_t send(const cidr &dst, const uint8_t *buf, size_t size) const;

    void register_event_handler(int events, event_handler eh);

protected:
    socket(int domain, int type, int protocol);

private:
    static std::vector<pollfd> _pollfds;
    static std::map<int, std::weak_ptr<socket>> _sockets;
    std::list<std::tuple<int, event_handler>> _event_handlers;

    int _fd;
};

//! Socket for sending and receiving ICMPv6 messages.
class icmp6_socket
        : public socket
{
public:
    NDPPD_SAFE_CONSTRUCTOR(icmp6_socket)

    //! Receive a neighbor advertisement message.
    ssize_t recv_na(cidr &src, cidr &tgt) const;

    //! Send a neighbor advertisement message.
    ssize_t send_na(const cidr &dst, const cidr &tgt, bool router) const;

    //! Send a neighbor solicitation message.
    ssize_t send_ns(const cidr &tgt) const;

private:
    explicit icmp6_socket(const std::string &ifname);
};

//! Socket for receiving neighbor solicitation messages.
class packet_socket
        : public socket
{
public:
    NDPPD_SAFE_CONSTRUCTOR(packet_socket)

    //! Receive neighbor solicitation message.
    ssize_t recv_ns(cidr &src, cidr &dst, cidr &tgt) const;

private:
    explicit packet_socket(const std::string &ifname);
};

NDPPD_NS_END

#endif // NDPPD_SOCKET_HPP
