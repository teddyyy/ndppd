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

#include "hwaddress.hpp"

NDPPD_NS_BEGIN

hwaddress::hwaddress()
{
}

hwaddress::hwaddress(const ether_addr &addr) :
    _addr(addr)
{
}

ether_addr &hwaddress::addr()
{
    return _addr;
}

const ether_addr &hwaddress::c_addr() const
{
    return _addr;
}

NDPPD_NS_END
