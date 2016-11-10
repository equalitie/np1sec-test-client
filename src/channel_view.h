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
#include "util.h"
#include "popup.h"

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
    PopupActions popup_actions;

    const std::string& name() const { return _name; }

    ~ChannelView();

private:
    std::string path() const;

private:
    RoomView& _room_view;
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
    : _room_view(room)
    , _name(name)
{
    gtk_tree_store_append(_room_view._tree_store, &_iter, NULL);
    gtk_tree_store_set(_room_view._tree_store, &_iter,
                       COL_NAME, _name.c_str(),
                       -1);

    auto p = path();

    _room_view._double_click_callbacks[p] = [this] {
        if (on_double_click) on_double_click();
    };

    _room_view._show_popup_callbacks[p] = [this] (GdkEventButton* e) {
        show_popup(e, popup_actions);
    };
}

inline
std::string ChannelView::path() const
{
    return util::tree_iter_to_path(_iter, _room_view._tree_store);
}

inline ChannelView::~ChannelView()
{
    _room_view._double_click_callbacks.erase(path());
    gtk_tree_store_remove(_room_view._tree_store, &_iter);
}

} // np1sec_plugin namespace

