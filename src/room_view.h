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
#include "notebook.h"

#include <pidgin/gtkimhtml.h>

namespace np1sec_plugin {

class ChannelPage;
class ChannelListPage;

class ChannelList;

class RoomView {
public:
    RoomView(PurpleConversation*, const std::string& username);

    RoomView(RoomView&&) = delete;
    RoomView(const RoomView&) = delete;
    RoomView& operator=(const RoomView&) = delete;
    RoomView& operator=(RoomView&&) = delete;

    ~RoomView();

    ChannelList& channel_list();
    Notebook& notebook() { return _notebook; }

    GtkIMHtml* input_imhtml();

    ChannelPage* current_channel_page() { return _current_channel_page; }

private:
    friend class ChannelPage;

private:
    const std::string _username;

    PurpleConversation* _conv;
    PidginConversation* _gtkconv;

    Notebook _notebook; // Where the tabs are
    std::unique_ptr<ChannelListPage> _channel_list_page;

    GtkWidget* _content;
    GtkWidget* _parent;
    GtkWidget* _input_widget;
    GtkWidget* _default_output_imhtml;

    ChannelPage* _current_channel_page = nullptr;
};

} // np1sec_plugin namespace

#include "channel_list.h"
#include "channel_list_page.h"

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

    _default_output_imhtml = _gtkconv->imhtml;

    g_object_ref(_content);
    _parent = gtk_widget_get_parent(_content);
    gtk_container_remove(GTK_CONTAINER(_parent), GTK_WIDGET(_content));
	gtk_container_add(GTK_CONTAINER(_parent), _notebook.root_widget());

    _input_widget = util::gtk::get_nth_child(4, GTK_CONTAINER(_content));
    g_object_ref(_input_widget);
    gtk_container_remove(GTK_CONTAINER(_content), _input_widget);
    gtk_box_pack_end(GTK_BOX(_parent), _input_widget, FALSE, FALSE, 0);

    _channel_list_page.reset(new ChannelListPage(*this, GTK_BOX(_content), _default_output_imhtml));
}

inline
ChannelList& RoomView::channel_list()
{
    return _channel_list_page->channel_list();
}

inline
GtkIMHtml* RoomView::input_imhtml()
{
    return GTK_IMHTML(_gtkconv->entry);
}

inline RoomView::~RoomView()
{
    _gtkconv->imhtml = _default_output_imhtml;

    if (auto p = gtk_widget_get_parent(_input_widget)) {
        gtk_container_remove(GTK_CONTAINER(p), _input_widget);
    }

	gtk_box_pack_start(GTK_BOX(_content), _input_widget, FALSE, TRUE, 0);

    gtk_container_remove(GTK_CONTAINER(_parent), _notebook.root_widget());
    _channel_list_page.reset();
    gtk_container_add(GTK_CONTAINER(_parent), _content);

    g_object_unref(_content);
    g_object_unref(_input_widget);
}

} // np1sec_plugin namespace

