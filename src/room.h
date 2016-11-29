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
class User;

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

    GtkWindow* gtk_window() const;

    void authorize(const std::string& name) { _room->authorize(name); }

    void join_channel(np1sec::Channel*);

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
    User* find_user_in_channel(const std::string& username);

private:
    friend class Channel;

    using ChannelMap = std::map<np1sec::Channel*, std::unique_ptr<Channel>>;

    PurpleConversation *_conv;
    std::string _username;
    TimerToken::Storage _timers;
    np1sec::PrivateKey _private_key;

    RoomView _room_view;
    ChannelMap _channels;

    std::unique_ptr<Toolbar> _toolbar;

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
    , _username(sanitize_name(conv->account->username))
    , _private_key(np1sec::PrivateKey::generate())
    , _room_view(conv, _username)
    , _toolbar(new Toolbar(PIDGIN_CONVERSATION(conv)))
{
    _toolbar->add_button("Create channel", [this] {
        _room->create_channel();
        _toolbar->remove_button("Create channel");
    });
}

inline
void Room::start()
{
    if (started()) return;

    _room.reset(new Np1SecRoom(this, _username, _private_key));

    _room->search_channels();
}

inline
User* Room::find_user_in_channel(const std::string& username)
{
    for (const auto& c : _channels | boost::adaptors::map_values) {
        if (auto u = c->find_user(_username)) {
            return u;
        }
    }
    return nullptr;
}

inline
void Room::send_chat_message(const std::string& message)
{
    if (interpret_as_command(message)) {
        return;
    }

    auto* u = find_user_in_channel(_username);

    auto channel = _room_view.focused_channel();

    if (!channel) {
        /*
         * We're sending from the main room (not a channel).
         * So send as plain text
         */
        return send_message(message);
    }

    if (!u || !u->in_chat()) {
        // TODO: This should be displayed in the 'page'.
        return inform("Not in chat");
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

        auto c = p.read_word();

        if (c == "help") {
            inform("<br>"
                   "Available commands:<br>"
                   "&nbsp;&nbsp;&nbsp;&nbsp;help<br>"
                   "&nbsp;&nbsp;&nbsp;&nbsp;whoami<br>"
                   "&nbsp;&nbsp;&nbsp;&nbsp;search-channels<br>"
                   "&nbsp;&nbsp;&nbsp;&nbsp;create-channel<br>"
                   "&nbsp;&nbsp;&nbsp;&nbsp;authorize &lt;user&gt;<br>"
                   "&nbsp;&nbsp;&nbsp;&nbsp;join-channel &lt;channel-id&gt;<br>");
        }
        else if (c == "whoami") {
            inform("You're ", _username);
        }
        else if (c == "search-channels") {
            _room->search_channels();
        }
        else if (c == "authorize") {
            auto user = p.read_word();
            authorize(user);
        }
        else if (c == "create-channel") {
            _room->create_channel();
        }
        else if (c == "join-channel") {
            auto channel_id_str = p.read_word();
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
            join_channel(channel);
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
void Room::join_channel(np1sec::Channel* channel)
{
    auto ch_i = _channels.find(channel);

    if (ch_i == _channels.end()) {
        assert(0 && "Trying to join a non existent channel");
        return;
    }

    auto& ch = *ch_i->second;

    if (ch.find_user(_username)) {
        inform("Already in that channel");
        return;
    }

    _room->join_channel(channel);
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
    inform("Room::new_channel(", size_t(channel),
           ") with participants: ", channel->users());

    if (_channels.count(channel) != 0) {
        assert(0 && "Channel already present");
        return nullptr;
    }

    auto result = _channels.emplace(channel, std::make_unique<Channel>(channel, *this));
    return result.first->second.get();
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

    auto channel_i = _channels.find(channel);

    if (channel_i == _channels.end()) {
        assert(0 && "Joined a non existent channel");
        return;
    }

    auto& ch = *channel_i->second;
    ch.set_user_public_key(_username, _private_key.public_key());

    ch.mark_as_joined();

    if (channel->in_chat()) {
        ch.joined_chat();
    }
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
    display(_username, util::inform_str(std::forward<Args>(args)...));
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
                              sender.c_str(),
                              message.c_str(),
                              PURPLE_MESSAGE_RECV,
                              time(NULL));
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

inline
GtkWindow* Room::gtk_window() const {
    return GTK_WINDOW(PIDGIN_CONVERSATION(_conv)->win->window);
}

} // namespace
