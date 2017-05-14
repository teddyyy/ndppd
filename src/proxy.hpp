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

#ifndef NDPPD_PROXY_HPP
#define NDPPD_PROXY_HPP

#include <list>
#include <string>
#include <vector>
#include <map>

#include <sys/poll.h>
#include <memory>

#include "ndppd.hpp"
#include "cidr.hpp"

NDPPD_NS_BEGIN

class session;
class rule;
class packet_socket;

class proxy : public std::enable_shared_from_this<proxy>
{
public:
    static std::shared_ptr<proxy> get_or_create(const std::string &name);

    void update(int elapsed);

    bool router() const { return _router; }

    void router(bool router) { _router = router; }

    int timeout() const { return _timeout; }

    void timeout(int timeout) { _timeout = timeout; }

    int ttl() const { return _ttl; }

    void ttl(int ttl) { _ttl = ttl; }


    const std::shared_ptr<class interface> &interface() const { return _interface; }

private:

    static std::map<std::string, std::shared_ptr<proxy>> _proxies;

    NDPPD_SAFE_CONSTRUCTOR(proxy)

    proxy(const std::string &ifname);

    void handle_ns(const cidr &source, const cidr &destination, const cidr &target);

    std::shared_ptr<class interface> _interface;

    std::shared_ptr<packet_socket> _socket;

    std::list<std::shared_ptr<rule>> _rules;

    std::list<std::shared_ptr<session>> _sessions;

    bool _router;

    int _ttl, _timeout;
};

NDPPD_NS_END

#endif // NDPPD_PROXY_HPP
