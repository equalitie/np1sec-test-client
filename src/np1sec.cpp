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
/* purple headers */
#include "pidgin.h"
#include "notify.h"
#include "version.h"
#include "util.h"
#include "debug.h"
#include "core.h"

/* np1sec headers */
#include "src/interface.h"
#include "src/userstate.h"

/* Purple GTK headers */
#include "gtkplugin.h"
#include "gtkconv.h"

#include "conversation.h"

extern "C" {

#define _(x) const_cast<char*>(x)

//------------------------------------------------------------------------------
static void set_np1sec_conversation(PurpleConversation* conv,
                                    np1sec_plugin::Conversation* np1sec_conversation)
{
    g_hash_table_insert(conv->data, strdup("np1sec_conversation"), np1sec_conversation);
}

static np1sec_plugin::Conversation* get_np1sec_conversation(PurpleConversation* conv)
{
    auto* p =  g_hash_table_lookup(conv->data, "np1sec_conversation");
    return static_cast<np1sec_plugin::Conversation*>(p);
}

//------------------------------------------------------------------------------
static void conversation_created_cb(PurpleConversation *conv)
{
    auto* np1sec_conversation = new np1sec_plugin::Conversation(conv);
    set_np1sec_conversation(conv, np1sec_conversation);
    // NOTE: We can't call np1sec_conversation->start() here because the
    // purple_conv_chat_get_id(PURPLE_CONV_CHAT(conv)) doesn't return
    // a valid ID yet and thus sending wouldn't work.
}

void conversation_updated_cb(PurpleConversation *conv, 
                             PurpleConvUpdateType type) {
    auto* np1sec_conversation = get_np1sec_conversation(conv);

    if (!np1sec_conversation) {
        // The conversation-created signal has not yet been called.
        return;
    }

    if (!np1sec_conversation->started()) {
        np1sec_conversation->start();
    }
}

static gboolean receiving_chat_msg_cb(PurpleAccount *account,
                                      char** sender,
                                      char **message,
                                      PurpleConversation* conv,
                                      PurpleMessageFlags flags)
{
    auto np1sec_conv = get_np1sec_conversation(conv);
    assert(np1sec_conv);
    if (!np1sec_conv) return FALSE;
    np1sec_conv->on_received_data(*sender, *message);

    // Returning TRUE causes this message not to be displayed.
    // Displaying is done explicitly from np1sec.
    return TRUE;
}

void sending_chat_msg_cb(PurpleAccount *account, char **message, int id) {
    //std::cout << "sending_chat_msg_cb " << *message << std::endl;
}

//------------------------------------------------------------------------------
static void setup_purple_callbacks(PurplePlugin* plugin)
{
    //void* conn_handle = purple_connections_get_handle();
    void* conv_handle = purple_conversations_get_handle();

	purple_signal_connect(conv_handle, "conversation-created", plugin, PURPLE_CALLBACK(conversation_created_cb), NULL);
	purple_signal_connect(conv_handle, "conversation-updated", plugin, PURPLE_CALLBACK(conversation_updated_cb), NULL);
    purple_signal_connect(conv_handle, "receiving-chat-msg", plugin, PURPLE_CALLBACK(receiving_chat_msg_cb), NULL);
    purple_signal_connect(conv_handle, "sending-chat-msg", plugin, PURPLE_CALLBACK(sending_chat_msg_cb), NULL);
}

gboolean np1sec_plugin_load(PurplePlugin* plugin)
{
    std::cout << "plugin load" << std::endl;
    setup_purple_callbacks(plugin);
    return true;
}

gboolean np1sec_plugin_unload(PurplePlugin* plugin)
{
    std::cout << "plugin unload" << std::endl;
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
