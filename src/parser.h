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

#include <sstream>

namespace np1sec_plugin {

struct Parser {
    using parse_error = std::runtime_error;

    size_t curpos = 0;
    std::string text;

    Parser(std::string text)
        : text(std::move(text))
    {
    }

    std::string read_word() {
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
};

} // np1sec_plugin namespace
