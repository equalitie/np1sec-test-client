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
#include "popup.h"

namespace np1sec_plugin {

//------------------------------------------------------------------------------
// UserList
//------------------------------------------------------------------------------
class UserList {
public:
    class User;

public:
    UserList(const std::string& label);

    UserList(UserList&&) = delete;
    UserList(const UserList&) = delete;
    UserList& operator=(const UserList&) = delete;
    UserList& operator=(UserList&&) = delete;

    ~UserList();

    GtkWidget* root_widget() { return GTK_WIDGET(_tree_view); }

    bool is_in(const User&) const;

private:
    void setup_callbacks(GtkTreeView* tree_view);

    static
    void on_double_click( GtkTreeView*
                        , GtkTreePath*
                        , GtkTreeViewColumn*
                        , UserList*);

    static
    gint on_button_pressed(GtkWidget*, GdkEventButton*, UserList*);

private:
    friend class User;

    GtkTreeView* _tree_view;
    GtkListStore* _store;

    //            +--> Path in the tree
    //            |
    std::map<std::string, std::function<void()>> _double_click_callbacks;
    std::map<std::string, std::function<void(GdkEventButton*)>> _show_popup_callbacks;

    std::set<User*> _users;
};

//------------------------------------------------------------------------------
// UserList::User
//------------------------------------------------------------------------------
class UserList::User {
    friend class UserList;

    enum
    {
      COL_NAME = 0,
      NUM_COLS
    };

public:
    User();

    void set_text(std::string);
    void bind(UserList&);

    User(const User&) = delete;
    User& operator=(const User&) = delete;

    User(User&&) = delete;
    User& operator=(User&&) = delete;

    std::function<void()> on_double_click;
    PopupActions popup_actions;

    ~User();

private:
    std::string path() const;

private:
    UserList* _user_list = nullptr;
    std::string _text;
    GtkTreeIter _iter;
};

//------------------------------------------------------------------------------
// UserList implementation
//------------------------------------------------------------------------------
inline UserList::UserList(const std::string& label)
{
    _tree_view = GTK_TREE_VIEW(gtk_tree_view_new());
    g_object_ref(_tree_view);

    gtk_widget_show(GTK_WIDGET(_tree_view));

    gtk_tree_view_insert_column_with_attributes( _tree_view
                                               , -1
                                               , label.c_str()
                                               , gtk_cell_renderer_text_new()
                                               , "text", User::COL_NAME
                                               , NULL);

    _store = gtk_list_store_new(User::NUM_COLS, G_TYPE_STRING);
    gtk_tree_view_set_model(_tree_view, GTK_TREE_MODEL(_store));

    setup_callbacks(_tree_view);
}

inline bool UserList::is_in(const User& u) const
{
    return u._user_list == this;
}

inline UserList::~UserList()
{
    g_object_unref(_store);
    g_object_unref(_tree_view);

    for (auto u : _users) {
        u->_user_list = nullptr;
    }
}

inline
void UserList::on_double_click( GtkTreeView *tree_view
                              , GtkTreePath *path
                              , GtkTreeViewColumn *column
                              , UserList* v)
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
gboolean UserList::on_button_pressed( GtkWidget*
                                    , GdkEventButton* event
                                    , UserList* v)
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
        gtk_tree_model_get_iter(GTK_TREE_MODEL(v->_store), &iter, path);

        auto path_str = util::gtk::tree_iter_to_path(iter, GTK_TREE_STORE(v->_store));

        auto action_i = v->_show_popup_callbacks.find(path_str);

        if (action_i == v->_show_popup_callbacks.end())
            return FALSE;

        action_i->second(event);

        return TRUE;
    }

    return FALSE;
}

inline
void UserList::setup_callbacks(GtkTreeView* tree_view)
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

//------------------------------------------------------------------------------
// UserList::User Implementation
//------------------------------------------------------------------------------
inline UserList::User::User()
{
}

inline void UserList::User::bind(UserList& list)
{
    assert(!_user_list || _user_list == &list);

    if (_user_list == &list) return;

    _user_list = &list;

    _user_list->_users.insert(this);

    gtk_list_store_append(_user_list->_store, &_iter);

    if (!_text.empty()) {
        gtk_list_store_set(_user_list->_store, &_iter,
                           COL_NAME, _text.c_str(),
                           -1);
    }

    auto p = path();

    _user_list->_double_click_callbacks[p] = [this] {
        if (on_double_click) on_double_click();
    };

    _user_list->_show_popup_callbacks[p] = [this] (GdkEventButton* e) {
        show_popup(e, popup_actions);
    };
}

inline void UserList::User::set_text(std::string str)
{
    _text = std::move(str);

    if (!_user_list) return;

    gtk_list_store_set(_user_list->_store, &_iter,
                       COL_NAME, _text.c_str(),
                       -1);
}

inline
std::string UserList::User::path() const
{
    return util::gtk::tree_iter_to_path(_iter, GTK_TREE_STORE(_user_list->_store));
}

inline UserList::User::~User()
{
    if (!_user_list) return;
    _user_list->_double_click_callbacks.erase(path());
    gtk_list_store_remove(_user_list->_store, &_iter);
}

} // np1sec_plugin namespace


