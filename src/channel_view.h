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
    ChannelView(const std::shared_ptr<Room>&, Channel&);
    ~ChannelView();

    void display(const std::string& username, const std::string& message);

    template<class... Args> void inform(Args&&...);

    UserList& joined_user_list() { return _joined_users; }
    UserList& invited_user_list() { return _invited_users; }
    UserList& other_user_list() { return _other_users; }

    Channel* channel() { return _channel; }
    void reset_channel() { _channel = nullptr; }

    void send_chat_message(const std::string&);

    bool purple_conversation_destroyed = false;

private:
    static gboolean
    entry_focus_cb(GtkWidget*, GdkEventFocus*, ChannelView* self);

    static gboolean
    entry_nofocus_cb(GtkWidget*, GdkEventFocus*, ChannelView* self);

private:
    std::shared_ptr<Room> _room;
    Channel* _channel;
    PurpleConversation* _conv;
    PidginConversation* _gtkconv;

    UserList _joined_users;
    UserList _invited_users;
    UserList _other_users;

    gint focus_in_signal_id, focus_out_signal_id;
    GtkWidget *_target, *_userlist, *_vbox;
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
    purple_conversation_set_data(conv, "np1sec_channel_view", cv);
}

inline
void ChannelView::send_chat_message(const std::string& msg)
{
    assert(_channel);
    _channel->send_chat_message(msg);
}

inline
ChannelView::ChannelView(const std::shared_ptr<Room>& room, Channel& channel)
    : _room(room)
    , _channel(&channel)
    , _joined_users("Joined")
    , _invited_users("Invited")
    , _other_users("Invite")
{
    assert(_room->get_view());
    auto conv = _room->get_view()->purple_conv();

    auto channel_name = _room->room_name() + ":" + channel.channel_name();

    // We don't want the on_conversation_created signal to create the default
    // np1sec layout on this conversation. So disable it temporarily.
    {
        auto& sigs = GlobalSignals::instance();

        auto f = std::move(sigs.on_conversation_created);
        _conv = purple_conversation_new(PURPLE_CONV_TYPE_CHAT, conv->account, channel_name.c_str());
        sigs.on_conversation_created = std::move(f);
    }

    //int id = purple_conv_chat_get_id(PURPLE_CONV_CHAT(conv));
    //purple_conv_chat_set_id(PURPLE_CONV_CHAT(_conv), id);

    _gtkconv = PIDGIN_CONVERSATION(_conv);

    focus_in_signal_id
        = g_signal_connect(G_OBJECT(_gtkconv->entry), "focus-in-event",
                           G_CALLBACK(entry_focus_cb), this);

    focus_out_signal_id
        = g_signal_connect(G_OBJECT(_gtkconv->entry), "focus-out-event",
                           G_CALLBACK(entry_nofocus_cb), this);

    auto content = gtk_widget_get_parent(_gtkconv->lower_hbox);

    const gint output_window_position = 2;

    _target = util::gtk::get_nth_child(output_window_position, GTK_CONTAINER(content));

    assert(GTK_IS_PANED(_target));

    if (!_target) {
        assert(_target);
        return;
    }

    _userlist = gtk_paned_get_child2(GTK_PANED(_target));
    g_object_ref_sink(_userlist);

    gtk_container_remove(GTK_CONTAINER(_target), _userlist);

    _vbox = gtk_vbox_new(TRUE, 0);

    gtk_box_pack_start(GTK_BOX(_vbox), _joined_users.root_widget(), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(_vbox), _invited_users.root_widget(), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(_vbox), _other_users.root_widget(), TRUE, TRUE, 0);

    gtk_paned_pack2(GTK_PANED(_target), _vbox, FALSE, TRUE);
    gtk_widget_show(_vbox);

    {
        gint width, height;
        gtk_widget_get_size_request(_userlist, &width, &height);
        gtk_widget_set_size_request(_vbox, width, height);
    }

    set_channel_view(_conv, this);
}

inline
ChannelView::~ChannelView()
{
    g_signal_handler_disconnect(G_OBJECT(_gtkconv->entry), focus_in_signal_id);
    g_signal_handler_disconnect(G_OBJECT(_gtkconv->entry), focus_out_signal_id);

    gtk_container_remove(GTK_CONTAINER(_target), _vbox);
    gtk_paned_pack2(GTK_PANED(_target), _userlist, FALSE, TRUE);
    g_object_unref(_userlist);

    if (_channel) {
        assert(!_channel->_channel_view || _channel->_channel_view == this);
    }

    set_channel_view(_conv, nullptr);

    if (!purple_conversation_destroyed) {
        purple_conversation_destroyed = true;

        auto& sigs = GlobalSignals::instance();

        auto f = std::move(sigs.on_conversation_deleted);
        purple_conversation_destroy(_conv);
        sigs.on_conversation_deleted = std::move(f);
    }

    // TODO: Is this necessary?
    if (_room->focused_channel() == this) {
        _room->set_channel_focus(nullptr);
    }

    if (_channel) {
        _channel->_channel_view = nullptr;
        _channel->self_destruct();
    }
}

template<class... Args>
inline
void ChannelView::inform(Args&&... args)
{
    assert(_channel);
    display(_channel->my_username(), util::inform_str(args...));
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

inline gboolean
ChannelView::entry_focus_cb(GtkWidget*, GdkEventFocus*, ChannelView* self)
{
    self->_room->set_channel_focus(self);
    return false;
}

inline gboolean
ChannelView::entry_nofocus_cb(GtkWidget*, GdkEventFocus*, ChannelView* self)
{
    self->_room->set_channel_focus(nullptr);
    return false;
}

} // np1sec_plugin namespace
