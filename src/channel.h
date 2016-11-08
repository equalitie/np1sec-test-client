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

/* np1sec headers */
#include "src/crypto.h"
#include "src/channel.h"

/* plugin headers */
#include "user_view.h"

namespace np1sec_plugin {

class Room;

class Channel final : public np1sec::ChannelInterface {
    using PublicKey = np1sec::PublicKey;

public:
    np1sec::Channel* delegate;

    Channel(np1sec::Channel*, Room&);

    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    Channel(Channel&&) = default;
    Channel& operator=(Channel&&) = default;

    void add_member(const std::string&);

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
    // virtual void message_received(const std::string& username, const std::string& message) = 0;
    
    // DEBUG
    void dump() override;

private:
    size_t channel_id() const { return size_t(delegate); }

private:
    Room& _room;
    ChannelView _view;
    std::map<std::string, std::unique_ptr<UserView>> _users;
};

} // np1sec_plugin namespace

/* plugin headers */
#include "room.h"

namespace np1sec_plugin {

//------------------------------------------------------------------------------
// Implementation
//------------------------------------------------------------------------------
Channel::Channel(np1sec::Channel* delegate, Room& room)
    : delegate(delegate)
    , _room(room)
    , _view(_room.get_view(), std::to_string(size_t(delegate)))
{
    _view.on_double_click = [this] {
        _room._room->join_channel(this->delegate);
    };
}

void Channel::add_member(const std::string& username)
{
    assert(_users.count(username) == 0);
    _users.emplace(username, std::make_unique<UserView>(_view, username));
}

void Channel::user_joined(const std::string& username)
{
    _room.inform("Channel::user_joined(", channel_id(), ", ", username, ")");
    add_member(username);
}

void Channel::user_left(const std::string& username)
{
    _room.inform("Channel::user_left(", channel_id(), ", ", username, ")");
    _users.erase(username);

    if (_users.empty()) {
        // Self destruct.
        _room._channels.erase(delegate);
    }
}

void Channel::user_authenticated(const std::string& username, const PublicKey& public_key)
{
    _room.inform("Channel::user_authenticated(", channel_id(), ", ", username, ")");
}

void Channel::user_authentication_failed(const std::string& username)
{
    _room.inform("Channel::user_authentication_failed(", channel_id(), ", ", username, ")");
}

void Channel::user_authorized_by(const std::string& user, const std::string& target)
{
    _room.inform("Channel::user_authorized_id(", channel_id(), ", ", user, ")");
}

void Channel::user_promoted(const std::string& username)
{
    _room.inform("Channel::user_promoted(", channel_id(), ", ", username, ")");
}

void Channel::joined()
{
    _room.inform("Channel::joined(", channel_id(), ")");
}

void Channel::authorized()
{
    _room.inform("Channel::authorized(", channel_id(), ")");
}


// virtual void message_received(const std::string& username, const std::string& message) = 0;

// DEBUG
void Channel::dump()
{
    _room.inform("Channel::dump(", channel_id(), ")");
}

} // np1sec_plugin namespace
