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
#include "user_list.h"

#include <boost/optional.hpp>

namespace np1sec_plugin {

class Channel;

class User {
    using PublicKey = np1sec::PublicKey;

public:
    User(Channel& channel, const std::string& name, const PublicKey&);

    User(const User&) = delete;
    User& operator=(const User&) = delete;

    User(User&&) = delete;
    User& operator=(User&&) = delete;

    const std::string& name() const { return _name; }

public:
    const PublicKey& public_key() const { return _public_key; }
    const std::set<std::string>& authorized_by() const { return _authorized_by; }

    void authorized_by(std::string name);
    void un_authorized_by(std::string name);
    void mark_joined();
    void mark_in_chat();
    void mark_as_invited();

    void update_view();

    const Channel& channel() const { return _channel; }

    void bind_user_list(UserList&);

private:
    std::string _name;
    PublicKey _public_key;
    Channel& _channel;
    bool _is_myself;
    bool _has_joined = false;
    bool _is_in_chat = false;
    bool _is_invited = false;
    std::set<std::string> _authorized_by;
    UserList::User _view_in_channel;
};

} // np1sec_plugin namespace

#include "channel.h"
#include "user_info_dialog.h"
#include "room_view.h"

namespace np1sec_plugin {
//------------------------------------------------------------------------------
// Implementation
//------------------------------------------------------------------------------
inline
User::User(Channel& channel, const std::string& name, const PublicKey& pubkey)
    : _name(name)
    , _public_key(pubkey)
    , _channel(channel)
    , _is_myself(name == channel._room.username())
    , _authorized_by({name})
{
    //_view_in_room.popup_actions["Info"] = [this] {
    //    auto gtk_window = _channel._room.gtk_window();
    //    UserInfoDialog::show(gtk_window, *this);
    //};

    update_view();
}

inline
void User::bind_user_list(UserList& user_list)
{
    _view_in_channel.bind(user_list);
    update_view();
}

inline void User::update_view()
{
    auto name = _name;

    bool can_invite = !_is_invited && !_has_joined && !_is_in_chat;

    _view_in_channel.popup_actions.clear();

    if (can_invite) {
        auto invite = [this] {
            _channel.invite(_name, _public_key);
        };

        _view_in_channel.popup_actions["Invite"] = invite;
    }

    if (_is_myself && _is_invited && !_has_joined && !_is_in_chat) {
        _view_in_channel.popup_actions["Join"] = [this] {
            _channel._delegate->join();
        };
    }

    if (_has_joined) {
        name += " j";
    }

    if (_is_in_chat) {
        name += " c";
    }

    if (_is_invited && !_has_joined && !_is_in_chat) {
        name += " i";
    }

    _view_in_channel.set_text(name);
}

inline void User::authorized_by(std::string name)
{
    _authorized_by.insert(name);
    update_view();
}

inline void User::un_authorized_by(std::string name)
{
    _authorized_by.erase(name);
    update_view();
}

inline void User::mark_joined() {
    _has_joined = true;
    update_view();
}

inline void User::mark_in_chat() {
    _is_in_chat = true;
    update_view();
}

inline void User::mark_as_invited() {
    _is_invited = true;
    update_view();
}

} // np1sec_plugin namespace
