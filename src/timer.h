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

class TimerToken final : public np1sec::TimerToken {
public:
    class Storage : private std::set<TimerToken*> {
    public:
        friend class TimerToken;

        ~Storage() {
            for (auto* p : *this) delete p;
        }
    };

public:

    TimerToken( Storage& storage
              , uint32_t interval_ms
              , np1sec::TimerCallback* callback)
        : _storage(storage)
        , _callback(callback)
    {
        _storage.insert(this);
        _timer_id = g_timeout_add(interval_ms, execute_timer_callback, this);
    }

    void unset() override {
        g_source_remove(_timer_id);
        _timer_id = 0;
        delete this;
    }

    ~TimerToken() {
        _storage.erase(this);
        if (_timer_id != 0) g_source_remove(_timer_id);
    }

private:
    static gboolean execute_timer_callback(gpointer gp_data);

private:
    Storage& _storage;
    np1sec::TimerCallback* _callback;
    guint _timer_id;
};

inline
gboolean TimerToken::execute_timer_callback(gpointer gp_data)
{
    auto* token = reinterpret_cast<TimerToken*>(gp_data);

    auto callback = token->_callback;

    token->_timer_id = 0;
    delete token;

    callback->execute();

    // Returning 0 stops the timer.
    return 0;
}

} // np1sec_plugin namespace
