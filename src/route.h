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

#include "ndppd.h"
#include "in6addr.h"
#include "cidr.h"

NDPPD_NS_BEGIN

struct route {
    std::shared_ptr<route> create(const cidr_s &cidr,
        const std::string &ifname);

    static std::shared_ptr<route> find(const in6addr_s &addr);

    static std::shared_ptr<iface_s> find_and_open(const in6addr_s &addr);

    static void load(const std::string &path);

    static void update(int elapsed_time);

    static int ttl();

    static void ttl(int ttl);

    const std::string &ifname() const;

    const cidr_s &cidr() const;

    std::shared_ptr<iface_s> iface();

private:
    static std::string token(const char *str);

    static std::list<std::shared_ptr<route> > _routes;

    static int _ttl;

    static int _c_ttl;

    cidr_s _cidr;

    std::string _ifname;

    std::shared_ptr<iface_s> _iface;

    static size_t hexdec(const char *str, unsigned char *buf, size_t size);

    route();
};

NDPPD_NS_END
