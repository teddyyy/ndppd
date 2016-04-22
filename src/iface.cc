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
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstddef>

#include <unistd.h>

#include <netinet/in.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <netinet/ether.h>
#include <netpacket/packet.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>

#include <linux/filter.h>

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>

#include "ndppd.h"
#include "iface.h"
#include "lladdr.h"

NDPPD_NS_BEGIN

int iface::_fd;
std::vector<std::weak_ptr<iface> > iface::_ifaces;
std::vector<pollfd> iface::_pollfds;

iface::iface()
{
}

iface::~iface()
{
    logger::debug() << "iface::~iface()";
    // TODO: Remove ALLMULTI membership.
}

void iface::create_socket()
{
    // Create a socket.
    if ((_fd = socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_IPV6))) < 0)
        throw_system_error("could not create socket");

    // Add a filter.
    static sock_filter filter[] = {
        // Load the IPv6 protocol type.
        BPF_STMT(BPF_LD | BPF_B | BPF_ABS, offsetof(ip6_hdr, ip6_nxt)),

        // Drop packet if ip6_nxt is not IPPROTO_ICMPV6.
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, IPPROTO_ICMPV6, 0, 3),

        // Load the ICMPv6 type.
        BPF_STMT(BPF_LD | BPF_B | BPF_ABS,
            sizeof(ip6_hdr) + offsetof(icmp6_hdr, icmp6_type)),

        // Keep packet if icmp6_type is ND_NEIGHBOR_ADVERT.
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, ND_NEIGHBOR_ADVERT, 2, 0),

        // Keep packet if icmp6_type is ND_NEIGHBOR_SOLICIT.
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, ND_NEIGHBOR_SOLICIT, 1, 0),

        // Drop packet.
        BPF_STMT(BPF_RET | BPF_K, 0),

        // Keep packet.
        BPF_STMT(BPF_RET | BPF_K, (u_int32_t)-1)
    };

    static sock_fprog fprog = {
        7,
        filter
    };

    if (setsockopt(_fd, SOL_SOCKET, SO_ATTACH_FILTER, &fprog,
            sizeof(fprog)) < 0)
        throw_system_error("failed to set up filter");

    // Switch to non-blocking I/O.
    int on = 1;
    if (ioctl(_fd, FIONBIO, (char *)&on) < 0)
        throw_system_error("failed to set non-blocking mode");
}

std::shared_ptr<iface> iface::open(const std::string &name)
{
    // Find an interface with the specified name.
    auto it = std::find_if(_ifaces.begin(), _ifaces.end(),
        [&name](std::weak_ptr<iface> &p) {
            return !p.expired() && p.lock()->_name == name;
        });

    if (it != _ifaces.end())
        // This should never return nullptr.
        return it->lock();

    int index;

    // Get the index of the interface.
    if (!(index = if_nametoindex(name.c_str())))
        throw_system_error("invalid interface");

    // Detect the link-layer address.
    struct ifreq ifr;
    strcpy(ifr.ifr_name, name.c_str());
    if (ioctl(_fd, SIOCGIFHWADDR, &ifr) < 0)
        throw_system_error("failed to determine link-layer in6addr");

    // TODO: Add ALLMULTI membership.

    // http://stackoverflow.com/questions/8147026
    struct make_shared_class : public iface {};
    std::shared_ptr<iface> iface(std::make_shared<make_shared_class>());
    iface->_name  = name;
    iface->_index = index;

    return iface;
}

bool iface::read(in6addr_s &saddr, packet_s &packet)
{
    sockaddr_ll from;

    ssize_t len;
    socklen_t from_size = sizeof(sockaddr_ll);
    if ((len = recvfrom(_fd, &packet, sizeof(packet_s), 0,
            reinterpret_cast<sockaddr *>(&from), &from_size)) < 0)
        throw_system_error("recvfrom() failed");

    logger::debug() << "iface::read() len=" << len;

    if (len < sizeof(ip6_hdr))
        return false;

    return true;
}

bool iface::write(const packet_s &packet, const lladdr_s &addr)
{
    ssize_t len;

    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(sll));
    sll.sll_family = AF_PACKET;

    memset(&sll.sll_addr, 0xff, ETH_ALEN);
    if (packet.c_daddr().is_multicast()) {
        auto lladdr = packet.c_daddr().get_multicast_lladdr();
        memcpy(&sll.sll_addr, &lladdr, 6);
    }

    //sll.sll_addr
    sll.sll_halen = ETH_ALEN;
    sll.sll_protocol = htons(ETH_P_IPV6);
    sll.sll_ifindex = _index;

    //if (!(sll.sll_ifindex = if_nametoindex(_name.c_str())))
    //    throw_system_error("invalid interface");

    if ((len = ::sendto(_fd, &packet, packet.length(), 0,
            (struct sockaddr *)&sll, sizeof(sll))) < 0)
        throw_system_error("sendmsg() failed");
    //if ((len = ::sendmsg(_fd, &mhdr, 0)) < 0)
    //    throw_system_error("sendmsg() failed");

    return true;
}

void iface::add_session(const std::shared_ptr<session> &session)
{
    _sessions.push_back(session);
}

void iface::cleanup()
{
    logger::debug() << "iface::fixup() _ifaces.size()=" << _ifaces.size();

    for (auto it = _ifaces.begin(); it != _ifaces.end(); ) {
        if (it->expired())
            it = _ifaces.erase(it);
        else
            it++;
    }
}

bool iface::poll()
{
    pollfd pfd;
    pfd.fd = _fd;
    pfd.events = POLLIN;

    int ret = ::poll(&pfd, 1, 100);
    if (ret < 0)
        throw_system_error("poll() failed");

    if (!ret)
        return false;

    packet_s packet;
    in6addr_s addr;
    if (!read(addr, packet))
        return false;

    return 0;
}

const std::string &iface::name() const
{
    return _name;
}

void iface::proxy(const std::shared_ptr<ndppd::proxy> &proxy)
{
    _proxy = proxy;
}

std::shared_ptr<ndppd::proxy> iface::proxy() const
{
    return _proxy.lock();
}

NDPPD_NS_END
