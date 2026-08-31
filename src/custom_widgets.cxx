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

#include "../include/custom_widgets.h"
#include <FL/Enumerations.H>
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Scroll.H>
#include <string>

int SearchInput::handle(int event) {
    std::string new_value;
    if (event == FL_KEYDOWN) {
        switch (Fl::event_key()) {
            case FL_Up:
                new_value = ytdlp_source->getNextInSearchHistory();
                last_pressed_key = FL_Up;
                if (!new_value.empty()) this->value(new_value.c_str());
                return 1;
            case FL_Down:
                new_value = ytdlp_source->getPreviousInSearchHistory();
                last_pressed_key = FL_Down;
                if (!new_value.empty()) this->value(new_value.c_str());
                return 1;
            case FL_Enter:
                if (last_pressed_key != FL_Enter) {
                    this->do_callback();
                    last_pressed_key = FL_Enter;
                }
                return 1;
        }
    }
    last_pressed_key = Fl::event_key();
    return Fl_Input::handle(event);
}

void MessageQueue::add_message(const std::string& id, const std::string& title, const std::string& desc) {
    for (const std::unique_ptr<Message>& m: this->mssg_q) {
        if (m->id == id) return;
    }
    this->mssg_q.push_back(std::make_unique<Message>(id, title, desc));
};

void MessageQueue::remove_message(const std::string& id) {
    for (auto it = mssg_q.begin(); it != mssg_q.end(); it++) {
        if (it->get()->id == id) {
            this->mssg_q.erase(it);
            break;
        }
    }
}

void MessageQueue::eraseAll() {
    this->mssg_q.clear();
}

const std::vector<std::unique_ptr<Message>>& MessageQueue::get_messages() const{
    return this->mssg_q;
};

Message* MessageQueue::get_message(const std::string& id) const{
    for (const std::unique_ptr<Message>& m: this->mssg_q) {
        if (m->id == id) return m.get();
    }
    return nullptr;
}

bool MessageQueue::isEmpty() const{
    return this->mssg_q.empty();
}

Warning_Window::Warning_Window(int W,int H, const char * label, std::shared_ptr<MessageQueue> mssg_q): Fl_Window(W, H, label), mq(mssg_q) {
    this->color(FL_BACKGROUND_COLOR);
    this->selection_color(FL_BACKGROUND_COLOR);
    this->labeltype(FL_NO_LABEL);
    this->labelfont(0);
    this->labelsize(14);
    this->labelcolor(FL_FOREGROUND_COLOR);
    this->align(Fl_Align(FL_ALIGN_TOP));
    this->when(FL_WHEN_RELEASE);
    begin();

    scroll = new Fl_Scroll(10, 10, w() - 25, h() - 25);
    scroll->begin();
    scroll->align(Fl_Align(FL_ALIGN_RIGHT));
    scroll->type(Fl_Scroll::VERTICAL);

    messages_w = new Fl_Pack(20, 10, w() - 25, h() - 25);
    messages_w->type(Fl_Pack::VERTICAL);
    messages_w->spacing(5);
    messages_w->begin();

    int group_height = 80;
    int pos_height = std::floor(group_height * 0.3);
    for (const auto& mssg: this->mq->get_messages()){
        Fl_Group *g = new Fl_Group(0,0,messages_w->w(), group_height);
        g->begin();

        Fl_Box *b = new Fl_Box(0, 0, messages_w->w() - 40, pos_height);
        b->box(FL_NO_BOX);
        b->labelfont(FL_HELVETICA_BOLD);
        // b->labelcolor(FL_SELECTION_COLOR);
        b->align(Fl_Align(FL_ALIGN_BOTTOM_LEFT|FL_ALIGN_INSIDE|FL_ALIGN_WRAP));
        b->copy_label(mssg->title.c_str());

        Fl_Box *b2 = new Fl_Box(0, pos_height, messages_w->w() - 40, std::ceil(g->h() * 0.7));
        b2->box(FL_THIN_DOWN_BOX);
        b2->labelfont(FL_HELVETICA_ITALIC);
        b2->align(Fl_Align(FL_ALIGN_TOP_LEFT|FL_ALIGN_INSIDE|FL_ALIGN_WRAP));
        b2->copy_label(mssg->description.c_str());

        g->insert(*b,0);
        g->insert(*b2,1);
        g->end();
    }

    messages_w->end();
    scroll->end();

    end();
    set_modal();
};
