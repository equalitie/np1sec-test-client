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

#include <pidgin/gtkimhtml.h>
#include <pidgin/gtkutils.h>
#include "user_list.h"
#include "global_signals.h"

namespace np1sec_plugin {

class Channel;
class RoomView;

class ChannelView {
public:
    ChannelView(RoomView&, Channel&);
    ~ChannelView();

    void display(const std::string& username, const std::string& message);

    UserList& user_list() { return _user_list; }

private:
    static gboolean
    entry_focus_cb(GtkWidget*, GdkEventFocus*, ChannelView* self)
    {
        self->_room_view.set_channel_focus(self);
        return false;
    }

    static gboolean
    entry_nofocus_cb(GtkWidget*, GdkEventFocus*, ChannelView* self)
    {
        self->_room_view.set_channel_focus(nullptr);
        return false;
    }

private:
    RoomView& _room_view;
    PurpleConversation* _conv;
    PidginConversation* _gtkconv;

    UserList _user_list;
};

} // np1sec_plugin namespace

#include "channel.h"
#include "room_view.h"

namespace np1sec_plugin {
//------------------------------------------------------------------------------
// Implementation
//------------------------------------------------------------------------------
inline ChannelView* get_channel_view(PurpleConversation* conv) {
    auto p = purple_conversation_get_data(conv, "np1sec_channel_view");
    return reinterpret_cast<ChannelView*>(p);
}

inline void set_channel_view(PurpleConversation* conv, ChannelView* cv) {
    if (auto p = get_channel_view(conv)) {
        delete p;
    }
    purple_conversation_set_data(conv, "np1sec_channel_view", cv);
}

inline
ChannelView::ChannelView(RoomView& room_view, Channel& channel)
    : _room_view(room_view)
{
    auto conv = room_view.purple_conv();

    auto channel_name = room_view.room_name() + ":" + channel.channel_name();

    // We don't want the on_conversation_created signal to create the default
    // np1sec layout on this conversation. So disable it temporarily.
    {
        auto f = std::move(GlobalSignals::instance().on_conversation_created);

        _conv = purple_conversation_new(PURPLE_CONV_TYPE_CHAT, conv->account, channel_name.c_str());

        // Put it back.
        GlobalSignals::instance().on_conversation_created = std::move(f);
    }

    //int id = purple_conv_chat_get_id(PURPLE_CONV_CHAT(conv));
    //purple_conv_chat_set_id(PURPLE_CONV_CHAT(_conv), id);

    _gtkconv = PIDGIN_CONVERSATION(_conv);

    g_signal_connect(G_OBJECT(_gtkconv->entry), "focus-in-event",
                     G_CALLBACK(entry_focus_cb), this);
    g_signal_connect(G_OBJECT(_gtkconv->entry), "focus-out-event",
                     G_CALLBACK(entry_nofocus_cb), this);

    auto content = gtk_widget_get_parent(_gtkconv->lower_hbox);

    const gint output_window_position = 2;

    auto target = util::gtk::get_nth_child(output_window_position, GTK_CONTAINER(content));

    assert(GTK_IS_PANED(target));

    if (!target) {
        assert(target);
        return;
    }

    auto userlist = gtk_paned_get_child2(GTK_PANED(target));

    gtk_container_remove(GTK_CONTAINER(target), userlist);

    gtk_paned_pack2(GTK_PANED(target), _user_list.root_widget(), TRUE, TRUE);

    // TODO: Not sure why the value of 1 gives a good result
    gtk_widget_set_size_request(_user_list.root_widget(), 1, -1);

    set_channel_view(_conv, this);
}

inline
ChannelView::~ChannelView()
{
    set_channel_view(_conv, nullptr);

    // TODO: Is this necessary?
    if (_room_view.focused_channel() == this) {
        _room_view.set_channel_focus(nullptr);
    }
}

inline
void ChannelView::display(const std::string& sender, const std::string& message)
{
    _conv->ui_ops->write_conv(_conv,
                              sender.c_str(),
                              sender.c_str(),
                              message.c_str(),
                              PURPLE_MESSAGE_RECV,
                              time(NULL));
}

} // np1sec_plugin namespace
