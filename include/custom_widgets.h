/*
 * Copyright (C) 2025-2026 - FLtube
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License, version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 */
#ifndef CUSTOM_WIDGETS_H
#define CUSTOM_WIDGETS_H

#include "ytdlp_helper.h"
#include <FL/Enumerations.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Scroll.H>
#include <memory>

struct Message {
    std::string id;
    std::string title;
    std::string description;

    Message(std::string id, std::string title, std::string description = ""):
        id(id), title(title), description(description) {};
};

class SearchInput: public Fl_Input {
private:
    std::shared_ptr<YtDlp_Helper> ytdlp_source;
    int last_pressed_key = 0;       // Used to track last pressed key before current, in order to avoid some undesired
                                    // behaviour for some graphics server (in particular, Wayland repeat infinitelly the
                                    // last pressed key after pressing FL_Enter, I don't know why...).
public:
    SearchInput(int X,int Y,int W,int H, const char * label): Fl_Input(X,Y,W,H,label){}
    void set_search_source(std::shared_ptr<YtDlp_Helper> source) {
        ytdlp_source = source;
    }
    int handle(int event);
};

class MessageQueue {
private:
    std::vector<std::unique_ptr<Message>> mssg_q{};
public:
    MessageQueue() = default;
    void add_message(const std::string& id, const std::string& title, const std::string& desc);
    void remove_message(const std::string& id);
    void eraseAll();
    Message* get_message(const std::string& id) const;
    const std::vector<std::unique_ptr<Message>>& get_messages() const;
    bool isEmpty() const;
};

class Warning_Window: public Fl_Window {
private:
    Fl_Scroll* scroll;
    Fl_Pack* messages_w;
    std::shared_ptr<MessageQueue> mq;
public:
    Warning_Window(int W,int H, const char * label, std::shared_ptr<MessageQueue> mq);
};


#endif  // CUSTOM_WIDGETS_H
