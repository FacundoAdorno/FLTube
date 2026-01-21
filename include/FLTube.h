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
#include <FL/Fl_File_Chooser.H>
#include <FL/Enumerations.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Text_Buffer.H>
#include <array>
#include <sstream>
#include <map>
#include <filesystem>
#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>
#include <thread>
#include <atomic>

/* The @VERSION MUST be expressed as numbers separated by dots (<number>.<number>.<number>), in example: 2.10.3.
 * MUST NOT BE something like "2.10.3.rc1"...
 */
static const char* VERSION = "2.1.0";

/**  Save this object as user_data in buttons callbacks for Video Info. */
struct DownloadVideoCBData {
    int video_resolution;
    std::string video_url;
};

void exitApp(unsigned short int exitStatusCode);
static void closeWindow_cb(Fl_Widget*, Fl_Window *targetWindow);
static void showMessageWindow(const char* message);
static void lock_buttons(bool lock);
static void preview_video_cb(Fl_Button* widget, void* video_url);
static void add_video_group(int posx, int posy);
static VideoInfo* create_video_group(int posx, int posy);
static void clear_video_info();
static void update_video_info();
static bool updateVideoMetadataFromVideoList();
static void getVideosAtList_cb(Fl_Choice* w, void* a);
static void selectCentralTab_cb(Fl_Choice* w, void* a);
static std::string getActiveTabName();
static void markLikedVideo_cb(Fl_Widget *wdg);
static void pre_init();
static void post_init();
static void doSearch(const char* input_text, bool is_a_channel = false);
static void getYTChannelVideo_cb(Fl_Button *bttn, void* channel_id_str);
static const char* getSearchValue(Fl_Input *input);
static void searchButtonAction_cb(Fl_Widget *wdgt, Fl_Input *input);
static void getPreviousSearchResults_cb(Fl_Widget*, Fl_Input *input);
static void getNextSearchResults_cb(Fl_Widget*, Fl_Input *input);
void showUsage(bool exitApp);
static void parseOptions(int argc, char **argv);
static int getIntVersion();

#endif
