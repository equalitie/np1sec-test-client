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
#include "plugin_toggle_button.h"

using ToggleButton = np1sec_plugin::PluginToggleButton;

static bool is_chat(PurpleConversation* conv) {
    return conv && conv->type == PURPLE_CONV_TYPE_CHAT;
}

//------------------------------------------------------------------------------
static ToggleButton* get_toggle_button(PurpleConversation* conv)
{
    auto p = purple_conversation_get_data(conv, "np1sec");
    return reinterpret_cast<ToggleButton*>(p);
}

static void set_toggle_button(PurpleConversation* conv, ToggleButton* tb)
{
    if (auto r = get_toggle_button(conv)) {
        delete r;
    }
    purple_conversation_set_data(conv, "np1sec", tb);
}

//------------------------------------------------------------------------------

extern "C" {

#define _(x) const_cast<char*>(x)

//------------------------------------------------------------------------------
static void chat_joined_cb(PurpleConversation* conv, void*)
{
    auto b = get_toggle_button(conv);
    assert(b);
    b->joined_chat();
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
void sending_chat_msg_cb(PurpleAccount *account, char **message, int id, PurpleConversation* conv) {
    assert(conv);

    auto tb = get_toggle_button(conv);
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
static void apply_np1sec(PurpleConversation* conv)
{
    assert(is_chat(conv));
    assert(!get_toggle_button(conv));

    void* conv_handle = purple_conversations_get_handle();

	purple_signal_connect(conv_handle, "chat-buddy-left", conv, PURPLE_CALLBACK(chat_buddy_left_cb), NULL);
	purple_signal_connect(conv_handle, "chat-joined", conv, PURPLE_CALLBACK(chat_joined_cb), NULL);
    purple_signal_connect(conv_handle, "receiving-chat-msg", conv, PURPLE_CALLBACK(receiving_chat_msg_cb), NULL);
    purple_signal_connect(conv_handle, "sending-chat-msg", conv, PURPLE_CALLBACK(sending_chat_msg_cb), conv);

    set_toggle_button(conv, new ToggleButton(conv));
}

static void unapply_np1sec(PurpleConversation* conv)
{
    assert(is_chat(conv));
    assert(get_toggle_button(conv));

    void* conv_handle = purple_conversations_get_handle();

	purple_signal_disconnect(conv_handle, "chat-buddy-left", conv, PURPLE_CALLBACK(chat_buddy_left_cb));
	purple_signal_disconnect(conv_handle, "chat-joined", conv, PURPLE_CALLBACK(chat_joined_cb));
    purple_signal_disconnect(conv_handle, "receiving-chat-msg", conv, PURPLE_CALLBACK(receiving_chat_msg_cb));
    purple_signal_disconnect(conv_handle, "sending-chat-msg", conv, PURPLE_CALLBACK(sending_chat_msg_cb));

    set_toggle_button(conv, nullptr);
}

//------------------------------------------------------------------------------
static void conversation_created_cb(PurpleConversation *conv)
{
    if (!is_chat(conv)) return;
    apply_np1sec(conv);
}

static void deleting_conversation_cb(PurpleConversation *conv) {
    if (!is_chat(conv)) return;
    unapply_np1sec(conv);
}

static void setup_purple_callbacks(PurplePlugin* plugin)
{
    void* conv_handle = purple_conversations_get_handle();

    purple_signal_connect(conv_handle, "conversation-created", plugin, PURPLE_CALLBACK(conversation_created_cb), NULL);
    purple_signal_connect(conv_handle, "deleting-conversation", plugin, PURPLE_CALLBACK(deleting_conversation_cb), NULL);
}

static void disconnect_purple_callbacks(PurplePlugin* plugin)
{
    void* conv_handle = purple_conversations_get_handle();

    purple_signal_disconnect(conv_handle, "conversation-created", plugin, PURPLE_CALLBACK(conversation_created_cb));
    purple_signal_disconnect(conv_handle, "deleting-conversation", plugin, PURPLE_CALLBACK(deleting_conversation_cb));
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
