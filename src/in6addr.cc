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

#include "in6addr.h"
#include "lladdr.h"

NDPPD_NS_BEGIN

lladdr in6addr::to_ll_multicast()
{
}

in6addr in6addr::to_ns_multicast()
{
    in6addr addr;
    addr._v[0] = htonl(0xff020000);
    addr._v[1] = htonl(0x00000000);
    addr._v[2] = htonl(0x00000001);
    addr._v[3] = htonl(0xff000000) | _v[3];
    return addr;
}

in6addr::in6addr(const std::string &str)
    : in6addr(str.c_str())
{
}

in6addr::in6addr(const char *str)
{
    if (inet_pton(AF_INET6, str, _v) <= 0)
        throw new std::invalid_argument("invalid ip6 in6addr");
}

bool in6addr::operator==(const in6addr_s &addr) const
{
    return
        addr._v[0] == _v[0] && addr._v[1] == _v[1] &&
        addr._v[2] == _v[2] && addr._v[3] == _v[3];
}

bool in6addr::operator!=(const in6addr_s &in6addr) const
{
    return !(*this == in6addr);
}

const std::string in6addr::to_string() const
{
    char buf[INET6_ADDRSTRLEN];

    if (!inet_ntop(AF_INET6, &_v, buf, INET6_ADDRSTRLEN))
        return "::1";

    return buf;
}

in6addr::operator std::string() const
{
    return to_string();
}

bool in6addr::is_multicast() const
{
    return *reinterpret_cast<const uint8_t *>(_v) == 0xff;
}

bool in6addr::is_unicast() const
{
    return *reinterpret_cast<const uint8_t *>(_v) != 0xff;
}

lladdr in6addr::get_multicast_lladdr() const
{
    auto *p = reinterpret_cast<const uint8_t *>(&_v[3]);
    uint8_t addr[] = { 0x33, 0x33, p[0], p[1], p[2], p[3] };
    return *reinterpret_cast<lladdr *>(addr);
}

in6addr::operator const in6_addr &() const
{
    *reinterpret_cast<const in6_addr *>(_v);
}

/*in6addr::operator const struct in6_addr &() const
{
    return c_addr();
}*/

NDPPD_NS_END
