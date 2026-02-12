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

#include "../include/ytdlp_helper.h"

const std::string YtDlp_Helper::PRINT_METADATA_TEMPLATE = "title=\\\"%(title)s\\\">>thumbnail=\\\"%(thumbnails.0.url)s" \
"\\\">>creators=\\\"%(uploader,playlist_channel)s\\\">>video_id=\\\"%(id)s\\\">>upload_date=\\\"%(upload_date>%Y-%m-%d)s" \
"\\\">>duration=\\\"%(duration>%H:%M:%S)s\\\">>channel_id=\\\"%(playlist_channel_id,channel_id)s\\\">>live_status=\\\"%(live_status)s\\\">>viewers_count=\\\"%(view_count,concurrent_view_count)s\\\">>";


/** Parse the metadata as define in @PRINT_METADATA_TEMPLATE. Returns a YTDLP_Video_Metadata struct. */
YTDLP_Video_Metadata* YtDlp_Helper::parse_metadata(const char ytdlp_video_metadata[512]){
    std::stringstream lineStream(ytdlp_video_metadata);
    YTDLP_Video_Metadata* metadata = new YTDLP_Video_Metadata();
    std::string keyvalue;

    // Find metadata key=value delimited by ">>" symbols...
    while(std::getline (lineStream,keyvalue,'>'))
    {
        if (lineStream.peek() == '>'){
            lineStream.ignore();
            size_t pos = keyvalue.find('=');
            if (pos != std::string::npos) {
                std::string key = keyvalue.substr(0, pos);
                std::string value = keyvalue.substr(pos + 2, keyvalue.size() - pos - 3); // Remove quotes
                //printf("key: %s, value: %s\n", key.c_str(), value.c_str());
                if (key == "title") {
                    metadata->title = value;
                } else if (key == "video_id"){
                    metadata->id = value;
                } else if (key == "creators"){
                    metadata->creators = value;
                } else if (key == "upload_date"){
                    metadata->upload_date = value;
                } else if (key == "duration"){
                    metadata->duration = value;
                } else if (key == "thumbnail"){
                    metadata->thumbnail_url = value;
                } else if (key == "channel_id"){
                    metadata->channel_id = value;
                } else if (key == "live_status"){
                    metadata->live_status = value;
                } else if (key == "viewers_count"){
                    metadata->viewers_count = value;
                }
            }
        }
    }
    metadata->url =  YOUTUBE_URL_PREFIX + metadata->id;
    return metadata;
}

/**
 * Make a search by term in the specified extractor (i.e. "youtube", etc.). See a complete list of extractors at yt-dlp docs.
 * For now, only do searchs at Youtube.
 */
std::string YtDlp_Helper::search(const char* search_term, Pagination_Info page_info){

    if (this->extractor == YTDLP_EXTRACTOR::YOUTUBE) {
        return this->do_youtube_search(search_term, page_info);
    } else {
        return "No extractor matched...";
    }
}

std::string YtDlp_Helper::do_youtube_search(const char* search_text ,Pagination_Info page_info){
    char search_component[128];
    Pagination_Info page_info_ = page_info;
    int start_list_pos, end_list_pos;
    switch (this->search_type) {
        case SEARCH_BY_TYPE::CHANNEL_URL:
            snprintf(search_component, sizeof(search_component), "%s", search_text);  //The term is a Channel URL...
            break;
        case SEARCH_BY_TYPE::VIDEO_URL:
            strcpy(search_component, search_text);
            page_info_ = Pagination_Info(1,0);
            break;
        case SEARCH_BY_TYPE::TERM:
            snprintf(search_component, sizeof(search_component), "ytsearch%d:%s", page_info.upper_end(), search_text);  // Else, do a normal search by term.
            break;
    }
    char ytdlp_cmd[512];
    char cmd_format[512] = "yt-dlp \"%s\" -I %d-%d --flat-playlist --print \"%s\" --extractor-args youtubetab:approximate_date";

    snprintf(ytdlp_cmd, sizeof(ytdlp_cmd), cmd_format, search_component,
             page_info_.lower_end(), page_info_.upper_end(), YtDlp_Helper::PRINT_METADATA_TEMPLATE.c_str());
    return exec(ytdlp_cmd);
}

void YtDlp_Helper::stream(const char* video_url) {
    char get_final_url_cmd[2048];
    char stream_videoplayer_cmd[2048];
    char stream_format[100];
    std::string url_result;
    snprintf(stream_format, sizeof(stream_format), "res:%d,+codec:avc1:m4a", this->video_resolution);
    if (this->is_live_flag) {
        snprintf(stream_videoplayer_cmd, sizeof(stream_videoplayer_cmd),
                 "yt-dlp -S \"%s\" -o - \"%s\" | %s %s %s -", stream_format, video_url, this->media_player->binary_path.c_str(), this->media_player->parameters.c_str(), this->media_player->extra_live_parameters.c_str());
    } else {
        // If video is not live, 1rst try to obtain final video URL using default method...
        snprintf(get_final_url_cmd, sizeof(get_final_url_cmd), "yt-dlp -S \"%s\" -g \"%s\" 2> %s/ytdlp_errors.log", stream_format, video_url, this->TEMP_WORKING_DIR.c_str());
        this->logger->debug("EXEC COMMAND = " + std::string(get_final_url_cmd) + "\n");
        url_result = exec(get_final_url_cmd);
        replace_all(url_result, "\n", "");
        if (url_result != "") {
            // Once final URL is obtained, then open at configured Media Player...
            snprintf(stream_videoplayer_cmd, sizeof(stream_videoplayer_cmd),
                    "%s %s \"%s\"", this->media_player->binary_path.c_str(), this->media_player->parameters.c_str(), url_result.c_str());
        } else {
            // If default method doesn't works, then try the alternative method (if configured this way)...
            if (this->enable_alternative_stream_method) {
                logger->warn(_("The default stream command doesn't work. Fallback to the alternative method to get final video URL."));
                snprintf(stream_format, sizeof(stream_format), "bv*[height<=%d][vcodec^=avc]+ba[acodec^=mp4a]", this->video_resolution);
                snprintf(stream_videoplayer_cmd, sizeof(stream_videoplayer_cmd),
                    "yt-dlp -f \"%s\" -o - --merge-output-format mkv \"%s\" | %s %s -", stream_format, video_url, this->media_player->binary_path.c_str(), this->media_player->parameters.c_str());
            } else {
                logger->error(_("Cannot obtain URL for specified video, and alternative stream method is disabled."));
            }
        }
    }
    this->logger->debug("EXEC COMMAND = " + std::string(stream_videoplayer_cmd) + "\n");
    system(stream_videoplayer_cmd);
}

/**
 * Download a video from a its URL using the configured multimedia player.
 */
void YtDlp_Helper::download_video(const char* video_url, const char* download_path, VCODEC_RESOLUTIONS v_resolution,
                    const char* vcodec = VIDEOCODEC_PREFERRED.c_str()){
    char download_cmd[2048], s_dwl_data[200], s_dwl_dir[200];
    const char* download_data_format= "bestvideo[height<=%d][vcodec^=%s]+bestaudio/best";
    const char* download_dir_format = "%s/%(id)s.%s";
    snprintf(s_dwl_data, sizeof(s_dwl_data), download_data_format, v_resolution, vcodec);
    snprintf(s_dwl_dir, sizeof(s_dwl_dir), download_dir_format, download_path, DOWNLOAD_VIDEO_PREFERRED_EXT.c_str());
    snprintf(download_cmd, sizeof(download_cmd),
             "yt-dlp -f \"%s\" \"%s\" -o \"%s\"", s_dwl_data, video_url, s_dwl_dir);
    printf("%s\n", download_cmd);
    system(download_cmd);
}

/**
* Return a metric abbreviation from a number, for example: 1520
* = 1.5K, 1.450.000 = 1.4M
*/
std::string* YtDlp_Helper::get_metric_abbreviation(int number) {
    char metric_abbr[32];
    if (number < 1000) {
        snprintf(metric_abbr, sizeof(metric_abbr), "%d", number );
    } else if (number < 1000000) {
        snprintf(metric_abbr, sizeof(metric_abbr), "%.1fK", (number/1000.0));
    } else {
        snprintf(metric_abbr, sizeof(metric_abbr), "%.1fM", (number/1000000.0));
    }
    return new std::string(metric_abbr);
}

/** Check if an URL starts with any valid Youtube URL format.**/
bool YtDlp_Helper::isYoutubeURL(const char* url){
    std::string url_s(url);
    return ((url_s.rfind("https://www.youtube.com/", 0) == 0) || (url_s.rfind("https://youtube.com/", 0) == 0) || (url_s.rfind("https://youtu.be/", 0) == 0));
}
