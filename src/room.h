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

/* Np1sec headers */
#include "src/interface.h"
#include "src/room.h"

/* Plugin headers */
#include "room_view.h"
#include "timer.h"
#include "toolbar.h"

#ifndef _NDEBUG
#   include "debug_proxy.h"
#endif

namespace np1sec_plugin {

class Channel;

struct Room final : private np1sec::RoomInterface {
#ifndef _NDEBUG
    using Np1SecRoom = DebugProxy;
#else
    using Np1SecRoom = np1sec::Room;
#endif

public:
    Room(PurpleConversation* conv);

    void start();
    bool started() const { return _room.get(); }
    void on_received_data(std::string sender, std::string message);

    void send_chat_message(const std::string& message);

    template<class... Args> void inform(Args&&... args);

    RoomView& get_view() { return _room_view; }

    const std::string& username() const { return _username; }

    void user_left(const std::string&);

private:
    void display(const std::string& message);
    void display(const std::string& sender, const std::string& message);

    /*
     * Callbacks for np1sec::Room
     */
    void send_message(const std::string& message) override;
    np1sec::TimerToken* set_timer(uint32_t interval_ms, np1sec::TimerCallback* callback) override;
    np1sec::ChannelInterface* new_channel(np1sec::Channel* channel) override;
    void channel_removed(np1sec::Channel* channel) override;
    void joined_channel(np1sec::Channel* channel) override;
    void disconnected() override;

    /*
     * Internal
     */
    static gboolean execute_timer_callback(gpointer);
    static std::string sanitize_name(std::string name);

    bool interpret_as_command(const std::string&);

private:
    friend class Channel;

    using ChannelMap = std::map<np1sec::Channel*, std::unique_ptr<Channel>>;

    PurpleConversation *_conv;
    PurpleAccount *_account;
    std::string _username;
    TimerToken::Storage _timers;
    np1sec::PrivateKey _private_key;

    RoomView _room_view;
    ChannelMap _channels;

    Toolbar _toolbar;

    std::unique_ptr<Np1SecRoom> _room;
};

} // np1sec_plugin namespace

/* Plugin headers */
#include "channel.h"
#include "parser.h"

namespace np1sec_plugin {
//------------------------------------------------------------------------------
// Implementation
//------------------------------------------------------------------------------
inline
Room::Room(PurpleConversation* conv)
    : _conv(conv)
    , _account(conv->account)
    , _username(sanitize_name(_account->username))
    , _private_key(np1sec::PrivateKey::generate())
    , _room_view(conv)
    , _toolbar(PIDGIN_CONVERSATION(conv))
{
    _toolbar.on_create_channel_clicked = [this] {
        _room->create_channel();
    };

    _toolbar.on_search_channel_clicked = [this] {
        _room->search_channels();
    };
}

inline
void Room::start()
{
    if (started()) return;

    _room.reset(new Np1SecRoom(this, _username, _private_key));
}

inline
void Room::send_chat_message(const std::string& message)
{
    if (interpret_as_command(message)) {
        return;
    }

    _room->send_chat(message);
}

inline
void Room::user_left(const std::string& user)
{
    _room->user_left(user);
}

inline
bool Room::interpret_as_command(const std::string& cmd)
{
    using namespace std;

    if (cmd.empty() || cmd[0] != '.') {
        return false;
    }

    Parser p(cmd.substr(1));

    try {
        inform("$ ", p.text);

        auto c = parse<string>(p);

        if (c == "help") {
            inform("<br>"
                   "Available commands:<br>"
                   "&nbsp;&nbsp;&nbsp;&nbsp;help<br>"
                   "&nbsp;&nbsp;&nbsp;&nbsp;whoami<br>"
                   "&nbsp;&nbsp;&nbsp;&nbsp;search-channels<br>"
                   "&nbsp;&nbsp;&nbsp;&nbsp;create-channel<br>"
                   "&nbsp;&nbsp;&nbsp;&nbsp;join-channel &lt;channel-id&gt;<br>");
        }
        else if (c == "whoami") {
            inform("You're ", _username);
        }
        else if (c == "search-channels") {
            _room->search_channels();
        }
        else if (c == "create-channel") {
            _room->create_channel();
        }
        else if (c == "join-channel") {
            auto channel_id_str = parse<string>(p);
            np1sec::Channel* channel = nullptr;
            try {
                channel = reinterpret_cast<np1sec::Channel*>(std::stoi(channel_id_str));

                if (_channels.count(channel) == 0) {
                    throw std::exception();
                }
            } catch(...) {
                inform("Channel id \"", channel_id_str, "\" not found");
                return true;
            }
            inform("joining channel ", channel, " ", channel_id_str);
            _room->join_channel(channel);
        }
        else  {
            inform("\"", p.text, "\" is not a valid np1sec command");
            return true;
        }
    }
    catch (...) {
        return true;
    }

    return true;
}

inline
void Room::send_message(const std::string& message)
{
    /*
     * Note: it's probably better to use this serv_chat_send function
     * instead of purple_conv_chat_send(PURPLE_CONV_CHAT(_conv), m.c_str()).
     * The latter would trigger the sending-chat-msg callback which
     * - again - calls this function. Thus we'd need an additional
     * mechanism to break this loop.
     */
    serv_chat_send( purple_conversation_get_gc(_conv)
                  , purple_conv_chat_get_id(PURPLE_CONV_CHAT(_conv))
                  , message.c_str()
                  , PURPLE_MESSAGE_SEND);
}

inline
np1sec::ChannelInterface*
Room::new_channel(np1sec::Channel* channel)
{
    auto users = channel->users();

    inform("Room::new_channel(</b>", size_t(channel),
           "<b>) with participants: ", encode_range(users));

    if (_channels.count(channel) != 0) {
        assert(0 && "Channel already present");
        return nullptr;
    }

    auto result = _channels.emplace(channel, std::make_unique<Channel>(channel, *this));

    auto& ch = result.first->second;

    for (const auto user : users) {
        ch->add_member(user);
    }

    return ch.get();
}

inline
void Room::channel_removed(np1sec::Channel* channel)
{
    _channels.erase(channel);
}

inline
void Room::joined_channel(np1sec::Channel* channel)
{
    inform("Room::joined_channel ", size_t(channel));
}

inline
void Room::disconnected()
{
    inform("Room::disconnected()");
}

template<class... Args>
inline
void Room::inform(Args&&... args)
{
    display(_username, "<b>" + util::str(std::forward<Args>(args)...) + "<b>");
}

inline
void Room::display(const std::string& message)
{
    display(_username, message);
}

inline
void Room::display(const std::string& sender, const std::string& message)
{
    /* _conv->ui_ops->write_chat isn't set (is NULL) on Pidgin. */
    assert(_conv && _conv->ui_ops && _conv->ui_ops->write_conv);
    _conv->ui_ops->write_conv(_conv,
                              sender.c_str(),
                              NULL,
                              message.c_str(),
                              PURPLE_MESSAGE_RECV,
                              0);
}

inline
void Room::on_received_data(std::string sender, std::string message)
{
    _room->message_received(sender, message);
}

inline
std::string Room::sanitize_name(std::string name)
{
    auto pos = name.find('@');
    if (pos == std::string::npos) {
        return name;
    }
    return std::string(name.begin(), name.begin() + pos);
}

inline
np1sec::TimerToken*
Room::set_timer(uint32_t interval_ms, np1sec::TimerCallback* callback) {
    return new TimerToken(_timers, interval_ms, callback);
}

} // namespace
