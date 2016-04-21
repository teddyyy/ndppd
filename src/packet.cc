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

NDPPD_NS_BEGIN

const uint8_t *packet::c_data() const
{
    return _data;
}

uint8_t *packet::data()
{
    return _data;
}

const struct ip6_hdr &packet::c_ip6() const
{
    return *reinterpret_cast<const struct ip6_hdr *>(_data);
}

struct ip6_hdr &packet::ip6()
{
    return *reinterpret_cast<struct ip6_hdr *>(_data);
}

const struct icmp6_hdr &packet::c_icmp6() const
{
    return *reinterpret_cast<const struct icmp6_hdr *>(
        _data + sizeof(struct ip6_hdr));
}

struct icmp6_hdr &packet::icmp6()
{
    return *reinterpret_cast<struct icmp6_hdr *>(
        _data + sizeof(struct ip6_hdr));
}

const struct in6_addr &packet::c_target() const
{
    return *reinterpret_cast<const struct in6_addr *>(
        _data + sizeof(struct ip6_hdr) + sizeof(struct icmp6_hdr));
}

struct in6_addr &packet::target()
{
    return *reinterpret_cast<struct in6_addr *>(
        _data + sizeof(struct ip6_hdr) + sizeof(struct icmp6_hdr));
}

struct nd_opt_hdr *packet::option(int type)
{
    uint8_t *p = _data + sizeof(struct ip6_hdr) +
        sizeof(struct icmp6_hdr) + sizeof(struct in6_addr);

    int size = length() - (int)(p - _data);

    while (1) {
        if (size < sizeof(struct nd_opt_hdr))
            return NULL;

        auto *hdr = reinterpret_cast<struct nd_opt_hdr *>(p);

        if (size < hdr->nd_opt_len * 8)
            return NULL;

        if (hdr->nd_opt_type == type)
            return hdr;

        size -= hdr->nd_opt_len * 8;
        p += hdr->nd_opt_len * 8;
    }
}

int packet::type() const
{
    return c_icmp6().icmp6_type;
}

size_t packet::length() const
{
    return sizeof(struct ip6_hdr) + ::ntohl(c_ip6().ip6_plen);
}

void packet::update_icmp6_checksum()
{
    if (c_ip6().ip6_nxt != IPPROTO_ICMPV6)
        throw new std::invalid_argument("not an ICMP6 packet");

    uint8_t buf[40];

    *reinterpret_cast<struct in6_addr *>(buf) = c_ip6().ip6_src;
    *reinterpret_cast<struct in6_addr *>(&buf[16]) = c_ip6().ip6_dst;
    *reinterpret_cast<uint32_t *>(&buf[32]) = c_ip6().ip6_plen;

    buf[36] = 0;
    buf[37] = 0;
    buf[38] = 0;
    buf[39] = c_ip6().ip6_nxt;

    uint32_t cksum = 0;
    for (int i = 0; i < 20; i++)
        cksum += *reinterpret_cast<uint16_t *>(buf + i * 2);

    icmp6().icmp6_cksum = 0;

    size_t icmp6_length = length() - sizeof(struct ip6_hdr);
    for (int i = 0; i < icmp6_length / 2; i++)
        cksum += *reinterpret_cast<const uint16_t *>(
            c_data() + sizeof(struct ip6_hdr) + i * 2); 

    if (icmp6_length % 2)
        cksum += *(c_data() + length() - 1);

    while (cksum & 0xffff0000)
        cksum = (cksum >> 16) + (cksum & 0xffff);

    icmp6().icmp6_cksum = cksum;
}

packet packet::create_solicit_packet()
{
    packet p;

    auto &ip6 = p.ip6();
    ip6.ip6_flow = htonl(6 << 28);
    ip6.ip6_plen = sizeof(struct icmp6_hdr) + sizeof(struct in6_addr);
    ip6.ip6_hops = 255;
    ip6.ip6_nxt = IPPROTO_ICMPV6;

    auto &icmp6 = p.icmp6();
    icmp6.icmp6_type = ND_NEIGHBOR_SOLICIT;
    icmp6.icmp6_id = htons(1000); // getpid()
    icmp6.icmp6_seq = 0;

    p.target() = {};

    return p;
}


NDPPD_NS_END

