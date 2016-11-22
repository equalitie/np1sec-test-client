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

#include <pidgin/gtkimhtml.h>
#include <pidgin/gtkutils.h>
#include "user_list.h"

namespace np1sec_plugin {

class Channel;
class RoomView;

class ChannelPage {
public:
    ChannelPage(RoomView&, Channel&);

    void set_current();

    void display(const std::string& username, const std::string& message);

    GtkIMHtml* output_imhtml() { return GTK_IMHTML(_output_imhtml); }

    UserList& user_list() { return _user_list; }

private:
    RoomView& _room_view;
    GtkWidget* _output_imhtml;
    std::unique_ptr<Notebook::Page> _tab;
    std::string _stored_input;
    UserList _user_list;
};

} // np1sec_plugin namespace

#include "channel.h"
#include "room_view.h"

namespace np1sec_plugin {
//------------------------------------------------------------------------------
// Implementation
//------------------------------------------------------------------------------
inline
ChannelPage::ChannelPage(RoomView& room_view, Channel& channel)
    : _room_view(room_view)
{
    _output_imhtml = gtk_imhtml_new(NULL, NULL);
	auto* sw = gtk_scrolled_window_new(NULL, NULL);

    gtk_widget_show(sw);
    gtk_widget_show(_output_imhtml);

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_NONE);

    gtk_container_add(GTK_CONTAINER(sw), _output_imhtml);

    auto paned = gtk_hpaned_new();

    gtk_paned_pack1(GTK_PANED(paned), sw, TRUE, TRUE);
    gtk_paned_pack2(GTK_PANED(paned), _user_list.root_widget(), FALSE, TRUE);

    _tab.reset(new Notebook::Page(room_view.notebook(), paned, channel.channel_name()));

    _tab->on_set_current([&] {
            room_view._current_channel_page = this;
            util::gtk::set_text(room_view.input_imhtml(), _stored_input);
        });

    _tab->on_set_not_current([&] {
            room_view._current_channel_page = nullptr;
            _stored_input = util::gtk::remove_text(room_view.input_imhtml());
        });
}

inline
void ChannelPage::display(const std::string& sender, const std::string& message)
{
    /* Temporarily tell pidgin to use different imhtml for output.  */
    auto& imhtml = _room_view._gtkconv->imhtml;

    auto prev = imhtml;
    auto put_back = defer([&] { imhtml = prev; });
    imhtml = _output_imhtml;

    /* Write the output */
    auto* conv = _room_view._conv;

    conv->ui_ops->write_conv(conv,
                             sender.c_str(),
                             sender.c_str(),
                             message.c_str(),
                             PURPLE_MESSAGE_RECV,
                             time(NULL));
}

inline
void ChannelPage::set_current()
{
    _tab->set_current();
}

} // np1sec_plugin namespace
