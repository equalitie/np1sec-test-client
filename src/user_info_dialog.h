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

namespace np1sec_plugin {

class User;

class UserInfoDialog {
public:
    static void show(GtkWindow* parent_window, const User&);

private:
    static void on_response(GtkDialog*, gint response_id, const User* user);
    static GtkWidget* create_content(const User& user);
};

} // np1sec_plugin

#include "user.h"
#include <gtk/gtk.h>

namespace np1sec_plugin {
//------------------------------------------------------------------------------
// Implementation
//------------------------------------------------------------------------------
inline
void UserInfoDialog::show(GtkWindow* window, const User& user)
{
    auto dialog = gtk_dialog_new_with_buttons ("User Information",
                                               window,
                                               GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_STOCK_OK,  // OK string
                                               GTK_RESPONSE_OK,
                                               NULL);

    auto content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    gtk_container_add(GTK_CONTAINER(content_area), create_content(user));

    //gtk_object_set_property(G_OBJECT(content_area), "halign", GTK_ALIGN_START);

    gtk_widget_show_all(dialog);

    g_signal_connect(GTK_DIALOG (dialog),
                     "response",
                     G_CALLBACK (on_response),
                     const_cast<User*>(&user));
}

inline
GtkWidget* UserInfoDialog::create_content(const User& user)
{
    auto vbox = gtk_vbox_new(FALSE, 4);

    auto append = [&] (const std::string& what, const std::string& value) {
        auto hbox = gtk_hbox_new(FALSE, 4);

        auto label = gtk_label_new((what + value).c_str());

        gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 4);

        gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 4);
    };

    append("Name: ", user.name());

    if (user.public_key) {
        append("Public key: ", user.public_key->dump_hex());
    }
    else {
        append("Public key: ", "not yet known");
    }

    append("Authorized by: ", util::str(user.authorized_by()));
    append("Promoted: ", user.was_promoted() ? "Yes" : "No");

    return vbox;
}

inline
void UserInfoDialog::on_response(GtkDialog* dialog, gint response_id, const User* user)
{
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

} // np1sec_plugin namespace
