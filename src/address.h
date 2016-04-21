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

NDPPD_NS_BEGIN

struct address {
    address() {}
    address(const address &addr);
    address(const std::string &str);
    address(const char *str);
    address(const in6_addr &addr);

    const struct in6_addr &c_addr() const;
    struct in6_addr &addr();

    // Compare _a/_m against a._a.
    bool operator==(const address_s& addr) const;
    bool operator!=(const address_s& addr) const;

    const std::string to_string() const;

    bool parse_string(const std::string& str);

    bool is_unicast() const;

    bool is_multicast() const;

    operator std::string() const;

private:
    uint32_t _addr[4];
};

NDPPD_NS_END
