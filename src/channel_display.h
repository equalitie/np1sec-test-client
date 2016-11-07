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

template<class ChannelData>
class ChannelDisplay {
    enum
    {
      COL_NAME = 0,
      NUM_COLS
    };

public:
    struct Channel {
        ChannelData data;

        template<class CD>
        Channel( ChannelDisplay& display
               , GtkTreeIter iter
               , CD&& data)
            : data(std::forward<CD>(data))
            , _display(display)
            , _tree_iterator(iter)
        {}

        void add_member(const std::string&);

        private:
        ChannelDisplay& _display;
        GtkTreeIter _tree_iterator;
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

    Channel& create(size_t, ChannelData&&);

    bool exists(size_t channel) const { return _channels.count(channel) != 0; }

    void erase(size_t channel);

    Channel* find_channel(size_t channel);

private:
    static GtkWidget* get_nth_child(gint n, GtkContainer* c);
    static void expand(GtkTreeStore*, GtkTreeView*, GtkTreeIter& iter);

private:
    GtkTreeView* _tree_view;
    GtkTreeStore* _tree_store;

    std::map<size_t, Channel> _channels;
};

//------------------------------------------------------------------------------
// Implementation
//------------------------------------------------------------------------------
template<class CD>
inline
ChannelDisplay<CD>::ChannelDisplay(GtkTreeView* tree_view)
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
}

template<class CD>
inline
void ChannelDisplay<CD>::Channel::add_member(const std::string& member) {
    std::cout << this << " add member " << member << std::endl;
    GtkTreeIter iter;
    gtk_tree_store_append(_display._tree_store, &iter, &_tree_iterator);
    gtk_tree_store_set(_display._tree_store, &iter,
                       COL_NAME, member.c_str(),
                       -1);

    expand(_display._tree_store, _display._tree_view, iter);
}

template<class CD> inline void ChannelDisplay<CD>::erase(size_t channel)
{
    assert(exists(channel));
    _channels.erase(channel);
}

template<class CD>
inline
typename ChannelDisplay<CD>::Channel&
ChannelDisplay<CD>::create(size_t channel_id, CD&& data) {
    assert(_channels.count(channel_id) == 0);

    GtkTreeIter iter;

    gtk_tree_store_append(_tree_store, &iter, NULL);
    gtk_tree_store_set(_tree_store, &iter,
                       COL_NAME, std::to_string(channel_id).c_str(),
                       -1);

    return _channels.emplace(channel_id, Channel(*this, iter, std::move(data))).first->second;
}

template<class CD>
inline
typename ChannelDisplay<CD>::Channel*
ChannelDisplay<CD>::find_channel(size_t channel_id) {
    auto i = _channels.find(channel_id);
    if (i == _channels.end()) return nullptr;
    return &i->second;
}

template<class CD>
inline
ChannelDisplay<CD>::~ChannelDisplay()
{
    g_object_unref(_tree_store);
}

template<class CD>
inline
std::unique_ptr<ChannelDisplay<CD>>
ChannelDisplay<CD>::create_new_in(PurpleConversation* conv)
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

    GtkWidget* userlist = gtk_paned_get_child2(GTK_PANED(target));

    g_object_ref(userlist);
    auto unref_userlist = defer([userlist] { g_object_unref(userlist); });

    gtk_container_remove(GTK_CONTAINER(target), userlist);

	GtkWidget* vpaned = gtk_vpaned_new();
    gtk_paned_pack2(GTK_PANED(target), vpaned, TRUE, TRUE);

	gtk_widget_show(vpaned);

	gtk_paned_pack1(GTK_PANED(vpaned), userlist, TRUE, TRUE);

    GtkWidget* tree_view = gtk_tree_view_new();
	gtk_paned_pack2(GTK_PANED(vpaned), tree_view, TRUE, TRUE);

    // TODO: Not sure why the value of 1 gives a good result
	gtk_widget_set_size_request(vpaned, 1, -1);
	gtk_widget_show(tree_view);

    return std::make_unique<ChannelDisplay>(GTK_TREE_VIEW(tree_view));
}

template<class CD>
inline
GtkWidget* ChannelDisplay<CD>::get_nth_child(gint n, GtkContainer* c) {
    GList* children = gtk_container_get_children(c);
    
    auto free_children = defer([children] { g_list_free(children); });
    
    for (auto* l = children; l != NULL; l = l->next) {
        if (n-- == 0) {
            return GTK_WIDGET(l->data);
        }
    }
    
    return nullptr;
}

template<class CD>
inline
void ChannelDisplay<CD>::expand(GtkTreeStore* tree_store, GtkTreeView* tree_view, GtkTreeIter& iter) {
    auto model = GTK_TREE_MODEL(tree_store);

    gchar* path_str = gtk_tree_model_get_string_from_iter(model, &iter);
    auto free_path_str = defer([path_str] { g_free(path_str); });

    GtkTreePath* path = gtk_tree_path_new_from_string(path_str);
    auto free_path = defer([path] { gtk_tree_path_free(path); });

    gtk_tree_view_expand_to_path(tree_view, path);
}

} // np1sec_plugin namespace
