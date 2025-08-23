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
#include <FL/Fl_Check_Button.H>
#include <cstdio>

const std::string* URL_TEST = new std::string("http://example.com/video.mp4");

static const char* VERSION = "0.2.2.alpha";

/** Main Fltube window. */
FLTubeMainWindow* mainWin =  (FLTubeMainWindow *)0;

/** Generic message window.*/
TinyMessageWindow* message_window = (TinyMessageWindow *)0;

// Current page index at search results navigation.
int SEARCH_PAGE_INDEX = 0;

// Count of search results per page. BEWARE: don't modify this value unless you change the view at Fltube_View.cxx file.'
const int SEARCH_PAGE_SIZE = 5;

// Array that holds the search results WIDGETS, in groups of size @SEARCH_PAGE_SIZE...
std::array <VideoInfo*,SEARCH_PAGE_SIZE> video_info_arr{ nullptr, nullptr, nullptr, nullptr, nullptr };
// Array that holds the search results METADATA, in groups of size @SEARCH_PAGE_SIZE...
std::array <YTDLP_Video_Metadata*,SEARCH_PAGE_SIZE> video_metadata{ nullptr, nullptr, nullptr, nullptr, nullptr };

const std::string FLTUBE_TEMPORAL_DIR(std::filesystem::temp_directory_path().generic_string() + "/fltube_tmp_files/");

/** Flag to warn that DOWNLOADING videos in high resolutions (720p, 1080p) may result in a high CPU usage. */
bool WARN_ABOUT_HIGH_CPU_USAGE_HD_V_DOWNLOAD = true;

/**
 * Callback for main window close action. By default, exit app with success status code (0).
 */
void exitApp(unsigned short int exitStatusCode = FLT_OK) {
    //Do some cleaning...
    printf("Cleaning temporal files at %s.\n.", FLTUBE_TEMPORAL_DIR.c_str());
    std::filesystem::remove_all(FLTUBE_TEMPORAL_DIR);
    //Exiting the app...
    printf("Closing FLtube... Bye!\n");
    exit(exitStatusCode);
}

/**
 * Hide the target window.
 */
void closeWindow_cb(Fl_Widget*, Fl_Window *targetWindow) {
    targetWindow->hide();
}

/**
 * Show a tiny window with a message.
 */
void showMessageWindow(const char* message){
    if (message_window == nullptr) {
        message_window = new TinyMessageWindow();
        message_window->close_bttn->callback((Fl_Callback*)closeWindow_cb, (void*)(message_window));
    }
    message_window->show();
    message_window->error_label->label(message);

    // Loop until the message window is closed...
    while (message_window->shown()) {
        Fl::wait();
    }
}

/** Show a choice window with a message. The user can accept or cancel to continue an action.
 *  @param acceptAlwaysFlag: a memory address to a boolean that tells if the user wants to continue see this window in the future.
 *
 * Returns @true only if the user accept the choice window.
 */
bool showChoiceWindow(const char* message, bool& keepShowingFlag) {
    bool choice_result = false;
    TinyChoiceWindow* choiceWindow = new TinyChoiceWindow();
    choiceWindow->cancel_bttn->callback( [] (Fl_Widget* widget, void* data) {
        bool* accept_ch = static_cast<bool*>(data);
        *accept_ch = false;
        widget->parent()->hide();
    }, &choice_result);
    choiceWindow->accept_bttn->callback( [] (Fl_Widget* widget, void* data) {
        bool* accept_ch = static_cast<bool*>(data);
        *accept_ch = true;
        widget->parent()->hide();
    }, &choice_result);
    choiceWindow->warnme_again_check->callback( [] (Fl_Widget* widget, void* data) {
        bool* keep_showing_message_flag = static_cast<bool*>(data);
        Fl_Check_Button* ch_bttn = static_cast<Fl_Check_Button*>(widget);
        *keep_showing_message_flag = ! (ch_bttn->value());
    }, &keepShowingFlag);
    choiceWindow->show();
    choiceWindow->choice_label->label(message);

    // Loop until the message window is closed...
    while (choiceWindow->shown()) {
        Fl::wait();
    }
    delete choiceWindow;
    return choice_result;
}



/** Callback to preview a video... */
void preview_video_cb(Fl_Button* widget, void* video_url){
    std::string* url = static_cast<std::string*>(video_url);
    if (url){
        logAtBuffer("Starting streaming preview of video...",LogLevel::INFO);
        stream_video(url->c_str());
    } else {
        logAtBuffer("Cannot get video URL. Review your the video metadata...", LogLevel::ERROR);
    }
}

/** Callback for download a video at resolution relative to button clicked...
 */
void download_video_specified_resolution_cb(Fl_Button* resltn_bttn, void* download_video_data){
    //VCODEC_RESOLUTIONS vc = VCODEC_RESOLUTIONS(resolution);
    DownloadVideoCBData* download_data =  static_cast<DownloadVideoCBData*>(download_video_data);
    bool continueToDownload = true;
    printf("VIDEO URL: %s - LA RESOLUCION A DESCARGAR ES: %dp\n", download_data->video_url.c_str(), download_data->video_resolution);
    if(WARN_ABOUT_HIGH_CPU_USAGE_HD_V_DOWNLOAD && download_data->video_resolution >= 720) {
        //Only show waning message if
        continueToDownload = showChoiceWindow("WARNING: This option may result in high CPU usage. It is not recommended on low-spec PCs, as it may freeze the desktop interface. Continue to download video?", WARN_ABOUT_HIGH_CPU_USAGE_HD_V_DOWNLOAD);
    }
    if(!continueToDownload){
        logAtBuffer("Download CANCELLED by user choice.", LogLevel::WARN);
        return;
    }
    std::string download_dir = mainWin->video_download_directory->value();
    download_video(download_data->video_url.c_str(), download_dir.c_str(), VCODEC_RESOLUTIONS(download_data->video_resolution));
    logAtBuffer("Video downloaded at " + download_dir, LogLevel::INFO);
}

/**
 * Create a new group to view video info.
 */
VideoInfo* create_video_group(int posx, int posy) {
    VideoInfo *video_info = new VideoInfo (posx, posy, 600, 90, "");
    DownloadVideoCBData* dv_data = new DownloadVideoCBData{VCODEC_RESOLUTIONS::R240p, ""};
    video_info->d240->callback((Fl_Callback*) download_video_specified_resolution_cb, dv_data);
    dv_data = new DownloadVideoCBData{VCODEC_RESOLUTIONS::R360p, ""};
    video_info->d360->callback((Fl_Callback*) download_video_specified_resolution_cb, dv_data);
    dv_data = new DownloadVideoCBData{VCODEC_RESOLUTIONS::R480p, ""};
    video_info->d480->callback((Fl_Callback*) download_video_specified_resolution_cb, dv_data);
    dv_data = new DownloadVideoCBData{VCODEC_RESOLUTIONS::R720p, ""};
    video_info->d720->callback((Fl_Callback*) download_video_specified_resolution_cb, dv_data);
    dv_data = new DownloadVideoCBData{VCODEC_RESOLUTIONS::R1080p, ""};
    video_info->d1080->callback((Fl_Callback*) download_video_specified_resolution_cb, dv_data);
    video_info->thumbnail->callback((Fl_Callback*)preview_video_cb);
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
            //Setting URL for streaming a video preview...
            video_info_arr[j]->thumbnail->user_data(static_cast<void*>(&video_metadata[j]->url));
            //Setting URL for download buttons...
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
    if ( !checkForYTDLP() ) {
        showMessageWindow("yt-dlp is not installed on your system or its binary is not at $PATH system variable." \
        "See how to install it at https://github.com/yt-dlp/yt-dlp.");
        printf("yt-dlp is not installed. Closing app...\n");
        exitApp(FLT_GENERAL_FAILED);
    }
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
 * Callback for search button: search by YT URL or search term. Input musn't be empty.'
 */
void doSearch_cb(Fl_Widget*, Fl_Input *input) {
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
        result = get_videoURL_metadata(input_text);

    } else {
        //printf("User input is a search term.\n");
        Pagination_Info page_i(SEARCH_PAGE_SIZE, SEARCH_PAGE_INDEX);
        if (SEARCH_PAGE_INDEX == 0) {
            mainWin->next_results_bttn->activate();
        }
        result = do_ytdlp_search(input_text, YOUTUBE_EXTRACTOR_NAME.c_str(), page_i);
        logAtBuffer(result, LogLevel::INFO);
    }
    // Read input lines until an empty line is encountered
    std::istringstream result_sstream(result);
    std::string line;
    for (int i=0; i < SEARCH_PAGE_SIZE; i++) {
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

/** Callback for Previous results button... */
void getPreviousSearchResults_cb(Fl_Widget* widget, Fl_Input *input){
    if (SEARCH_PAGE_INDEX>0) {
        SEARCH_PAGE_INDEX--;
    }
    if (SEARCH_PAGE_INDEX == 0) {
        mainWin->previous_results_bttn->deactivate();
    }
    doSearch_cb(widget, input);
}
/** Callback for Next results button.. */
void getNextSearchResults_cb(Fl_Widget* widget, Fl_Input *input){
    //TODO: what to do if there is no more results? It must be controlled in some way...
    SEARCH_PAGE_INDEX++;
    mainWin->previous_results_bttn->activate();
    doSearch_cb(widget, input);
}

/** Change video download directory callback.  */
void select_directory_cb(Fl_Widget* widget, void* output) {
    const char *selected_directory;

    Fl_Input* output_widget = static_cast<Fl_Input*>(output);

    //printf("DIR: %s\n", output_widget->value());
    const char* output_value = output_widget->value();
    if (output_value != nullptr && output_value[0] != '\0') {
        selected_directory = output_value;
    } else if ((selected_directory = getenv("HOME")) == NULL) {
        selected_directory = nullptr;
    }

    // Open dialog to select directory
    char* dir = fl_dir_chooser("Select a download directory", selected_directory);

    // If a directory is selected, save it at output value.
    if (dir && std::filesystem::exists(dir)) {
        if(!canWriteOnDir(dir)){
            std::string directory(dir);
            logAtBuffer("Don't have write permissions on " + directory + ". Select another one.", LogLevel::WARN);
            return;
        }
        output_widget->value(dir);
        output_widget->tooltip(dir);
    } else {
        std::cout << "No directory was selected." << std::endl;
    }
}

//////////////////////////////////////////////////////////////////////////////////
///////////////// MAIN     MAIN     MAIN        MAIN        MAIN   //////////////
//////////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv) {
    printf("Starting FLtube v.%s\n", VERSION);
    pre_init();

    char win_title[30] = "FLtube ";
    mainWin = new FLTubeMainWindow(800, 618, strcat(win_title, VERSION));
    mainWin->callback((Fl_Callback*)exitApp);
    mainWin->do_search_bttn->callback((Fl_Callback*)doSearch_cb, (void*)(mainWin->search_term_or_url));
    mainWin->previous_results_bttn->callback((Fl_Callback*)getPreviousSearchResults_cb, (void*)(mainWin->search_term_or_url));
    mainWin->previous_results_bttn->deactivate();
    mainWin->next_results_bttn->callback((Fl_Callback*)getNextSearchResults_cb, (void*)(mainWin->search_term_or_url));
    mainWin->next_results_bttn->deactivate();
    Fl_Text_Buffer* buffer = new Fl_Text_Buffer();
    mainWin->output_text_display->buffer(buffer);
    mainWin->video_download_directory->value(getHomePathOr(FLTUBE_TEMPORAL_DIR.c_str()));
    mainWin->video_download_directory->readonly(1);
    mainWin->change_dwl_dir_bttn->callback((Fl_Callback*)select_directory_cb, mainWin->video_download_directory);

    post_init();
    mainWin->show(argc, argv);
    return Fl::run();

}
