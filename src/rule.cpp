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

#include "ndppd.hpp"
#include "session.hpp"
#include "rule.hpp"
#include "proxy.hpp"
#include "interface.hpp"

NDPPD_NS_BEGIN

// ================================================================================================================= //

rule::rule(const std::shared_ptr<proxy> &proxy, const cidr &cidr)
        : _proxy(proxy), _cidr(cidr)
{
}

// ================================================================================================================= //

static_rule::static_rule(const std::shared_ptr<proxy> &proxy, const cidr &cidr)
        : rule(proxy, cidr)
{
}

void static_rule::execute(std::shared_ptr<session> session) const
{

    session->handle_na();
}

// ================================================================================================================= //

auto_rule::auto_rule(const std::shared_ptr<proxy> &proxy, const cidr &cidr)
        : rule(proxy, cidr)
{
}

void auto_rule::execute(std::shared_ptr<session> session) const
{
    // TODO: Implement auto-detection.
}

// ================================================================================================================= //

forward_rule::forward_rule(const std::shared_ptr<proxy> &proxy, const cidr &cidr,
                           const std::shared_ptr<interface> &interface)
        : rule(proxy, cidr), _interface(interface)
{
}

void forward_rule::execute(std::shared_ptr<session> session) const
{
    _interface->socket()->send_ns(session->tgt());
}

// ================================================================================================================= //

NDPPD_NS_END
