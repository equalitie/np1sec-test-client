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

class RoomView;
class UserView;

class ChannelView {
    friend class RoomView;
    friend class UserView;

    enum
    {
      COL_NAME = 0,
      NUM_COLS
    };

public:
    ChannelView(RoomView& room, const std::string& name);

    ChannelView(const ChannelView&) = delete;
    ChannelView& operator=(const ChannelView&) = delete;

    ChannelView(ChannelView&&) = delete;
    ChannelView& operator=(ChannelView&&) = delete;

    std::function<void()> on_double_click;

    const std::string& name() const { return _name; }

    ~ChannelView();

private:
    std::string path() const;

private:
    RoomView& _room;
    std::string _name;
    GtkTreeIter _iter;
};

} // np1sec_plugin namespace

#include "room_view.h"

namespace np1sec_plugin {
//------------------------------------------------------------------------------
// Implementation
//------------------------------------------------------------------------------
inline ChannelView::ChannelView(RoomView& room, const std::string& name)
    : _room(room)
    , _name(name)
{
    gtk_tree_store_append(_room._tree_store, &_iter, NULL);
    gtk_tree_store_set(_room._tree_store, &_iter,
                       COL_NAME, _name.c_str(),
                       -1);

    _room._double_click_callbacks[path()] = [this] {
        if (on_double_click) on_double_click();
    };
}

inline
std::string ChannelView::path() const
{
    gchar* c_str = gtk_tree_model_get_string_from_iter
        ( GTK_TREE_MODEL(_room._tree_store)
        , const_cast<GtkTreeIter*>(&_iter));

    auto free_str = defer([c_str] { g_free(c_str); });

    return std::string(c_str);
}

inline ChannelView::~ChannelView()
{
    _room._double_click_callbacks.erase(path());
    gtk_tree_store_remove(_room._tree_store, &_iter);
}

} // np1sec_plugin namespace

