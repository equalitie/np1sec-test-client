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
        exec("constructor", [&] { delegate.reset(new np1sec::Room(ri, username, k)); });
    }

    void join_channel(np1sec::Channel* c) {
        exec("join_channel", [&] { delegate->join_channel(c); });
    }

    void create_channel() {
        exec("create_channel", [&] { delegate->create_channel(); });
    }

    void search_channels() {
        exec("search_channels", [&] { delegate->search_channels(); });
    }

    void send_chat(const std::string& str) {
        exec("send_chat", [&] { delegate->send_chat(str); });
    }

    void message_received(const std::string& sender, const std::string& msg) {
        exec("message_received", [&] { delegate->message_received(sender, msg); });
    }

    void user_left(const std::string& user) {
        exec("user_left", [&] { delegate->user_left(user); });
    }

    void authorize(const std::string& user) {
        exec("authorize", [&] { delegate->authorize(user); });
    }

private:
    float diff_in_secs(clock::time_point start, clock::time_point end)
    {
        using namespace std::chrono;
        auto diff_ms = duration_cast<milliseconds>(end - start).count();
        return (float) diff_ms / 1000;
    }

    template<class F>
    void exec(const char* msg, F&& f)
    {
        auto start = clock::now();

        auto future = std::async(std::launch::async, std::move(f));
        auto status = future.wait_for(std::chrono::milliseconds(300));

        assert(status != std::future_status::deferred);

        if (status == std::future_status::timeout) {
            std::cout << _username << ": np1sec's function '"
                      << msg << "' execution takes too long"
                      << std::endl;

            future.get();
            auto end = clock::now();

            std::cout << _username << "     took: "
                      << diff_in_secs(start, end)
                      << " seconds" << std::endl;

            return;
        }

        assert(status == std::future_status::ready);
    }

private:
    std::string _username;
    std::unique_ptr<np1sec::Room> delegate;

};

} // namespace
