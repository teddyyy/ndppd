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

#ifndef NDPPD_ADDRESS_H
#define NDPPD_ADDRESS_H

#include <string>
#include <netinet/ip6.h>

#include "ndppd.hpp"

NDPPD_NS_BEGIN

//! Class for working with IPv6 addresses.
class address
{
public:
    //! Default constructor.
    address();

    //! Copy constructor.
    address(const address &address);

    address(const std::string &str);
    address(const char *str);
    address(const in6_addr &address);
    address(const in6_addr &address, const in6_addr &addr);
    address(const in6_addr &address, int prefix);

    // Compare _a/_m against a._a.
    bool operator==(const address &address) const;

    bool operator!=(const address &address) const;

    bool in(const address &address) const;

    void reset();

    const std::string to_string() const;

    bool parse_string(const std::string &str);

    int prefix() const;

    void prefix(int value);

    operator std::string() const;

    bool is_unicast() const { return _addr.s6_addr[0] != 0xff; }

    bool is_multicast() const { return _addr.s6_addr[0] == 0xff; }

    struct in6_addr &addr() { return _addr; }

    const struct in6_addr &const_addr() const { return _addr; }

    struct in6_addr &mask() { return _mask; }

    const struct in6_addr &const_mask() const { return _mask; }

private:
    struct in6_addr _addr, _mask;
};

NDPPD_NS_END

#endif // NDPPD_ADDRESS_H
