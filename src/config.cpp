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

#include <memory>
#include <iostream>
#include <fstream>

#include <cstdarg>
#include <cstring>

#include "ndppd.hpp"
#include "config.hpp"

NDPPD_NS_BEGIN

// ================================================================================================================= //

std::string config_error::build_error_string(const config_reader &reader, const std::string &what)
{
    return what;
}

config_error::config_error(const config_reader &reader, const std::string &what)
    : std::runtime_error(build_error_string(reader, what))
{
}

// ================================================================================================================= //

config_reader::config_reader(const std::string &str)
        : _str(str), _it(_str.cbegin())
{
}

void config_reader::skip()
{
    for ( ; _it != _str.cend(); _it++) {
        if (*_it == '/') {
            // Parse comments that begin with // or /*.
            if (_it + 1 == _str.cend())
                return;
            else if (*(_it + 1) == '/') {
                for (_it += 2; _it != _str.cend() && *_it != '\n'; _it++)
                    ;
            } else if (*(_it + 1) == '*') {
                for (_it += 2; _it != _str.cend(); _it++) {
                    if (*_it == '*' && (_it + 1) != _str.cend() && *(_it + 1) == '/') {
                        _it++;
                        break;
                    }
                }
            }
        } else if (*_it == '#') {
            for (_it++; _it != _str.cend() && *_it != '\n'; _it++)
                ;
        } else if (!isspace(*_it))
            break;
    }
}

bool config_reader::read_string(std::string &value, bool required, bool tolower)
{
    skip();

    value.clear();

    if (_it != _str.cend()) {
        auto it = _it;

        if (*it == '"' || *it == '\'') {
            auto c = *it++;

            while (it != _str.cend()) {
                if (*it == '\\') {
                    if (++it != _str.cend())
                        value += tolower ? std::tolower(*it++) : *it++;
                } else if (*it == c) {
                    _it = it + 1;
                    return true;
                } else {
                    value += tolower ? std::tolower(*it++) : *it++;
                }
            }
        } else {
            while (it != _str.cend() && !isspace(*it))
                value += tolower ? std::tolower(*it++) : *it++;
            _it = it;
            return true;
        }
    }

    if (required)
        throw config_error(*this, "unterminated string");
    return false;
}

bool config_reader::read_address(cidr &value, bool required)
{
    std::string temp;
    temp.reserve(64);

    auto it = _it;

    if (read_string(temp, false) && value.parse_string(temp))
        return true;

    _it = it;

    if (required)
        throw config_error(*this, "expected ipv6 address");

    return false;
}

bool config_reader::read_int(int &value, bool required)
{
    std::string temp;
    temp.reserve(64);

    auto it = _it;

    try {
        if (read_string(temp, false)) {
            value = std::stoi(temp);
            return true;
        }
    } catch (...) {

    }

    _it = it;

    if (required)
        throw config_error(*this, "expected integer");

    return false;
}

bool config_reader::read_bool(bool &value, bool required)
{
    std::string temp;
    temp.reserve(64);

    auto it = _it;

    if (read_string(temp, false, true)) {
        if (temp == "1" || temp == "yes" || temp == "on" || temp == "true") {
            value = true;
            return true;
        }
        if (temp == "0" || temp == "no" || temp == "off" || temp == "false") {
            value = false;
            return true;
        }
    }

    _it = it;

    if (required)
        throw config_error(*this, "expected boolean");

    return true;
}

bool config_reader::read_lbr(bool required)
{
    skip();

    if (_it != _str.cend() && *_it == '{') {
        _it++;
        return true;
    }

    if (required)
        throw config_error(*this, "expected left brace '{'");

    return false;
}

bool config_reader::read_rbr(bool required)
{
    skip();

    if (_it != _str.cend() && *_it == '}') {
        _it++;
        return true;
    }

    if (required)
        throw config_error(*this, "expected right brace '}'");

    return false;
}

// ================================================================================================================= //

config_rule_section::config_rule_section(config_reader &reader)
{
    reader.read_address(_address);
    reader.read_lbr();

    while (!reader.read_rbr(false)) {
        std::string key;
        reader.read_string(key, true, true);
    }
}

// ================================================================================================================= //

config_proxy_section::config_proxy_section(config_reader &reader)
{
    if (!reader.read_string(_interface, false, false))
        throw config_error(reader, "expected interface name");

    reader.read_lbr();

    std::string key;
    while (!reader.read_rbr(false)) {
        reader.read_string(key, true, true);

        if (key == "ttl")
            reader.read_int(_ttl);
        else if (key == "router")
            reader.read_bool(_router);
        else if (key == "timeout")
            reader.read_int(_timeout);
        else if (key == "rule")
            _rules.push_back(config_rule_section::create(reader));
        else
            throw config_error(reader, "unknown configuration option '" +  key + "'");
    }
}

// ================================================================================================================= //

config::config(const std::string &path)
{
    std::ifstream ifs;
    ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    ifs.open(path.c_str(), std::ios::in);
    ifs.exceptions(std::ifstream::badbit);
    std::string buf((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

    config_reader reader(buf);

    std::string key;
    while (reader.read_string(key, false, true)) {
        if (key == "route-ttl")
            reader.read_int(_route_ttl);
        else if (key == "proxy")
            _proxies.push_back(config_proxy_section::create(reader));
        else
            throw config_error(reader, "unknown configuration option '" +  key + "'");
    }
}

// ================================================================================================================= //

NDPPD_NS_END
