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

#include <set>
#include <list>
#include <ostream>
#include <sstream>
#include <pidgin/gtkimhtml.h>
#include <future>
#include "defer.h"

template<class T>
std::ostream& operator<<(std::ostream& os, const std::set<T>& s) {
    os << "{";
    for (auto i = s.begin(); i != s.end(); ++i) {
        if (i != s.begin()) os << ", ";
        os << *i;
    }
    os << "}";
    return os;
}

template<class T>
std::ostream& operator<<(std::ostream& os, const std::list<T>& s) {
    os << "[";
    for (auto i = s.begin(); i != s.end(); ++i) {
        if (i != s.begin()) os << ", ";
        os << *i;
    }
    os << "]";
    return os;
}

template<class T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& s) {
    os << "(";
    for (auto i = s.begin(); i != s.end(); ++i) {
        if (i != s.begin()) os << ", ";
        os << *i;
    }
    os << ")";
    return os;
}

namespace np1sec_plugin {
namespace util {

namespace _detail {
    void stringify(std::ostream& os) {
    }

    template<class Arg, class... Args>
    void stringify(std::ostream& os, Arg&& arg, Args&&... args) {
        os << arg;
        stringify(os, std::forward<Args>(args)...);
    }
}

template<class... Args>
std::string str(Args&&... args) {
    std::stringstream ss;
    _detail::stringify(ss, std::forward<Args>(args)...);
    return ss.str();
}

template<class... Args>
std::string inform_str(Args&&... args) {
    std::stringstream ss;
    _detail::stringify(ss, std::forward<Args>(args)...);
    return "<font color=\"#9A9A9A\">" + ss.str() + "</font>";
}

/**
 * Execute the function f and report if the execution takes too long.
 */
template<class F, class... Args>
auto exec(const char* msg, F&& f, Args&&... args)
{
    using namespace std::chrono;

    auto start = steady_clock::now();

    auto future = std::async(std::launch::async, std::move(f));
    auto status = future.wait_for(milliseconds(300));

    assert(status != std::future_status::deferred);

    if (status == std::future_status::timeout) {
        std::cout << "function '"
                  << msg << "' takes too long to execute"
                  << std::endl;

        future.wait();
        auto end = steady_clock::now();

        auto diff_ms = duration_cast<milliseconds>(end - start).count();

        std::cout << "     took: " << ((float) diff_ms / 1000)
                  << " seconds" << std::endl;
    }

    assert(status == std::future_status::ready);

    return future.get();
}

namespace gtk {

inline
GtkWidget* get_nth_child(gint n, GtkContainer* c) {
    GList* children = gtk_container_get_children(c);

    auto free_children = defer([children] { g_list_free(children); });

    for (auto* l = children; l != NULL; l = l->next) {
        if (n-- == 0) {
            return GTK_WIDGET(l->data);
        }
    }

    return nullptr;
}

inline
std::string remove_text(GtkIMHtml* imhtml) {
    auto text = gtk_imhtml_get_text(imhtml, NULL, NULL);
    auto free_text = defer([text] { g_free(text); });
    gtk_imhtml_clear(imhtml);
    return std::string(text);
}

inline
void set_text(GtkIMHtml* imhtml, const std::string& str) {
    gtk_imhtml_clear(imhtml);
    gtk_imhtml_append_text(imhtml, str.c_str(), GtkIMHtmlOptions(0));
}

inline
std::string tree_iter_to_path(const GtkTreeIter& iter, GtkTreeStore* store)
{
    gchar* c_str = gtk_tree_model_get_string_from_iter
        ( GTK_TREE_MODEL(store)
        , const_cast<GtkTreeIter*>(&iter));

    auto free_str = defer([c_str] { g_free(c_str); });

    return std::string(c_str);
}

} // gtk namespace
} // util namespace
} // np1sec_plugin namespace
