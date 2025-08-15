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

#include "../include/fltube_utils.h"
#include <filesystem>

const std::string HTTP_PREFIX = "http://";
const std::string HTTPS_PREFIX = "https://";
const std::string YOUTUBE_URL_PREFIX = HTTPS_PREFIX + "youtu.be/";

/**
 * Execute a system command and returns its output.
 */
std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, int(*)(FILE*)> pipe(popen(cmd, "r"), pclose);

    if (!pipe) {
        printf("Hubo un error al ejecutar el siguiente comando: %s \n", cmd);
        printf("Cerrando el programa por un error.");
        exit(2);
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        printf("%s", buffer.data());
        result += buffer.data();
    }

    return result;
}

/** Attempts to make a SIMPLE check if a text is an URL. */
bool isUrl(const char* user_input) {
    std::string input_text(user_input);
    return (input_text.compare(0,7, HTTP_PREFIX) == 0 || input_text.compare(0,8, HTTPS_PREFIX) == 0);
}

/** Check if an URL starts with any valid Youtube URL format.**/
bool isYoutubeURL(const char* url){
    std::string url_s(url);
    return ((url_s.rfind("https://youtube.com/", 0) == 0) || (url_s.rfind("https://youtu.be/", 0) == 0));
}

/** Get the current date/time. The format is YYYY-MM-DD.HH:mm:ss. */
const std::string currentDateTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
    // for more information about the date/time format
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}

/**
 * Returns true if yt-dlp exists on system path...
 */
bool checkForYTDLP() {
    printf("Cheking if yt-dlp is at your system PATH...\n");
    int yt_dlp_version_status = system ("yt-dlp --version");
    if ( yt_dlp_version_status != 0 ) {
        return false;
    } else {
        return true;
    }
}

/** Metadata print template for youtube search videos.  **/
const std::string PRINT_METADATA_TEMPLATE = "title=\\\"%(title)s\\\">>thumbnail=\\\"%(thumbnails.0.url)s" \
"\\\">>creators=\\\"%(uploader)s\\\">>video_id=\\\"%(id)s\\\">>upload_date=\\\"%(upload_date>%Y-%m-%d)s" \
"\\\">>duration=\\\"%(duration>%H:%M:%S)s\\\">>channel_id=\\\"%(playlist_channel_id,channel_id)s\\\">>";

static std::string do_youtube_search(const char* byTerm){
    char ytdlp_cmd[512];
    char cmd[512] = "yt-dlp \"ytsearch5:%s\" --flat-playlist --print \"%s\" --extractor-args youtubetab:approximate_date";

    snprintf(ytdlp_cmd, sizeof(ytdlp_cmd), cmd, byTerm, PRINT_METADATA_TEMPLATE.c_str());
    return exec(ytdlp_cmd);

}

/**
 * Make a search by term in the specified extractor (i.e. "youtube", etc.). See a complete list of extractors at yt-dlp docs.
 * For now, only do searchs at Youtube.
 */
std::string do_ytdlp_search(const char* search_term, const char* extractor_name){

    if (extractor_name == YOUTUBE_EXTRACTOR_NAME) {
        return do_youtube_search(search_term);
    } else {
        return "No extractor matched...";
    }
}


/**
 * Stream a video from a its URL using the configured multimedia player.
 */
void stream_video(const char* video_url) {
    //Open Video in MPlayer
    char stream_videoplayer_cmd[2048];
    const char* stream_format = "bestvideo[height<=360][vcodec=avc1]+bestaudio[ext=m4a]/best";
    snprintf(stream_videoplayer_cmd, sizeof(stream_videoplayer_cmd),
                "%s \"$(yt-dlp -f \"%s\" -g \"%s\")\"", DEFAULT_STREAM_PLAYER.c_str(), stream_format, video_url);
    printf("%s\n", stream_videoplayer_cmd);
    system(stream_videoplayer_cmd);
}

/**
 * Download a video from a its URL using the configured multimedia player.
 */
void download_video(const char* video_url, const char* download_path, VCODEC_RESOLUTIONS v_resolution){
    char download_cmd[2048];
    const char* download_format = "bestvideo[height<=%s][vcodec=avc1]+bestaudio[ext=m4a]/best";
    snprintf(download_cmd, sizeof(download_cmd),
             "%s \"$(yt-dlp -f \"%s\" -g \"%s\")\"", DEFAULT_STREAM_PLAYER.c_str(), download_format, video_url);
    printf("%s\n", download_cmd);
    system(download_cmd);
}

/** Parse the metadata as define in @PRINT_METADATA_TEMPLATE. Returns a YTDLP_Video_Metadata struct. */
YTDLP_Video_Metadata* parse_YT_Video_Metadata(const char ytdlp_video_metadata[512]){
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
                } else if (key == "playlist_channel_id"){
                    metadata->channel_id = value;
                }
            }
        }
    }
    metadata->url =  YOUTUBE_URL_PREFIX + metadata->id;
    return metadata;
}

//Function for download a file to a local output directory, and set a custom name. Returns 0 if all is ok.
int download_file(std::string url, std::string output_dir, std::string outfilename, bool overwrite) {
    CURL *curl;
    FILE *fp;
    CURLcode response;
    std::string fullpath = output_dir + outfilename;
    //If must not overwrite the file, check if exists before do any download...
    if (!overwrite && std::filesystem::exists(fullpath)) {
        return FLT_DOWNLOAD_FL_BYPASSED;
    }
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl) {
        fp = fopen(fullpath.c_str(),"wb");
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
        response = curl_easy_perform(curl);

        if (response != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(response));
            return FLT_DOWNLOAD_FL_FAILED;
        }

        /* always cleanup */
        curl_easy_cleanup(curl);
        fclose(fp);
    }
    return FLT_OK;
}

/** Returns a resized Fl_Image widget from an existing JPG image. If original image doesn't exists, a @nullptr is returned.*/
Fl_Image* create_resized_image_from_jpg(std::string jpg_filepath, int target_width){
    if(!std::filesystem::exists(jpg_filepath)) {
        return nullptr;
    }
    Fl_Image* original_image = new Fl_JPEG_Image(jpg_filepath.c_str());

    /*** Resize method --> https://es.elitescreens.eu/pages/screen-size-calculator ***/
    // (h / w) * target_width
    int new_height = ceil((original_image->h() / static_cast<double>(original_image->w())) * target_width);

    Fl_Image* resized_image = original_image->copy(target_width, new_height);

    //Free unused memory.
    delete original_image;

    return  resized_image;
}
