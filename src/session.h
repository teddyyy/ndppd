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

#include <vector>
#include <list>

#include "ndppd.h"
#include "address.h"

NDPPD_NS_BEGIN

class proxy;
class iface;

struct session {
    enum class status_enum {
        WAITING, VALID, INVALID
    };

    static void update_all(int elapsed_time);

    // Destructor.
    ~session();

    static std::shared_ptr<session> create(
        const std::shared_ptr<proxy> &proxy, const address &saddr,
        const address &daddr, const address &taddr);

    void add_iface(const std::shared_ptr<iface> &iface);

    const address &taddr() const;

    const address &daddr() const;

    const address &saddr() const;

    status_enum status() const;

    void status(status_enum status);

    void handle_advert();

    void send_advert();

    void send_solicit();

    void refesh();

private:
    static std::list<std::weak_ptr<session> > _sessions;

    std::weak_ptr<proxy> _proxy;

    address _saddr, _daddr, _taddr;

    // An array of interfaces this session is monitoring for ND_NEIGHBOR_ADVERT.
    std::list<std::shared_ptr<iface> > _ifaces;

    // The remaining time in miliseconds the object will stay in the interfaces'
    // session array or cache.
    int _ttl;

    status_enum _status;

    session() { }
};

NDPPD_NS_END
