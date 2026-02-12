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
#include "fltube_utils.h"

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
};

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

/* Use this class for search and streams videos using "yt-dlp" commandline tool.
 * Search a video/videos using a single URL, a Channel URL or a search term.
 * Stream a video from its URL using the configured multimedia player at @DEFAULT_STREAM_PLAYER.
 * The default stream resolution is 360p, unless changed at configuration. Also, you can specify if the video to stream is a "live video".
 */
class YtDlp_Helper {
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

        /* Method to define the specific search parameters for Youtube Extractor, and make the videos search.  */
        std::string do_youtube_search(const char* search_text, Pagination_Info page_info);

        std::shared_ptr<TerminalLogger> logger;

    public:
        /** Metadata print template for youtube search videos.  **/
        const static std::string PRINT_METADATA_TEMPLATE;
        VCODEC_RESOLUTIONS video_resolution;
        YTDLP_EXTRACTOR extractor;


        YtDlp_Helper(VCODEC_RESOLUTIONS v_resolution, MediaPlayerInfo* mp, bool enable_alt_stream, std::shared_ptr<TerminalLogger> const& lgg, std::string working_dir):
            is_live_flag(false), video_resolution(v_resolution), media_player(mp), extractor(YTDLP_EXTRACTOR::YOUTUBE), enable_alternative_stream_method(enable_alt_stream), logger(lgg)
            {
                if (working_dir == "")
                    TEMP_WORKING_DIR = std::filesystem::temp_directory_path().generic_string() + "/fltube_tmp_files/";
                else
                    TEMP_WORKING_DIR = working_dir;
            };

        /*  Search one or more videos. This will be determined according to the type of search is configured. */
        std::string search(const char* search_text_parameter, Pagination_Info page_info);

        /*  Start the streaming of an specific video URL. If the video is live, you should specify this before stream.
         *  If the default stream method is not working, stream using the alternative method (if configured this way). */
        void stream(const char* video_url);

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


        static YTDLP_Video_Metadata* parse_metadata(const char ytdlp_video_metadata[512]);

        void download_video(const char* video_url, const char* download_path, VCODEC_RESOLUTIONS v_resolution, const char* vcodec);

        static std::string* get_metric_abbreviation(int number);

        static bool isYoutubeURL(const char* url);
};

#endif
