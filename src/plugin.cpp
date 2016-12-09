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

/* As per libpurple documentation:
 * Failure to have #define PURPLE_PLUGINS in your source file leads
 * to very strange errors that are difficult to diagnose. Just don't
 * forget to do it!
 */
#define PURPLE_PLUGINS

#include <string.h>
#include <boost/algorithm/string/predicate.hpp>

/* Purple headers */
#include <pidgin/pidgin.h>
#include <libpurple/notify.h>
#include <libpurple/version.h>
#include <libpurple/util.h>
#include <libpurple/debug.h>
#include <libpurple/core.h>

/* Purple GTK headers */
#include <pidgin/gtkplugin.h>
#include <pidgin/gtkconv.h>

/* Plugin headers */
#include "room.h"
#include "global_signals.h"

using Room = np1sec_plugin::Room;

static bool is_chat(PurpleConversation* conv) {
    return conv && conv->type == PURPLE_CONV_TYPE_CHAT;
}

//------------------------------------------------------------------------------
static Room* get_room(PurpleAccount* account)
{
    return reinterpret_cast<Room*>(account->ui_data);
}

static void set_room(PurpleAccount* account, Room* room)
{
    account->ui_data = room;
}

static Room* get_room(PurpleConversation* conv)
{
    auto account = purple_conversation_get_account(conv);
    return get_room(account);
}

static void set_room(PurpleConversation* conv, Room* room)
{
    auto account = purple_conversation_get_account(conv);
    set_room(account, room);
}

//------------------------------------------------------------------------------
static bool is_in_chat(PurpleConversation* conv) {
    auto chat = PURPLE_CONV_CHAT(conv);
    return purple_conv_chat_get_id(chat) != 0
        && !purple_conv_chat_has_left(chat);
}

//------------------------------------------------------------------------------

extern "C" {

#define _(x) const_cast<char*>(x)

//------------------------------------------------------------------------------
static void chat_joined_cb(PurpleConversation* conv, void*)
{
    /*
     * Note: We can't call room->joined_chat() here because
     * at this time PURPLE_CONV_CHAT(conv)->nick is still set to
     * the XMPP JID and not the OccupantID. The latter is what
     * we get when a message arrives so it must also be
     * the OccupantID that we initialize np1sec::Room with.
     *
     * As a consequence we call the room->joined_chat()
     * in the chat-buddy-joined callback.
     */
}

static void chat_left_cb(PurpleConversation* conv, void*)
{
    auto room = get_room(conv);
    assert(room);
    room->chat_left();
}

static gboolean receiving_chat_msg_cb(PurpleAccount *account,
                                      char** sender,
                                      char** message,
                                      PurpleConversation* conv,
                                      PurpleMessageFlags *flags)
{
    using namespace np1sec_plugin;

    if (!is_chat(conv)) return FALSE;

    auto room = get_room(conv);
    assert(room);
    if (!room) return FALSE;

    static const std::string np1sec_header = ":o3np1sec0:";

    if (std::string(*message).substr(0, np1sec_header.size()) != np1sec_header) {
        return FALSE;
    }

    // Ignore historic messages.
    if (*flags & PURPLE_MESSAGE_DELAYED) return TRUE;

    room->on_received_data(util::normalize_name(account, *sender), *message);

    // Returning TRUE causes this message not to be displayed.
    // Displaying is done explicitly from np1sec.
    return TRUE;
}

static
void sending_chat_msg_cb(PurpleAccount *account, char **message, int id, void*) {
    auto room = get_room(account);

    if (!room) return;

    room->send_chat_message(*message);

    g_free(*message);
    *message = NULL;
}

static
void chat_buddy_joined_cb(PurpleConversation *conv, const char *name,
                          PurpleConvChatBuddyFlags flags,
                          gboolean new_arrival)
{
    if (!is_chat(conv)) return;

    auto room = get_room(conv);
    if (!room) return;

    // Note the comment in the chat_joined_cb function.
    room->chat_joined();
}

static
void chat_buddy_left_cb(PurpleConversation* conv, const char* name, const char*, void*)
{
    if (!is_chat(conv)) return;

    auto room = get_room(conv);
    assert(room);
    if (room) room->user_left(name);
}

//------------------------------------------------------------------------------
static void  apply_np1sec(PurpleConversation* conv)
{
    using namespace np1sec_plugin;

    assert(is_chat(conv));
    assert(!get_room(conv));

    auto room = std::make_shared<Room>(conv);
    auto room_view = new RoomView(conv, room);

    set_room_view(conv, room_view);
    set_room(conv, room.get());

    // This only happens when we enable the plugin and
    // conversations have already been created.
    if (is_in_chat(conv)) {
        room->chat_joined();
    }
}

static void unapply_np1sec(PurpleConversation* conv)
{
    using np1sec_plugin::log;

    assert(is_chat(conv));

    auto channel_view = np1sec_plugin::get_channel_view(conv);
    auto room_view    = np1sec_plugin::get_room_view(conv);

    log("unapply_np1sec conv:", conv, " rv:", room_view, " cv:", channel_view, " start");

    /*
     * A pidgin conversation can't at the same time represent a room and
     * a channel.
     */
    assert(!(channel_view && room_view));

    /*
     * It is important here to first set the channel view to null to
     * before deleting the channel view. This way we tell the channel view
     * not to call purple_conversation_destroy recursively.
     */
    np1sec_plugin::set_channel_view(conv, nullptr);
    np1sec_plugin::set_room_view(conv, nullptr);

    if (room_view) {
        set_room(conv, nullptr);
    }

    delete channel_view;
    delete room_view;

    log("unapply_np1sec end");
}

//------------------------------------------------------------------------------
static ::np1sec_plugin::GlobalSignals* g_signals = nullptr;

static void setup_purple_callbacks(PurplePlugin* plugin)
{
    assert(!g_signals);
    g_signals = new ::np1sec_plugin::GlobalSignals();

    g_signals->on_conversation_created = [](PurpleConversation* conv) {
        if (!is_chat(conv)) return;
        apply_np1sec(conv);
    };

    g_signals->on_conversation_deleted = [](PurpleConversation* conv) {
        if (!is_chat(conv)) return;
        unapply_np1sec(conv);
    };

    void* conv_handle = purple_conversations_get_handle();

	purple_signal_connect(conv_handle, "chat-buddy-joined", plugin, PURPLE_CALLBACK(chat_buddy_joined_cb), NULL);
	purple_signal_connect(conv_handle, "chat-buddy-left", plugin, PURPLE_CALLBACK(chat_buddy_left_cb), NULL);
	purple_signal_connect(conv_handle, "chat-joined", plugin, PURPLE_CALLBACK(chat_joined_cb), NULL);
	purple_signal_connect(conv_handle, "chat-left", plugin, PURPLE_CALLBACK(chat_left_cb), NULL);
    purple_signal_connect(conv_handle, "receiving-chat-msg", plugin, PURPLE_CALLBACK(receiving_chat_msg_cb), NULL);
    purple_signal_connect(conv_handle, "sending-chat-msg", plugin, PURPLE_CALLBACK(sending_chat_msg_cb), NULL);
}

static void disconnect_purple_callbacks(PurplePlugin* plugin)
{
    delete g_signals;
    g_signals = nullptr;

    void* conv_handle = purple_conversations_get_handle();

	purple_signal_disconnect(conv_handle, "chat-buddy-joined", plugin, PURPLE_CALLBACK(chat_buddy_joined_cb));
	purple_signal_disconnect(conv_handle, "chat-buddy-left", plugin, PURPLE_CALLBACK(chat_buddy_left_cb));
	purple_signal_disconnect(conv_handle, "chat-joined", plugin, PURPLE_CALLBACK(chat_joined_cb));
	purple_signal_disconnect(conv_handle, "chat-left", plugin, PURPLE_CALLBACK(chat_left_cb));
    purple_signal_disconnect(conv_handle, "receiving-chat-msg", plugin, PURPLE_CALLBACK(receiving_chat_msg_cb));
    purple_signal_disconnect(conv_handle, "sending-chat-msg", plugin, PURPLE_CALLBACK(sending_chat_msg_cb));
}

gboolean np1sec_plugin_load(PurplePlugin* plugin)
{
    setup_purple_callbacks(plugin);

    //---------------------------------------------------
    // Apply the plugin to chats which were created before
    // this plugin was loaded.
	GList *convs = purple_get_conversations();

	while (convs) {

		PurpleConversation *conv = (PurpleConversation *)convs->data;

        if (is_chat(conv) && !get_room(conv)) {
            /* We'll make use of this variable, so make sure pidgin
             * isn't using it for some other purpose. */
            assert(!conv->account->ui_data);
            apply_np1sec(conv);
        }

		convs = convs->next;
	}

    return true;
}

gboolean np1sec_plugin_unload(PurplePlugin* plugin)
{
    disconnect_purple_callbacks(plugin);

	GList *convs = purple_get_conversations();

	while (convs) {

		PurpleConversation *conv = (PurpleConversation *)convs->data;

        if (is_chat(conv)) {
            unapply_np1sec(conv);
        }

		convs = convs->next;
	}

    return true;
}

//------------------------------------------------------------------------------
static PurplePluginInfo info =
{
    PURPLE_PLUGIN_MAGIC,

    /* Use the 2.0.x API */
    PURPLE_MAJOR_VERSION,                             /* major version  */
    PURPLE_MINOR_VERSION,                             /* minor version  */

    PURPLE_PLUGIN_STANDARD,                           /* type           */
    _(PIDGIN_PLUGIN_TYPE),                            /* ui_requirement */
    0,                                                /* flags          */
    NULL,                                             /* dependencies   */
    PURPLE_PRIORITY_DEFAULT,                          /* priority       */
    _("gtk-equalitie-np1sec"),                        /* id             */
    _("(n+1)sec"),                                    /* name           */
    _("0.4"),                                         /* version        */
    NULL,                                             /* summary        */
    NULL,                                             /* description    */

    _("eQualit.ie"),                                  /* author         */
    _("https://equalit.ie"),                          /* homepage       */

    np1sec_plugin_load,                               /* load           */
    np1sec_plugin_unload,                             /* unload         */
    NULL,                                             /* destroy        */

    NULL,                                             /* ui_info        */
    NULL,                                             /* extra_info     */
    NULL,                                             /* prefs_info     */
    NULL,                                             /* actions        */
};


static void
__init_plugin(PurplePlugin *plugin)
{
    info.name        = _("(n+1)sec Secure Messaging");
    info.summary     = _("Provides private and secure conversations for multi-user chats");
    info.description = _("Preserves the privacy of IM communications "
             "by providing encryption, authentication, "
             "deniability, and perfect forward secrecy.");
}

PURPLE_INIT_PLUGIN(np1sec, __init_plugin, info)

} // extern "C"
