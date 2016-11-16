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

#include <boost/optional.hpp>

namespace np1sec_plugin {

class Notebook {
public:
    class Page {
    public:
        Page(Notebook&, GtkWidget* content, const std::string& label);

        void set_current();
        void on_set_current(std::function<void()>);
        void on_set_not_current(std::function<void()>);

        Page(const Page&) = delete;
        Page(Page&&) = delete;
        Page& operator=(const Page&) = delete;
        Page& operator=(Page&&) = delete;

        ~Page();

    private:
        Notebook& _notebook;
        GtkWidget* _content;
        gint _id;
    };

public:
    Notebook();

    Notebook(const Notebook&) = delete;
    Notebook(Notebook&&) = delete;

    Notebook& operator=(const Notebook&) = delete;
    Notebook& operator=(Notebook&&) = delete;

    GtkWidget* root_widget() { return _notebook; }

    ~Notebook();

private:
    static gboolean on_page_selected(GtkNotebook*, gboolean, guint, Notebook*);

private:
    GtkWidget* _notebook;
    std::map<guint, std::function<void()>> _on_current_callbacks;
    std::map<guint, std::function<void()>> _on_not_current_callbacks;
    boost::optional<guint> _current_page;
};

inline
Notebook::Notebook()
{
    _notebook = gtk_notebook_new();
    g_object_ref_sink(_notebook);
    gtk_widget_show(_notebook);

    g_signal_connect(GTK_WIDGET(_notebook), "switch-page",
                     G_CALLBACK(on_page_selected), this);
}

inline
Notebook::~Notebook()
{
    g_object_unref(_notebook);
}

inline
gboolean Notebook::on_page_selected(GtkNotebook*, gboolean, guint page_num, Notebook* self)
{
    auto prev = self->_current_page;
    self->_current_page = page_num;

    if (prev) {
        auto cb_i = self->_on_not_current_callbacks.find(*prev);

        if (cb_i != self->_on_not_current_callbacks.end()) {
            auto f = cb_i->second;
            f();
        }
    }

    auto cb_i = self->_on_current_callbacks.find(page_num);

    if (cb_i == self->_on_current_callbacks.end()) {
        return false;
    }

    auto f = cb_i->second;
    f();

    return true;
}

inline
Notebook::Page::Page(Notebook& notebook, GtkWidget* content, const std::string& label)
    : _notebook(notebook)
    , _content(content)
{
    auto* label_w = gtk_label_new(label.c_str());

    _id = gtk_notebook_append_page( GTK_NOTEBOOK(_notebook._notebook)
                                  , content
                                  , label_w);

    gtk_widget_show(content);
}

inline
void Notebook::Page::set_current()
{
    if (_id == -1) return;
    gtk_notebook_set_current_page(GTK_NOTEBOOK(_notebook._notebook), _id);
}

inline
void Notebook::Page::on_set_current(std::function<void()> f)
{
    if (f) {
        _notebook._on_current_callbacks[_id] = std::move(f);
    }
    else {
        _notebook._on_current_callbacks.erase(_id);
    }
}

inline
void Notebook::Page::on_set_not_current(std::function<void()> f)
{
    if (f) {
        _notebook._on_not_current_callbacks[_id] = std::move(f);
    }
    else {
        _notebook._on_not_current_callbacks.erase(_id);
    }
}

inline
Notebook::Page::~Page()
{
    on_set_current(nullptr);
    on_set_not_current(nullptr);
    gtk_container_remove(GTK_CONTAINER(_notebook._notebook), _content);
}

} // np1sec_plugin namespace
