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

#include "room.h"

namespace np1sec_plugin {

class PluginToggleButton {
public:
    PluginToggleButton(PurpleConversation*);

    std::unique_ptr<Room> room;

    ~PluginToggleButton();

private:
    static void on_click(GtkWidget*, PluginToggleButton*);

private:
    PurpleConversation* _conv;
    PidginConversation* _gtkconv;
    GtkWidget* _button;
};

//------------------------------------------------------------------------------
// Implementation
//------------------------------------------------------------------------------
PluginToggleButton::PluginToggleButton(PurpleConversation* conv)
    : _conv(conv)
    , _gtkconv(PIDGIN_CONVERSATION(conv))
{
    _button = gtk_button_new_with_label("Toggle np1sec");
    gtk_box_pack_start(GTK_BOX(_gtkconv->infopane_hbox), _button, FALSE, FALSE, 0);

    gtk_widget_show(_button);

    gtk_signal_connect(GTK_OBJECT(_button), "clicked"
            , GTK_SIGNAL_FUNC(on_click), this);

}

void PluginToggleButton::on_click(GtkWidget*, PluginToggleButton* self)
{
    if (self->room) {
        self->room.reset();
    }
    else {
        self->room.reset(new Room(self->_conv));
        self->room->start();
    }
}

PluginToggleButton::~PluginToggleButton()
{
    gtk_container_remove(GTK_CONTAINER(_gtkconv->infopane_hbox), _button);
}

} // np1sec_plugin namespace
