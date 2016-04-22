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

#include "ndppd.h"
#include "in6addr.h"

NDPPD_NS_BEGIN

class iface;

class cidr {
private:
    uint32_t _addr[4];
    uint32_t _mask[4];

    void prefix(int prefix);

public:
    cidr();
    cidr(const std::string &str);
    cidr(const char *str);
    cidr(const in6addr &addr, int prefix = 128);

    const in6addr &addr() const;
    const in6addr &mask() const;

    const std::string to_string() const;

    // Returns true if the CIDR contains the specified in6addr. */
    bool contains(const in6addr &in6addr) const;

    // Returns the prefix length. */
    int prefix() const;

    bool is_unicast() const;

    bool is_multicast() const;

    operator std::string() const;

};

NDPPD_NS_END
