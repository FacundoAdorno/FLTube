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
#include <cstring>

#include <FL/Fl_Image.H>
#include <FL/Fl_JPEG_Image.H>
#include "gnugettext_utils.h"


//TODO some bellow configurations can be put at fltube.conf config file.
/** Currently, AVC1 is the most supported codec for MP4 in old PC systems... **/
const std::string VIDEOCODEC_PREFERRED = "avc1";
const std::string AUDIOCODEC_PREFERRED = "m4a";

/** You can change here for the player of your like (in example: vlc, mpv, etc.). It must be installed in your system. **/
const std::string DEFAULT_STREAM_PLAYER = "mplayer";
/** Modify the launch parameters of the DEFAULT_STREAM_PLAYER. */
const std::string DEFAULT_PLAYER_PARAMS = "-zoom -ao alsa";
/** Extra parameters for optimize the stream of a live video. */
const std::string DEFAULT_PLAYER_EXTRAPARAMS_LIVE = "-demuxer lavf -cache 2048";

const std::string DOWNLOAD_VIDEO_PREFERRED_EXT = "mp4";

const std::string HTTP_PREFIX = "http://";
const std::string HTTPS_PREFIX = "https://";
const std::string YOUTUBE_URL_PREFIX = HTTPS_PREFIX + "youtu.be/";


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

enum LogLevel { INFO, WARN, ERROR, DEBUG };

/**
 * Class for write logs at terminal, for differents @LogLevel's.
 * If @debug_enable is true, messages of every log level will be printed.
 */
class TerminalLogger {
    private:
        const std::string PRINT_FORMAT = "\n%s [%s] - - -  %s\n";
        bool debug_enabled;
    public:
        TerminalLogger(bool enable_debug)
            : debug_enabled(enable_debug) {};

        void log(std::string log_message, LogLevel log_lvl);
        void info(std::string log_message);
        void warn(std::string log_message);
        void error(std::string log_message);
        void debug(std::string log_message);
};

/** Info for page search results. Initial page has index 0. See parameter -I at yt-dlp documentation. */
struct Pagination_Info {
    int size;
    int index;

    Pagination_Info():size(0), index(0) {};

    Pagination_Info(int size, int index): size(size), index(index) {};

    /** Minimum lower end is 1. */
    int lower_end() const {
        return (index * size) + 1;
    }

    int upper_end() const {
        return ((index + 1) * size);
    }
};

struct MediaPlayerInfo {
    // The name of the binary (or system path) of the media player.
    std::string binary_path;
    // Parameters to media playar (optional).
    std::string parameters;
    // Extra parameters for stream a live video (optional).
    std::string extra_live_parameters;
};

std::string exec(const char* cmd);

bool isUrl(const char* user_input);

const std::string currentDateTime();

const char* getHomePathOr(const char* defaultPath);

std::vector<int> getUserGroupsIds(int uid);

bool checkDirectoryPermissions(const char* directory, std::array<SIMPLE_FS_PERMISSION,3> target_perms);

bool canWriteOnDir(const char* directory);

bool isInstalledYTDLP();

static CURL* get_curl_handle(const char* forURL, FILE* output_file = nullptr);

FLTUBE_STATUS_CODES download_file(std::string url, std::string output_dir, std::string outfilename, bool overwrite = false);

bool verify_network_connection();

Fl_Image* create_resized_image_from_jpg(std::string jpg_filepath, int target_width);

std::string getOptionValue(int argc, char* argv[], const std::string& option);

bool existsCmdOption(int argc, char* argv[], const std::string& option);

void trim(std::string &s);

std::vector<std::string> tokenize(std::string s, const char delimiter);

void replace_all(std::string &original_text, const std::string &toReplace, const std::string &theReplace);

bool isNumber(const std::string& str);

#endif
