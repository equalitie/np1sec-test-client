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

class GlobalSignals {
public:
    GlobalSignals();
    ~GlobalSignals();

    static GlobalSignals& instance();

    std::function<void(PurpleConversation*)> on_conversation_created;
    std::function<void(PurpleConversation*)> on_conversation_deleted;

private:
    static GlobalSignals** ptrptr();

    static void conversation_created_cb(PurpleConversation *conv, GlobalSignals*);
    static void deleting_conversation_cb(PurpleConversation *conv, GlobalSignals*);
};

inline GlobalSignals::GlobalSignals()
{
    assert(!*ptrptr());
    *ptrptr() = this;

    void* conv_handle = purple_conversations_get_handle();
    purple_signal_connect(conv_handle, "conversation-created", this, PURPLE_CALLBACK(conversation_created_cb), this);
    purple_signal_connect(conv_handle, "deleting-conversation", this, PURPLE_CALLBACK(deleting_conversation_cb), this);
}

inline GlobalSignals::~GlobalSignals()
{
    void* conv_handle = purple_conversations_get_handle();
    purple_signal_disconnect(conv_handle, "conversation-created", this, PURPLE_CALLBACK(conversation_created_cb));
    purple_signal_disconnect(conv_handle, "deleting-conversation", this, PURPLE_CALLBACK(deleting_conversation_cb));
}

inline void GlobalSignals::conversation_created_cb(PurpleConversation *conv, GlobalSignals* self)
{
    if (self->on_conversation_created) {
        auto f = self->on_conversation_created;
        f(conv);
    }
}

inline void GlobalSignals::deleting_conversation_cb(PurpleConversation *conv, GlobalSignals* self)
{
    if (self->on_conversation_deleted) {
        auto f = self->on_conversation_deleted;
        f(conv);
    }
}

inline GlobalSignals** GlobalSignals::ptrptr()
{
    static GlobalSignals* _instance = nullptr;
    return &_instance;
}

inline GlobalSignals& GlobalSignals::instance() {
    assert(*ptrptr());
    return **ptrptr();
}

} // np1sec_plugin namespace
