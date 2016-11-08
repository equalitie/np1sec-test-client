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
#include "pidgin.h"
#include "notify.h"
#include "version.h"
#include "util.h"
#include "debug.h"
#include "core.h"

/* Purple GTK headers */
#include "gtkplugin.h"
#include "gtkconv.h"

/* Plugin headers */
#include "room.h"

/*
 * Rant: The only reason for this global is because the sending-chat-msg
 * callback doesn't give us PurpleConversation*. Instead it gives us
 * an ID of the chat.
 */
static std::map<int, PurpleConversation*> g_conversations;

extern "C" {

#define _(x) const_cast<char*>(x)

//------------------------------------------------------------------------------
static np1sec_plugin::Room* get_conv_room(PurpleConversation* conv)
{
    auto* p =  g_hash_table_lookup(conv->data, "np1sec_plugin_room");
    return static_cast<np1sec_plugin::Room*>(p);
}

static void set_conv_room(PurpleConversation* conv, np1sec_plugin::Room* room)
{
    if (auto r = get_conv_room(conv)) {
        delete r;
    }
    g_hash_table_insert(conv->data, strdup("np1sec_plugin_room"), room);
}

//------------------------------------------------------------------------------
static void conversation_created_cb(PurpleConversation *conv)
{
    auto* room = new np1sec_plugin::Room(conv);
    set_conv_room(conv, room);

    // NOTE: We can't call room->start() here because the
    // purple_conv_chat_get_id(PURPLE_CONV_CHAT(conv)) doesn't return
    // a valid ID yet and thus sending wouldn't work.
}

void conversation_updated_cb(PurpleConversation *conv, 
                             PurpleConvUpdateType type) {
    auto* room = get_conv_room(conv);

    if (!room) {
        // The conversation-created signal has not yet been called.
        return;
    }

    if (!room->started()) {
        auto id = purple_conv_chat_get_id(PURPLE_CONV_CHAT(conv));
        assert(g_conversations.count(id) == 0);
        g_conversations[id] = conv;
        room->start();
    }
}

static void deleting_conversation_cb(PurpleConversation *conv) {
    auto id = purple_conv_chat_get_id(PURPLE_CONV_CHAT(conv));
    g_conversations.erase(id);
    auto room = get_conv_room(conv);
    delete room;
}

static gboolean receiving_chat_msg_cb(PurpleAccount *account,
                                      char** sender,
                                      char **message,
                                      PurpleConversation* conv,
                                      PurpleMessageFlags *flags)
{
    auto room = get_conv_room(conv);
    assert(room);
    if (!room) return FALSE;

    // Ignore historic messages.
    if (*flags & PURPLE_MESSAGE_DELAYED) return TRUE;

    room->on_received_data(*sender, *message);

    // Returning TRUE causes this message not to be displayed.
    // Displaying is done explicitly from np1sec.
    return TRUE;
}

static
void sending_chat_msg_cb(PurpleAccount *account, char **message, int id) {
    auto conv_i = g_conversations.find(id);
    assert(conv_i != g_conversations.end());

    auto room = get_conv_room(conv_i->second);
    assert(room);

    room->send_chat_message(*message);

    g_free(*message);
    *message = NULL;
}

//------------------------------------------------------------------------------
bool signals_connected = false;
static void setup_purple_callbacks(PurplePlugin* plugin)
{
    if (signals_connected) return;
    signals_connected = true;
    void* conv_handle = purple_conversations_get_handle();

    purple_signal_connect(conv_handle, "conversation-created", plugin, PURPLE_CALLBACK(conversation_created_cb), NULL);
    purple_signal_connect(conv_handle, "conversation-updated", plugin, PURPLE_CALLBACK(conversation_updated_cb), NULL);
    purple_signal_connect(conv_handle, "deleting-conversation", plugin, PURPLE_CALLBACK(deleting_conversation_cb), NULL);
    purple_signal_connect(conv_handle, "receiving-chat-msg", plugin, PURPLE_CALLBACK(receiving_chat_msg_cb), NULL);
    purple_signal_connect(conv_handle, "sending-chat-msg", plugin, PURPLE_CALLBACK(sending_chat_msg_cb), NULL);
}

static void disconnect_purple_callbacks(PurplePlugin* plugin)
{
    if (!signals_connected) return;
    signals_connected = false;

    void* conv_handle = purple_conversations_get_handle();

    purple_signal_disconnect(conv_handle, "conversation-created", plugin, PURPLE_CALLBACK(conversation_created_cb));
    purple_signal_disconnect(conv_handle, "conversation-updated", plugin, PURPLE_CALLBACK(conversation_updated_cb));
    purple_signal_disconnect(conv_handle, "deleting-conversation", plugin, PURPLE_CALLBACK(deleting_conversation_cb));
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

        if (!get_conv_room(conv)) {
            auto* room = new np1sec_plugin::Room(conv);
            set_conv_room(conv, room);

            auto id = purple_conv_chat_get_id(PURPLE_CONV_CHAT(conv));
            g_conversations[id] = conv;
        }

		convs = convs->next;
	}

    //---------------------------------------------------

    std::cout << "(n+1)sec plugin loaded" << std::endl;

    return true;
}

gboolean np1sec_plugin_unload(PurplePlugin* plugin)
{
    disconnect_purple_callbacks(plugin);

	GList *convs = purple_get_conversations();

	while (convs) {

		PurpleConversation *conv = (PurpleConversation *)convs->data;

        set_conv_room(conv, nullptr);

		convs = convs->next;
	}

    g_conversations.clear();

    std::cout << "(n+1)sec plugin unloaded" << std::endl;
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
    _("0.1"),                                         /* version        */
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
    info.name        = _("(n+1)sec Secure messaging");
    info.summary     = _("Provides private and secure conversations for multi-user chats");
    info.description = _("Preserves the privacy of IM communications "
             "by providing encryption, authentication, "
             "deniability, and perfect forward secrecy.");
}

PURPLE_INIT_PLUGIN(np1sec, __init_plugin, info)

} // extern "C"
