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

#include <boost/range/adaptor/map.hpp>
#include <experimental/optional>
#include <set>
#include <sstream>

#include "../debug.h"
#include "parser.h"

namespace np1sec_plugin { namespace mock {

// -----------------------------------------------------------------------------
class Np1Sec {
public:
    using ChannelId = std::set<std::string>;

    struct Channel {
    };

    struct TransportInterface
    {
        virtual void send(std::string) = 0;
        virtual ~TransportInterface() {}
    };

    struct RoomInterface
    {
        virtual void new_channel(const ChannelId&, Channel*) = 0;
        virtual void display(std::string sender, std::string message) = 0;
        virtual ~RoomInterface() {};
    };

    Np1Sec(std::string username,
           TransportInterface* transport_interface,
           RoomInterface* room_interface);

    void receive_handler(std::string sender, std::string message);

private:
    bool interpret_command(const std::string& command);
    void interpret_list_channels();
    void interpret_channels(std::set<ChannelId>);

private:
    std::string _username;
    std::unique_ptr<TransportInterface> _transport;
    std::unique_ptr<RoomInterface> _room;
    std::map<ChannelId, Channel> _channels;
};

// -----------------------------------------------------------------------------
// Implementation
// -----------------------------------------------------------------------------
Np1Sec::Np1Sec(std::string username,
               TransportInterface* transport_interface,
               RoomInterface* room_interface)
    : _username(std::move(username))
    , _transport(transport_interface)
    , _room(room_interface)
{
    assert(_username.size());
    _channels.emplace(ChannelId{_username}, Channel());
    _transport->send("#np1sec list-channels");
}

void Np1Sec::receive_handler(std::string sender, std::string message)
{
    using namespace std;

    bool is_command = interpret_command(message);

    if (true || !is_command) {
        _room->display(std::move(sender), std::move(message));
    }
}

bool Np1Sec::interpret_command(const std::string& message)
{
    using namespace std;

    bool is_command = true;

    try {
        if (message.find("#np1sec ") != 0)
        {
            is_command = false;
            throw std::runtime_error("not a np1sec command");
        }

        Parser parser(message.substr(8));

        auto command = parse<string>(parser);

        if (command == "list-channels")
        {
            interpret_list_channels();
        }
        else if (command == "channels") {
            interpret_channels(parse<set<ChannelId>>(parser));
        }
    }
    catch(std::runtime_error& e) {
        if (is_command) {
            std::cout << _username << ": err: " << e.what() << std::endl;
            std::cout << _username << "    " << message << std::endl;
        }
    }

    return is_command;
}

void Np1Sec::interpret_list_channels()
{
     auto channel_list = encode_range(_channels | boost::adaptors::map_keys);

    _transport->send("#np1sec channels " + channel_list);
}

void Np1Sec::interpret_channels(std::set<ChannelId> channel_ids)
{
    for (const auto& channel_id : channel_ids) {
        if (_channels.count(channel_id) == 0) {
            auto iter = _channels.emplace(channel_id, Channel()).first;
            _room->new_channel(iter->first, &iter->second);
        }
    }
}

} // np1sec_plugin namespace
} // mock namespace

