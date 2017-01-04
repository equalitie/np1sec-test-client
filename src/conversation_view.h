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

class Conversation;
class RoomView;

class ConversationView {
public:
    ConversationView(const std::shared_ptr<Room>&, Conversation&);
    ~ConversationView();

    void display(const std::string& username, const std::string& message);

    template<class... Args> void inform(Args&&...);

    UserList& joined_user_list() { return _joined_users; }
    UserList& invited_user_list() { return _invited_users; }
    UserList& other_user_list() { return _other_users; }

    Conversation* conversation() { return _conversation; }
    void reset_conversation() { _conversation = nullptr; }

    void send_chat_message(const std::string&);

    void close_window();

private:
    static gboolean
    entry_focus_cb(GtkWidget*, GdkEventFocus*, ConversationView* self);

    static gboolean
    entry_nofocus_cb(GtkWidget*, GdkEventFocus*, ConversationView* self);

    void disconnect_focus_signals(PurpleConversation* conv);

private:
    std::shared_ptr<Room> _room;
    Conversation* _conversation;
    PurpleConversation* _conv;
    PidginConversation* _gtkconv;

    UserList _joined_users;
    UserList _invited_users;
    UserList _other_users;

    gint focus_in_signal_id, focus_out_signal_id;
    GtkWidget *_target, *_userlist, *_vbox;

    std::string _my_username;
};

} // np1sec_plugin namespace

#include "conversation_.h"
#include "room_view.h"

namespace np1sec_plugin {
//------------------------------------------------------------------------------
// Implementation
//------------------------------------------------------------------------------
inline ConversationView* get_conversation_view(PurpleConversation* conv) {
    auto p = purple_conversation_get_data(conv, "np1sec_conversation_view");
    return reinterpret_cast<ConversationView*>(p);
}

inline void set_conversation_view(PurpleConversation* conv, ConversationView* cv) {
    purple_conversation_set_data(conv, "np1sec_conversation_view", cv);
}

inline
void ConversationView::send_chat_message(const std::string& msg)
{
    if (!_conversation) {
        inform("Can't send, the conversation has bee destroyed");
        return;
    }
    _conversation->send_chat_message(msg);
}

inline
ConversationView::ConversationView(const std::shared_ptr<Room>& room, Conversation& conversation)
    : _room(room)
    , _conversation(&conversation)
    , _joined_users("Joined")
    , _invited_users("Invited")
    , _other_users("Invite")
    , _my_username(conversation.my_username())
{
    assert(_room->get_view());
    auto conv = _room->get_view()->purple_conv();

    auto conversation_name = _room->room_name() + ":" + conversation.conversation_name();

    // We don't want the on_conversation_created signal to create the default
    // np1sec layout on this conversation. So disable it temporarily.
    {
        auto& sigs = GlobalSignals::instance();

        auto f = std::move(sigs.on_conversation_created);
        _conv = purple_conversation_new(PURPLE_CONV_TYPE_CHAT, conv->account, conversation_name.c_str());
        sigs.on_conversation_created = std::move(f);
    }

    _gtkconv = PIDGIN_CONVERSATION(_conv);

    //gtk_notebook_set_current_page(GTK_NOTEBOOK(_gtkconv->win->notebook), -1);

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

    set_conversation_view(_conv, this);
}

inline
void ConversationView::disconnect_focus_signals(PurpleConversation* conv)
{
    auto gtkconv = PIDGIN_CONVERSATION(conv);
    g_signal_handler_disconnect(G_OBJECT(gtkconv->entry), focus_in_signal_id);
    g_signal_handler_disconnect(G_OBJECT(gtkconv->entry), focus_out_signal_id);
}

inline
void ConversationView::close_window()
{
    if (!_conv) return;

    auto conv = _conv;
    _conv = nullptr;

    disconnect_focus_signals(conv);

    auto& sigs = GlobalSignals::instance();
    auto f = std::move(sigs.on_conversation_deleted);
    purple_conversation_destroy(conv);
    sigs.on_conversation_deleted = std::move(f);
}

inline
ConversationView::~ConversationView()
{
    if (_conversation) {
        assert(!_conversation->_conversation_view || _conversation->_conversation_view == this);
    }

    if (_conv) {
        disconnect_focus_signals(_conv);
        gtk_container_remove(GTK_CONTAINER(_target), _vbox);
        gtk_paned_pack2(GTK_PANED(_target), _userlist, FALSE, TRUE);
        g_object_unref(_userlist);
        set_conversation_view(_conv, nullptr);
    }

    /* Note: we don't want to explicitly call close_window (or
     * purple_conversation_destroy) because in most cases it'll be
     * called implicitly from pidgin (e.g. when the user closes the
     * conversation or the main window) which would result in double
     * free. */

    // TODO: Is this necessary?
    if (_room->focused_conversation() == this) {
        _room->set_conversation_focus(nullptr);
    }

    if (_conversation) {
        _conversation->_conversation_view = nullptr;
        _conversation->self_destruct();
    }
}

template<class... Args>
inline
void ConversationView::inform(Args&&... args)
{
    display(_my_username, util::inform_str(args...));
}

inline
void ConversationView::display(const std::string& sender, const std::string& message)
{
    if (!_conv) return;

    _conv->ui_ops->write_conv(_conv,
                              sender.c_str(),
                              sender.c_str(),
                              message.c_str(),
                              PURPLE_MESSAGE_RECV,
                              time(NULL));
}

inline gboolean
ConversationView::entry_focus_cb(GtkWidget*, GdkEventFocus*, ConversationView* self)
{
    self->_room->set_conversation_focus(self);
    return false;
}

inline gboolean
ConversationView::entry_nofocus_cb(GtkWidget*, GdkEventFocus*, ConversationView* self)
{
    self->_room->set_conversation_focus(nullptr);
    return false;
}

} // np1sec_plugin namespace
