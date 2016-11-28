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

#include <pidgin/gtkimhtml.h>

namespace np1sec_plugin {

class ChannelList;

class RoomView {
public:
    RoomView(PurpleConversation*, const std::string& username);
    ~RoomView();

    RoomView(RoomView&&) = delete;
    RoomView(const RoomView&) = delete;
    RoomView& operator=(const RoomView&) = delete;
    RoomView& operator=(RoomView&&) = delete;

    ChannelList& channel_list();

    PurpleConversation* purple_conv() { return _conv; }

private:
    const std::string _username;

    PurpleConversation* _conv;
    PidginConversation* _gtkconv;

    std::unique_ptr<ChannelList> _channel_list;

    GtkWidget* _content;
    GtkWidget* _parent;

    /*
     * This is a pointer to the widget that holds the output
     * window and the user list. We modify it in the constructor
     * and will need to change it back in the destructor.
     */
    GtkWidget* _target;
    GtkWidget* _vpaned;
    GtkWidget* _userlist;
};

} // np1sec_plugin namespace

#include "channel_list.h"

namespace np1sec_plugin {

//------------------------------------------------------------------------------
// Implementation
//------------------------------------------------------------------------------
inline RoomView::RoomView(PurpleConversation* conv, const std::string& username)
    : _username(username)
{
    // TODO: Throw instead of assert.

    _conv = conv;
    _gtkconv = PIDGIN_CONVERSATION(conv);

    if (!_gtkconv) {
        assert(0 && "Not a pidgin conversation");
        return;
    }

    _content = gtk_widget_get_parent(_gtkconv->lower_hbox);

    if (!GTK_IS_CONTAINER(_content)) {
        assert(0);
        return;
    }

    if (!GTK_IS_BOX(_content)) {
        assert(0);
        return;
    }

    _parent = gtk_widget_get_parent(_content);

    /*
     * The widget with the chat output window is at this position
     * in the VBox as coded in pidin/gtkconv.c/setup_common_pane.
     * I haven't found a better way to find that window unfortunatelly
     * other than this hardcoded position.
     * TODO: I have a hunch that for the IM window (as opposed to
     * Group Chat window) this number is going to be different.
     */
    const gint output_window_position = 2;

    _target = util::gtk::get_nth_child(output_window_position, GTK_CONTAINER(_content));

    assert(GTK_IS_PANED(_target));

    if (!_target) {
        assert(_target);
        return;
    }

    _userlist = gtk_paned_get_child2(GTK_PANED(_target));
    g_object_ref_sink(_userlist);

    gtk_container_remove(GTK_CONTAINER(_target), _userlist);

    _vpaned = gtk_vpaned_new();
    g_object_ref_sink(_vpaned);

    gtk_paned_pack2(GTK_PANED(_target), _vpaned, TRUE, TRUE);
    gtk_paned_pack1(GTK_PANED(_vpaned), _userlist, TRUE, TRUE);

    _channel_list.reset(new ChannelList());
    gtk_paned_pack2(GTK_PANED(_vpaned), _channel_list->root_widget(), TRUE, TRUE);

    // TODO: Not sure why the value of 1 gives a good result
    gtk_widget_set_size_request(_vpaned, 1, -1);
    gtk_widget_show(_vpaned);
}

inline
RoomView::~RoomView()
{
    gtk_container_remove(GTK_CONTAINER(_vpaned), _userlist);
    gtk_container_remove(GTK_CONTAINER(_target), _vpaned);

    gtk_paned_pack2(GTK_PANED(_target), _userlist, FALSE, TRUE);

    g_object_unref(_userlist);
    g_object_unref(_vpaned);
}

inline
ChannelList& RoomView::channel_list()
{
    return *_channel_list;
}

} // np1sec_plugin namespace

