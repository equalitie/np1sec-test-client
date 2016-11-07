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

namespace np1sec_plugin {

class ChannelView;

class RoomView {
public:
    RoomView(PurpleConversation*);

    RoomView(RoomView&&) = delete;
    RoomView(const RoomView&) = delete;
    RoomView& operator=(const RoomView&) = delete;
    RoomView& operator=(RoomView&&) = delete;

    ~RoomView();

private:
    static GtkWidget* get_nth_child(gint n, GtkContainer* c);

private:
    friend class ChannelView;
    friend class UserView;

    GtkTreeView* _tree_view;
    GtkTreeStore* _tree_store;

    /*
     * This is a pointer to the widget that holds the output
     * window and the user list. We modify it in the constructor
     * and will need to change it back in the destructor.
     */
    GtkPaned* _target;
    GtkWidget* _userlist;
};

} // np1sec_plugin namespace

#include "channel_view.h"

namespace np1sec_plugin {

//------------------------------------------------------------------------------
// Implementation
//------------------------------------------------------------------------------
inline RoomView::RoomView(PurpleConversation* conv)
{
    // TODO: Throw instead of assert.

    auto* gtkconv = PIDGIN_CONVERSATION(conv);

    if (!gtkconv) {
        assert(0 && "Not a pidgin conversation");
        return;
    }

    GtkWidget* parent = gtk_widget_get_parent(gtkconv->lower_hbox);

    if (!GTK_IS_CONTAINER(parent)) {
        assert(0);
        return;
    }

    if (!GTK_IS_BOX(parent)) {
        assert(0);
        return;
    }

    /*
     * The widget with the chat output window is at this position
     * in the VBox as coded in pidin/gtkconv.c/setup_common_pane.
     * I haven't found a better way to find that window unfortunatelly
     * other than this hardcoded position.
     * TODO: I have a hunch that for the IM window (as opposed to
     * Group Chat window) this number is going to be different.
     */
    const gint output_window_position = 2;

    GtkWidget* target = get_nth_child(output_window_position, GTK_CONTAINER(parent));

    assert(GTK_IS_PANED(target));

    if (!target) {
        assert(target);
        return;
    }

    _target = GTK_PANED(target);

    _userlist = gtk_paned_get_child2(_target);

    g_object_ref(_userlist);
    auto unref_userlist = defer([ul = _userlist] { g_object_unref(ul); });

    gtk_container_remove(GTK_CONTAINER(target), _userlist);

	GtkWidget* vpaned = gtk_vpaned_new();
    gtk_paned_pack2(_target, vpaned, TRUE, TRUE);

	gtk_widget_show(vpaned);

	gtk_paned_pack1(GTK_PANED(vpaned), _userlist, TRUE, TRUE);

    GtkWidget* tree_view = gtk_tree_view_new();
	gtk_paned_pack2(GTK_PANED(vpaned), tree_view, TRUE, TRUE);

    // TODO: Not sure why the value of 1 gives a good result
	gtk_widget_set_size_request(vpaned, 1, -1);
	gtk_widget_show(tree_view);

    _tree_view = GTK_TREE_VIEW(tree_view);

    gtk_tree_view_insert_column_with_attributes (_tree_view,
                                                 -1,      
                                                 "",  
                                                 gtk_cell_renderer_text_new(),
                                                 "text", ChannelView::COL_NAME,
                                                 NULL);

    _tree_store = gtk_tree_store_new (ChannelView::NUM_COLS, G_TYPE_STRING);
    gtk_tree_view_set_model(_tree_view, GTK_TREE_MODEL(_tree_store));
}

inline RoomView::~RoomView()
{
    g_object_unref(_tree_store);

    g_object_ref(_userlist);
    auto unref_userlist = defer([ul = _userlist] { g_object_unref(ul); });

    gtk_container_remove(GTK_CONTAINER(_target), gtk_paned_get_child2(_target));

    gtk_paned_pack2(_target, _userlist, TRUE, TRUE);
}

inline
GtkWidget* RoomView::get_nth_child(gint n, GtkContainer* c) {
    GList* children = gtk_container_get_children(c);
    
    auto free_children = defer([children] { g_list_free(children); });
    
    for (auto* l = children; l != NULL; l = l->next) {
        if (n-- == 0) {
            return GTK_WIDGET(l->data);
        }
    }
    
    return nullptr;
}

} // np1sec_plugin namespace

