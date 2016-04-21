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

#include "ndppd.h"
#include "iface.h"

NDPPD_NS_BEGIN

std::map<std::string, std::weak_ptr<iface> > iface::_map;
std::vector<struct pollfd> iface::_pollfds;

iface::iface() :
    _fd(-1)
{
    logger::debug() << "iface::iface()";
}

iface::~iface()
{
    logger::debug() << "iface::~iface()";
    // TODO: Restore ALLMULTI flag.
    if (_fd >= 0)
        close(_fd);
}

std::shared_ptr<iface> iface::open(const std::string& name)
{
    auto it = _map.find(name);
    if (it != _map.end() && !it->second.expired())
        return it->second.lock();

    // http://stackoverflow.com/questions/8147027
    struct make_shared_class : public iface {};
    std::shared_ptr<iface> iface(std::make_shared<make_shared_class>());

    // Create a socket.
    if ((iface->_fd = socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_IPV6))) < 0)
        throw_system_error("could not create socket");

    // Bind to the specified interface.
    {
        struct sockaddr_ll lladdr;

        memset(&lladdr, 0, sizeof(struct sockaddr_ll));
        lladdr.sll_family   = AF_PACKET;
        lladdr.sll_protocol = htons(ETH_P_IPV6);

        if (!(lladdr.sll_ifindex = if_nametoindex(name.c_str())))
            throw_system_error("invalid interface");

        if (bind(iface->_fd, (struct sockaddr *)&lladdr,
                sizeof(struct sockaddr_ll)) < 0)
            throw_system_error("failed to bind to interface");
    }

    // Detect the link-layer address.
    {
        struct ifreq ifr;
        strcpy(ifr.ifr_name, name.c_str());
        if (ioctl(iface->_fd, SIOCGIFHWADDR, &ifr) < 0)
            throw_system_error("failed to determine link-layer address");
    }

    // Switch to non-blocking mode.
    {
        int on = 1;

        if (ioctl(iface->_fd, FIONBIO, (char *)&on) < 0)
            throw_system_error("failed to set non-blocking mode");
    }

    // Set up the filter.
    {
        static struct sock_filter filter[] = {
            // Load the IPv6 protocol type.
            BPF_STMT(BPF_LD | BPF_B | BPF_ABS,
                offsetof(struct ip6_hdr, ip6_nxt)),

            // Drop packet if ip6_nxt is not IPPROTO_ICMPV6.
            BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, IPPROTO_ICMPV6, 0, 4),

            // Load the ICMPv6 type.
            BPF_STMT(BPF_LD | BPF_B | BPF_ABS,
                sizeof(struct ip6_hdr) +
                offsetof(struct icmp6_hdr, icmp6_type)),

            // Keep packet if icmp6_type is ND_NEIGHBOR_ADVERT.
            BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, ND_NEIGHBOR_ADVERT, 2, 0),

            // Keep packet if icmp6_type is ND_NEIGHBOR_SOLICIT.
            BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, ND_NEIGHBOR_SOLICIT, 1, 0),

            // Drop packet.
            BPF_STMT(BPF_RET | BPF_K, 0),

            // Keep packet.
            BPF_STMT(BPF_RET | BPF_K, (u_int32_t)-1)
        };

        static struct sock_fprog fprog = {
            7,
            filter
        };

        if (setsockopt(iface->_fd, SOL_SOCKET, SO_ATTACH_FILTER, &fprog,
                sizeof(fprog)) < 0)
            throw_system_error("failed to set up filter");
    }

    return iface;
}

ssize_t iface::read(address_s &saddr, packet_s &packet)
{

    struct iovec iov;
    iov.iov_len = sizeof(packet_s);
    iov.iov_base = (caddr_t)&packet;

    struct sockaddr_in6 sin6;

    struct msghdr mhdr;
    memset(&mhdr, 0, sizeof(struct msghdr));
    mhdr.msg_name = (caddr_t)&sin6;
    mhdr.msg_namelen = sizeof(struct sockaddr_in6);
    mhdr.msg_iov = &iov;
    mhdr.msg_iovlen = 1;

    ssize_t len;
    if ((len = ::recvmsg(_fd, &mhdr, 0)) < 0)
        return -1;

    if (len < sizeof(struct ip6_hdr) + sizeof(struct icmp6_hdr))
        return -1;

    logger::debug() << "iface::read() len=" << len;

    saddr = sin6.sin6_addr;
    return len;
}

ssize_t iface::write(const address_s &daddr, const packet_s &packet)
{
    struct sockaddr_in6 sin6;
    memset(&sin6, 0, sizeof(struct sockaddr_in6));
    sin6.sin6_family = AF_INET6;
    sin6.sin6_port   = htons(IPPROTO_ICMPV6); // Needed?
    sin6.sin6_addr   = daddr.c_addr();

    struct iovec iov;
    iov.iov_len = packet.length();
    iov.iov_base = (caddr_t)&packet;

    struct msghdr mhdr;
    memset(&mhdr, 0, sizeof(struct msghdr));
    mhdr.msg_name = (caddr_t)&sin6;
    mhdr.msg_namelen = sizeof(struct sockaddr_in6);
    mhdr.msg_iov = &iov;
    mhdr.msg_iovlen = 1;

    logger::debug() << "iface::write() daddr=" << daddr.to_string() << ", len="
                    << packet.length();

    ssize_t len;

    if ((len = ::sendmsg(_fd, &mhdr, 0)) < 0)
        return -1;

    return len;
}

void iface::add_session(const std::shared_ptr<session> &session)
{
    _sessions.push_back(session);
}

void iface::fixup()
{
    logger::debug() << "iface::fixup() _map.size()=" << _map.size();

    bool dirty = _map.size() != _pollfds.size();

    for (auto it = _map.begin(); it != _map.end(); ) {
        if (it->second.expired()) {
            it = _map.erase(it);
            dirty = dirty | true;
        } else
            it++;
    }

    if (!dirty)
        return;

    _pollfds.resize(_map.size());

    int i = 0;
    for (auto it = _map.begin(); it != _map.end(); it++) {
        _pollfds[i++] = {
            // No need to check for null here; it's been handled above.
            .fd      = it->second.lock()->_fd,
            .events  = POLLIN,
            .revents = 0
        };
    }
}

int iface::poll_all()
{
    fixup();

    if (_pollfds.size() == 0) {
        ::sleep(1);
        return 0;
    }

    int len;
    if ((len = ::poll(&_pollfds[0], _pollfds.size(), 50)) < 0) {
        return -1;
    }

    if (len == 0) {
        return 0;
    }

    std::map<std::string, std::weak_ptr<iface> >::iterator i_it = _map.begin();

    int i = 0;

    for (std::vector<struct pollfd>::iterator f_it = _pollfds.begin();
            f_it != _pollfds.end(); f_it++) {

        if (!(f_it->revents & POLLIN)) {
            continue;
        }

        std::shared_ptr<iface> iface = i_it->second.lock();

        address saddr, daddr, taddr;
        packet packet;

        if (iface->read(saddr, packet) < 0) {
            logger::error() << "junk packet";
            continue;
        }

    }

    return 0;
}

bool iface::allmulti()
{
    struct ifreq ifr;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, _name.c_str(), IFNAMSIZ);

    if (ioctl(_fd, SIOCGIFFLAGS, &ifr) < 0)
        throw std::system_error(errno, std::system_category());

    return !!(ifr.ifr_flags & IFF_ALLMULTI);
}

bool iface::allmulti(bool value)
{
    struct ifreq ifr;

    logger::debug()
        << "iface::allmulti() value="
        << value << ", _name=\"" << _name << "\"";

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, _name.c_str(), IFNAMSIZ);

    if (ioctl(_fd, SIOCGIFFLAGS, &ifr) < 0) 
        throw std::system_error(errno, std::system_category(),
            "failed to get device flags");

    if (!!(ifr.ifr_flags &IFF_ALLMULTI) == value)
        return value;

    if (value)
        ifr.ifr_flags |= IFF_ALLMULTI;
    else
        ifr.ifr_flags &= ~IFF_ALLMULTI;

    if (ioctl(_fd, SIOCSIFFLAGS, &ifr) < 0)
        throw std::system_error(errno, std::system_category(),
            "failed to set device flags");

    return !value;
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
