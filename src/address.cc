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
#include <string>
#include <vector>
#include <map>

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cctype>

#include <netinet/ip6.h>
#include <arpa/inet.h>

#include "address.h"

NDPPD_NS_BEGIN

address::address(const std::string &str)
    : address(str.c_str())
{
}

address::address(const char *str)
{
    if (inet_pton(AF_INET6, str, this) <= 0)
        throw new std::invalid_argument("invalid ip6 address");
}

address::address(const in6_addr& addr)
{
    _addr[0] = addr.s6_addr32[0];
    _addr[1] = addr.s6_addr32[1];
    _addr[2] = addr.s6_addr32[2];
    _addr[3] = addr.s6_addr32[3];
}

bool address::operator==(const address_s &address) const
{
    return
        address._addr[0] == _addr[0] && address._addr[1] == _addr[1] &&
        address._addr[2] == _addr[2] && address._addr[3] == _addr[3];
}

bool address::operator!=(const address_s &address) const
{
    return !(*this == address);
}

const std::string address::to_string() const
{
    char buf[INET6_ADDRSTRLEN];

    if (!inet_ntop(AF_INET6,& _addr, buf, INET6_ADDRSTRLEN))
        return "::1";

    return buf;
}

address::operator std::string() const
{
    return to_string();
}

struct in6_addr &address::addr()
{
    return *reinterpret_cast<struct in6_addr *>(this);
}

const struct in6_addr& address::c_addr() const
{
    return *reinterpret_cast<const struct in6_addr *>(this);
}

bool address::is_multicast() const
{
    return *reinterpret_cast<const uint8_t *>(this) == 0xff;
}

bool address::is_unicast() const
{
    return *reinterpret_cast<const uint8_t *>(this) != 0xff;
}

/*address::operator const struct in6_addr &() const
{
    return c_addr();
}*/

NDPPD_NS_END
