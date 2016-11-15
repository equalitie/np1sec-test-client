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

#include <memory>
#include <iostream>
#include <boost/optional.hpp>
#include <boost/range/adaptor/map.hpp>

/* np1sec headers */
#include "src/crypto.h"
#include "src/channel.h"

namespace np1sec_plugin {

class Room;
class User;

class Channel final : public np1sec::ChannelInterface {
    using PublicKey = np1sec::PublicKey;

public:
    Channel(np1sec::Channel*, Room&);

    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    Channel(Channel&&) = default;
    Channel& operator=(Channel&&) = default;

    /*
     * Internal
     */
    void add_member(const std::string&);
    void set_user_public_key(const std::string& username, const PublicKey&);
    void promote_user(const std::string& username);
    User* find_user(const std::string&);
    bool everyone_promoted_everyone() const;
    size_t size() const { return _users.size(); }
    const std::string& my_username() const { return _room.username(); }

public:
    /*
     * A user joined this channel. No cryptographic identity is known yet.
     */
    void user_joined(const std::string& username) override;

    /*
     * A user left the channel.
     */
    void user_left(const std::string& username) override;

    /*
     * The cryptographic identity of a user was confirmed.
     */
    void user_authenticated(const std::string& username, const PublicKey& public_key) override;

    /*
     * A user failed to authenticate. This indicates an attack!
     */
    void user_authentication_failed(const std::string& username) override;

    /*
     * A user <authenticatee> was accepted into the channel by a user <authenticator>.
     */
    void user_authorized_by(const std::string& user, const std::string& target) override;

    /*
     * A user got authorized by all participants and is now a full participant.
     */
    void user_promoted(const std::string& username) override;


    /*
     * You joined this channel.
     */
    void joined() override;

    /*
     * You got authorized by all participants and are now a full participant.
     */
    void authorized() override;


    /*
     * Not implemented yet.
     */
    void message_received(const std::string& username, const std::string& message) override;

    void user_joined_chat(const std::string& username) override;
    void joined_chat() override;

private:
    /*
     * Internal
     */
    size_t channel_id() const { return size_t(_delegate); }

private:
    friend class User;

    np1sec::Channel* _delegate;

    Room& _room;
    ChannelView _view;
    std::map<std::string, std::unique_ptr<User>> _users;
};

} // np1sec_plugin namespace

/* plugin headers */
#include "room.h"
#include "user.h"

namespace np1sec_plugin {

//------------------------------------------------------------------------------
// Implementation
//------------------------------------------------------------------------------
inline Channel::Channel(np1sec::Channel* delegate, Room& room)
    : _delegate(delegate)
    , _room(room)
    , _view(_room.get_view(), std::to_string(size_t(_delegate)))
{
    for (const auto& user : _delegate->users()) {
        add_member(user);
        promote_user(user);
    }

    _view.on_double_click = [this] {
        /**
         * Switching channels currently causes assertion in the np1sec
         * library.
         */
        if (_room.find_user_in_channel(_room.username())) {
            _room.inform("Already in a channel");
            return;
        }
        _room.join_channel(_delegate);
    };
}

inline
bool Channel::everyone_promoted_everyone() const
{
    std::set<std::string> all_users;

    for (const auto& u : _users | boost::adaptors::map_keys) {
        all_users.insert(u);
    }

    for (const auto& u : _users | boost::adaptors::map_values) {
        if (u->authorized_by() != all_users) {
            return false;
        }
    }

    return true;
}

inline
void Channel::promote_user(const std::string& username)
{
    auto user = find_user(username);

    if (!user) {
        assert(0 && "Authorized user is not in the channel");
        return;
    }

    user->set_promoted(true);
}

inline
void Channel::set_user_public_key(const std::string& username, const PublicKey& public_key)
{
    auto user_i = _users.find(username);

    if (user_i == _users.end()) {
        assert(0 && "Authenticated user is not in the channel");
        return;
    }

    user_i->second->public_key = public_key;
}

inline
void Channel::add_member(const std::string& username)
{
    assert(_users.count(username) == 0);

    _users.emplace(username,
                   std::make_unique<User>(*this, username));
}

inline
void Channel::user_joined(const std::string& username)
{
    _room.inform("Channel::user_joined(", channel_id(), ", ", username, ")");

    for (const auto& user : _users | boost::adaptors::map_values) {
        user->set_promoted(false);
    }

    add_member(username);

    find_user(username)->set_promoted(false);
}

void Channel::user_left(const std::string& username)
{
    _room.inform("Channel::user_left(", channel_id(), ", ", username, ")");
    _users.erase(username);

    if (_users.empty()) {
        // Self destruct.
        _room._channels.erase(_delegate);
        return;
    }

    for (auto& user : _users | boost::adaptors::map_values) {
        user->un_authorized_by(username);
    }
}

void Channel::user_authenticated(const std::string& username, const PublicKey& public_key)
{
    _room.inform("Channel::user_authenticated(", channel_id(), ", ", username, ")");
    set_user_public_key(username, public_key);
}

void Channel::user_authentication_failed(const std::string& username)
{
    _room.inform("Channel::user_authentication_failed(", channel_id(), ", ", username, ")");
}

void Channel::user_authorized_by(const std::string& user, const std::string& target)
{
    _room.inform("Channel::user_authorized_by(", channel_id(), ", ", target, " by ", user, ")");

    auto target_p = find_user(target);

    if (!target_p || !find_user(user)) {
        assert(0 && "Either target or user is not in this channel");
        return;
    }

    target_p->authorized_by(user);
}

void Channel::user_promoted(const std::string& username)
{
    _room.inform("Channel::user_promoted(", channel_id(), ", ", username, ")");
    promote_user(username);
}

void Channel::joined()
{
    _room.inform("Channel::joined(", channel_id(), ")");
}

void Channel::authorized()
{
    _room.inform("Channel::authorized(", channel_id(), ")");
}


void Channel::message_received(const std::string& username, const std::string& message)
{
    _room.display(username, message);
}

void Channel::user_joined_chat(const std::string& username)
{
    _room.inform("Channel::user_joined_chat(", username, ")");
    if (auto u = find_user(username)) {
        u->in_chat(true);
    }
}

void Channel::joined_chat()
{
    _room.inform("Channel::joined_chat()");
    auto u = find_user(my_username());
    assert(u);
    if (u) u->in_chat(true);
}

User* Channel::find_user(const std::string& user) {
    auto user_i = _users.find(user);
    if (user_i == _users.end()) return nullptr;
    return user_i->second.get();
}

} // np1sec_plugin namespace
