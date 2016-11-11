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
#include "plugin_toggle_button.h"

/*
 * Rant: The only reason for this global is because the sending-chat-msg
 * callback doesn't give us PurpleConversation*. Instead it gives us
 * an ID of the chat.
 */
static std::map<int, PurpleConversation*> g_conversations;

using ToggleButton = np1sec_plugin::PluginToggleButton;

extern "C" {

#define _(x) const_cast<char*>(x)

//------------------------------------------------------------------------------
static ToggleButton* get_toggle_button(PurpleConversation* conv)
{
    auto* p =  g_hash_table_lookup(conv->data, "np1sec_plugin");
    return static_cast<ToggleButton*>(p);
}

static void set_toggle_button(PurpleConversation* conv, ToggleButton* tb)
{
    if (auto r = get_toggle_button(conv)) {
        delete r;
    }
    g_hash_table_insert(conv->data, strdup("np1sec_plugin"), tb);
}

//------------------------------------------------------------------------------
static void conversation_created_cb(PurpleConversation *conv)
{
    auto* tb = new ToggleButton(conv);
    set_toggle_button(conv, tb);
}

void conversation_updated_cb(PurpleConversation *conv, 
                             PurpleConvUpdateType type) {
    auto* tb = get_toggle_button(conv);

    if (!tb) {
        // The conversation-created signal has not yet been called.
        return;
    }

    auto id = purple_conv_chat_get_id(PURPLE_CONV_CHAT(conv));

    if (g_conversations.count(id) == 0) {
        g_conversations[id] = conv;
    }
}

static void deleting_conversation_cb(PurpleConversation *conv) {
    auto id = purple_conv_chat_get_id(PURPLE_CONV_CHAT(conv));
    g_conversations.erase(id);
    set_toggle_button(conv, nullptr);
}

static gboolean receiving_chat_msg_cb(PurpleAccount *account,
                                      char** sender,
                                      char **message,
                                      PurpleConversation* conv,
                                      PurpleMessageFlags *flags)
{
    auto tb = get_toggle_button(conv);
    assert(tb);
    if (!tb) return FALSE;

    if (!tb->room) return FALSE;

    // Ignore historic messages.
    if (*flags & PURPLE_MESSAGE_DELAYED) return TRUE;

    tb->room->on_received_data(*sender, *message);

    // Returning TRUE causes this message not to be displayed.
    // Displaying is done explicitly from np1sec.
    return TRUE;
}

static
void sending_chat_msg_cb(PurpleAccount *account, char **message, int id) {
    auto conv_i = g_conversations.find(id);
    assert(conv_i != g_conversations.end());

    auto tb = get_toggle_button(conv_i->second);
    assert(tb);

    if (!tb->room) return;

    tb->room->send_chat_message(*message);

    g_free(*message);
    *message = NULL;
}

static
void chat_buddy_left_cb(PurpleConversation* conv, const char* name, const char*, void*)
{
    auto tb = get_toggle_button(conv);
    assert(tb);
    if (tb && tb->room) tb->room->user_left(name);
}

//------------------------------------------------------------------------------
bool g_signals_connected = false;

static void setup_purple_callbacks(PurplePlugin* plugin)
{
    if (g_signals_connected) return;
    g_signals_connected = true;

    void* conv_handle = purple_conversations_get_handle();

	purple_signal_connect(conv_handle, "chat-buddy-left", plugin, PURPLE_CALLBACK(chat_buddy_left_cb), NULL);
    purple_signal_connect(conv_handle, "conversation-created", plugin, PURPLE_CALLBACK(conversation_created_cb), NULL);
    purple_signal_connect(conv_handle, "conversation-updated", plugin, PURPLE_CALLBACK(conversation_updated_cb), NULL);
    purple_signal_connect(conv_handle, "deleting-conversation", plugin, PURPLE_CALLBACK(deleting_conversation_cb), NULL);
    purple_signal_connect(conv_handle, "receiving-chat-msg", plugin, PURPLE_CALLBACK(receiving_chat_msg_cb), NULL);
    purple_signal_connect(conv_handle, "sending-chat-msg", plugin, PURPLE_CALLBACK(sending_chat_msg_cb), NULL);
}

static void disconnect_purple_callbacks(PurplePlugin* plugin)
{
    if (!g_signals_connected) return;
    g_signals_connected = false;

    void* conv_handle = purple_conversations_get_handle();

	purple_signal_disconnect(conv_handle, "chat-buddy-left", plugin, PURPLE_CALLBACK(chat_buddy_left_cb));
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

        if (!get_toggle_button(conv)) {
            auto* tb = new ToggleButton(conv);
            set_toggle_button(conv, tb);

            auto id = purple_conv_chat_get_id(PURPLE_CONV_CHAT(conv));
            assert (g_conversations.count(id) == 0);
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

        set_toggle_button(conv, nullptr);

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
    info.name        = _("(n+1)sec Secure messaging");
    info.summary     = _("Provides private and secure conversations for multi-user chats");
    info.description = _("Preserves the privacy of IM communications "
             "by providing encryption, authentication, "
             "deniability, and perfect forward secrecy.");
}

PURPLE_INIT_PLUGIN(np1sec, __init_plugin, info)

} // extern "C"
