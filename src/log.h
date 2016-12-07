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

#include <algorithm>
#include <iostream>
#include "util.h"

namespace np1sec_plugin {

template<class... Args> inline void log(Args&&... args)
{
    static boost::optional<bool> print_output;

    if (!print_output) {
        const char* e = std::getenv("NP1SEC_TEST_CLIENT_PRINT_LOG");

        if (e) {
            std::string env(e);
            std::transform(env.begin(), env.end(), env.begin(), ::tolower);
            print_output = (env == "1" || env == "true" || env == "yes");
        }
        else {
            print_output = false;
        }
    }

    if (*print_output) {
        std::cout << "np1sec_plugin: "
                  << util::str(std::forward<Args>(args)...)
                  << std::endl;
    }
}

} // np1sec_plugin namespace
