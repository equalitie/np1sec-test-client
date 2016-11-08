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

class Toolbar {
public:
    Toolbar(PidginConversation*);

    std::function<void()> on_create_channel_clicked;
    std::function<void()> on_search_channel_clicked;

    ~Toolbar();

private:
    static void exec_callback(GtkWidget*, gpointer data);

private:
    PidginConversation* _gtkconv;
    GtkWidget* _toolbar_box;
};

//------------------------------------------------------------------------------
// Implementation
//------------------------------------------------------------------------------
inline Toolbar::Toolbar(PidginConversation* gtkconv)
    : _gtkconv(gtkconv)
{
    if (!gtkconv) {
        assert(0 && "Not a pidgin conversation");
        return;
    }

    _toolbar_box = gtk_vbox_new(FALSE, PIDGIN_HIG_BOX_SPACE);

    gtk_box_pack_start(GTK_BOX(_gtkconv->lower_hbox), _toolbar_box, FALSE, FALSE, 0);
    gtk_widget_show(_toolbar_box);

    GtkWidget* create_channel_button = gtk_button_new_with_label("Create channel");
    GtkWidget* search_channel_button = gtk_button_new_with_label("Search channels");

    gtk_box_pack_start(GTK_BOX(_toolbar_box), create_channel_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(_toolbar_box), search_channel_button, FALSE, FALSE, 0);

    gtk_widget_show(create_channel_button);
    gtk_widget_show(search_channel_button);

    gtk_signal_connect(GTK_OBJECT(create_channel_button), "clicked"
            , GTK_SIGNAL_FUNC(exec_callback), &on_create_channel_clicked);

    gtk_signal_connect(GTK_OBJECT(search_channel_button), "clicked"
            , GTK_SIGNAL_FUNC(exec_callback), &on_search_channel_clicked);
}

void Toolbar::exec_callback(GtkWidget*, gpointer data)
{
    auto& func = *reinterpret_cast<std::function<void()>*>(data);
    if (!func) return;
    func();
}

inline Toolbar::~Toolbar()
{
    gtk_container_remove(GTK_CONTAINER(_gtkconv->lower_hbox), _toolbar_box);
}

} // np1sec_plugin namespace
