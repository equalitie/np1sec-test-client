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

namespace np1sec_plugin { namespace mock {

namespace stdx = std::experimental;

struct Parser {
    using parse_error = std::runtime_error;

    size_t curpos = 0;
    std::string text;

    Parser(std::string text)
        : text(std::move(text))
    {
    }

    bool finished() const { return curpos == text.size(); }

    std::string read_command() {
        auto thrw = [](const char* err) {
            throw std::runtime_error(std::string("read_command: ") + err);
        };
        if (curpos == text.size()) thrw("attempt to read past the end");
        if (text[curpos] == ' ') thrw("command starts with space");

        auto p = text.find(' ', curpos);

        if (p != std::string::npos) {
            auto retval = text.substr(curpos, (p - curpos));
            curpos = p + 1;
            return retval;
        }

        auto retval = text.substr(curpos, text.size() - curpos);
        curpos = text.size();
        return retval;
    }

    // Example input: "(alice bob) charlie"
    //         output: "alice bob"
    std::string read_list() {
        auto thrw = [](const char* err) {
            throw std::runtime_error(std::string("parse_list: ") + err);
        };

        if (curpos == text.size()) thrw("attempt to read past the end");
        if (text[curpos] != '(') thrw("list doesn't start with '('");
        ++curpos;
        auto start = curpos;
        if (curpos == text.size()) thrw("input ends after '('");

        for (; curpos < text.size(); ++curpos) {
            while (text[curpos] == '(') {
                auto a = read_list();
            }
            if (text[curpos] == ')') {
                auto p = curpos++;
                consume_white_space();
                return text.substr(start, p - start);
            }
        } 

        thrw("input ends before ')'");
        return "";
    }

    void consume_white_space() {
        while (curpos < text.size() && text[curpos] == ' ') ++curpos;
    }

};

namespace detail {
    inline void parse(Parser& p, std::string& destination) {
        destination = p.read_command();
    }
    
    template<class T>
    inline void parse(Parser& p, std::list<T>& destination) {
        auto list = p.read_list();
        Parser p2(list);
        while (!p2.finished()) {
            T val;
            parse(p2, val);
            destination.push_back(std::move(val));
        }
    }
    
    template<class T>
    inline void parse(Parser& p, std::set<T>& destination) {
        auto list = p.read_list();
        Parser p2(list);
        while (!p2.finished()) {
            T val;
            parse(p2, val);
            destination.insert(std::move(val));
        }
    }
} // detail namespace

template<class T> inline T parse(Parser& p) {
    T retval;
    detail::parse(p, retval);
    return retval;
}

template<class Range> inline std::string encode_range(const Range& r);

inline
std::string encode(const std::string v) {
    return v;
}

template<class T> std::string encode(const std::list<T>& l) {
    return encode_range(l);
}

template<class T> std::string encode(const std::set<T>& l) {
    return encode_range(l);
}

template<class Range>
inline
std::string encode_range(const Range& r) {
    std::stringstream ss;
    ss << "(";
    for (auto i = r.begin(); i != r.end(); ++i) {
        if (i != r.begin()) ss << " ";
        ss << encode(*i);
    }
    ss << ")";
    return ss.str();
}


// TODO: Move this to a separate testing cpp file.
inline void run_parser_tests() {
    try {
        {
            Parser p("foo");
            auto c = p.read_command();
            assert(c == "foo");
        }

        {
            Parser p("foo bar");
            auto c1 = p.read_command();
            assert(c1 == "foo");
            auto c2 = p.read_command();
            assert(c2 == "bar");
        }

        {
            Parser p("(foo)");
            auto c = p.read_command();
            assert(c == "(foo)");
        }

        {
            Parser p("(foo)");
            auto c = p.read_list();
            assert(c == "foo");
        }

        {
            Parser p("(foo) (bar)");
            auto c1 = p.read_list();
            assert(c1 == "foo");
            auto c2 = p.read_list();
            assert(c2 == "bar");
        }

        {
            Parser p("(foo bar)");
            auto c = p.read_list();
            assert(c == "foo bar");
        }

        {
            Parser p("((foo) (bar))");
            auto c = p.read_list();
            assert(c == "(foo) (bar)");
        }
        {
            Parser p("((alice bob charlie) (dave) (eve filip))");
            auto c1 = p.read_list();
            assert(c1 == "(alice bob charlie) (dave) (eve filip)");
            Parser p2(c1);
            auto c2 = p2.read_list();
            assert(c2 == "alice bob charlie");
            auto c3 = p2.read_list();
            assert(c3 == "dave");
            auto c4 = p2.read_list();
            assert(c4 == "eve filip");
        }
    }
    catch(std::exception& e) {
        assert(0 && "run_parser_test failed");
    }
}

} // np1sec_plugin namespace
} // mock namespace
