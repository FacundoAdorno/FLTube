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
#ifndef FLTUBE_UTILS_H
#define FLTUBE_UTILS_H

#include <string>
#include <memory>
#include <array>
#include <map>
#include <vector>
#include <sstream>
#include <iostream>
#include <ctime>
#include <curl/curl.h>
#include <filesystem>
#include <cmath>

#include <FL/Fl_Image.H>
#include <FL/Fl_JPEG_Image.H>

const std::string YOUTUBE_EXTRACTOR_NAME = "youtube";
/*Enum for the target video resolutions. */
enum VCODEC_RESOLUTIONS {
    R240p = 240, R360p = 360, R480p = 480, R720p = 720, R1080p = 1080 };


/** */
enum FLTUBE_STATUS_CODES {
    FLT_OK=0, FLT_GENERAL_FAILED= 1, FLT_DOWNLOAD_FL_FAILED = 2, FLT_DOWNLOAD_FL_BYPASSED = 3,
};

/** Currently, AVC1 is the most supported codec for MP4 in old PC systems... **/
const std::string VIDEOCODEC_PREFERRED = "avc1";
const std::string AUDIOCODEC_PREFERRED = "m4a";

/** You can change here for the player of your like, in example: vlc, mpv, etc. It must be installed in your system. **/
const std::string DEFAULT_STREAM_PLAYER = "mplayer";

/** Youtube video metadata. */
struct YTDLP_Video_Metadata{
    std::string id;
    std::string title;
    std::string url;
    std::string upload_date;
    std::string creators;
    std::string duration;
    std::string channel_id;
    std::string thumbnail_url;
};

std::string exec(const char* cmd);

bool isUrl(const char* user_input);

bool isYoutubeURL(const char* url);

const std::string currentDateTime();

bool checkForYTDLP();

std::string do_ytdlp_search(const char* search_term, const char* extractor_name);

static std::string do_youtube_search(const char* byTerm);

void stream_video(const char* video_url);

void download_video(const char* video_url, const char* download_path, const char* extractor_name, VCODEC_RESOLUTIONS v_resolution);

YTDLP_Video_Metadata* parse_YT_Video_Metadata(const char ytdlp_video_metadata[512]);

int download_file(std::string url, std::string output_dir, std::string outfilename, bool overwrite = false);

Fl_Image* create_resized_image_from_jpg(std::string jpg_filepath, int target_width);


#endif
