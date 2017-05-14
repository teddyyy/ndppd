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

#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <netinet/icmp6.h>
#include <linux/filter.h>
#include <linux/if_packet.h>

#include <cstddef>
#include <cstring>
#include <system_error>

#include "socket.hpp"
#include "logger.hpp"

NDPPD_NS_BEGIN

std::vector<pollfd> socket::_pollfds;
std::map<int, std::weak_ptr<socket>> socket::_sockets;

void socket::poll_all()
{
    _pollfds.clear();
    _pollfds.reserve(_sockets.size());

    for (auto it = _sockets.begin(); it != _sockets.end(); ) {
        auto socket = it->second.lock();

        if (!socket) {
            _sockets.erase(it++);
            continue;
        }

        it++;

        short events = 0;
        for (auto tuple : socket->_event_handlers)
            events |= std::get<0>(tuple);

        _pollfds.push_back(pollfd {
                .fd      = socket->_fd,
                .events  = events,
                .revents = 0,
        });
    }

    if (_pollfds.empty()) {
        ::sleep(1);
        return;
    }

    if (::poll(&_pollfds[0], (nfds_t)_pollfds.size(), 50))
        throw std::system_error(errno, std::system_category(), "poll() failed");

    for (const auto &pfd : _pollfds) {
        if (!pfd.revents)
            continue;

        auto socket = _sockets[pfd.fd].lock();

        if (!socket)
            continue;

        for (const auto &eh : socket->_event_handlers) {
            if (std::get<0>(eh) & pfd.revents)
                std::get<1>(eh)(socket, std::get<0>(eh) & pfd.revents);
        }
    }
}

socket::socket(int domain, int type, int protocol)
{
    if ((_fd = ::socket(domain, type, protocol)) < 0)
        throw std::system_error(errno, std::system_category(), "Failed to create socket");

    int on = 1;

    if (::ioctl(_fd, FIONBIO, (char *)&on) < 0)
        throw std::system_error(errno, std::system_category(), "Failed to enable non-blocking");
}

socket::~socket()
{
    ::close(_fd);
}

void socket::register_event_handler(int events, event_handler eh)
{
    if (_sockets.find(_fd) == _sockets.end())
        _sockets[_fd] = shared_from_this();

    _event_handlers.push_back(std::make_tuple(events, eh));
}

ssize_t socket::recv(sockaddr *saddr, uint8_t *buf, size_t size) const
{
    iovec iov;
    iov.iov_base = (caddr_t)buf;
    iov.iov_len = size;

    msghdr msg;
    memset(&msg, 0, sizeof(msghdr));
    msg.msg_name = (caddr_t)saddr;
    msg.msg_namelen = sizeof(struct sockaddr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    ssize_t len;
    if ((len = recvmsg(_fd, &msg, 0)) < 0)
        return -1;

    if (len < (ssize_t)sizeof(icmp6_hdr))
        return -1;

    logger::debug() << "socket::read() len=" << len;
    return len;
}

ssize_t socket::send(const cidr &dst, const uint8_t *buf, size_t size) const
{
    sockaddr_in6 addr;
    memset(&addr, 0, sizeof(sockaddr_in6));

    addr.sin6_family = AF_INET6;
    addr.sin6_port   = htons(IPPROTO_ICMPV6); // Needed?
    addr.sin6_addr   = dst.const_addr();

    iovec iov;
    iov.iov_len = size;
    iov.iov_base = (caddr_t)buf;

    struct msghdr hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.msg_name = (caddr_t)&addr;
    hdr.msg_namelen = sizeof(sockaddr_in6);
    hdr.msg_iov = &iov;
    hdr.msg_iovlen = 1;

    logger::debug() << "iface::write() daddr=" << dst.to_string() << ", len=" << size;

    ssize_t len;
    if ((len = sendmsg(_fd, &hdr, 0)) < 0) {
        logger::error() << "iface::write() failed! errno=" << errno;
        return -1;
    }

    return len;
}

hwaddress socket::get_hwaddress(const std::string &ifname) const
{
    ifreq ifr;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname.c_str(), IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';

    if (ioctl(*this, SIOCGIFHWADDR, &ifr) < 0)
        throw std::system_error(errno, std::system_category(), "Failed to get link-layer address");

    //return ifr.ifr_ifru.ifru_hwaddr
}

// ==================================================================================================================//

packet_socket::packet_socket(const std::string &ifname)
        : socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IPV6))
{
    // Bind to interface.

    sockaddr_ll lladdr;
    memset(&lladdr, 0, sizeof(struct sockaddr_ll));
    lladdr.sll_family   = AF_PACKET;
    lladdr.sll_protocol = htons(ETH_P_IPV6);

    if (!(lladdr.sll_ifindex = if_nametoindex(ifname.c_str()))) {
        throw std::system_error(errno, std::system_category(), "Could not determine interface index for " + ifname);
    }

    if (::bind(*this, reinterpret_cast<sockaddr *>(&lladdr), sizeof(sockaddr_ll)) < 0)
        throw std::system_error(errno, std::system_category(), "Could not bind to interface");

    // Set up filter.

    const sock_filter filter[] = {
            // Load the ether_type.
            BPF_STMT(BPF_LD | BPF_H | BPF_ABS,
                    offsetof(ether_header, ether_type)),
            // Bail if it's* not* ETHERTYPE_IPV6.
            BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, ETHERTYPE_IPV6, 0, 5),
            // Load the next header type.
            BPF_STMT(BPF_LD | BPF_B | BPF_ABS,
                    sizeof(ether_header) + offsetof(ip6_hdr, ip6_nxt)),
            // Bail if it's* not* IPPROTO_ICMPV6.
            BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, IPPROTO_ICMPV6, 0, 3),
            // Load the ICMPv6 type.
            BPF_STMT(BPF_LD | BPF_B | BPF_ABS,
                    sizeof(ether_header) + sizeof(ip6_hdr) + offsetof(icmp6_hdr, icmp6_type)),
            // Bail if it's* not* ND_NEIGHBOR_SOLICIT.
            BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, ND_NEIGHBOR_SOLICIT, 0, 1),
            // Keep packet.
            BPF_STMT(BPF_RET | BPF_K, (u_int32_t)-1),
            // Drop packet.
            BPF_STMT(BPF_RET | BPF_K, 0)
    };

    const sock_fprog fprog = {
            8,
            const_cast<sock_filter *>(&filter[0])
    };

    if (::setsockopt(*this, SOL_SOCKET, SO_ATTACH_FILTER, &fprog, sizeof(fprog)) < 0)
        throw std::system_error(errno, std::system_category(), "Failed to set filter");
}

ssize_t packet_socket::recv_ns(cidr &src, cidr &dst, cidr &tgt) const
{
    sockaddr_ll t_saddr;
    uint8_t msg[256];
    ssize_t len;

    // TODO: Validate size.

    if ((len = recv((sockaddr *)&t_saddr, msg, sizeof(msg))) < 0)
        return -1;

    auto ip6h = reinterpret_cast<ip6_hdr *>(msg + ETH_HLEN);
    src = ip6h->ip6_src;
    dst = ip6h->ip6_dst;

    auto ns = reinterpret_cast<nd_neighbor_solicit *>(msg + ETH_HLEN + sizeof(ip6_hdr));
    tgt = ns->nd_ns_target;

    logger::debug() << "iface::read_solicit() saddr=" << src.to_string()
                    << ", daddr=" << dst.to_string() << ", len=" << len;

    return len;
}

// ==================================================================================================================//

icmp6_socket::icmp6_socket(const std::string &ifname)
        : socket(PF_INET6, SOCK_RAW, IPPROTO_ICMPV6)
{
    // Bind to interface.

    ifreq ifr;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname.c_str(), IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';

    if (::setsockopt(*this, SOL_SOCKET, SO_BINDTODEVICE,& ifr, sizeof(ifr)) < 0)
        throw std::system_error(errno, std::system_category(), "Could not bind to interface");

    // Set max hops.

    int hops = 255;

    if (::setsockopt(*this, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &hops, sizeof(hops)) < 0)
        throw std::system_error(errno, std::system_category(), "Failed to set max multicast hops");

    if (::setsockopt(*this, IPPROTO_IPV6, IPV6_UNICAST_HOPS, &hops, sizeof(hops)) < 0)
        throw std::system_error(errno, std::system_category(), "Failed to set max unicast hops");

    // Set up filter.

    icmp6_filter filter;
    ICMP6_FILTER_SETBLOCKALL(&filter);
    ICMP6_FILTER_SETPASS(ND_NEIGHBOR_ADVERT, &filter);

    if (::setsockopt(*this, IPPROTO_ICMPV6, ICMP6_FILTER, &filter, sizeof(filter)) < 0)
        throw std::system_error(errno, std::system_category(), "Failed to set icmp filter");
}

ssize_t icmp6_socket::recv_na(cidr &src, cidr &tgt) const
{
    sockaddr_in6 t_saddr;
    uint8_t msg[256];
    ssize_t len;

    if ((len = recv((sockaddr *)&t_saddr, msg, sizeof(msg))) < 0)
        return -1;

    src = t_saddr.sin6_addr;

    if (((struct icmp6_hdr* )msg)->icmp6_type != ND_NEIGHBOR_ADVERT)
        return -1;

    tgt.addr() = ((nd_neighbor_solicit *)msg)->nd_ns_target;

    logger::debug() << "iface::read_advert() saddr=" << src.to_string()
                    << ", taddr=" << tgt.to_string() << ", len=" << len;

    return len;
}

ssize_t icmp6_socket::send_na(const cidr &dst, const cidr &tgt, bool router) const
{
    uint8_t buf[128];

    memset(buf, 0, sizeof(buf));

    auto opt = reinterpret_cast<nd_opt_hdr *>(&buf[sizeof(nd_neighbor_advert)]);
    opt->nd_opt_type         = ND_OPT_TARGET_LINKADDR;
    opt->nd_opt_len          = 1;

    auto na = reinterpret_cast<nd_neighbor_advert *>(buf);
    na->nd_na_type           = ND_NEIGHBOR_ADVERT;
    na->nd_na_flags_reserved = (uint32_t)((dst.is_multicast() ? 0 : ND_NA_FLAG_SOLICITED) |
                                          (router ? ND_NA_FLAG_ROUTER : 0));
    na->nd_na_target         = tgt.const_addr();

    // TODO: _hwaddress
    //*static_cast<ether_addr *>(buf + sizeof(nd_neighbor_advert) + sizeof(nd_opt_hdr)) = _hwaddress.c_addr();

    logger::debug() << "iface::write_advert() destination=" << dst.to_string()
                    << ", target=" << tgt.to_string();

    return send(dst, buf, sizeof(nd_neighbor_advert) + sizeof(nd_opt_hdr) + sizeof(ether_addr));
}

ssize_t icmp6_socket::send_ns(const cidr &tgt) const
{
    uint8_t buf[128];
    memset(buf, 0, sizeof(buf));

    auto ns = reinterpret_cast<nd_neighbor_solicit *>(buf);
    ns->nd_ns_type   = ND_NEIGHBOR_SOLICIT;
    ns->nd_ns_target = tgt.const_addr();

    auto opt = reinterpret_cast<nd_opt_hdr *>(buf + sizeof(nd_neighbor_solicit));
    opt->nd_opt_type = ND_OPT_SOURCE_LINKADDR;
    opt->nd_opt_len  = 1;

    // TODO: _hwaddress
    //*(ether_addr *)(buf + sizeof(nd_neighbor_solicit) + sizeof(nd_opt_hdr)) = _hwaddress.c_addr();

    // FIXME: Alright, I'm lazy.
    static cidr multicast("ff02::1:ff00:0000");

    cidr dst;

    dst = multicast;

    dst.addr().s6_addr[13] = tgt.const_addr().s6_addr[13];
    dst.addr().s6_addr[14] = tgt.const_addr().s6_addr[14];
    dst.addr().s6_addr[15] = tgt.const_addr().s6_addr[15];

    logger::debug() << "iface::write_solicit() taddr=" << tgt.to_string() << ", daddr=" << dst.to_string();

    return send(dst, buf, sizeof(nd_neighbor_solicit) + sizeof(nd_opt_hdr) + sizeof(ether_addr));
}

NDPPD_NS_END
