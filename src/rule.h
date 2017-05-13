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

#ifndef NDPPD_RULE_HPP
#define NPPPD_RULE_HPP

#include <string>
#include <vector>
#include <map>
#include <list>

#include <sys/poll.h>
#include <memory>

#include "ndppd.hpp"
#include "address.hpp"

NDPPD_NS_BEGIN

class interface;
class proxy;

enum class rule_type
{
    INTERFACE,
    STATIC,
    AUTO
};

class rule
{
public:
    NDPPD_SAFE_CONSTRUCTOR(rule)

    const address &address() const;
    const std::shared_ptr<interface> &interface_() const;

    bool is_auto() const;

    bool check(const address &address) const;

private:
    rule(const std::shared_ptr<proxy> &proxy, const address &address, const std::shared_ptr &interface);

    std::shared_ptr<ndppd::interface> _interface;
    address _address;

    bool _auto;
};

NDPPD_NS_END
