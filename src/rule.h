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
#include <vector>
#include <map>

#include <sys/poll.h>

#include "ndppd.h"
#include "cidr.h"

NDPPD_NS_BEGIN

struct rule {
    /* Returns a new rule instance. */
    static std::shared_ptr<rule_s> create(
        const std::shared_ptr<proxy_s> &proxy, const cidr &cidr,
        const std::shared_ptr<iface_s> &iface = {}, bool auto_ = false);

    const cidr_s &cidr() const;

    std::shared_ptr<iface_s> iface() const;

    bool is_auto() const;

    bool check(const in6addr &addr) const;

private:
    std::weak_ptr<proxy_s> _proxy;
    std::shared_ptr<iface_s> _iface;
    cidr_s _cidr;
    bool _auto;

    rule();
};

NDPPD_NS_END
