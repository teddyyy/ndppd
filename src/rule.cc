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
#include <memory>

#include "ndppd.h"
#include "rule.h"
#include "proxy.h"
#include "iface.h"

NDPPD_NS_BEGIN

rule::rule() {
}

std::shared_ptr<rule_s> rule::create(const std::shared_ptr<proxy_s> &proxy,
    const cidr_s &cidr, const std::shared_ptr<iface_s> &iface, bool auto_)
{
    logger::debug()
        << "rule::create() iface=" << proxy->iface()->name()
        << ", cidr=" << cidr;

    // http://stackoverflow.com/questions/8147027
    struct make_shared_class : public rule_s {};
    auto rule(std::make_shared<make_shared_class>());
    rule->_cidr  = cidr;
    rule->_iface = iface;
    rule->_proxy = proxy;
    rule->_auto  = auto_;
    return rule;
}

const cidr_s &rule::cidr() const
{
    return _cidr;
}

std::shared_ptr<iface_s> rule::iface() const
{
    return _iface;
}

bool rule::is_auto() const
{
    return _auto;
}

bool rule::check(const ip6addr_s &ip6addr) const
{
    return _cidr.contains(ip6addr);
}

NDPPD_NS_END
