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

#include <future>

/* Np1sec headers */
#include "src/interface.h"
#include "src/room.h"

#include "util.h"

namespace np1sec_plugin {

/* This is a structure that behaves exactly as the np1sec::Room
 * except it reports when a function takes too long to execute.
 */
class DebugProxy {
    using clock = std::chrono::steady_clock;

public:
    DebugProxy( np1sec::RoomInterface* ri
              , std::string username
              , const np1sec::PrivateKey& k)
        : _username(username)
    {
        auto msg = _username + ":np1sec::Room";
        util::exec(msg.c_str(), [&] { delegate.reset(new np1sec::Room(ri, username, k)); });
    }

    void join_channel(np1sec::Channel* c) {
        auto msg = _username + ":np1sec::Room::join_channel";
        util::exec(msg.c_str(), [&] { delegate->join_channel(c); });
    }

    void create_channel() {
        auto msg = _username + ":np1sec::Room::create_channel";
        util::exec(msg.c_str(), [&] { delegate->create_channel(); });
    }

    void search_channels() {
        auto msg = _username + ":np1sec::Room::search_channels";
        util::exec(msg.c_str(), [&] { delegate->search_channels(); });
    }

    void send_chat(const std::string& str) {
        auto msg = _username + ":np1sec::Room::send_chat";
        util::exec(msg.c_str(), [&] { delegate->send_chat(str); });
    }

    void message_received(const std::string& sender, const std::string& message) {
        auto msg = _username + ":np1sec::Room::message_received";
        util::exec(msg.c_str(), [&] { delegate->message_received(sender, message); });
    }

    void user_left(const std::string& user) {
        auto msg = _username + ":np1sec::Room::user_left";
        util::exec(msg.c_str(), [&] { delegate->user_left(user); });
    }

    void authorize(const std::string& user) {
        auto msg = _username + ":np1sec::Room::authorize";
        util::exec(msg.c_str(), [&] { delegate->authorize(user); });
    }

private:
    std::string _username;
    std::unique_ptr<np1sec::Room> delegate;

};

} // namespace
