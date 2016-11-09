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

#include "defer.h"
#include "util.h"

namespace np1sec_plugin {

using PopupActions = std::map<std::string, std::function<void()>>;

namespace popup_detail {
    static
    void on_popup_item_pressed(GtkWidget*, gpointer* data)
    {
        auto& func = *reinterpret_cast<PopupActions::mapped_type*>(data);
        func();
    }
} // popup_detail namespace


void show_popup( GdkEventButton* event, const PopupActions& actions)
{
    using A = PopupActions::mapped_type;

    if (actions.empty()) return;

    auto menu = gtk_menu_new();

    for (const auto& action : actions) {
        auto menuitem = gtk_menu_item_new_with_label(action.first.c_str());

        g_signal_connect(menuitem, "activate",
                         (GCallback) popup_detail::on_popup_item_pressed,
                         const_cast<A*>(&action.second));

        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    }

    gtk_widget_show_all(menu);

    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
                   event ? event->button : 0,
                   gdk_event_get_time((GdkEvent*)event));
}

} // np1sec_plugin
