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

/* Np1sec headers */
#include "src/interface.h"
#include "src/room.h"

namespace np1sec_plugin {

class Channel;

struct Room final : private np1sec::RoomInterface {
    struct TimerToken final : public np1sec::TimerToken {
        guint timer_id;
        np1sec::TimerCallback* callback;

        void unset() override {
            g_source_remove(timer_id);
            timer_id = 0;
            delete this;
        }

        ~TimerToken() {
            if (timer_id != 0) g_source_remove(timer_id);
        }
    };

public:
    Room(PurpleConversation* conv);

    void start();
    bool started() const { return _room.get(); }
    void on_received_data(std::string sender, std::string message);

    void send_chat_message(const std::string& message);

    template<class... Args> void inform(Args&&... args);

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
    using ChannelMap = std::map<np1sec::Channel*, std::unique_ptr<Channel>>;

    PurpleConversation *_conv;
    PurpleAccount *_account;
    std::string _username;
    std::unique_ptr<np1sec::Room> _room;
    np1sec::PrivateKey _private_key;

    std::set<TimerToken*> _timer_tokens;
    ChannelMap _channels;
    ChannelMap::iterator _current_channel = _channels.end();
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
{
}

inline
void Room::start()
{
    if (started()) return;

    _room.reset(new np1sec::Room(this, _username, _private_key));
}

inline
void Room::send_chat_message(const std::string& message)
{
    if (interpret_as_command(message)) {
        return;
    }

    _room->send_message(message);
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
        auto c = parse<string>(p);

        if (c == "help") {
            inform("<br>"
                   "possible commands:<br>"
                   "&nbsp;&nbsp;&nbsp;&nbsp;help<br>"
                   "&nbsp;&nbsp;&nbsp;&nbsp;search-channels<br>"
                   "&nbsp;&nbsp;&nbsp;&nbsp;create-channel<br>"
                   "&nbsp;&nbsp;&nbsp;&nbsp;join-channel &lt;channel-id&gt;<br>");
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
                auto channel_id = std::stoi(channel_id_str);
                channel = reinterpret_cast<np1sec::Channel*>(channel_id);

                if (_channels.count(channel) == 0) {
                    throw std::exception();
                }
            } catch(...) {
                inform("Channel id \"", channel_id_str, "\" not found");
                return true;
            }
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
    auto m = "send_raw:" + message;
    purple_conv_chat_send(PURPLE_CONV_CHAT(_conv), m.c_str());
}

inline
np1sec::ChannelInterface*
Room::new_channel(np1sec::Channel* channel)
{
    inform("New channel: </b>", ((size_t) channel), "<b> ", encode_range(channel->users()));

    auto p = new Channel(channel, *this);
    auto result = _channels.emplace(channel, std::unique_ptr<Channel>(p));

    assert(result.second && "Already got this channel");

    return p;
}

inline
void Room::channel_removed(np1sec::Channel* channel)
{
    auto channel_i = _channels.find(channel);

    assert(channel_i != _channels.end());

    _channels.erase(channel_i);
}

inline
void Room::joined_channel(np1sec::Channel* channel)
{
    inform("Joined channel ", ((size_t) channel));
}

inline
void Room::disconnected()
{
    assert(0 && "TODO Room::disconnected");
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
    // TODO: I need to store these tokens somewhere so that I can
    //       can destroy them if neither the timer was fired nor
    //       TimerToken::unset was called (could happen when this
    //       destructs).
    auto timer_token = new TimerToken;

    auto timer_id = g_timeout_add(interval_ms, execute_timer_callback, timer_token);

    timer_token->timer_id = timer_id;
    timer_token->callback = callback;

    return timer_token;
}

inline
gboolean Room::execute_timer_callback(gpointer gp_data)
{
    auto* token = reinterpret_cast<TimerToken*>(gp_data);

    auto callback = token->callback;

    token->timer_id = 0;
    delete token;

    callback->execute();

    // Returning 0 stops the timer.
    return 0;
}

} // namespace
