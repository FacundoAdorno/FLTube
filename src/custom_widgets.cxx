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
