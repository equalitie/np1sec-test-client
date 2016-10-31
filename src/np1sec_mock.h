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

#include <experimental/optional>
#include <set>
#include <sstream>

#include "debug.h"

namespace np1sec_plugin {

namespace stdx = std::experimental;

struct Parser {
    using parse_error = std::runtime_error;

    size_t curpos = 0;
    std::string text;

    Parser(std::string text)
        : text(std::move(text))
    {
    }

    bool finished() const { return curpos == text.size(); }

    std::string read_command() {
        auto thrw = [](const char* err) {
            throw std::runtime_error(std::string("read_command: ") + err);
        };
        if (curpos == text.size()) thrw("attempt to read past the end");
        if (text[curpos] == ' ') thrw("command starts with space");

        auto p = text.find(' ', curpos);

        if (p != std::string::npos) {
            auto retval = text.substr(curpos, (p - curpos));
            curpos = p + 1;
            return retval;
        }

        auto retval = text.substr(curpos, text.size() - curpos);
        curpos = text.size();
        return retval;
    }

    // Example input: "(alice bob) charlie"
    //         output: "alice bob"
    std::string read_list() {
        auto thrw = [](const char* err) {
            throw std::runtime_error(std::string("parse_list: ") + err);
        };

        if (curpos == text.size()) thrw("attempt to read past the end");
        if (text[curpos] != '(') thrw("list doesn't start with '('");
        ++curpos;
        auto start = curpos;
        if (curpos == text.size()) thrw("input ends after '('");

        for (; curpos < text.size(); ++curpos) {
            if (text[curpos] == '(') {
                read_list();
            }
            if (text[curpos] == ')') {
                auto p = curpos++;
                consume_white_space();
                return text.substr(start, p - start);
            }
        } 

        thrw("input ends before ')'");
        return "";
    }

    void consume_white_space() {
        while (curpos < text.size() && text[curpos] == ' ') ++curpos;
    }
};

// -----------------------------------------------------------------------------
class Np1SecMock {
public:
    struct Channel;

    struct TransportInterface
    {
        virtual void send(std::string) = 0;
        virtual ~TransportInterface() {}
    };

    struct RoomInterface
    {
        virtual void new_channel(Channel*) = 0;
        virtual ~RoomInterface() {};
    };

    Np1SecMock(std::string username,
               TransportInterface* transport_interface,
               RoomInterface* room_interface);

    void receive_handler(std::string sender, std::string message);

private:
    std::string construct_channel_list();

private:
    std::string _username;
    std::unique_ptr<TransportInterface> _transport;
    std::unique_ptr<RoomInterface> _room;
    std::set<std::set<std::string>> _channels;
};

// -----------------------------------------------------------------------------
// Implementation
// -----------------------------------------------------------------------------
template<class T>
T unwrap(stdx::optional<T>&& opt) {
    if (!opt) throw std::runtime_error("parse error");
    return std::move(*opt);
}

// -----------------------------------------------------------------------------
Np1SecMock::Np1SecMock(std::string username,
                       TransportInterface* transport_interface,
                       RoomInterface* room_interface)
    : _username(std::move(username))
    , _transport(transport_interface)
    , _room(room_interface)
{
    assert(_username.size());
    _channels.insert(std::set<std::string>{_username});
    _transport->send("#np1sec list-channels");
}

void Np1SecMock::receive_handler(std::string sender, std::string message)
{
    std::cout << "receive handler " << sender << " " << message << std::endl;


    try {
        if (message.find("#np1sec ") != 0) {
            throw std::runtime_error("not a np1sec command");
        }

        Parser parser(message.substr(8));

        auto command = parser.read_command();

        std::cout << "cmd: " << command << std::endl;
        if (command == "list-channels") {
            _transport->send("#np1sec channels " + construct_channel_list());
        }
        else if (command == "channels") {
            Parser channels_parser(parser.read_list());
            while (!channels_parser.finished()) {
                Parser channel_parser(channels_parser.read_list());
                while (!channel_parser.finished()) {
                    auto user = channel_parser.read_command();
                }
            }
        }
    }
    catch(std::runtime_error& e) {
        std::cout << "err: " << e.what() << std::endl;
    }
}

// Constructs a sting in this form: "((alice bob) (charlie) (dave eve filip))"
std::string Np1SecMock::construct_channel_list() {
    std::stringstream ss;
    ss << "(";
    for (auto i = _channels.begin(); i != _channels.end(); ++i) {
        if (i != _channels.begin()) ss << " ";
        ss << "(";
        for (auto j = i->begin(); j != i->end(); ++j) {
            if (j != i->begin()) ss << " ";
            ss << *j;
        }
        ss << ")";
    }
    ss << ")";

    return ss.str();
}

} // namespace
