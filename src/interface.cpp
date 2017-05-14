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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstddef>

#include <net/if.h>

#include <errno.h>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <system_error>

#include "ndppd.hpp"
#include "interface.hpp"
#include "logger.hpp"

NDPPD_NS_BEGIN

std::map<std::string, std::weak_ptr<interface>> interface::_interfaces;

std::shared_ptr<interface> interface::get_or_create(const std::string &name)
{
    auto it = _interfaces.find(name);

    std::shared_ptr<interface> ptr;
    if (it != _interfaces.end() && (ptr = it->second.lock()))
        return ptr;

    ptr = create(name);
    _interfaces[name] = ptr;
    return ptr;
}

interface::interface(const std::string &name)
        : _name(name), _socket(icmp6_socket::create(name))
{
    if (!(_index = if_nametoindex(_name.c_str())))
        throw std::system_error(errno, std::system_category(), "Could not determine interface index");
}

interface::~interface()
{
    logger::debug() << "interface::~interface()";
}

NDPPD_NS_END
