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
#include <queue>

/* Np1sec headers */
#include "src/interface.h"
#include "src/room.h"

/* Plugin headers */
#include "timer.h"
#include "toolbar.h"

#ifndef _NDEBUG
#   include "debug_proxy.h"
#endif

#include "user_list.h"

namespace np1sec_plugin {

class Channel;
class ChannelView;
class User;
class RoomView;

struct Room final : private np1sec::RoomInterface
                  , public std::enable_shared_from_this<Room> {
    using PublicKey = np1sec::PublicKey;

#if false && !defined(_NDEBUG)
    using Np1SecRoom = DebugProxy;
#else
    using Np1SecRoom = np1sec::Room;
#endif

public:
    Room(PurpleConversation* conv);
    ~Room();

    void start();
    bool started() const { return _room.get(); }
    void on_received_data(std::string sender, std::string message);

    void send_chat_message(const std::string& message);

    template<class... Args> void inform(Args&&... args);

    void set_view(RoomView* v) { _room_view = v; }
    RoomView* get_view() { return _room_view; }

    const std::string& username() const { return _username; }

    void user_left(const std::string&);

    GtkWindow* gtk_window() const;

    void set_channel_focus(ChannelView*);
    ChannelView* focused_channel() const;

    std::string room_name() const;

    void close_all_channels();

private:
    void display(const std::string& message);
    void display(const std::string& sender, const std::string& message);

    /*
     * Callbacks for np1sec::Room
     */
    void send_message(const std::string& message) override;
    np1sec::TimerToken* set_timer(uint32_t interval_ms, np1sec::TimerCallback* callback) override;


    void connected() override;
    void disconnected() override;

    void user_joined(const std::string& username, const PublicKey&) override;
    void user_left(const std::string& username, const PublicKey&) override;

    np1sec::ConversationInterface* created_conversation(np1sec::Conversation*) override;
    np1sec::ConversationInterface* invited_to_conversation(np1sec::Conversation*, const std::string&) override;

    /*
     * Internal
     */
    static gboolean execute_timer_callback(gpointer);
    static std::string sanitize_name(std::string name);

    bool interpret_as_command(const std::string&);
    User* find_user_in_channel(const std::string& username);
    void add_user(const std::string& username, const PublicKey&);
    void remove_user(const std::string& username);

private:
    friend class Channel;

    using ChannelMap = std::map<np1sec::Conversation*, std::unique_ptr<Channel>>;

    PurpleConversation *_conv;
    std::string _username;
    TimerToken::Storage _timers;
    np1sec::PrivateKey _private_key;

    RoomView* _room_view = nullptr;
    ChannelMap _channels;
    std::map<std::string, std::unique_ptr<UserList::User>> _users;

    std::unique_ptr<Toolbar> _toolbar;

    std::unique_ptr<Np1SecRoom> _room;

    ChannelView* _focused_channel = nullptr;

    struct Invitee {
        std::string name;
        PublicKey public_key;

        bool operator<(const Invitee& other) const {
            return std::tie(name, public_key)
                 < std::tie(other.name, other.public_key);
        }
    };

    std::queue<std::set<Invitee>> _invite_queue;
};

} // np1sec_plugin namespace

/* Plugin headers */
#include "channel.h"
#include "parser.h"
#include "room_view.h"

namespace np1sec_plugin {
//------------------------------------------------------------------------------
// Implementation
//------------------------------------------------------------------------------
inline
Room::Room(PurpleConversation* conv)
    : _conv(conv)
    , _username(sanitize_name(conv->account->username))
    , _private_key(np1sec::PrivateKey::generate(true))
    , _toolbar(new Toolbar(PIDGIN_CONVERSATION(conv)))
{
    _toolbar->add_button("Create conversation", [this] {
        _room->create_conversation();
    });
}

inline
void Room::start()
{
    if (started()) return;

    _room.reset(new Np1SecRoom(this, _username, _private_key));
    _room->connect();
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
Room::~Room()
{
}

inline
void Room::close_all_channels()
{
    for (auto& c : _channels | boost::adaptors::map_values) {
        c->channel_view()->purple_conversation_destroyed = true;
    }
    _channels.clear();
}

inline
void Room::send_chat_message(const std::string& message)
{
    auto channel_view = focused_channel();

    if (!channel_view) {
        if (interpret_as_command(message)) {
            return;
        }
        /*
         * We're sending from the main room (not a channel).
         * So send as plain text
         */
        return send_message(message);
    }

    channel_view->send_chat_message(message);
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
                   "&nbsp;&nbsp;&nbsp;&nbsp;create-conversation<br>");
        }
        else if (c == "whoami") {
            inform("You're ", _username);
        }
        else if (c == "create-conversation") {
            _room->create_conversation();
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
    if (!_room_view) {
        // TODO: Inform the user that the main chat window has been closed.
        return;
    }

    auto conv = _room_view->purple_conv();

    /*
     * Note: it's probably better to use this serv_chat_send function
     * instead of purple_conv_chat_send(PURPLE_CONV_CHAT(conv), m.c_str()).
     * The latter would trigger the sending-chat-msg callback which
     * - again - calls this function. Thus we'd need an additional
     * mechanism to break this loop.
     */
    serv_chat_send( purple_conversation_get_gc(conv)
                  , purple_conv_chat_get_id(PURPLE_CONV_CHAT(conv))
                  , message.c_str()
                  , PURPLE_MESSAGE_SEND);
}

inline
void Room::add_user(const std::string& username, const PublicKey& pubkey)
{
    if (_users.count(username)) {
        assert(0 && "User is already in the room");
        return;
    }

    auto u = new UserList::User();

    if (auto v = get_view()) {
        u->bind(v->user_list());
    }

    _users[username] = std::unique_ptr<UserList::User>(u);

    if (username == _username) {
        u->set_text(username + " (self)");
    }
    else {
        u->set_text(username);
        
        auto invite = [this, username, pubkey] {
            _invite_queue.emplace(std::set<Invitee>{Invitee{username, pubkey}});
            _room->create_conversation();
        };

        u->popup_actions["Invite"] = invite;
        u->on_double_click = invite;
    }

    for (auto& c : _channels | boost::adaptors::map_values) {
        c->add_user(username, pubkey);
    }
}

inline
void Room::user_joined(const std::string& username, const PublicKey& pubkey)
{
    inform("Room::user_joined ", username);
    add_user(username, pubkey);
}

inline
void Room::remove_user(const std::string& username)
{
    _users.erase(username);

    for (auto& c : _channels | boost::adaptors::map_values) {
        c->remove_user(username);
    }
}

inline
void Room::user_left(const std::string& username, const PublicKey&)
{
    inform("Room::user_left ", username, " (event from np1sec)");
    remove_user(username);
}

inline
void Room::user_left(const std::string& username)
{
    inform("Room::user_left ", username, " (event from pidgin)");
    _room->user_left(username);
    remove_user(username);
}

inline
np1sec::ConversationInterface*
Room::created_conversation(np1sec::Conversation* c)
{
    inform("Room::created_conversation ", size_t(c));

    if (_channels.count(c) != 0) {
        assert(0 && "Conversation already present");
        return nullptr;
    }

    auto channel = new Channel(c, *this);
    _channels.emplace(c, std::unique_ptr<Channel>(channel));

    if (!_invite_queue.empty()) {
        auto invitees = std::move(_invite_queue.front());
        _invite_queue.pop();

        for (const auto& invitee : invitees) {
            channel->invite(invitee.name, invitee.public_key);
        }
    }

    return channel;
}

inline
np1sec::ConversationInterface*
Room::invited_to_conversation(np1sec::Conversation* c, const std::string& by)
{
    inform("Room::invited_to_conversation by ", by);

    if (_channels.count(c) != 0) {
        assert(0 && "Conversation already present");
        return nullptr;
    }

    auto p = new Channel(c, *this);
    _channels.emplace(c, std::unique_ptr<Channel>(p));
    return p;
}

inline
void Room::connected()
{
    inform("Room::connected()");
    add_user(_username, _private_key.public_key());
}

inline
void Room::disconnected()
{
    inform("Room::disconnected()");
    _users.erase(_username);
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
    if (!_room_view) return;

    // TODO: Move the below code into RoomView.

    auto conv = _room_view->purple_conv();

    /* conv->ui_ops->write_chat isn't set (is NULL) on Pidgin. */
    assert(conv && conv->ui_ops && conv->ui_ops->write_conv);
    conv->ui_ops->write_conv(conv,
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
    if (!_room_view) return nullptr;
    auto conv = _room_view->purple_conv();
    return GTK_WINDOW(PIDGIN_CONVERSATION(conv)->win->window);
}

inline
ChannelView* Room::focused_channel() const
{
    return _focused_channel;
}

inline
void Room::set_channel_focus(ChannelView* cv)
{
    _focused_channel = cv;
}

inline
std::string Room::room_name() const
{
    return _conv->title;
}

} // namespace
