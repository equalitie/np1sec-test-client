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
        ~Button();
    private:
        friend class Toolbar;

        Button(GtkBox*, const char* label, std::function<void()>);

        GtkWidget* _parent_widget;
        GtkWidget* _button_widget;
        std::function<void()> _on_click;

        static void on_clicked(GtkWidget*, Button* self);
    };

public:
    Toolbar(PidginConversation*);

    void add_button(const std::string&, std::function<void()>);
    void remove_button(const std::string&);

    ~Toolbar();

private:
    PidginConversation* _gtkconv;
    GtkWidget* _toolbar_box;
    std::map<std::string, std::unique_ptr<Button>> _buttons;
};

//------------------------------------------------------------------------------
// Implementation
//------------------------------------------------------------------------------
inline Toolbar::Button::Button(GtkBox* box, const char* label, std::function<void()> on_click)
    : _parent_widget(GTK_WIDGET(box))
    , _on_click(std::move(on_click))
{
    _button_widget = gtk_button_new_with_label(label);
    gtk_box_pack_start(GTK_BOX(box), _button_widget, FALSE, FALSE, 0);
    gtk_widget_show(_button_widget);

    gtk_signal_connect(GTK_OBJECT(_button_widget), "clicked"
            , GTK_SIGNAL_FUNC(on_clicked), this);
}

inline Toolbar::Button::~Button()
{
    gtk_container_remove(GTK_CONTAINER(_parent_widget), _button_widget);
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
}

inline void Toolbar::add_button(const std::string& text, std::function<void()> f)
{
    auto& b = _buttons[text];
    b.reset(new Button(GTK_BOX(_toolbar_box), text.c_str(), std::move(f)));
}

inline void Toolbar::remove_button(const std::string& text)
{
    _buttons.erase(text);
}

inline Toolbar::~Toolbar()
{
    gtk_container_remove(GTK_CONTAINER(_gtkconv->lower_hbox), _toolbar_box);
}

} // np1sec_plugin namespace
