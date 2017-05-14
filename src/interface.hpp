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

#ifndef NDPPD_INTERFACE_HPP
#define NDPPD_INTERFACE_HPP

#include <string>
#include <list>
#include <vector>
#include <map>

#include <sys/poll.h>
#include <net/ethernet.h>

#include "ndppd.hpp"
#include "cidr.hpp"
#include "hwaddress.hpp"
#include "socket.hpp"

NDPPD_NS_BEGIN

class inet6_socket;
class packet_socket;
class session;
class proxy;

class interface
{
public:
    static std::shared_ptr<interface> get_or_create(const std::string &name);

    //! Destructor.
    ~interface();

    //! Returns the name of the interface.
    const std::string &name() const { return _name; }

    int index() const { return _index; }

    const std::shared_ptr<icmp6_socket> &socket() const { return _socket; };

private:
    static std::map<std::string, std::weak_ptr<interface>> _interfaces;

    NDPPD_SAFE_CONSTRUCTOR(interface)

    interface(const std::string &name);

    std::string _name;

    int _index;

    std::shared_ptr<icmp6_socket> _socket;
};

NDPPD_NS_END

#endif // NDPPD_INTERFACE_HPP
