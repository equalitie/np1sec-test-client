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
    void setup_callbacks(GtkTreeView* tree_view);

    static
    void on_double_click( GtkTreeView*
                        , GtkTreePath*
                        , GtkTreeViewColumn*
                        , RoomView*);

    static
    gint on_button_pressed(GtkWidget*, GdkEventButton*, RoomView*);

private:
    friend class ChannelView;
    friend class UserView;

    GtkTreeView* _tree_view;
    GtkTreeStore* _tree_store;

    //            +--> Path in the tree
    //            |
    std::map<std::string, std::function<void()>> _double_click_callbacks;
    std::map<std::string, std::function<void(GdkEventButton*)>> _show_popup_callbacks;

    /*
     * This is a pointer to the widget that holds the output
     * window and the user list. We modify it in the constructor
     * and will need to change it back in the destructor.
     */
    GtkPaned* _target;
    GtkWidget* _vpaned;
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

    GtkWidget* userlist = gtk_paned_get_child2(_target);

    g_object_ref(userlist);
    auto unref_userlist = defer([ul = userlist] { g_object_unref(ul); });

    gtk_container_remove(GTK_CONTAINER(target), userlist);

    _vpaned = gtk_vpaned_new();
    gtk_paned_pack2(_target, _vpaned, TRUE, TRUE);

    gtk_widget_show(_vpaned);

    gtk_paned_pack1(GTK_PANED(_vpaned), userlist, TRUE, TRUE);

    GtkWidget* tree_view = gtk_tree_view_new();
    gtk_paned_pack2(GTK_PANED(_vpaned), tree_view, TRUE, TRUE);

    // TODO: Not sure why the value of 1 gives a good result
    gtk_widget_set_size_request(_vpaned, 1, -1);
    gtk_widget_show(tree_view);

    _tree_view = GTK_TREE_VIEW(tree_view);

    gtk_tree_view_insert_column_with_attributes (_tree_view,
                                                 -1,      
                                                 "Channels",
                                                 gtk_cell_renderer_text_new(),
                                                 "text", ChannelView::COL_NAME,
                                                 NULL);

    _tree_store = gtk_tree_store_new (ChannelView::NUM_COLS, G_TYPE_STRING);
    gtk_tree_view_set_model(_tree_view, GTK_TREE_MODEL(_tree_store));

    setup_callbacks(_tree_view);
}

inline RoomView::~RoomView()
{
    g_object_unref(_tree_store);

    // Remove the channel list and add the userlist back as it was.
    GtkWidget* userlist = gtk_paned_get_child1(GTK_PANED(_vpaned));

    g_object_ref(userlist);
    auto unref_userlist = defer([ul = userlist] { g_object_unref(ul); });

    gtk_container_remove(GTK_CONTAINER(_vpaned), userlist);
    gtk_container_remove(GTK_CONTAINER(_target), _vpaned);

    gtk_paned_pack2(_target, userlist, FALSE, TRUE);
    gtk_widget_show(GTK_WIDGET(_target));
    gtk_widget_show(GTK_WIDGET(userlist));
}

void RoomView::on_double_click( GtkTreeView *tree_view
                              , GtkTreePath *path
                              , GtkTreeViewColumn *column
                              , RoomView* v)
{
    gchar* c_str = gtk_tree_path_to_string(path);
    auto free_str = defer([c_str] { g_free(c_str); });

    auto cb_i = v->_double_click_callbacks.find(c_str);

    if (cb_i == v->_double_click_callbacks.end()) {
        return;
    }

    cb_i->second();
}

// Return TRUE if we handled it.
inline
gboolean RoomView::on_button_pressed( GtkWidget*
                                    , GdkEventButton* event
                                    , RoomView* v)
{

    /* Is right mouse button? */
    if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
        GtkTreePath *path;
        gtk_tree_view_get_path_at_pos(v->_tree_view,
                                      event->x, event->y,
                                      &path, NULL, NULL, NULL);
        auto free_path = defer([path] { gtk_tree_path_free(path); });

        if (path == NULL)
            return FALSE;

        GtkTreeIter iter;
        gtk_tree_model_get_iter(GTK_TREE_MODEL(v->_tree_store), &iter, path);

        auto path_str = util::tree_iter_to_path(iter, v->_tree_store);

        auto action_i = v->_show_popup_callbacks.find(path_str);

        if (action_i == v->_show_popup_callbacks.end())
            return FALSE;

        action_i->second(event);

        return TRUE;
    }

    return FALSE;
}

inline
void RoomView::setup_callbacks(GtkTreeView* tree_view)
{
    g_signal_connect(GTK_WIDGET(tree_view), "row-activated",
                     G_CALLBACK(on_double_click), this);

    g_signal_connect(G_OBJECT(tree_view), "button-press-event",
                     (GCallback) on_button_pressed, this);

    // // TODO: popup-menu
    // // http://scentric.net/tutorial/sec-selections-context-menus.html
    // g_signal_connect(G_OBJECT(tree_view), "popup-menu",
    //                  (GCallback) on_show_popup, this);
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

