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

namespace np1sec_plugin {

class Toolbar {
public:
    struct Button {
    public:
        void on_click(std::function<void()>);

    private:
        friend class Toolbar;

        Button(GtkBox*, const char* label);

        GtkWidget* _button_widget;
        std::function<void()> _on_click;

        static void on_clicked(GtkWidget*, Button* self);
    };
public:
    Toolbar(PidginConversation*);

    std::unique_ptr<Button> create_channel_button;
    std::unique_ptr<Button> search_channel_button;

    ~Toolbar();

private:
    PidginConversation* _gtkconv;
    GtkWidget* _toolbar_box;
    GtkWidget* _create_channel_button;
    GtkWidget* _search_channel_button;
};

//------------------------------------------------------------------------------
// Implementation
//------------------------------------------------------------------------------
inline Toolbar::Button::Button(GtkBox* box, const char* label)
{
    _button_widget = gtk_button_new_with_label(label);
    gtk_box_pack_start(GTK_BOX(box), _button_widget, FALSE, FALSE, 0);
    gtk_widget_show(_button_widget);
    gtk_widget_set_sensitive(_button_widget, false);

    gtk_signal_connect(GTK_OBJECT(_button_widget), "clicked"
            , GTK_SIGNAL_FUNC(on_clicked), this);
}

inline void Toolbar::Button::on_click(std::function<void()> f)
{
    gtk_widget_set_sensitive(_button_widget, bool(f));
    _on_click = std::move(f);
}

inline void Toolbar::Button::on_clicked(GtkWidget*, Button* self)
{
    if (!self->_on_click) return;
    /* _on_click may be replaced during its execution so need to
     * create a copy */
    auto func = self->_on_click;
    func();
}

inline Toolbar::Toolbar(PidginConversation* gtkconv)
    : _gtkconv(gtkconv)
{
    if (!gtkconv) {
        assert(0 && "Not a pidgin conversation");
        return;
    }

    _toolbar_box = gtk_vbox_new(FALSE, PIDGIN_HIG_BOX_SPACE);

    gtk_box_pack_start(GTK_BOX(_gtkconv->lower_hbox), _toolbar_box, FALSE, FALSE, 0);
    gtk_widget_show(_toolbar_box);

    create_channel_button.reset(new Button(GTK_BOX(_toolbar_box), "Create channel"));
    search_channel_button.reset(new Button(GTK_BOX(_toolbar_box), "Search channels"));
}

inline Toolbar::~Toolbar()
{
    gtk_container_remove(GTK_CONTAINER(_gtkconv->lower_hbox), _toolbar_box);
}

} // np1sec_plugin namespace
