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
#include <memory>

#include <sys/poll.h>

#include "ndppd.h"

NDPPD_NS_BEGIN

struct proxy : std::enable_shared_from_this<proxy> {
    static std::shared_ptr<proxy_s> create(
        const std::shared_ptr<iface_s> &iface);

    static std::shared_ptr<proxy_s> create(const std::string &ifname);

    void handle_solicit(const in6addr_s &saddr, const in6addr_s &daddr,
        const class in6addr &taddr);

    void remove_session(const std::shared_ptr<session_s> &session);

    std::shared_ptr<rule_s> add_rule(const cidr_s &cidr,
        const std::shared_ptr<iface_s> &iface);

    std::shared_ptr<rule_s> add_rule(const cidr_s &cidr,
        bool aut = false);

    std::shared_ptr<iface_s> iface() const;

    bool router() const;

    void router(bool val);

    int timeout() const;

    void timeout(int val);

    int ttl() const;

    void ttl(int val);

private:
    static std::list<std::shared_ptr<proxy> > _list;

    std::shared_ptr<iface_s> _iface;

    std::list<std::shared_ptr<rule_s> > _rules;

    std::list<std::shared_ptr<session_s> > _sessions;

    bool _router;

    int _ttl, _timeout;

    proxy();
};

NDPPD_NS_END
