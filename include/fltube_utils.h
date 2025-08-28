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
#include <fstream>
#include <sstream>
#include <iostream>
#include <ctime>
#include <curl/curl.h>
#include <filesystem>
#include <cmath>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <algorithm>

#include <FL/Fl_Image.H>
#include <FL/Fl_JPEG_Image.H>
#include "gnugettext_utils.h"

const std::string DOWNLOAD_VIDEO_PREFERRED_EXT = "mp4";

const std::string YOUTUBE_EXTRACTOR_NAME = "youtube";
/*Enum for the target video resolutions. */
enum VCODEC_RESOLUTIONS {
    R240p = 240, R360p = 360, R480p = 480, R720p = 720, R1080p = 1080 };


/** FLtube custom status codes definition... */
enum FLTUBE_STATUS_CODES {
    FLT_OK=0, FLT_GENERAL_FAILED= 1, FLT_DOWNLOAD_FL_FAILED = 2, FLT_DOWNLOAD_FL_BYPASSED = 3,
    FLT_INVALID_CMD_PARAM=4,
};

/** Simplet permissions scheme. */
enum SIMPLE_FS_PERMISSION {
        //Can this program READ this file/directory (Whether by 'user', 'group' or 'others' permission)?
        CAN_READ,
        //Can this program WRITE this file/directory (Whether by 'user', 'group' or 'others' permission)?
        CAN_WRITE,
        //Can this program EXECUTE this file/directory (Whether by 'user', 'group' or 'others' permission)?
        CAN_EXECUTE,
};

/** Info for page search results. Initial page has index 0. See parameter -I at yt-dlp documentation. */
struct Pagination_Info {
    int size;
    int index;

    Pagination_Info(int size, int index): size(size), index(index) {}

    /** Minimum lower end is 1. */
    int lower_end() const {
        return (index * size) + 1;
    }

    int upper_end() const {
        return ((index + 1) * size);
    }
};

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

const char* getHomePathOr(const char* defaultPath);

std::vector<int> getUserGroupsIds(int uid);

bool checkDirectoryPermissions(const char* directory, std::array<SIMPLE_FS_PERMISSION,3> target_perms);

bool canWriteOnDir(const char* directory);

bool checkForYTDLP();

std::string do_ytdlp_search(const char* search_term, const char* extractor_name, Pagination_Info page_info);

static std::string do_youtube_search(const char* byTerm);

std::string get_videoURL_metadata(const char* video_url);

void stream_video(const char* video_url);

void download_video(const char* video_url, const char* download_path, VCODEC_RESOLUTIONS v_resolution);

YTDLP_Video_Metadata* parse_YT_Video_Metadata(const char ytdlp_video_metadata[512]);

int download_file(std::string url, std::string output_dir, std::string outfilename, bool overwrite = false);

Fl_Image* create_resized_image_from_jpg(std::string jpg_filepath, int target_width);

std::string getOptionValue(int argc, char* argv[], const std::string& option);

bool existsCmdOption(int argc, char* argv[], const std::string& option);

inline void trim(std::string &s);

std::unique_ptr<std::map<std::string, std::string>> loadConfFile(const char* path_to_conf);

#endif
