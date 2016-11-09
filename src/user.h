/**
 * Multiparty Off-the-Record Messaging library
 * Copyright (C) 2016, eQualit.ie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of version 3 of the GNU Lesser General
 * Public License as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#pragma once

#include "src/crypto.h"
#include "popup.h"
#include "user_view.h"

#include <boost/optional.hpp>

namespace np1sec_plugin {

class Channel;

class User {
public:
    User(Channel& channel, const std::string& name);

    const std::string& name() const { return _name; }

public:
    boost::optional<np1sec::PublicKey> public_key;

private:
    std::string _name;
    UserView _view;
};

} // np1sec_plugin namespace

#include "channel.h"
#include "user_info_dialog.h"

namespace np1sec_plugin {
//------------------------------------------------------------------------------
// Implementation
//------------------------------------------------------------------------------
inline
User::User(Channel& channel, const std::string& name)
    : _name(name)
    , _view(channel._view, name)
{
    _view.popup_actions["info"] = [this, &channel] {
        UserInfoDialog::show(channel._room.gtk_window(), *this);
    };
}

} // np1sec_plugin namespace
