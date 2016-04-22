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

#include <netinet/ip6.h>
#include <netinet/icmp6.h>

#include "ndppd.h"

NDPPD_NS_BEGIN

class packet {
    uint8_t _data[128];

    void ip_checksum_add(uint32_t &current, uint32_t value);
    void ip_checksum_add(uint32_t &current, const void *data, size_t len);

    // A number of helper methods to access various parts of the packet.
    uint8_t *data();
    const uint8_t *c_data() const;
    struct ip6_hdr &ip6();
    const struct ip6_hdr &c_ip6() const;
    struct icmp6_hdr &icmp6();
    const struct icmp6_hdr &c_icmp6() const;
    struct in6_addr &target();
    const struct in6_addr &c_target() const;
    struct nd_opt_hdr *option(int type);
    void update_icmp6_checksum();

public:
    void make_solicit_packet();

    const ip6addr_s &c_daddr() const;

    int type() const;
    size_t length() const;


};

NDPPD_NS_END
