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

using ToggleButton = np1sec_plugin::PluginToggleButton;

static bool is_chat(PurpleConversation* conv) {
    return conv && conv->type == PURPLE_CONV_TYPE_CHAT;
}

//------------------------------------------------------------------------------
static ToggleButton* get_toggle_button(PurpleAccount* account)
{
    auto* p =  g_hash_table_lookup(account->ui_settings, "np1sec_plugin");
    return static_cast<ToggleButton*>(p);
}

static ToggleButton* get_toggle_button(PurpleConversation* conv)
{
    return get_toggle_button(conv->account);
}

static void set_toggle_button(PurpleAccount* account, ToggleButton* tb)
{
    if (auto r = get_toggle_button(account)) {
        delete r;
    }
    g_hash_table_insert(account->ui_settings, strdup("np1sec_plugin"), tb);
}

static void set_toggle_button(PurpleConversation* conv, ToggleButton* tb)
{
    set_toggle_button(conv->account, tb);
}

//------------------------------------------------------------------------------

extern "C" {

#define _(x) const_cast<char*>(x)

//------------------------------------------------------------------------------
static void conversation_created_cb(PurpleConversation *conv)
{
    if (!is_chat(conv)) return;

    auto* tb = new ToggleButton(conv);
    set_toggle_button(conv, tb);
}

static void deleting_conversation_cb(PurpleConversation *conv) {
    if (!is_chat(conv)) return;
    set_toggle_button(conv, nullptr);
}

static gboolean receiving_chat_msg_cb(PurpleAccount *account,
                                      char** sender,
                                      char **message,
                                      PurpleConversation* conv,
                                      PurpleMessageFlags *flags)
{
    if (!is_chat(conv)) return FALSE;

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
    auto tb = get_toggle_button(account);
    assert(tb);

    if (!tb->room) return;

    tb->room->send_chat_message(*message);

    g_free(*message);
    *message = NULL;
}

static
void chat_buddy_left_cb(PurpleConversation* conv, const char* name, const char*, void*)
{
    if (!is_chat(conv)) return;

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

        if (is_chat(conv) && !get_toggle_button(conv)) {
            auto* tb = new ToggleButton(conv);
            set_toggle_button(conv, tb);
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

        if (!is_chat(conv)) continue;

        set_toggle_button(conv, nullptr);

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
    info.name        = _("(n+1)sec Secure messaging");
    info.summary     = _("Provides private and secure conversations for multi-user chats");
    info.description = _("Preserves the privacy of IM communications "
             "by providing encryption, authentication, "
             "deniability, and perfect forward secrecy.");
}

PURPLE_INIT_PLUGIN(np1sec, __init_plugin, info)

} // extern "C"
