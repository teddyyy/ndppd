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
#pragma once

#include <string>
#include <netinet/ip6.h>
#include <netinet/ether.h>

#include "ndppd.h"

NDPPD_NS_BEGIN

// This struct represents a link-layer, or MAC, address.
struct lladdr {
    lladdr() {}

    std::string to_string() const;

    operator std::string() const;

private:
    uint8_t _v[6];
};

NDPPD_NS_END
