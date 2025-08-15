/*
 * Copyright (C) 2025 - FLtube
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
#ifndef FLTUBE_H
#define FLTUBE_H

#include "FLTube_View.h"
#include <array>
#include <sstream>
#include <map>
#include <filesystem>

enum LogLevel { INFO, WARN, ERROR };

/**  Save this object as user_data in buttons callbacks for Video Info. */
struct DownloadVideoCBData {
    int video_resolution;
    std::string video_url;
};


void exitApp(unsigned short int exitStatusCode);
static void closeWindow(Fl_Widget*, Fl_Window *targetWindow);
static void showMessageWindow(const char* message);
static void logAtBuffer(std::string log_message, LogLevel log_lvl);
static void add_video_group(int posx, int posy);
static VideoInfo* create_video_group(int posx, int posy);
static void pre_init();
static void post_init();

#endif
