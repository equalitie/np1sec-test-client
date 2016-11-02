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

/* As per libpurple documentation:
 * Failure to have #define PURPLE_PLUGINS in your source file leads
 * to very strange errors that are difficult to diagnose. Just don't
 * forget to do it!
 */

#pragma once

#include <memory>
#include <iostream>

#include "mock/np1sec.h"

namespace np1sec_plugin {

struct Conversation {
    using Self = Conversation;
    using Np1Sec = mock::Np1Sec;

public:
    Conversation(PurpleConversation* conv);

    void start();
    bool started() const { return np1sec.get(); }
    void on_received_data(std::string sender, std::string message);

private:
    static std::string sanitize_name(std::string name);
    void send(std::string message);
    void display(std::string sender, std::string message);

private:
    PurpleConversation *_conv;
    PurpleAccount *_account;
    std::string _username;
    std::unique_ptr<Np1Sec> np1sec;
};

//------------------------------------------------------------------------------
// Implementation
//------------------------------------------------------------------------------
Conversation::Conversation(PurpleConversation* conv)
    : _conv(conv)
    , _account(conv->account)
    , _username(sanitize_name(_account->username))
{
}

void Conversation::start() {
    if (started()) return;

    struct Transport : public Np1Sec::TransportInterface {
        Self& self;

        Transport(Self* self) : self(*self) {} 

        void send(std::string message) override {
            self.send(std::move(message));
        }
    };

    struct RoomActions : public Np1Sec::RoomInterface {
        Self& self;

        RoomActions(Self* self) : self(*self) {} 

        void new_channel(const Np1Sec::ChannelId& id, Np1Sec::Channel*) override {
            std::cout << self._username << ": new channel " << id << std::endl;
        }

        void display(std::string sender, std::string message) override {
            self.display(std::move(sender), std::move(message));
        }
    };

    np1sec.reset(new Np1Sec(_username,
                            new Transport(this),
                            new RoomActions(this)));
}

void Conversation::display(std::string sender, std::string message) {
    assert(_conv && _conv->ui_ops && _conv->ui_ops->write_conv);
    // _conv->ui_ops->write_chat isn't set (is NULL) on Pidgin.
    _conv->ui_ops->write_conv(_conv,
                              sender.c_str(),
                              NULL,
                              message.c_str(),
                              PURPLE_MESSAGE_RECV,
                              0);
}

void Conversation::on_received_data(std::string sender, std::string message) {
    np1sec->receive_handler(std::move(sender), std::move(message));
}

std::string Conversation::sanitize_name(std::string name) {
    auto pos = name.find('@');
    if (pos == std::string::npos) {
        return name;
    }
    return std::string(name.begin(), name.begin() + pos);
}

void Conversation::send(std::string message)
{
    purple_conv_chat_send(PURPLE_CONV_CHAT(_conv), message.c_str());
}

} // namespace
