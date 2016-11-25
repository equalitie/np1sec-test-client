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
public:
    User(Channel& channel, const std::string& name);

    User(const User&) = delete;
    User& operator=(const User&) = delete;

    User(User&&) = delete;
    User& operator=(User&&) = delete;

    const std::string& name() const { return _name; }

public:
    boost::optional<np1sec::PublicKey> public_key;
    const std::set<std::string>& authorized_by() const { return _authorized_by; }

    void authorized_by(std::string name);
    void un_authorized_by(std::string name);
    bool is_authorized() const;
    void set_promoted(bool);
    bool was_promoted() const;
    bool was_promoted_by_me() const;
    bool is_myself() const { return _is_myself; }

    bool in_chat() const;

    void update_view();

    const Channel& channel() const { return _channel; }

    bool needs_authorization_from_me() const;

    void bind_user_list(UserList&);

private:
    std::string _name;
    Channel& _channel;
    bool _is_myself;
    bool _was_promoted = false;
    std::set<std::string> _authorized_by;
    std::unique_ptr<ChannelList::User> _view;
    UserList::User _view_in_channel;
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
    , _channel(channel)
    , _is_myself(name == channel._room.username())
    , _authorized_by({name})
    , _view(new ChannelList::User(channel._view))
{
    _view->popup_actions["Info"] = [this] {
        auto gtk_window = _channel._room.gtk_window();
        UserInfoDialog::show(gtk_window, *this);
    };

    update_view();
}

inline
void User::bind_user_list(UserList& user_list)
{
    _view_in_channel.bind(user_list);
    update_view();
}

inline
bool User::needs_authorization_from_me() const
{
    std::cout << _channel.my_username() << " --- 1" << std::endl;
    if (is_myself()) return false;

    auto myname = channel().my_username();

    if (!channel().find_user(myname)) {
        std::cout << _channel.my_username() << " --- 2" << std::endl;
        return false;
    }

    if (!was_promoted_by_me()) {
        std::cout << _channel.my_username() << " --- 3" << std::endl;
        if (channel().user_in_chat(myname)) {
            std::cout << _channel.my_username() << " --- 4" << std::endl;
            return true;
        }
        else {
            if (is_authorized()) {
                std::cout << _channel.my_username() << " --- 5" << std::endl;
                return true;
            }
        }
    }
    std::cout << _channel.my_username() << " --- 6" << std::endl;

    return false;
}

inline bool User::is_authorized() const
{
    return _channel._delegate->user_is_authorized(_name);
}

inline void User::update_view()
{
    auto name = _name;

    std::cout << _channel.my_username() << " update_view " << name << std::endl;
    if (needs_authorization_from_me()) {
        name = "*" + name;

        auto authorize = [this] {
            _channel._room.authorize(_name);
        };

        _view->popup_actions["Authorize"] = authorize;
        _view->on_double_click = authorize;
        _view_in_channel.on_double_click = authorize;
    }

    if (in_chat()) {
        name += " (chatting)";
    }

    _view->set_text(name);
    _view_in_channel.set_text(name);
}

inline bool User::in_chat() const
{
    return _channel._delegate->user_in_chat(_name);
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

inline void User::set_promoted(bool value) {
    _was_promoted = value;
    update_view();
}

inline bool User::was_promoted() const {
    return _was_promoted;
}

inline bool User::was_promoted_by_me() const {
    return _authorized_by.count(_channel._room.username());
}

} // np1sec_plugin namespace
