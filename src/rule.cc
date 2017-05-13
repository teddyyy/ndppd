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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <net/if.h>

#include "ndppd.hpp"
#include "rule.h"
#include "proxy.hpp"
#include "interface.hpp"

NDPPD_NS_BEGIN

bool rule::_any_aut = false;

rule::rule(const address &address, const std::shared_ptr<ndppd::interface> &interface) :
    _address(address), _interface()
{

}

ptr<rule> rule::create(const ptr<proxy>& pr, const address& addr, const ptr<iface>& ifa)
{
    ptr<rule> ru(new rule());
    ru->_ptr  = ru;
    ru->_pr   = pr;
    ru->_ifa  = ifa;
    ru->_addr = addr;
    ru->_aut  = false;
    unsigned int ifindex;

    ifindex = if_nametoindex(pr->ifa()->name().c_str());
#ifdef WITH_ND_NETLINK
    if_add_to_list(ifindex, pr->ifa());
#endif
    ifindex = if_nametoindex(ifa->name().c_str());
#ifdef WITH_ND_NETLINK
    if_add_to_list(ifindex, ifa);
#endif

    logger::debug() << "rule::create() if=" << pr->ifa()->name() << ", addr=" << addr;

    return ru;
}

ptr<rule> rule::create(const ptr<proxy>& pr, const address& addr, bool aut)
{
    ptr<rule> ru(new rule());
    ru->_ptr   = ru;
    ru->_pr    = pr;
    ru->_addr  = addr;
    ru->_aut   = aut;
    _any_aut   = _any_aut || aut;

    logger::debug()
        << "rule::create() if=" << pr->ifa()->name().c_str() << ", addr=" << addr
        << ", auto=" << (aut ? "yes" : "no");

    return ru;
}

bool rule::check(const address &address) const
{
    return _address == address;
}

NDPPD_NS_END
