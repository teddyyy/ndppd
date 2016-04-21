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
#include <list>
#include <vector>
#include <map>

#include <sys/poll.h>
#include <net/ethernet.h>

#include "ndppd.h"
#include "proxy.h"
#include "packet.h"
#include "address.h"

NDPPD_NS_BEGIN

class iface {
    static std::map<std::string, std::weak_ptr<iface> > _map;

    static bool _map_dirty;

    // An array of objects used with ::poll.
    static std::vector<struct pollfd> _pollfds;

    // Updates the array above.
    static void fixup_pollfds();

    static void cleanup();

    // The "generic" ICMPv6 socket for reading/writing NB_NEIGHBOR_ADVERT
    // messages as well as writing NB_NEIGHBOR_SOLICIT messages.
    int _fd;

    // Previous state of ALLMULTI for the interface.
    int _prev_allmulti;

    // Name of this interface.
    std::string _name;

    // An array of sessions that are monitoring this interface for
    // ND_NEIGHBOR_ADVERT messages.
    std::list<std::weak_ptr<session> > _sessions;

    std::weak_ptr<ndppd::proxy> _proxy;

    // The link-layer address of this interface.
    struct ether_addr _hwaddr;

    // Turns on/off ALLMULTI for this interface - returns the previous state
    // or -1 if there was an error.
    bool allmulti(bool state);

    bool allmulti();

    // Constructor.
    iface();
public:
    ~iface();

    static void fixup();

    static std::shared_ptr<iface> open(const std::string &name);

    static int poll_all();

    ssize_t read(address &address, packet &packet);

    ssize_t write(const address &address, const packet &packet);

    // Returns the name of the interface.
    const std::string& name() const;

    // Adds a session to be monitored for ND_NEIGHBOR_ADVERT messages.
    void add_session(const std::shared_ptr<session> &session);

    void remove_session(const std::shared_ptr<session> &session);

    void proxy(const std::shared_ptr<ndppd::proxy> &proxy);

    std::shared_ptr<ndppd::proxy> proxy() const;
};

NDPPD_NS_END
