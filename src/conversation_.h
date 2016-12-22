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
#include "src/conversation.h"

/* np1sec_plugin headers */
#include "user_list.h"
#include "log.h"

namespace np1sec_plugin {

class Room;
class User;
class ConversationPage;
class ConversationView;

class Conversation final : public np1sec::ConversationInterface {
    using PublicKey = np1sec::PublicKey;

public:
    Conversation(np1sec::Conversation*, Room&);
    ~Conversation();

    Conversation(const Conversation&) = delete;
    Conversation& operator=(const Conversation&) = delete;

    Conversation(Conversation&&) = default;
    Conversation& operator=(Conversation&&) = default;

    User& add_user(const std::string&, const PublicKey&);
    void remove_user(const std::string&);
    User* find_user(const std::string&);
    const User* find_user(const std::string&) const;
    const std::string& my_username() const;
    std::string conversation_name() const;

    void send_chat_message(const std::string&);
    void invite(const std::string&, const PublicKey&);

    void self_destruct();

    ConversationView* conversation_view() { return _conversation_view; }

public:
    void user_invited(const std::string& inviter, const std::string& invitee) override;
    void invitation_cancelled(const std::string& inviter, const std::string& invitee) override;
    void user_authenticated(const std::string& username, const PublicKey& public_key) override;
    void user_authentication_failed(const std::string& username) override;
    void user_joining(const std::string& username) override;
    void user_left(const std::string& username) override;
    void votekick_registered(const std::string& kicker, const std::string& victim, bool kicked) override;
    
    void user_joined(const std::string& username) override;
    void message_received(const std::string& sender, const std::string& message) override;
    
    void joining() override;
    void joined() override;
    void left() override;

private:
    /*
     * Internal
     */
    size_t conversation_id() const { return size_t(_delegate); }

    template<class... Args> void inform(Args&&...);
    void interpret_as_command(const std::string& msg);
    std::map<std::string, PublicKey> get_users() const;
    User& add_user(const std::string&,
                   const PublicKey&,
                   const std::set<std::string>& participants,
                   const std::set<std::string>& invitees);

private:
    friend class User;
    friend class ConversationView;

    np1sec::Conversation* _delegate;

    Room& _room;
    std::map<std::string, std::unique_ptr<User>> _users;

    ConversationView* _conversation_view;
};

} // np1sec_plugin namespace

/* plugin headers */
#include "room.h"
#include "user.h"
#include "conversation_view.h"
#include "parser.h"

namespace np1sec_plugin {

//------------------------------------------------------------------------------
// Implementation
//------------------------------------------------------------------------------
inline Conversation::Conversation(np1sec::Conversation* delegate, Room& room)
    : _delegate(delegate)
    , _room(room)
    , _conversation_view(new ConversationView(_room.shared_from_this(), *this))
{
    log(this, " Conversation::Conversation delegate:", delegate, " room:", &room);

    auto participants = _delegate->participants();
    auto invitees     = _delegate->invitees();

    for (const auto& username_and_key : get_users()) {

        const auto& username = username_and_key.first;
        const auto& key      = username_and_key.second;

        auto& u = add_user(username, key, participants, invitees);

        u.update_view();
    }
}

inline Conversation::~Conversation()
{
    log(this, " Conversation::~Conversation start");
    _users.clear();

    if (_room.in_chat()) {
        _delegate->leave(true /* Don't want to receive the 'left' callback */);
    }

    if (_conversation_view) {
        _conversation_view->reset_conversation();
    }

    delete _conversation_view;
    log(this, " Conversation::~Conversation end");
}

inline
void Conversation::send_chat_message(const std::string& msg)
{
    if (msg.substr(0, 1) == ".") {
        return interpret_as_command(msg.substr(1));
    }

    if (!_delegate->joined()) {
        return inform("You have not joined the conversation yet");
    }

    _delegate->send_chat(msg);
}

inline
std::map<std::string, np1sec::PublicKey> Conversation::get_users() const
{
    return _room._room->users();
}

inline
void Conversation::interpret_as_command(const std::string& cmd)
{
    using namespace std;

    Parser p(cmd);

    try {
        inform("$ ", p.text);

        auto c = p.read_word();

        if (c == "help") {
            inform("<br>"
                   "Available commands:<br>"
                   "&nbsp;&nbsp;&nbsp;&nbsp;help<br>"
                   "&nbsp;&nbsp;&nbsp;&nbsp;whoami<br>"
                   "&nbsp;&nbsp;&nbsp;&nbsp;list-users<br>");
        }
        else if (c == "whoami") {
            inform("You're ", my_username());
        }
        else if (c == "list-users") {
            inform("Users: ", util::collection(get_users() | boost::adaptors::map_keys));
        }
        else if (c == "list-participants") {
            inform("Users: ", util::collection(_delegate->participants()));
        }
        else if (c == "invite") {
            auto who = p.read_word();
            auto users = get_users();
            auto ui = users.find(who);

            if (ui == users.end()) {
                return inform("No such user \"", who, "\"");
            }

            invite(ui->first, ui->second);
        } 
        else if (c == "join") {
            _delegate->join();
        }
        else  {
            inform("\"", p.text, "\" is not a valid np1sec command");
        }
    }
    catch (...) {}
}

inline
void Conversation::invite(const std::string& invitee, const PublicKey& pubkey) {
    inform("Conversation::invite ", invitee);
    _delegate->invite(invitee, pubkey);
}

inline std::string Conversation::conversation_name() const
{
    return std::to_string(size_t(_delegate));
}

inline const std::string& Conversation::my_username() const
{
    return _room.username();
}

inline
void Conversation::user_invited(const std::string& inviter, const std::string& invitee)
{
    inform("Conversation::user_invited ", invitee, " by ", inviter);
    if (auto u = find_user(invitee)) {
        u->mark_as_invited();
    }
}

inline
void Conversation::invitation_cancelled(const std::string& inviter, const std::string& invitee)
{
    inform("Conversation::invitation_cancelled ", inviter, " ", invitee);
    if (auto u = find_user(invitee)) {
        if (_delegate->invitees().count(invitee) == 0) {
            u->mark_as_not_invited();
        }
    }
}

inline
void Conversation::self_destruct()
{
    _room._conversations.erase(_delegate);
}

inline User& Conversation::add_user( const std::string& username
                                   , const PublicKey& pubkey)
{
    return add_user(username,
                    pubkey,
                    _delegate->participants(),
                    _delegate->invitees());
}

inline User& Conversation::add_user( const std::string& username
                                   , const PublicKey& pubkey
                                   , const std::set<std::string>& participants
                                   , const std::set<std::string>& invitees)
{
    assert(_users.count(username) == 0);

    auto u = new User(*this, username, pubkey);
    auto i = _users.emplace(username, std::unique_ptr<User>(u));

    if (participants.count(username)) {
        u->mark_joining();
    }

    if (invitees.count(username)) {
        u->mark_as_invited();
    }

    if (username == my_username() && _delegate->joined()) {
        u->mark_joined();
    }

    return *(i.first->second.get());
}

inline void Conversation::remove_user(const std::string& username)
{
    _users.erase(username);

    if (_users.empty()) {
        return self_destruct();
    }
}

inline
void Conversation::user_joining(const std::string& username)
{
    inform("Conversation::user_joining(", username, ")");

    auto ui = _users.find(username);

    assert(ui != _users.end());

    if (ui == _users.end()) {
        return inform("Unknown user \"", username, "\" joine conversation");
    }

    ui->second->mark_joining();
}

inline
void Conversation::user_left(const std::string& username)
{
    inform("Conversation::user_left(", username, ")");

    if (auto u = find_user(username)) {
        u->mark_not_joined();
    }
}

inline
void Conversation::votekick_registered(const std::string& kicker, const std::string& victim, bool kicked)
{
    inform("Conversation::votekick_registered(", kicker, " ", victim, " ", kicked);
}

inline
void Conversation::user_authenticated(const std::string& username, const PublicKey& public_key)
{
    inform("TODO: Conversation::user_authenticated(", username, ")");
}

inline
void Conversation::user_authentication_failed(const std::string& username)
{
    inform("Conversation::user_authentication_failed(", username, ")");
}

inline void Conversation::joining()
{
    /* This function is a bit useless, it is called right after
     * the 'user_joining(user == myself)` function. So everything
     * necessary can be done there instead. */
}

template<class... Args>
inline
void Conversation::inform(Args&&... args)
{
    log(this, " Conversation: ", util::str(args...));
    _conversation_view->display(my_username(), util::inform_str(args...));
}

inline
void Conversation::message_received(const std::string& username, const std::string& message)
{
    _conversation_view->display(username, message);
}

inline
void Conversation::user_joined(const std::string& username)
{
    inform("Conversation::user_joined(", username, ")");
    if (auto u = find_user(username)) {
        u->mark_joined();
    }
}

inline
void Conversation::joined()
{
    inform("Conversation::joined()");

    /* Can't just mark myself as being in chat and call it a day because
     * only now I can determine whether other participants are in
     * chat or not. */
    for (const auto& p : _delegate->participants()) {
        auto u = find_user(p);
        assert(u);
        if (u && _delegate->participant_joined(p)) {
            u->mark_joined();
        }
    }
}

inline void Conversation::left()
{
    inform("Conversation::left()");
    if (auto u = find_user(my_username())) {
        if (_conversation_view && u->never_started_joining()) {
            _conversation_view->close_window();
        }
    }
    self_destruct();
}

inline
User* Conversation::find_user(const std::string& user) {
    auto user_i = _users.find(user);
    if (user_i == _users.end()) return nullptr;
    return user_i->second.get();
}

const User* Conversation::find_user(const std::string& user) const {
    auto user_i = _users.find(user);
    if (user_i == _users.end()) return nullptr;
    return user_i->second.get();
}

} // np1sec_plugin namespace
