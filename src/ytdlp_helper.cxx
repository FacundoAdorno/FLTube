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
#include <cstdio>
#include <string>

const std::string YtDlp_Helper::DEFAULT_YTDLP_PATH = "yt-dlp";

const std::string YtDlp_Helper::ALTERN_YT_PLAYER_CLIENT = "web_embedded";

/** Parse the metadata printed by the exec of a yt-dlp command. Returns a YTDLP_Video_Metadata struct. */
YTDLP_Video_Metadata* YtDlp_Helper::parse_metadata(const char ytdlp_video_metadata[1024]){
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
                } else if (key == "is_live"){
                    std::string lw_val = value;
                    std::transform(lw_val.begin(), lw_val.end(), lw_val.begin(), ::tolower);    //lowercase
                    metadata->is_live = (lw_val == "true") ? true : false;
                } else if (key == "viewers_count"){
                    metadata->viewers_count = value;
                } else if (key == "concurrent_viewers_count"){
                    metadata->concurrent_viewers_count = value;
                } else if (key == "followers"){
                    metadata->channel_follower_count = value;
                } else if (key == "like_count"){
                    metadata->like_count = value;
                }  else if (key == "timestamp"){
                    metadata->timestamp = value;
                }  else if (key == "timestamp_text"){
                    metadata->timestamp_text = value;
                }  else if (key == "media_type"){
                    if (value == "video")   metadata->media_type = YT_VIDEO_TYPE::VIDEO;
                    else if (value == "short")    metadata->media_type = YT_VIDEO_TYPE::SHORT;
                    else if (value == "livestream")    metadata->media_type = YT_VIDEO_TYPE::LIVESTREAM;
                    else metadata->media_type = YT_VIDEO_TYPE::UNKNONW;
                }  else if (key == "category"){
                    metadata->category = value;
                }   else if (key == "description"){
                    metadata->description = value;
                }   else if (key == "tags"){
                    if (value != "") metadata->tags = tokenize(value, ',');
                }
            }
        }
    }
    metadata->url =  YOUTUBE_URL_PREFIX + metadata->id;
    return metadata;
}

std::string YtDlp_Helper::getIdFor(std::string video_url) {
    return video_url + ":" + std::to_string(this->video_resolution);
}

std::vector<YTDLP_Video_Metadata*> YtDlp_Helper::retrieve_metadata(const char* ytdlp_cmd) {
    logger->debug(ytdlp_cmd);
    std::string result = exec(ytdlp_cmd);
    std::vector<YTDLP_Video_Metadata*> metadata;
    logger->debug(result);
    // Read input lines until an empty line is encountered
    std::istringstream result_sstream(result);
    std::string line;
    if (this->metadata_profile == YT_METADATA_PROFILE::DETAILED) {
        line = result_sstream.str();
        metadata.push_back(YtDlp_Helper::parse_metadata(line.c_str()));
    } else {
        getline(result_sstream, line);
        while (!line.empty()) {
            //Set the video metadata at the array...
            metadata.push_back(YtDlp_Helper::parse_metadata(line.c_str()));
            getline(result_sstream, line);
        }
    }
    return metadata;
}

/**
 * Make a search by term in the specified extractor (i.e. "youtube", etc.). See a complete list of extractors at yt-dlp docs.
 * For now, only do searchs at Youtube.
 */
yt_metadata_arr YtDlp_Helper::search(const char* search_term, Pagination_Info page_info){
    std::string clean_text = std::string(search_term);
    trim_and_clean(clean_text);
    if (this->extractor == YTDLP_EXTRACTOR::YOUTUBE) {
        return this->do_youtube_search(clean_text.c_str(), page_info);
    } else {
        return {};
    }
}

yt_metadata_arr YtDlp_Helper::do_youtube_search(const char* search_text ,Pagination_Info page_info){
    char search_component[128];
    yt_metadata_arr result_yt_metadata;
    result_yt_metadata.fill(nullptr);
    std::vector<YTDLP_Video_Metadata*> mtd;
    Pagination_Info page_info_ = page_info;
    // Define search yt-dlp command, that will be used if necessary...
    switch (this->search_type) {
        case SEARCH_BY_TYPE::CHANNEL_URL:
            snprintf(search_component, sizeof(search_component), "%s", search_text);  //The term is a Channel URL...
            break;
        case SEARCH_BY_TYPE::VIDEO_URL:
            strcpy(search_component, search_text);
            page_info_ = Pagination_Info(1,0);
            break;
        case SEARCH_BY_TYPE::TERM:
            // Else, do a normal search by term.
            break;
    }
    char ytdlp_cmd[1024];
    char cmd_format[1024] = "%s \"%s\" -I %d-%d --flat-playlist --print \"%s\" --extractor-args youtubetab:approximate_date";

    // Check if exists cached results for this type of search...
    if (search_type != SEARCH_BY_TYPE::VIDEO_URL) {
        auto search_data = search_cache.find(search_text);
        // If not cached, OR have to get more results, then retrieve and cache new results...
        if (search_data == search_cache.end()
                || (search_data != search_cache.end() && search_data->second.size() < page_info_.upper_end())) {

            int start, end;
            if (search_data == search_cache.end()) {
                search_cache[search_text] = {};
                search_data = search_cache.find(search_text);
                start = 1; end = batch_search_size;
                search_history.push_back(search_text);
            } else {
                start = search_data->second.size();
                end = search_data->second.size() + batch_search_size;
            }

            if ( search_type == SEARCH_BY_TYPE::TERM) {
                snprintf(search_component, sizeof(search_component), "ytsearch%d:%s", end, search_text);
            }
            snprintf(ytdlp_cmd, sizeof(ytdlp_cmd), cmd_format, YTDLP_BIN_PATH.c_str(), search_component, start, end, get_metadata_template().c_str());
            mtd = retrieve_metadata(ytdlp_cmd);

            for ( YTDLP_Video_Metadata* video_m: mtd) {
                search_data->second.push_back(std::make_pair(video_m->id, video_m));
            }
        }
        auto search_cached_results = search_data->second;
        int retrieve_position;
        for (int i=0; i < PaginationManager::SEARCH_PAGE_SIZE; i++) {
            retrieve_position = (page_info_.lower_end() - 1) + i;
            if (retrieve_position < search_cached_results.size()) {
                result_yt_metadata[i] = search_cached_results[retrieve_position].second;
            }
        }
    } else {
        // If search only one video (SEARCH_BY_TYPE::VIDEO_URL), then get its metadata...
        snprintf(ytdlp_cmd, sizeof(ytdlp_cmd), cmd_format, YTDLP_BIN_PATH.c_str(), search_component,
                 page_info_.lower_end(), page_info_.upper_end(), get_metadata_template().c_str());
        mtd = retrieve_metadata(ytdlp_cmd);
        if (!mtd.empty()) result_yt_metadata[0] = mtd[0];
    }
    return result_yt_metadata;
}

std::string YtDlp_Helper::get_stream_url(const char* video_url, const char* stream_format, bool &is_dash_format, std::vector<std::string> &urls, std::string alt_player_client) {
    char get_final_url_cmd[2048];
    std::string final_url_result;
    urls.clear();
    is_dash_format = false;

    // If video is not live, 1rst try to obtain final video URL using default method...
    // 1rst: lookup final video URL if exists at cache...
    final_url_result = cache->get_entry_value(getIdFor(video_url));
    if (final_url_result == CacheEntry::EMPTY_VALUE) {
        std::string alt_player_arg = "";
        if (alt_player_client != "") {
            alt_player_arg = "--extractor-args \"youtube:player_client=" + alt_player_client + "\"";
        }
        // 2nd: if final video url is not cached, then obtain it using yt-dlp.
        snprintf(get_final_url_cmd, sizeof(get_final_url_cmd), "%s -S \"%s\" -g \"%s\" %s 2> %s/ytdlp_errors.log",
                 YTDLP_BIN_PATH.c_str(), stream_format, video_url, alt_player_arg.c_str(), this->TEMP_WORKING_DIR.c_str());
        this->logger->debug("EXEC COMMAND = " + std::string(get_final_url_cmd) + "\n");
        final_url_result = exec(get_final_url_cmd);
        urls = tokenize(final_url_result, '\n');
    } else {
        urls = tokenize(final_url_result, DASH_URL_CACHE_SEPARATOR);
    }

    // Check if yt-dlp result is at DASH format...
    if (urls.size() > 1) {
        // Verify if yt-dlp returns a Progressive format (video+audio in one URL) or DASH (video and audio in differents URLs).
        // Progressive format is desired for older PC's, but if no available for required format, then multiplex DASH urls using FFmpeg...
        if (urls.size() == 2) {
            logger->debug(_("No progressive format is available for the requested resolution. Instead, yt-dlp returned a DASH format. Resolution: ") + std::to_string(this->video_resolution));
            is_dash_format = true;
        } else {
            logger->error(_("yt-dlp returns more than 2 URLS. Aborting stream operation for unknown response format."));
            logger->debug(_("Unkonwn URL Format response: ") + final_url_result);
            return "";
        }
    }

    if (is_dash_format) {
        final_url_result = urls.at(0) + DASH_URL_CACHE_SEPARATOR + urls.at(1);
    } else {
        // Sanitize URL if not in DASH format...
        replace_all(final_url_result, "\n", "");
    }

    return final_url_result;
}

FLTUBE_STATUS_CODES YtDlp_Helper::stream(const char* video_url) {
    char stream_videoplayer_cmd[3072];
    char stream_format[100];
    std::string final_url_result;
    snprintf(stream_format, sizeof(stream_format), "res:%d,+codec:avc1:m4a", this->video_resolution);
    if (this->is_live_flag) {
        snprintf(stream_videoplayer_cmd, sizeof(stream_videoplayer_cmd),
                 "%s -S \"%s\" -o - \"%s\" | %s %s %s -", YTDLP_BIN_PATH.c_str(), stream_format, video_url, this->media_player->getBinaryPath().c_str(), this->media_player->getParams().c_str(), this->media_player->getExtraParams().c_str());
    } else {
        bool is_dash_format = false;
        std::vector<std::string> urls;
        final_url_result = this->get_stream_url(video_url, stream_format, is_dash_format, urls);

        FLTUBE_STATUS_CODES res = check_url_access(urls[0]);
        if (res != FLT_OK) {
            for (std::string alt_player: this->alt_player_clients) {
                if (res == FLT_HTTP_FORBIDDEN) {
                    logger->debug(_("yt-dlp resolved to an INVALID URL (403 FORBIDDEN code was returned). Trying with another player_client: ") + alt_player);
                    final_url_result = this->get_stream_url(video_url, stream_format, is_dash_format, urls, alt_player);
                    res = check_url_access(urls[0]);
                } else if (res == FLT_OK) {
                    break;
                }
            }
        }

        if (res != FLT_OK) {
            logger->error(_("Cannot obtain a valid stream URL. Please check if your yt-dlp installation is up to date. More info at: ") + std::string("https://github.com/yt-dlp/yt-dlp/releases/latest"));
            return res;
        }

        if (final_url_result != "") {
            // Once final URL is obtained, then open at configured Media Player...
            if (is_dash_format) {
                snprintf(stream_videoplayer_cmd, sizeof(stream_videoplayer_cmd),
                    "ffmpeg -i \"%s\" -i \"%s\" -c copy -f nut - | %s %s -", urls.at(0).c_str(), urls.at(1).c_str(),
                            this->media_player->getBinaryPath().c_str(), this->media_player->getParams().c_str());
            } else {
                snprintf(stream_videoplayer_cmd, sizeof(stream_videoplayer_cmd),
                        "%s %s \"%s\"", this->media_player->getBinaryPath().c_str(), this->media_player->getParams().c_str(), final_url_result.c_str());
            }

            cache->add_entry(getIdFor(video_url), final_url_result);
        } else {
            // If default method doesn't works, then try the alternative method (if configured this way)...
            if (this->enable_alternative_stream_method) {
                logger->warn(_("The default stream command doesn't work. Fallback to the alternative method to get final video URL."));
                snprintf(stream_format, sizeof(stream_format), "bv*[height<=%d][vcodec^=avc]+ba[acodec^=mp4a]", this->video_resolution);
                snprintf(stream_videoplayer_cmd, sizeof(stream_videoplayer_cmd),
                    "%s -f \"%s\" -o - --merge-output-format mkv \"%s\" | %s %s -", YTDLP_BIN_PATH.c_str(), stream_format, video_url, this->media_player->getBinaryPath().c_str(), this->media_player->getParams().c_str());
            } else {
                logger->error(_("Cannot obtain URL for specified video, and alternative stream method is disabled."));
                return FTL_HTTP_GENERAL_ERROR;
            }
        }
    }
    this->logger->debug("EXEC COMMAND = " + std::string(stream_videoplayer_cmd) + "\n");
    system(stream_videoplayer_cmd);
    return FLT_OK;
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
             "%s -f \"%s\" \"%s\" -o \"%s\"", YTDLP_BIN_PATH.c_str(), s_dwl_data, video_url, s_dwl_dir);
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

std::string YtDlp_Helper::getNextInSearchHistory() {
    if (search_history.empty()) return "";
    if (current_search_history_index + 1 <= search_history.size() - 1) {
        current_search_history_index++;
    }
    return search_history.at(current_search_history_index);
}

std::string YtDlp_Helper::getPreviousInSearchHistory() {
    if (search_history.empty()) return "";
    if (current_search_history_index - 1 >= 0) {
        current_search_history_index--;
    }
    return search_history.at(current_search_history_index);
}

void YtDlp_Helper::resetSearchHistoryPos() {
    current_search_history_index = 0;
}

const std::string YtDlp_Helper::get_metadata_template() {
    switch (this->metadata_profile) {
        case YT_METADATA_PROFILE::SIMPLE:
            return YtDlp_Helper::PRINT_SEARCH_METADATA_TEMPLATE;
        case YT_METADATA_PROFILE::DETAILED:
            return YtDlp_Helper::PRINT_DETAILED_METADATA_TEMPLATE;
    }
}
