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
#include <boost/optional.hpp>

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

    static
    boost::optional<size_t> path_to_pos(GtkTreePath*);

    void add_user(User*);
    void remove_user(User*);

private:
    friend class User;

    GtkTreeView* _tree_view;
    GtkListStore* _store;

    //            +--> Path in the tree
    //            |
    //std::map<std::string, std::function<void()>> _double_click_callbacks;
    //std::map<std::string, std::function<void(GdkEventButton*)>> _show_popup_callbacks;

    std::list<User*> _users;
    std::set<gint>   _signal_handlers;
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
    void do_set_text(const std::string&);

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
    for (auto h_id : _signal_handlers) {
        g_signal_handler_disconnect(G_OBJECT(_tree_view), h_id);
    }

    g_object_unref(_store);
    g_object_unref(_tree_view);

    for (auto u : _users) {
        u->_user_list = nullptr;
    }
}

inline
void UserList::add_user(UserList::User* u)
{
    _users.push_back(u);
    gtk_list_store_append(_store, &u->_iter);
}

inline
void UserList::remove_user(UserList::User* u)
{
    auto ui = std::find(_users.begin(), _users.end(), u);

    assert(ui != _users.end());
    _users.erase(ui);

    gtk_list_store_remove(_store, &u->_iter);
}

inline
boost::optional<size_t> UserList::path_to_pos(GtkTreePath* path)
{
    if (!path) return boost::none;

    gchar* c_str = gtk_tree_path_to_string(path);
    auto free_str = defer([c_str] { g_free(c_str); });

    try {
        return std::stoi(c_str);
    }
    catch (const std::exception&) { }

    return boost::none;
}

inline
void UserList::on_double_click( GtkTreeView *tree_view
                              , GtkTreePath *path
                              , GtkTreeViewColumn *column
                              , UserList* v)
{
    auto pos = path_to_pos(path);

    if (!pos) return;

    auto i = std::next(v->_users.begin(), *pos);

    if (i == v->_users.end()) return;

    if ((*i)->on_double_click) {
        // Make a copy in case the callback wants to
        // reset it.
        auto f = (*i)->on_double_click;
        f();
    }
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

        auto pos = path_to_pos(path);

        if (!pos) return FALSE;

        auto user_i = std::next(v->_users.begin(), *pos);

        if (user_i == v->_users.end()) return FALSE;

        auto* user = *user_i;

        if (!user->popup_actions.empty()) {
            show_popup(event, user->popup_actions);
            return TRUE;
        }
    }

    return FALSE;
}

inline
void UserList::setup_callbacks(GtkTreeView* tree_view)
{
    auto id1 = g_signal_connect(GTK_WIDGET(tree_view), "row-activated",
                                G_CALLBACK(on_double_click), this);

    auto id2 = g_signal_connect(G_OBJECT(tree_view), "button-press-event",
                                (GCallback) on_button_pressed, this);

    _signal_handlers.insert(id1);
    _signal_handlers.insert(id2);

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
    if (_user_list == &list) {
        return;
    }

    if (_user_list) {
        _user_list->remove_user(this);
    }

    _user_list = &list;

    list.add_user(this);

    if (!_text.empty()) {
        do_set_text(_text);
    }
}

inline void UserList::User::set_text(std::string str)
{
    _text = std::move(str);
    if (!_user_list) return;
    do_set_text(_text);
}

inline void UserList::User::do_set_text(const std::string& str)
{
    assert(_user_list);
    gtk_list_store_set(_user_list->_store, &_iter,
                       COL_NAME, str.c_str(), -1);
}

inline
std::string UserList::User::path() const
{
    return util::gtk::tree_iter_to_path(_iter, GTK_TREE_STORE(_user_list->_store));
}

inline UserList::User::~User()
{
    if (!_user_list) return;
    _user_list->remove_user(this);
}

} // np1sec_plugin namespace


