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

#ifndef NDPPD_CONFIG_H
#define NDPPD_CONFIG_H

#include <vector>
#include <map>
#include <stack>

#include "ndppd.hpp"
#include "address.hpp"
#include "logger.hpp"

NDPPD_NS_BEGIN

//! Used to read tokens from a string.
class config_reader
{
public:
    explicit config_reader(const std::string &str);

    void skip();
    bool read_string(std::string &value, bool required = true, bool tolower = false);
    bool read_address(address &value, bool required = true);
    bool read_bool(bool &value, bool required = true);
    bool read_int(int &value, bool required = true);
    bool read_lbr(bool required = true);
    bool read_rbr(bool required = true);

private:
    std::string _str;
    std::string::const_iterator _it;
};

class config_error : public std::runtime_error
{
public:
    config_error(const config_reader &reader, const std::string &what);

private:
    static std::string build_error_string(const config_reader &reader, const std::string &what);
};

//! Class that represents a rule configuration section.
class config_rule_section
{
public:
    NDPPD_SAFE_CONSTRUCTOR(config_rule_section)

    const ndppd::address &address() const { return _address; }

private:
    config_rule_section(config_reader &reader);

    ndppd::address _address;
};

//! Class that represents a proxy configuration section.
class config_proxy_section
{
public:
    NDPPD_SAFE_CONSTRUCTOR(config_proxy_section)

    const std::string &interface() const { return _interface; }

    bool router() const { return _router; }

    int timeout() const { return _timeout; }

    int ttl() const { return _ttl; }

private:
    config_proxy_section(config_reader &reader);

    std::string _interface;

    bool _router;

    int _timeout;

    int _ttl;

    std::vector<std::shared_ptr<config_rule_section>> _rules;
};

class config
{
public:
    NDPPD_SAFE_CONSTRUCTOR(config)

    int route_ttl() const { return _route_ttl; }

private:
    config(const std::string &path);

    int _route_ttl;
    std::vector<std::shared_ptr<config_proxy_section>> _proxies;
};

NDPPD_NS_END

#endif // NDPPD_CONFIG_HPP
