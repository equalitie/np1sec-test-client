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

#include <boost/optional.hpp>

namespace np1sec_plugin {

class Channel;
class UserView;

class User {
public:
    User(Channel& channel, const std::string& name);

    const std::string& name() const { return _name; }

public:
    boost::optional<np1sec::PublicKey> public_key;
    const std::set<std::string>& authorized_by() const { return _authorized_by; }

    void authorized_by(std::string name);
    void un_authorized_by(std::string name);
    void set_promoted(bool);
    bool was_promoted() const;
    bool was_promoted_by_me() const;
    bool is_myself() const { return _is_myself; }

    bool in_chat() const { return _in_chat; }
    void in_chat(bool v) { _in_chat = v; }

private:
    std::string _name;
    Channel& _channel;
    bool _is_myself;
    bool _was_promoted = false;
    std::set<std::string> _authorized_by;
    std::unique_ptr<UserView> _view;
    bool _in_chat = false;
};

} // np1sec_plugin namespace

#include "channel.h"
#include "user_view.h"
#include "user_info_dialog.h"

namespace np1sec_plugin {
//------------------------------------------------------------------------------
// Implementation
//------------------------------------------------------------------------------
inline
User::User(Channel& channel, const std::string& name)
    : _name(name)
    , _channel(channel)
    , _is_myself(name == channel._room.username())
    , _authorized_by({name})
    , _view(new UserView(channel._view, *this))
{
    auto gtk_window = channel._room.gtk_window();

    _view->popup_actions["Info"] = [this, gtk_window] {
        UserInfoDialog::show(gtk_window, *this);
    };

    auto& room = channel._room;

    if (name != channel._room.username()) {
        _view->popup_actions["Authorize"] = [this, &room, name] {
            room.authorize(name);
        };
    }
}

inline void User::authorized_by(std::string name)
{
    _authorized_by.insert(name);
    _view->update(*this);
}

inline void User::un_authorized_by(std::string name)
{
    _authorized_by.erase(name);
    _view->update(*this);
}

inline void User::set_promoted(bool value) {
    _was_promoted = value;
    _view->update(*this);
}

inline bool User::was_promoted() const {
    return _was_promoted;
}

inline bool User::was_promoted_by_me() const {
    return _authorized_by.count(_channel._room.username());
}

} // np1sec_plugin namespace
