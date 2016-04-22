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
#include <netinet/icmp6.h>
#include <netinet/ip6.h>

#include "ndppd.h"
#include "packet.h"
#include "ip6addr.h"

NDPPD_NS_BEGIN

const uint8_t *packet::c_data() const
{
    return _data;
}

uint8_t *packet::data()
{
    return _data;
}

const ip6_hdr &packet::c_ip6() const
{
    return *reinterpret_cast<const ip6_hdr *>(_data);
}

struct ip6_hdr &packet::ip6()
{
    return *reinterpret_cast<ip6_hdr *>(_data);
}

const icmp6_hdr &packet::c_icmp6() const
{
    return *reinterpret_cast<const icmp6_hdr *>(_data + sizeof(ip6_hdr));
}

struct icmp6_hdr &packet::icmp6()
{
    return *reinterpret_cast<icmp6_hdr *>(_data + sizeof(ip6_hdr));
}

const in6_addr &packet::c_target() const
{
    return *reinterpret_cast<const in6_addr *>(
        _data + sizeof(ip6_hdr) + sizeof(icmp6_hdr));
}

in6_addr &packet::target()
{
    return *reinterpret_cast<in6_addr *>(
        _data + sizeof(ip6_hdr) + sizeof(icmp6_hdr));
}

nd_opt_hdr *packet::option(int type)
{
    uint8_t *p = _data + sizeof(ip6_hdr) + sizeof(icmp6_hdr) + sizeof(in6_addr);

    int size = length() - (int)(p - _data);

    while (1) {
        if (size < sizeof(nd_opt_hdr))
            return NULL;

        auto *hdr = reinterpret_cast<nd_opt_hdr *>(p);

        if (size < hdr->nd_opt_len * 8)
            return NULL;

        if (hdr->nd_opt_type == type)
            return hdr;

        size -= hdr->nd_opt_len * 8;
        p += hdr->nd_opt_len * 8;
    }
}

const ip6addr_s &packet::c_daddr() const
{
    return *reinterpret_cast<const ip6addr_s *>(&c_ip6().ip6_dst);
}

int packet::type() const
{
    return c_icmp6().icmp6_type;
}

size_t packet::length() const
{
    return sizeof(ip6_hdr) + ntohs(c_ip6().ip6_plen);
}
 
void packet::update_icmp6_checksum()
{
    if (c_ip6().ip6_nxt != IPPROTO_ICMPV6)
        throw new std::invalid_argument("not an ICMP6 packet");

    icmp6().icmp6_cksum = 0;

    uint32_t cksum = 0;
    ip_checksum_add(cksum, &c_ip6().ip6_src, sizeof(in6_addr));
    ip_checksum_add(cksum, &c_ip6().ip6_dst, sizeof(in6_addr));
    ip_checksum_add(cksum, (uint32_t)ntohs(c_ip6().ip6_plen));
    cksum += c_ip6().ip6_nxt;

    ip_checksum_add(cksum, &c_icmp6(), ntohs(c_ip6().ip6_plen));

    cksum += cksum >> 16;
    icmp6().icmp6_cksum = htons(~cksum);
}

void packet::ip_checksum_add(uint32_t &current, const void *data, size_t len)
{
    while (len > 1) {
        current += ntohs(*reinterpret_cast<const uint16_t *>(data));
        data = reinterpret_cast<const uint16_t *>(data) + 1;
        len -= 2;
    }

    if (len)
        current += *reinterpret_cast<const uint8_t *>(data);
}

void packet::ip_checksum_add(uint32_t &current, uint32_t value)
{
    value = ntohl(value);
    ip_checksum_add(current, &value, sizeof(uint32_t));
}

void packet::make_solicit_packet()
{
    auto &ip6 = this->ip6();
    ip6.ip6_flow = htonl(6 << 28);
    ip6.ip6_plen = htons(sizeof(icmp6_hdr));
    ip6.ip6_hops = 255;
    ip6.ip6_nxt = IPPROTO_ICMPV6;
    ip6.ip6_src = ip6addr_s();
    ip6.ip6_dst = ip6addr_s("ff02::1:ff00:0000");

    auto &icmp6 = this->icmp6();
    icmp6.icmp6_type = ND_NEIGHBOR_SOLICIT;
    icmp6.icmp6_id = htons(1000); // getpid()
    icmp6.icmp6_seq = 0;

    update_icmp6_checksum();

    target() = {};
}


NDPPD_NS_END

