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

#ifndef NDPPD_RULE_HPP
#define NPPPD_RULE_HPP

#include <string>
#include <vector>
#include <map>
#include <list>

#include <sys/poll.h>
#include <memory>

#include "ndppd.hpp"
#include "cidr.hpp"

NDPPD_NS_BEGIN

class proxy;
class session;

class rule
{
public:
    virtual void execute(std::shared_ptr<session> session) const = 0;

    const class cidr &cidr() const { return _cidr; }

protected:
    rule(const std::shared_ptr<proxy> &proxy, const cidr &cidr);

private:
    std::shared_ptr<proxy> _proxy;
    class cidr _cidr;
};

class static_rule
        : public rule
{
public:
    static_rule(const std::shared_ptr<proxy> &proxy, const cidr &cidr);

    void execute(std::shared_ptr<session> session) const override;

};

class auto_rule
        : public rule
{
public:
    auto_rule(const std::shared_ptr<proxy> &proxy, const cidr &cidr);

    void execute(std::shared_ptr<session> session) const override;

};

class forward_rule
        : public rule
{
public:
    forward_rule(const std::shared_ptr<proxy> &proxy, const cidr &cidr, const std::shared_ptr<interface> &interface);

    void execute(std::shared_ptr<session> session) const override;

private:
    std::shared_ptr<interface> _interface;
};

NDPPD_NS_END

#endif // NDPPD_RULE_HPP
