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
#pragma once

#include <string>
#include <netinet/ip6.h>
#include <netinet/ether.h>

#include "ndppd.h"

NDPPD_NS_BEGIN

struct in6addr {
    friend cidr_s;

    in6addr() {}
    in6addr(const in6addr &addr);
    in6addr(const std::string &str);
    in6addr(const char *str);
    //in6addr(const in6_addr &addr);

    // Compare _a/_m against a._a.
    bool operator==(const in6addr_s &addr) const;
    bool operator!=(const in6addr_s &addr) const;

    const std::string to_string() const;

    bool parse_string(const std::string& str);

    bool is_unicast() const;
    bool is_multicast() const;
    lladdr get_multicast_lladdr() const;

    operator std::string() const;

    operator const in6_addr &() const;

    // Returns a neighbor solicitation multicast address derived from this
    // address.
    in6addr to_ns_multicast();

    lladdr to_ll_multicast();

private:
    uint32_t _v[4];
};

NDPPD_NS_END
