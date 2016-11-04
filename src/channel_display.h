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

class ChannelDisplay {
    enum
    {
      COL_NAME = 0,
      NUM_COLS
    };

    struct Channel {
        GtkTreeStore* _tree_store;
        GtkTreeIter _tree_iterator;
        std::set<std::string> _members;

        Channel(GtkTreeStore* tree_store, GtkTreeIter iter)
            : _tree_store(tree_store)
            , _tree_iterator(iter)
        {}

        void add_member(const std::string&);
    };

public:
    static
    std::unique_ptr<ChannelDisplay> create_new_in(PurpleConversation* conv);

    ChannelDisplay(GtkTreeView* tree_view);

    ChannelDisplay(ChannelDisplay&&) = delete;
    ChannelDisplay(const ChannelDisplay&) = delete;
    ChannelDisplay& operator=(const ChannelDisplay&) = delete;
    ChannelDisplay& operator=(ChannelDisplay&&) = delete;

    ~ChannelDisplay();

    Channel& operator[](size_t);

private:
    static GtkWidget* get_nth_child(gint n, GtkContainer* c);

private:
    GtkTreeView* _tree_view;
    GtkTreeStore* _tree_store;

    std::map<size_t, Channel> _channels;
};

//------------------------------------------------------------------------------
// Implementation
//------------------------------------------------------------------------------
inline
ChannelDisplay::ChannelDisplay(GtkTreeView* tree_view)
    : _tree_view(tree_view)
{
    gtk_tree_view_insert_column_with_attributes (_tree_view,
                                                 -1,      
                                                 "",  
                                                 gtk_cell_renderer_text_new(),
                                                 "text", COL_NAME,
                                                 NULL);

    _tree_store = gtk_tree_store_new (NUM_COLS, G_TYPE_STRING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(_tree_view), GTK_TREE_MODEL(_tree_store));

    (*this)[0].add_member("alice");
    (*this)[0].add_member("bob");
    (*this)[1].add_member("charlie");
    (*this)[1].add_member("dave");
}

inline
void ChannelDisplay::Channel::add_member(const std::string& member) {
    GtkTreeIter iter;
    gtk_tree_store_append(_tree_store, &iter, &_tree_iterator);
    gtk_tree_store_set(_tree_store, &iter,
                       COL_NAME, member.c_str(),
                       -1);

}

inline
ChannelDisplay::Channel& ChannelDisplay::operator[](size_t channel_id) {
    auto channel_i = _channels.find(channel_id);

    if (channel_i == _channels.end()) {
        GtkTreeIter iter;

        gtk_tree_store_append(_tree_store, &iter, NULL);
        gtk_tree_store_set(_tree_store, &iter,
                           COL_NAME, std::to_string(channel_id).c_str(),
                           -1);

        channel_i = _channels.emplace(channel_id, Channel(_tree_store, iter)).first;
    }

    return channel_i->second;
}

inline
ChannelDisplay::~ChannelDisplay()
{
    g_object_unref(_tree_store);
}

inline
std::unique_ptr<ChannelDisplay>
ChannelDisplay::create_new_in(PurpleConversation* conv)
{
    auto* gtkconv = PIDGIN_CONVERSATION(conv);

    if (!gtkconv) {
        assert(0 && "Not a pidgin conversation");
        return nullptr;
    }

    GtkWidget* parent = gtk_widget_get_parent(gtkconv->lower_hbox);

    if (!GTK_IS_CONTAINER(parent)) {
        assert(0);
        return nullptr;
    }

    if (!GTK_IS_BOX(parent)) {
        assert(0);
        return nullptr;
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
        return nullptr;
    }

    g_object_ref(target);
    auto unref_target = defer([target] { g_object_unref(target); });

    gtk_container_remove(GTK_CONTAINER(parent), target);

	GtkWidget* hpaned = gtk_hpaned_new();
	gtk_box_pack_start(GTK_BOX(parent), hpaned, TRUE, TRUE, 0);
	gtk_widget_show(hpaned);
	gtk_paned_pack1(GTK_PANED(hpaned), target, TRUE, TRUE);

    GtkWidget* tree_view = gtk_tree_view_new();
	gtk_paned_pack2(GTK_PANED(hpaned), tree_view, TRUE, TRUE);
	gtk_widget_show(tree_view);

	//GtkWidget* button = gtk_button_new_with_label("_Button");
	//gtk_paned_pack2(GTK_PANED(hpaned), button, TRUE, TRUE);
	//gtk_widget_show(button);

	gtk_box_pack_start(GTK_BOX(parent), hpaned, TRUE, TRUE, 0);
	gtk_box_reorder_child(GTK_BOX(parent), hpaned, output_window_position);

    return std::make_unique<ChannelDisplay>(GTK_TREE_VIEW(tree_view));
}

inline
GtkWidget* ChannelDisplay::get_nth_child(gint n, GtkContainer* c) {
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
