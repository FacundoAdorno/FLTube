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

const std::string HTTP_PREFIX = "http://";
const std::string HTTPS_PREFIX = "https://";
const std::string YOUTUBE_URL_PREFIX = HTTPS_PREFIX + "youtu.be/";

/** Mapping FS_PERMISSION_NAMES to corresponding std::filesystem::perms. */
static const std::map<SIMPLE_FS_PERMISSION, std::map<std::string, std::filesystem::perms>> perms_map = {
    {CAN_READ,
        {{"OWNER", std::filesystem::perms::owner_read}, {"GROUP", std::filesystem::perms::group_read}, {"OTHERS", std::filesystem::perms::others_read}}},
    {CAN_WRITE,
        {{"OWNER", std::filesystem::perms::owner_write}, {"GROUP", std::filesystem::perms::group_write}, {"OTHERS", std::filesystem::perms::others_write}}},
    {CAN_EXECUTE,
        {{"OWNER", std::filesystem::perms::owner_exec}, {"GROUP", std::filesystem::perms::group_exec}, {"OTHERS", std::filesystem::perms::others_exec}}},
};

/**
 * Execute a system command and returns its output.
 */
std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, int(*)(FILE*)> pipe(popen(cmd, "r"), pclose);

    if (!pipe) {
        printf(_("There was an error executing the following command: %s \n"), cmd);
        printf(_("Closing the program due to an error.\n"));
        exit(2);
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        //printf("%s", buffer.data());
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

/**  Get the $HOME system path, or (if not set) the default specified path, or TEMPORAL System path. */
const char* getHomePathOr(const char* defaultPath) {
    const char* target_dir;
    if ((target_dir = getenv("HOME")) == NULL && !defaultPath) {
        target_dir = std::filesystem::temp_directory_path().c_str();
    }
    return target_dir;
}

/** For an specified UID, return a list of its corresponding GIDs - group IDs. */
std::vector<int> getUserGroupsIds(int uid){
    // Original implementation at https://stackoverflow.com/a/57896725.
    std::vector<int> user_groups = {};

    struct passwd* pw = getpwuid(uid);
    if(pw == NULL){
        perror("getpwuid error: ");
    } else {
        int ngroups = 0;
        //this call is just to get the correct ngroups
        getgrouplist(pw->pw_name, pw->pw_gid, NULL, &ngroups);
        __gid_t groups[ngroups];
        //here we actually get the groups
        getgrouplist(pw->pw_name, pw->pw_gid, groups, &ngroups);
        for (int i = 0; i < ngroups; i++){
            user_groups.push_back(getgrgid(groups[i])->gr_gid);
        }
    }
    return user_groups;
}

/** Check if a target directory has the requested permissions defined at @FS_PERMISSION_NAMES. */
bool checkDirectoryPermissions(const char* target_directory, std::array<SIMPLE_FS_PERMISSION,3> target_perms){
    using namespace std::filesystem;
    if(target_directory && exists(target_directory)){
        //Get the user and group id for the user running the process for this program.
        uid_t process_user_id = geteuid();

        //Get user and group for target directory
        struct stat fileStat;
        if( stat(target_directory, &fileStat) < 0 ){
            //Stat failed for some reason...
            printf("Cannot make stat() over the directory %s\n", target_directory);
            return false;
        }

        // printf("DIR: %s (user: %d, group: %d)\n", target_directory, fileStat.st_uid, fileStat.st_gid);

        bool isOwner = (process_user_id == fileStat.st_uid); //True if process user is owner of directory.
        bool isInGroup = false; //True if process user belong to the directory's group.'
        for (int gid: getUserGroupsIds(process_user_id)){
            if(gid == fileStat.st_gid){
                isInGroup = true;
                break;
            }
        }

        // Get the file status, which includes permissions
        std::filesystem::file_status s = std::filesystem::status(target_directory);
        // Iterate over each requested permission and verify it exists. If any not, then returns false..
        for (SIMPLE_FS_PERMISSION perm: target_perms) {
            bool cond1, cond2, cond3;
            cond1 = (isOwner && ((s.permissions() & perms_map.at(perm).at("OWNER")) != std::filesystem::perms::none));
            cond2 = (isInGroup && ((s.permissions() & perms_map.at(perm).at("GROUP")) != std::filesystem::perms::none));
            cond3 = ((s.permissions() & perms_map.at(perm).at("OTHERS")) != std::filesystem::perms::none);
            if ( ! (cond1 || cond2 || cond3) ) {
                //This is like write all conditions as nested if's.
                //If don't have any of requested permissions, return false.
                return false;
            }
        }
    }
    return true;
}

/** Check write permissions on specified directory. */
bool canWriteOnDir(const char* directory){
    return checkDirectoryPermissions(directory, {CAN_WRITE});
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

static std::string do_youtube_search(const char* byTerm, Pagination_Info page_info){
    char ytdlp_cmd[512];
    char cmd_format[512] = "yt-dlp \"ytsearch%d:%s\" -I %d-%d --flat-playlist --print \"%s\" --extractor-args youtubetab:approximate_date";

    snprintf(ytdlp_cmd, sizeof(ytdlp_cmd), cmd_format, page_info.upper_end(), byTerm,
             page_info.lower_end(), page_info.upper_end(), PRINT_METADATA_TEMPLATE.c_str());
    return exec(ytdlp_cmd);
}

/**
 * Make a search by term in the specified extractor (i.e. "youtube", etc.). See a complete list of extractors at yt-dlp docs.
 * For now, only do searchs at Youtube.
 */
std::string do_ytdlp_search(const char* search_term, const char* extractor_name, Pagination_Info page_info){

    if (extractor_name == YOUTUBE_EXTRACTOR_NAME) {
        return do_youtube_search(search_term, page_info);
    } else {
        return "No extractor matched...";
    }
}

/**
 * Retrieve metadata for an specified video URL.
 */
std::string get_videoURL_metadata(const char* video_url){
    char ytdlp_cmd[512];
    char cmd_format[512] = "yt-dlp --flat-playlist --print \"%s\" --extractor-args youtubetab:approximate_date %s";
    snprintf(ytdlp_cmd, sizeof(ytdlp_cmd), cmd_format, PRINT_METADATA_TEMPLATE.c_str(), video_url);
    return exec(ytdlp_cmd);
}

/**
 * Stream a video from a its URL using the configured multimedia player at @DEFAULT_STREAM_PLAYER. The stream resolution is 360p.
 */
void stream_video(const char* video_url, const MediaPlayerInfo* mp) {
    //Open Video in MPlayer
    char stream_videoplayer_cmd[2048];
    const char* stream_format = "bestvideo[height<=360][vcodec=avc1]+bestaudio[ext=m4a]/best";
    snprintf(stream_videoplayer_cmd, sizeof(stream_videoplayer_cmd),
                "%s %s \"$(yt-dlp -f \"%s\" -g \"%s\")\"", mp->binary_path.c_str(), mp->parameters.c_str(), stream_format, video_url);
    printf("%s\n", stream_videoplayer_cmd);
    system(stream_videoplayer_cmd);
}

/**
 * Download a video from a its URL using the configured multimedia player.
 */
void download_video(const char* video_url, const char* download_path, VCODEC_RESOLUTIONS v_resolution,
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

/**
 *  Create a CURL handle, for an specific URL (not null) and an optional output_file;
 */
static CURL* get_curl_handle(const char* forURL, FILE* output_file) {
    if(forURL == nullptr || forURL[0] == '\0'){
        return nullptr;
    }
    CURL *curl;
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, forURL);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);  //This allow redirections (i.e. HTTP 301 code)
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
        if (output_file != nullptr)     curl_easy_setopt(curl, CURLOPT_WRITEDATA, output_file);
    }
    return curl;
}

//Function for download a file to a local output directory, and set a custom name. Returns 0 if all is ok.
FLTUBE_STATUS_CODES download_file(std::string url, std::string output_dir, std::string outfilename, bool overwrite) {
    CURL *curl;
    FILE *fp;
    CURLcode response;
    FLTUBE_STATUS_CODES returnCode = FLT_OK;
    std::string fullpath = output_dir + outfilename;
    //If must not overwrite the file, check if exists before do any download...
    if (!overwrite && std::filesystem::exists(fullpath)) {
        return FLT_DOWNLOAD_FL_BYPASSED;
    }

    fp = fopen(fullpath.c_str(),"wb");
    if (fp == nullptr) {
       perror("Error creating download file");
        //TODO: what else we can do if fp is nullptr?
    }
    curl = get_curl_handle(url.c_str(), fp);
    if (curl) {
        response = curl_easy_perform(curl);

        if (response != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(response));
            returnCode = FLT_DOWNLOAD_FL_FAILED;
        }
        curl_easy_cleanup(curl);
        fclose(fp);
    }
    return returnCode;
}

/**
 *  Check if there is network connectivity. Returns true if Internet is reachable.
 */
bool verify_network_connection() {
    CURL *curl;
    CURLcode response;
    const char* url_test = "https://youtube.com";
    // Use null device to redirect CURL output...
    FILE* null_file = fopen("/dev/null", "w");
    if (null_file == nullptr)  {
        //TODO what else we can do if null_file is nullptr?
        perror("Error opening file");
    }
    curl = get_curl_handle(url_test, null_file);
    if (curl) {
        response = curl_easy_perform(curl);
        if ( response != CURLE_OK) {
            printf(_("Connection testing failure: %s. Check your connectivity.\n"), url_test);
            curl_easy_cleanup(curl);
            return false;
        }
        curl_easy_cleanup(curl);
        if (null_file != nullptr) fclose(null_file);
        return true;
    }
    return false;
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

/*
 * Parse a parameter with the form "key=value". In example: "--aKey=aValue" will return "aValue".
 * If parameter doesn't exists, or don't have a value, return an empty string ("").
 */
std::string getOptionValue(int argc, char* argv[], const std::string& option) {
    std::string opt_value("");
    for( int i = 0; i < argc; ++i) {
        std::string arg = argv[i];
        if(0 == arg.find(option)) {
            std::size_t found = arg.find_first_of("=");
            if (found != std::string::npos){
                //Only is a value specified if "=" is present.
                opt_value =arg.substr(found + 1);
            }
            return opt_value;
        }
    }
    return opt_value;
}

/*
 * Search for a parameter in the form "key" or "key=value" (In example: "--help", or "-h", or "--ip=W.X.Y.Z").
 * If exists, return true.
 */
bool existsCmdOption(int argc, char* argv[], const std::string& option) {
    for( int i = 0; i < argc; ++i) {
        std::string arg = argv[i];
        if(0 == arg.find(option)) {
            return true;
        }
    }
    return false;
}

/*
 * Erase all spaces at the begining or end of the parameter, modifing it.
 */
void trim(std::string &s){
    // Left trim
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));

    // Right trim
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}



// TODO: all of this, the configuration map and the logic to check the configs, should be abstracted in a Configuration Class...
/**
 * Load configurations at a .conf file, with the following format: config_key=value.
 */
std::unique_ptr<std::map<std::string, std::string>> loadConfFile(const char* path_to_conf){
    auto configs = std::make_unique<std::map<std::string,std::string>>();
    std::ifstream infile(path_to_conf);

    std::string line, config_key, config_value;
    while (std::getline(infile, line)) {
        config_key = config_value = "";
        trim(line);
        if(!line.empty() && line.at(0) != '#') {
            std::size_t found = line.find_first_of("=");
            if (found != std::string::npos){
                //Only is a value specified if "=" is present.
                config_key = line.substr(0, found);
                config_value = line.substr(found + 1);
                trim(config_key);
                trim(config_value);
                (*configs)[config_key] = config_value;
            }
        }
    }

    return configs;
}

/*
 * Return the value of a property configuration loaded with @loadConfFile() method.
 * If no property is found, returns @default_value or empty string ("") if no default value is specified..
 */
std::string getProperty(const char* config_name, const char* default_value, const std::unique_ptr<std::map<std::string, std::string>> &config_map){
    if (!config_name || std::strlen(config_name) == 0) {
        return "";
    }
    else if (config_map && config_map->find(config_name) != config_map->end()) {
        return config_map->find(config_name)->second;
    }
    return std::string((default_value) ? default_value : "");
}

bool existProperty(const char* config_name, const std::unique_ptr<std::map<std::string, std::string>> &config_map) {
    return (config_name && config_map && config_map->find(config_name) != config_map->end());
}
