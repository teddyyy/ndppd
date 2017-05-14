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

#ifndef NDPPD_HPP
#define NDPPD_HPP

#define NDPPD_NS_BEGIN   namespace ndppd {
#define NDPPD_NS_END     }

#define NDPPD_VERSION   "1.0.0"

#define NDPPD_SAFE_CONSTRUCTOR(_Tp) \
    template<typename... _Args> \
    static std::shared_ptr<_Tp> create(_Args&&... __args) \
    { \
        struct _S : public _Tp { _S(_Args&&... __args) : _Tp(std::forward<_Args>(__args)...) {} }; \
        return std::static_pointer_cast<_Tp>(std::make_shared<_S>(std::forward<_Args>(__args)...)); \
    }

NDPPD_NS_BEGIN

class interface;
class socket;

NDPPD_NS_END

#endif // NDPPD_HPP
