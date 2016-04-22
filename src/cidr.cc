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
#include <cstring>
#include <cctype>
#include <netinet/ip6.h>
#include <arpa/inet.h>

#include "ndppd.h"
#include "cidr.h"

NDPPD_NS_BEGIN

cidr::cidr()
{
    _mask[0] = 0xffffffff;
    _mask[1] = 0xffffffff;
    _mask[2] = 0xffffffff;
    _mask[3] = 0xffffffff;
}

cidr::cidr(const std::string &str)
    : cidr(str.c_str())
{
}

cidr::cidr(const char *str)
{
    char buf[INET6_ADDRSTRLEN], *bp = buf;

    while (isspace(*str))
        str++;

    while (isxdigit(*str) || *str == ':')
        *bp++ = *str++;

    *bp = '\0';

    if (inet_pton(AF_INET6, buf, _addr) != 1) {
        throw std::invalid_argument("invalid ipv6 ip6addr");
    }

    while (isspace(*str))
        str++;

    if (!*str) {
        _mask[0] = 0xffffffff;
        _mask[1] = 0xffffffff;
        _mask[2] = 0xffffffff;
        _mask[3] = 0xffffffff;
        return;
    }

    if (*str++ != '/')
        throw std::invalid_argument("invalid cidr");

    char *endptr;
    int prefix = (int)strtol(str, &endptr, 10);
    if (str == endptr || prefix < 0 || prefix > 128)
        throw std::invalid_argument("invalid prefix");

    while (isspace(*endptr))
        endptr++;

    if (*endptr)
        throw std::invalid_argument("invalid cidr");

    this->prefix(prefix);
}

cidr::cidr(const ip6addr &addr, int prefix)
{
    *reinterpret_cast<ip6addr *>(_addr) = addr;
    this->prefix(prefix);
}

void cidr::prefix(int prefix)
{
    for (int i = 0; i < 4; i++) {
        if (prefix >= 32) {
            _mask[i] = 0xffffffff;
            prefix -= 32;
        } else if (prefix > 0) {
            _mask[i] = htonl(((1 << prefix) - 1) << (32 - prefix));
            logger::error() << "err mask " << i << ", " << _mask[i];
            prefix = 0;
        } else {
            _mask[i] = 0;
        }
    }
}

int cidr::prefix() const
{
    int prefix = 0;
    for (int i = 0; i < 4; i++) {
        // See: https://en.wikipedia.org/wiki/Hamming_weight
        uint32_t x = ntohl(_mask[i]);
        x -= (x >> 1) & 0x55555555;
        x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
        x = (x + (x >> 4)) & 0x0f0f0f0f;
        prefix += (x * 0x01010101) >> 24;
    }
    return prefix;
}

bool cidr::contains(const ip6addr &addr) const
{
    return !(((_addr[0] ^ addr._addr[0]) & _mask[0]) |
             ((_addr[1] ^ addr._addr[1]) & _mask[1]) |
             ((_addr[2] ^ addr._addr[2]) & _mask[2]) |
             ((_addr[3] ^ addr._addr[3]) & _mask[3]));
}

const std::string cidr::to_string() const
{
    char buf[INET6_ADDRSTRLEN + 4];

    if (!inet_ntop(AF_INET6,& _addr, buf, INET6_ADDRSTRLEN))
        return "::1";

    int pre = prefix();

    if (pre < 128) {
        sprintf(buf + strlen(buf), "/%d", pre);
    }

    return buf;
}

cidr::operator std::string() const
{
    return to_string();
}

const struct ip6addr &cidr::addr() const
{
    return *reinterpret_cast<const struct ip6addr *>(_addr);
}

const struct ip6addr &cidr::mask() const
{
    return *reinterpret_cast<const struct ip6addr *>(_mask);
}

bool cidr::is_multicast() const
{
    return *(uint8_t *)_addr == 0xff;
}

bool cidr::is_unicast() const
{
    return *(uint8_t *)_addr != 0xff;
}

NDPPD_NS_END
