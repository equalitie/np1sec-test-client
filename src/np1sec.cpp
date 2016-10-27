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

/* Purple GTK headers */
#include "gtkplugin.h"

#include <iostream>

extern "C" {

#define _(x) const_cast<char*>(x)

// Callback which is called right before the message is sent.
static gboolean sending_chat_msg_cb(PurpleAccount *account,
	char **buffer, int id, void* data)
{
	char *tmp = g_strdup_printf("<br/>(np1sec metadata)%s", *buffer);
	g_free(*buffer);
	*buffer = tmp;
    return FALSE;
}

gboolean np1sec_plugin_load(PurplePlugin* plugin) {
    void *conv_handle = purple_conversations_get_handle();

    purple_signal_connect(conv_handle, "sending-chat-msg", plugin,
	    PURPLE_CALLBACK(sending_chat_msg_cb), NULL);

    return true;
}

gboolean np1sec_plugin_unload(PurplePlugin* plugin) {
    return true;
}


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
