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

    void mark_joining();
    void mark_not_joined();
    void mark_joined();
    void mark_as_invited();
    void mark_as_not_invited();

    void update_view();

    const Channel& channel() const { return _channel; }

    void bind_user_list(UserList&);

    void insert_into(UserList& list);

    bool has_joined() const;
    bool is_invited() const;

    bool never_started_joining() const { return _never_started_joining; }

private:
    UserList& joined_list() const;
    UserList& invited_list() const;
    UserList& other_list() const;

private:
    std::string _name;
    PublicKey _public_key;
    Channel& _channel;
    bool _is_myself;
    bool _is_joined = false;
    bool _never_started_joining = true;
    std::unique_ptr<UserList::User> _view;
};

} // np1sec_plugin namespace

#include "channel.h"
#include "user_info_dialog.h"
#include "room_view.h"
#include "channel_view.h"

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
{
    insert_into(other_list());

    //_view_in_room.popup_actions["Info"] = [this] {
    //    auto gtk_window = _channel._room.gtk_window();
    //    UserInfoDialog::show(gtk_window, *this);
    //};

    update_view();
}

inline void User::insert_into(UserList& list)
{
    _view.reset(new UserList::User());
    _view->bind(list);
    update_view();
}

inline void User::update_view()
{
    if (!_view) return;

    auto name = _name;

    bool can_invite = !is_invited() && !has_joined() && !_is_joined;

    _view->popup_actions.clear();

    if (can_invite) {
        auto invite = [this] {
            _channel.invite(_name, _public_key);
        };

        _view->popup_actions["Invite"] = invite;
        _view->on_double_click = invite;
    }

    if (_is_myself && is_invited() && !has_joined() && !_is_joined) {
        auto join = [this] {
            _channel._delegate->join();
        };

        _view->popup_actions["Join"] = join;
        _view->on_double_click = join;
    }

    if (has_joined() && !_is_joined) {
        if (auto main_user = _channel.find_user(_channel.my_username())) {
            if (main_user->has_joined()) {
                name += " !c";
            }
        }
    }

    _view->set_text(name);
}

inline bool User::has_joined() const
{
    if (!_view) return false;
    return joined_list().is_in(*_view);
}

inline bool User::is_invited() const
{
    if (!_view) return false;
    return invited_list().is_in(*_view);
}

inline void User::mark_joining()
{
    _never_started_joining = false;
    insert_into(joined_list());
    update_view();
}

inline void User::mark_not_joined()
{
    _is_joined = false;
    insert_into(other_list());
    update_view();
}

inline void User::mark_joined()
{
    _is_joined = true;
    update_view();
}

inline void User::mark_as_not_invited()
{
    _is_joined = false;
    insert_into(other_list());
    update_view();
}

inline void User::mark_as_invited()
{
    _is_joined = false;
    insert_into(invited_list());
    update_view();
}

inline UserList& User::joined_list()  const { return _channel._channel_view->joined_user_list(); }
inline UserList& User::invited_list() const { return _channel._channel_view->invited_user_list(); }
inline UserList& User::other_list()   const { return _channel._channel_view->other_user_list(); }

} // np1sec_plugin namespace
