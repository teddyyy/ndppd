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
#include <list>
#include <memory>
#include <fstream>

#include "ndppd.h"
#include "route.h"

#include "iface.h"

NDPPD_NS_BEGIN

std::list<std::shared_ptr<route> > route::_routes;

int route::_ttl;

int route::_c_ttl;

route::route()
{
}

size_t route::hexdec(const char* str, unsigned char* buf, size_t size)
{
    for (size_t i = 0; ; i++) {
        if (i >= size) {
            return i;
        }

        char c1 = tolower(str[i*  2]), c2 = tolower(str[i*  2 + 1]);

        if (!isxdigit(c1) || !isxdigit(c2)) {
            return i;
        }

        if ((c1 >= '0') && (c1 <= '9')) {
            buf[i] = (c1 - '0') << 4;
        } else {
            buf[i] = ((c1 - 'a') + 10) << 4;
        }

        if ((c2 >= '0') && (c2 <= '9')) {
            buf[i] |= c2 - '0';
        } else {
            buf[i] |= (c2 - 'a') + 10;
        }
    }
}

std::string route::token(const char* str)
{
    while (*str && isspace(*str)) {
        str++;
    }

    if (!*str) {
        return "";
    }

    std::stringstream ss;

    while (*str && !isspace(*str)) {
        ss <<* str++;
    }

    return ss.str();
}

void route::load(const std::string& path)
{
    _routes.clear();

    logger::debug() << "reading routes";

    try {
        std::ifstream ifs;
        ifs.exceptions(std::ifstream::badbit | std::ifstream::failbit);
        ifs.open(path.c_str(), std::ios::in);
        ifs.exceptions(std::ifstream::badbit);

        while (!ifs.eof()) {
            char buf[1024];
            ifs.getline(buf, sizeof(buf));

            if (ifs.gcount() < 149) {
                continue;
            }

            /*in6addr addr;

            unsigned char pfx;

            if (hexdec(buf, (unsigned char *)&addr.addr(), 16) != 16) {
                // TODO: Warn here?
                continue;
            }

            if (hexdec(buf + 33, &pfx, 1) != 1) {
                // TODO: Warn here?
                continue;
            }

            addr.prefix((int)pfx);

            route::create(addr, token(buf + 141));
*/
        }
    } catch (std::ifstream::failure e) {
        logger::warning() << "Failed to parse IPv6 routing data from '" << path << "'";
        logger::error() << e.what();
    }
}

void route::update(int elapsed_time)
{
    if ((_c_ttl -= elapsed_time) <= 0) {
        load("/proc/net/ipv6_route");
        _c_ttl = _ttl;
    }
}

std::shared_ptr<route> route::create(const cidr_s &cidr,
    const std::string& ifname)
{
    // http://stackoverflow.com/questions/8147027
    struct make_shared_class : public route {};
    auto route = std::make_shared<make_shared_class>();

    route->_cidr = cidr;
    route->_ifname = ifname;

    // logger::debug() << "route::create() addr=" << addr << ", ifname=" << ifname;
    _routes.push_back(route);
    return route;
}

std::shared_ptr<route> route::find(const in6addr_s &in6addr)
{
    for (auto it = _routes.begin(); it != _routes.end(); it++) {
        if ((*it)->cidr().contains(in6addr))
            return *it;
    }

    return std::shared_ptr<route>();
}

std::shared_ptr<iface> route::find_and_open(const in6addr_s &in6addr)
{
    std::shared_ptr<route> route;

    if (route = find(in6addr)) {
        return route->iface();
    }

    return nullptr;
}

const std::string& route::ifname() const
{
    return _ifname;
}

std::shared_ptr<iface_s> route::iface()
{
    if (!_iface) {
        logger::debug() << "router::ifa() opening interface '" << _ifname << "'";
        return _iface = iface_s::open(_ifname);
    }

    return nullptr;
}

const cidr_s &route::cidr() const
{
    return _cidr;
}

int route::ttl()
{
    return _ttl;
}

void route::ttl(int ttl)
{
    _ttl = ttl;
}

NDPPD_NS_END

