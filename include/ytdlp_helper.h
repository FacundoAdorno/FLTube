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

#ifndef YTDLP_HELPER_H
#define YTDLP_HELPER_H

#include <string>
#include <array>
#include <exception>
#include <stdio.h>
#include "fltube_utils.h"
#include "cache.h"


/* Specify if the video is a normal video, a short video or a livestream video. */
enum YT_VIDEO_TYPE { UNKNONW, VIDEO, SHORT, LIVESTREAM };

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
    std::string live_status;
    std::string viewers_count;
    // For a more detailed metadata....
    std::string description = "NA";
    std::string like_count = "NA";
    std::string channel_follower_count = "NA";
    bool is_live = false;   // A livestream that has ended is now a regular video.
    std::string concurrent_viewers_count;  // Concurrent viewers of a livestream, use only for DETAILED METADATA...
    std::string timestamp = "NA";      //UTC Epoch time
    std::string timestamp_text = "NA";
    YT_VIDEO_TYPE media_type = YT_VIDEO_TYPE::UNKNONW;     // Options are: video | short | livestream
    std::string category = "NA";
    std::vector<std::string> tags;
};

/**
 * Use this Exception when the process of initializing the YT-DLP object cannot be completed.
 * In example, when yt-dlp is not installed at your system.
 */
class YtDlpInitException : public std::exception {
private:
    std::string message;
public:

    YtDlpInitException(const char* msg) :
    message(msg) {}

    const char* what() const noexcept {
        return message.c_str();
    }
};

typedef std::array<YTDLP_Video_Metadata*, PaginationManager::SEARCH_PAGE_SIZE> yt_metadata_arr;

/* This profile define the metadata printed when make a video metadata search. */
enum YT_METADATA_PROFILE {
    // When search videos by a search term, channel's videos, or a single video result, only a few metadata are required.
    SIMPLE,
    // Used when is required a detailed metadata view for a specific video.
    DETAILED };

enum YTDLP_EXTRACTOR { YOUTUBE };

/*  Used to define if a search is going to be by a search term, the videos from an specific channel or user,
 *  or is a single video search by URL.
 */
enum SEARCH_BY_TYPE { TERM, CHANNEL_URL, VIDEO_URL };

/*Enum for the target video resolutions. */
enum VCODEC_RESOLUTIONS {
    R240p = 240, R360p = 360, R480p = 480, R720p = 720, R1080p = 1080 };

static std::array<const char*,2> VCODEC_IMPL_NAMES = {"avc1", "av01"};
// The default resolution used to streaming videos.
const int DEFAULT_STREAM_VIDEO_RESOLUTION = VCODEC_RESOLUTIONS::R360p;

// Separator for cache value when multiple URLs are saved for DASH format (for example, audio_URL<video_URL).
static const char DASH_URL_CACHE_SEPARATOR = '<';

/* Use this class for search and streams videos using "yt-dlp" commandline tool.
 * Search a video/videos using a single URL, a Channel URL or a search term.
 * Stream a video from its URL using the configured multimedia player at @DEFAULT_STREAM_PLAYER.
 * The default stream resolution is 360p, unless changed at configuration. Also, you can specify if the video to stream is a "live video".
 */
class YtDlp_Helper {

    //TODO: in the future, this class must be proxied by a YtDlp_Helper_Cached class or similar, that handles references to the real
    //      "YtDlp_Helper" and only comunicates to the yt-dlp binnary if some request is not cached...
    //        MORE INFO: https://en.wikipedia.org/wiki/Proxy_pattern
    private:
        std::string TEMP_WORKING_DIR;

        MediaPlayerInfo* media_player;

        /* If true, if the default stream method fails, try an alternative method using "--merge-output-format mkv".
         * If @use_alternative_method is true, then the video stream will be considered as a VBR stream, instead a CBR (like as 360p resolution).
         *  More info about this at: https://getstream.io/glossary/cbr-vs-vbr/. */
        bool enable_alternative_stream_method;

        SEARCH_BY_TYPE search_type;

        /*  If true, the video for stream is at live. This implies the use of custom parameters on yt-dlp for stream. */
        bool is_live_flag;

        YT_METADATA_PROFILE metadata_profile;

        std::shared_ptr<TerminalLogger> logger;

        std::shared_ptr<PermanentDiskCache> cache;

        unsigned int batch_search_size;

        std::string YTDLP_BIN_PATH;

        // TODO: in the future, this must be a template T @GeneralCache, when implemented...
        /* This is a simple results cache, with the following semantics:
         *      map <"search term / channel ID <map <"video_id", parsed metadata>>>"  */
        std::map<std::string, std::vector<std::pair<std::string, YTDLP_Video_Metadata*>>> search_cache;

        /* List of alternative YouTube player client in case of default fails with HTTP 403 Forbidden code,
         * as defined in https://github.com/yt-dlp/yt-dlp#youtube. */
        std::vector<std::string> alt_player_clients;

        /* When doing a search, a inner search history is saved, in order to recall previous searches results... */
        std::vector<std::string> search_history;
        int current_search_history_index;

        /* Method to define the specific search parameters for Youtube Extractor, and make the videos search.  */
        yt_metadata_arr do_youtube_search(const char* search_text, Pagination_Info page_info);

        std::string get_stream_url(const char* video_url, const char* stream_format, bool& is_dash_format, std::vector<std::string> &urls, std::string alt_player_client = "");

        std::vector<YTDLP_Video_Metadata*> retrieve_metadata(const char* search_text);

    public:
        /** Current version of yt-dlp installed at the running system. If its value is -1, no version was detected... **/
        std::string installed_version;
        /** Metadata print template for youtube search videos.  **/
        std::string PRINT_SEARCH_METADATA_TEMPLATE;
        /** Metadata print template for a detailed view of a specific youtube video.  **/
        std::string PRINT_DETAILED_METADATA_TEMPLATE;

        VCODEC_RESOLUTIONS video_resolution;
        YTDLP_EXTRACTOR extractor;
        const static int DEFAULT_MIN_BATCH_SIZE = 40;
        const static int DEFAULT_MAX_BATCH_SIZE = 200;
        const static std::string DEFAULT_YTDLP_PATH;
        /* Alternative YouTube player client in case of default fails with HTTP 403 Forbidden code,
         * as defined in https://github.com/yt-dlp/yt-dlp#youtube. */
        const static std::string ALTERN_YT_PLAYER_CLIENT;


        YtDlp_Helper(VCODEC_RESOLUTIONS v_resolution, MediaPlayerInfo* mp, bool enable_alt_stream, std::shared_ptr<TerminalLogger> const& lgg, std::shared_ptr<PermanentDiskCache> const& cache, std::string working_dir, unsigned int batch_size, std::string ytdlp_path):
            is_live_flag(false), video_resolution(v_resolution), media_player(mp), extractor(YTDLP_EXTRACTOR::YOUTUBE), enable_alternative_stream_method(enable_alt_stream), logger(lgg), cache(cache),
            batch_search_size(batch_size), search_cache({}), search_history({}), current_search_history_index(0),
            metadata_profile(YT_METADATA_PROFILE::SIMPLE)
            {
                if (ytdlp_path == "") {
                    YTDLP_BIN_PATH = DEFAULT_YTDLP_PATH;
                } else {
                    YTDLP_BIN_PATH = ytdlp_path;
                }

                int exitStatus = 0;
                char cmd[256];
                snprintf(cmd, sizeof(cmd), "%s --version", YTDLP_BIN_PATH.c_str());
                this->installed_version = exec(cmd, exitStatus);
                if ( exitStatus == -1 ) {
                    throw YtDlpInitException(_("Cannot execute yt-dlp command for some reason. Aborting..."));
                } else if (exitStatus > 0) {
                    throw YtDlpInitException(_("Check your yt-dlp installation. The 'yt-dlp' command is not found. Aborting..."));
                }

                if (working_dir == "")
                    TEMP_WORKING_DIR = std::filesystem::temp_directory_path().generic_string() + "/fltube_tmp_files/";
                else
                    TEMP_WORKING_DIR = working_dir;

                if (batch_size < PaginationManager::SEARCH_PAGE_SIZE || batch_size > DEFAULT_MAX_BATCH_SIZE) {
                    logger->warn(_("Fallbacks to default minimun batch size for yt-dlp search. Check property 'PREFETCH_BATCH_RESULTS_SIZE' at fltube.conf file. Configured value cannot be greater than ") + DEFAULT_MAX_BATCH_SIZE);
                    batch_search_size = DEFAULT_MIN_BATCH_SIZE;
                }

                std::string dateFormat = _("%Y-%m-%d");
                std::string fullDateFormat = _("%Y-%m-%d %H:%M:%S");
                PRINT_SEARCH_METADATA_TEMPLATE = std::string("title=\\\"%(title)s\\\">>thumbnail=\\\"%(thumbnails.0.url)s" \
                "\\\">>creators=\\\"%(uploader,playlist_channel)s\\\">>video_id=\\\"%(id)s\\\">>upload_date=\\\"%(upload_date>")
                + dateFormat +
                ")s\\\">>duration=\\\"%(duration>%H:%M:%S)s\\\">>channel_id=\\\"%(playlist_channel_id,channel_id)s\\\">>live_status=\\\"%(live_status)s\\\">>viewers_count=\\\"%(view_count,concurrent_view_count)s\\\">>";

                PRINT_DETAILED_METADATA_TEMPLATE =
                std::string("title=\\\"%(title)s\\\">>thumbnail=\\\"%(thumbnails.0.url)s" \
                "\\\">>creators=\\\"%(uploader,playlist_channel)s\\\">>video_id=\\\"%(id)s\\\">>upload_date=\\\"%(upload_date>")
                + dateFormat + ")s\\\">>duration=\\\"%(duration>%H:%M:%S)s\\\">>channel_id=\\\"%(playlist_channel_id,channel_id)s\\\">>is_live=\\\"%(is_live)s\\\">>viewers_count=\\\"%(view_count)s\\\">>concurrent_viewers_count=\\\"%(concurrent_view_count)s\\\">>timestamp=\\\"%(timestamp)s\\\">>timestamp_text=\\\"%(timestamp>"
                + fullDateFormat +
                ")s\\\">>media_type=\\\"%(media_type)s\\\">>followers=\\\"%(channel_follower_count)s\\\">>like_count=\\\"%(like_count)s\\\">>description=\\\"%(description)s\\\">>tags=\\\"%(tags)l\\\">>category=\\\"%(categories.0)s\\\">>";

            };

        ~YtDlp_Helper() {
            for (auto& pair : search_cache) {
                for (auto& pair2: pair.second) {
                    if (pair2.second != nullptr) delete pair2.second;
                }
                pair.second.clear();
            }
            search_cache.clear();
            delete media_player;
        }

        /*  Search one or more videos. This will be determined according to the type of search is configured. */
        yt_metadata_arr search(const char* search_text_parameter, Pagination_Info page_info);

        /*  Start the streaming of an specific video URL. If the video is live, you should specify this before stream.
         *  If the default stream method is not working, stream using the alternative method (if configured this way). */
        FLTUBE_STATUS_CODES stream(const char* video_url);

        /*  Change the configured search type permanently. */
        void set_search_type(SEARCH_BY_TYPE s) {
            this->search_type = s;
        }

        void is_live(bool is_live = true) {
            this->is_live_flag = is_live;
        }

        /*  Change the configured extractor permanently. */
        void set_extractor(YTDLP_EXTRACTOR extrct) {
            this->extractor = extrct;
        }

        /*  Change the configured extractor permanently. */
        void set_resolution(VCODEC_RESOLUTIONS res) {
            this->video_resolution = res;
        }

        /* Set the @YT_METADATA_PROFILE used in next video search. */
        void set_metadata_profile(YT_METADATA_PROFILE profile) {
            this->metadata_profile = profile;
        }

        void add_alt_player_client(std::string new_player) {
            if (new_player.empty()) return;
            if (std::find(alt_player_clients.begin(), alt_player_clients.end(), new_player) == alt_player_clients.end()) {
                this->alt_player_clients.push_back(new_player);
            }
        }

        /* Returns the metadata template used when printing video metadata... Values available are:
            @PRINT_SEARCH_METADATA_TEMPLATE and @PRINT_DETAILED_METADATA_TEMPLATE.    */
        const std::string get_metadata_template();


        static YTDLP_Video_Metadata* parse_metadata(const char ytdlp_video_metadata[1024]);

        /* Returns a unique ID for the specified URL, taking into account the resolution configured for this instance of YtDlp_Helper. */
        std::string getIdFor(std::string video_url);

        void download_video(const char* video_url, const char* download_path, VCODEC_RESOLUTIONS v_resolution, const char* vcodec);

        static std::string* get_metric_abbreviation(int number);

        static bool isYoutubeURL(const char* url);

        std::string getNextInSearchHistory();

        std::string getPreviousInSearchHistory();

        void resetSearchHistoryPos();
};

#endif
