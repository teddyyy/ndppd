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

#include "ip6addr.h"
#include "lladdr.h"

NDPPD_NS_BEGIN

ip6addr::ip6addr(const std::string &str)
    : ip6addr(str.c_str())
{
}

ip6addr::ip6addr(const char *str)
{
    if (inet_pton(AF_INET6, str, _addr) <= 0)
        throw new std::invalid_argument("invalid ip6 ip6addr");
}

ip6addr::ip6addr(const in6_addr &addr)
{
    _addr[0] = addr.s6_addr32[0];
    _addr[1] = addr.s6_addr32[1];
    _addr[2] = addr.s6_addr32[2];
    _addr[3] = addr.s6_addr32[3];
}

bool ip6addr::operator==(const ip6addr_s &addr) const
{
    return
        addr._addr[0] == _addr[0] && addr._addr[1] == _addr[1] &&
        addr._addr[2] == _addr[2] && addr._addr[3] == _addr[3];
}

bool ip6addr::operator!=(const ip6addr_s &ip6addr) const
{
    return !(*this == ip6addr);
}

const std::string ip6addr::to_string() const
{
    char buf[INET6_ADDRSTRLEN];

    if (!inet_ntop(AF_INET6, &_addr, buf, INET6_ADDRSTRLEN))
        return "::1";

    return buf;
}

ip6addr::operator std::string() const
{
    return to_string();
}

bool ip6addr::is_multicast() const
{
    return *reinterpret_cast<const uint8_t *>(this) == 0xff;
}

bool ip6addr::is_unicast() const
{
    return *reinterpret_cast<const uint8_t *>(this) != 0xff;
}

lladdr ip6addr::get_multicast_lladdr() const
{
    auto *p = reinterpret_cast<const uint8_t *>(&_addr[3]);
    uint8_t addr[] = { 0x33, 0x33, p[0], p[1], p[2], p[3] };
    return *reinterpret_cast<lladdr *>(addr);
}

ip6addr::operator const in6_addr &() const
{
    return *reinterpret_cast<const in6_addr *>(_addr);
}

/*ip6addr::operator const struct in6_addr &() const
{
    return c_addr();
}*/

NDPPD_NS_END
