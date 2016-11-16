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

#include "channel_list.h"
#include "util.h"

namespace np1sec_plugin {

class RoomView;

class ChannelListPage {
public:
    ChannelListPage(RoomView&, GtkBox* content, GtkWidget* ouput_imhtml);

    ChannelList& channel_list() { return *_channel_list; }

    ~ChannelListPage();

private:
    std::unique_ptr<Notebook::Page> _tab;
    GtkBox* _content;
    GtkWidget* _content_orig_parent;
    std::unique_ptr<ChannelList> _channel_list;
    /*
     * This is a pointer to the widget that holds the output
     * window and the user list. We modify it in the constructor
     * and will need to change it back in the destructor.
     */
    GtkWidget* _target;
    GtkWidget* _vpaned;
    GtkWidget* _userlist;
    GtkWidget* _input;
    GtkWidget* _output_imhtml;
    std::string _stored_input;
};

} // np1sec_plugin namespace

#include "room_view.h"

namespace np1sec_plugin {
//------------------------------------------------------------------------------
// Implementation
//------------------------------------------------------------------------------
inline
ChannelListPage::ChannelListPage(RoomView& rv, GtkBox* content, GtkWidget* output_imhtml)
    : _content(content)
    , _output_imhtml(output_imhtml)
{
    /*
     * The widget with the chat output window is at this position
     * in the VBox as coded in pidin/gtkconv.c/setup_common_pane.
     * I haven't found a better way to find that window unfortunatelly
     * other than this hardcoded position.
     * TODO: I have a hunch that for the IM window (as opposed to
     * Group Chat window) this number is going to be different.
     */
    const gint output_window_position = 2;
    const gint input_window_position = 4;

    _target = util::gtk::get_nth_child(output_window_position, GTK_CONTAINER(_content));
    _input  = util::gtk::get_nth_child(input_window_position, GTK_CONTAINER(_content));

    assert(GTK_IS_PANED(_target));

    if (!_target) {
        assert(_target);
        return;
    }

    _userlist = gtk_paned_get_child2(GTK_PANED(_target));
    g_object_ref_sink(_userlist);

    gtk_container_remove(GTK_CONTAINER(_target), _userlist);

    _vpaned = gtk_vpaned_new();
    g_object_ref_sink(_vpaned);

    gtk_paned_pack2(GTK_PANED(_target), _vpaned, TRUE, TRUE);
    gtk_paned_pack1(GTK_PANED(_vpaned), _userlist, TRUE, TRUE);

    _channel_list.reset(new ChannelList());
    gtk_paned_pack2(GTK_PANED(_vpaned), _channel_list->root_widget(), TRUE, TRUE);

    // TODO: Not sure why the value of 1 gives a good result
    gtk_widget_set_size_request(_vpaned, 1, -1);
    gtk_widget_show(_vpaned);

    _tab.reset(new Notebook::Page(rv.notebook(), GTK_WIDGET(_content), "Channels"));

    _tab->on_set_current([&] {
            util::gtk::set_text(rv.input_imhtml(), _stored_input);
            rv.set_output_window(_output_imhtml);
        });

    _tab->on_set_not_current([&] {
            _stored_input = util::gtk::remove_text(rv.input_imhtml());
        });
}

inline
ChannelListPage::~ChannelListPage()
{
    gtk_container_remove(GTK_CONTAINER(_vpaned), _userlist);
    gtk_container_remove(GTK_CONTAINER(_target), _vpaned);

    gtk_paned_pack2(GTK_PANED(_target), _userlist, FALSE, TRUE);

    g_object_unref(_userlist);
    g_object_unref(_vpaned);
}

} // np1sec_plugin namespace


