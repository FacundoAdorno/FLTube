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

#include "../include/FLTube.h"
#include "../include/FLTube_View.h"
#include "../include/fltube_utils.h"
#include <cstdio>
#include <filesystem>
#include <string>

/** Main Fltube window. */
FLTubeMainWindow* mainWin =  (FLTubeMainWindow *)0;

/** Generic message window.*/
TinyMessageWindow* message_window = (TinyMessageWindow *)0;

// Array that holds the search results WIDGETS, in groups of 5...
std::array <VideoInfo*,5> video_info_arr{ nullptr, nullptr, nullptr, nullptr, nullptr };
// Array that holds the search results METADATA, in groups of 5...
std::array <YTDLP_Video_Metadata*,5> video_metadata{ nullptr, nullptr, nullptr, nullptr, nullptr };

const std::string FLTUBE_TEMPORAL_DIR(std::filesystem::temp_directory_path().generic_string() + "/fltube_tmp_files/");

/**
 * Callback for main window close action. By default, exit app with success status code (0).
 */
void exitApp(unsigned short int exitStatusCode = FLT_OK) {
    //Do some cleaning...
    std::filesystem::remove_all(FLTUBE_TEMPORAL_DIR);
    //Exiting the app...
    printf("Closing FLtube... Bye!\n");
    exit(exitStatusCode);
}

/**
 * Hide the target window.
 */
void closeWindow(Fl_Widget*, Fl_Window *targetWindow) {
    targetWindow->hide();
}

/**
 *
 */
static void showMessageWindow(const char* message){
    if (message_window == nullptr) {
        message_window = new TinyMessageWindow();
        message_window->close_bttn->callback((Fl_Callback*)closeWindow, (void*)(message_window));
    }
    message_window->show();
    message_window->error_label->label(message);

    // Loop until the message window is closed...
    while (message_window->shown()) {
        Fl::wait();
    }
}

void download_video_specified_resolution(Fl_Button* resltn_bttn, void* download_video_data){
    //VCODEC_RESOLUTIONS vc = VCODEC_RESOLUTIONS(resolution);
    DownloadVideoCBData* download_data =  static_cast<DownloadVideoCBData*>(download_video_data);
    printf("VIDEO URL: %s - LA RESOLUCION A DESCARGAR ES: %dp\n", download_data->video_url.c_str(), download_data->video_resolution);
}

/**
 * Create a new group to view video info.
 */
VideoInfo* create_video_group(int posx, int posy) {
    VideoInfo *video_info = new VideoInfo (posx, posy, 600, 90, "");
    DownloadVideoCBData* dv_data = new DownloadVideoCBData{VCODEC_RESOLUTIONS::R240p, ""};
    video_info->d240->callback((Fl_Callback*) download_video_specified_resolution, dv_data);
    dv_data = new DownloadVideoCBData{VCODEC_RESOLUTIONS::R360p, ""};
    video_info->d360->callback((Fl_Callback*) download_video_specified_resolution, dv_data);
    dv_data = new DownloadVideoCBData{VCODEC_RESOLUTIONS::R480p, ""};
    video_info->d480->callback((Fl_Callback*) download_video_specified_resolution, dv_data);
    dv_data = new DownloadVideoCBData{VCODEC_RESOLUTIONS::R720p, ""};
    video_info->d720->callback((Fl_Callback*) download_video_specified_resolution, dv_data);
    dv_data = new DownloadVideoCBData{VCODEC_RESOLUTIONS::R1080p, ""};
    video_info->d1080->callback((Fl_Callback*) download_video_specified_resolution, dv_data);
    return video_info;
}

/**
 * Update the VideoInfo widgets data, based on the data on the video_metadata array.
 */
void update_video_info() {
    YTDLP_Video_Metadata* ytvm = nullptr;
    for (int j=0; j < video_metadata.size(); j++) {
        if(video_metadata[j] != nullptr){
            video_info_arr[j]->show();
            video_info_arr[j]->title->label(video_metadata[j]->title.c_str());
            video_info_arr[j]->title->tooltip(video_metadata[j]->title.c_str());
            video_info_arr[j]->duration->label(video_metadata[j]->duration.c_str());
            video_info_arr[j]->uploadDate->label(video_metadata[j]->upload_date.c_str());
            video_info_arr[j]->userUploader->label(video_metadata[j]->creators.c_str());
            // Update the URL in every Download Button in VideoInfo
            const std::vector<DownloadVideoCBData*> download_buttons = {
                static_cast<DownloadVideoCBData*>(video_info_arr[j]->d240->user_data()),
                static_cast<DownloadVideoCBData*>(video_info_arr[j]->d360->user_data()),
                static_cast<DownloadVideoCBData*>(video_info_arr[j]->d480->user_data()),
                static_cast<DownloadVideoCBData*>(video_info_arr[j]->d720->user_data()),
                static_cast<DownloadVideoCBData*>(video_info_arr[j]->d1080->user_data())
            };
            for(DownloadVideoCBData* dvcbd:download_buttons){
                dvcbd->video_url = video_metadata[j]->url;
            }
            //Download, resize and update the video thumbnail image...
            std::string thumbn_url = video_metadata[j]->thumbnail_url.substr(0, video_metadata[j]->thumbnail_url.find("?"));
            std::string thumbn_name = "th_" + video_metadata[j]->id + ".jpg";
            download_file(thumbn_url, FLTUBE_TEMPORAL_DIR, thumbn_name);
            int targetWidth = video_info_arr[j]->thumbnail->w();
            Fl_Image* resized_thumbnail = create_resized_image_from_jpg(FLTUBE_TEMPORAL_DIR + thumbn_name, targetWidth);
            if (resized_thumbnail == nullptr) {
                logAtBuffer("Something went wrong when generating a resize thumbnail for video with ID=" + video_metadata[j]->id, LogLevel::ERROR);
            } else {
                delete video_info_arr[j]->thumbnail->image();
                video_info_arr[j]->thumbnail->image(resized_thumbnail);
                video_info_arr[j]->thumbnail->redraw();
            }

        } else {
            // Hide the VideoInfo widget if there are no more results to show...
            video_info_arr[j]->hide();
        }
    }
}

/**
 * Hook: Actions to execute before main window is drawed...
 */
void pre_init() {
    //Create temporal directory and change current working directory to that dir.
    std::filesystem::create_directory(FLTUBE_TEMPORAL_DIR);
    std::filesystem::current_path(FLTUBE_TEMPORAL_DIR);
}

/**
 * Hook: Actions to execute after main window is drawed...
 */
void post_init() {
    int y_refernce_pos = mainWin->search_result_selectors->y() + 10;

    for (int i = 0; i < video_info_arr.size(); i++) {
        video_info_arr[i] = create_video_group(15, y_refernce_pos + (90 * i));
        mainWin->add(video_info_arr[i]);
    }

    for (VideoInfo* videoInfo: video_info_arr) {
        videoInfo->hide();
    }

    // Redraw the window to show the new button
    mainWin->redraw();
}

/** Write a log message at main application text buffer.*/
void logAtBuffer(std::string log_message, LogLevel log_lvl) {
    std::string logleveltext;
    switch (log_lvl) {
        case LogLevel::INFO:
            logleveltext = "[INFO] ";
            break;
        case LogLevel::WARN:
            logleveltext = "[WARN] ";
            break;
        case LogLevel::ERROR:
            logleveltext = "[ERROR] ";
            break;
        default:
            logleveltext = "[UNKNOWN] ";
    }
    mainWin->output_text_display->buffer()->append((logleveltext +  currentDateTime() + " --- "+ log_message + "\n").c_str());
}

/**
 * Callback for search button.
 */
void doSearch(Fl_Widget*, Fl_Input *input) {
    if (input == nullptr) {
        printf("User input is empty!!!");
        return;
    }
    const char* input_text = input->value();
    std::string result;
    if (isUrl(input_text)) {
        if(!isYoutubeURL(input_text)){
            std::string warn_message = "For now, only Youtube URL's are valid for download. Please, edit your input text or search using a generic term.";
            showMessageWindow(warn_message.c_str());
            logAtBuffer(warn_message, LogLevel::WARN);
            return;
        }
        //TODO if it a youtube URL, get and show video info....
    } else {
        //printf("User input is a search term.\n");
        result = do_ytdlp_search(input_text, YOUTUBE_EXTRACTOR_NAME.c_str());
        logAtBuffer(result, LogLevel::INFO);
    }
    // Read input lines until an empty line is encountered
    std::istringstream result_sstream(result);
    std::string line;
    for (int i=0; i<5; i++) {
        getline(result_sstream, line);
        if (!line.empty()) {
            //Set the video metadata at the array...
            video_metadata[i] = parse_YT_Video_Metadata(line.c_str());
        } else if(video_metadata[i] != nullptr){
            //Or release the unused pointers to free memory...
            delete video_metadata[i];
            video_metadata[i] = nullptr;
        }
    }
    update_video_info();
}

//////////////////////////////////////////////////////////////////////////////////
///////////////// MAIN     MAIN     MAIN        MAIN        MAIN   //////////////
//////////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv) {
    //CONSTANTS DECLARATION
    static const char* VERSION = "0.2.2.alpha";
    printf("Starting FLtube v.%s\n", VERSION);
    pre_init();
    if ( !checkForYTDLP() ) {
        showMessageWindow("yt-dlp is not installed on your system or its binary is not at $PATH system variable." \
        "See how to install it at https://github.com/yt-dlp/yt-dlp.");
        printf("yt-dlp is not installed. Closing app...\n");
        exitApp(FLT_GENERAL_FAILED);
    }
    mainWin = new FLTubeMainWindow(800, 618, "FLtube (0.2)");
    mainWin->callback((Fl_Callback*)exitApp);

    mainWin->do_search_bttn->callback((Fl_Callback*)doSearch, (void*)(mainWin->search_term_or_url));
    Fl_Text_Buffer* buffer = new Fl_Text_Buffer();
    mainWin->output_text_display->buffer(buffer);

    post_init();
    mainWin->show(argc, argv);
    return Fl::run();

}
