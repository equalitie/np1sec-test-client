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

#include "defer.h"

namespace np1sec_plugin {
namespace util {

inline
std::string tree_iter_to_path(const GtkTreeIter& iter, GtkTreeStore* store)
{
    gchar* c_str = gtk_tree_model_get_string_from_iter
        ( GTK_TREE_MODEL(store)
        , const_cast<GtkTreeIter*>(&iter));

    auto free_str = defer([c_str] { g_free(c_str); });

    return std::string(c_str);
}

} // util namespace
} // np1sec_plugin namespace
