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

#ifndef NDPPD_INTERFACE_HPP
#define NDPPD_INTERFACE_HPP

#include <string>
#include <list>
#include <vector>
#include <map>

#include <sys/poll.h>
#include <net/ethernet.h>

#include "ndppd.hpp"
#include "address.hpp"
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
    const std::string &name() const;

    const std::shared_ptr<ndppd::packet_socket> &packet_socket();

private:
    static std::map<std::string, std::weak_ptr<interface>> _interfaces;

    NDPPD_SAFE_CONSTRUCTOR(interface)

    interface(const std::string &name);

    std::string _name;

    int _index;

    std::shared_ptr<icmp6_socket> _icmp6_socket;

    std::shared_ptr<ndppd::packet_socket> _packet_socket;
};

NDPPD_NS_END

#endif // NDPPD_INTERFACE_HPP
