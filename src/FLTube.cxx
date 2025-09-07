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

#include "../include/gnugettext_utils.h"
#include "../include/FLTube.h"
#include "../include/FLTube_View.h"
#include "../include/fltube_utils.h"


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

std::string CONFIGFILE_PATH = "/usr/local/etc/fltube/fltube.conf";

//If value is 0, enable the debug mode. Defaults to false (or 1).
int DEBUG_ENABLED = 1;

// This variable holds the configured video codec used when download a video.
std::string DOWNLOAD_VIDEO_CODEC;

MediaPlayerInfo* media_player;

std::unique_ptr<std::map<std::string, std::string>> configParameters = {};

/**
 * Callback for main window close action. By default, exit app with success status code (0).
 */
void exitApp(unsigned short int exitStatusCode = FLT_OK) {
    //Do some cleaning...
    char message[1024];
    snprintf(message, sizeof(message), _("Cleaning temporal files at %s."), FLTUBE_TEMPORAL_DIR.c_str());
    logAtTerminal(std::string(message), LogLevel::INFO);
    std::filesystem::remove_all(FLTUBE_TEMPORAL_DIR);
    //Exiting the app...
    logAtTerminal(_("Closing FLtube... Bye!\n"), LogLevel::INFO);
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
        message_window->set_modal();
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
    choiceWindow->set_modal();
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
    if (! verify_network_connection()) {
        logAtTerminal(_("Your device is offline. Check your internet connection."), LogLevel::WARN);
        showMessageWindow( _("There seems that you don't have access to the Internet. "
        "Please, verify you network connection before proceed..."));
        return;
    }
    std::string* url = static_cast<std::string*>(video_url);
    if (url){
        char message[256];
        snprintf(message, sizeof(message), _("Starting streaming preview of video '%s'..."), url->c_str());
        logAtTerminal(message,LogLevel::INFO);
        stream_video(url->c_str(), media_player);
    } else {
        logAtTerminal(_("Cannot get video URL. Review the video metadata enabling app debugging..."), LogLevel::ERROR);
    }
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

            //Setting URL for streaming a video preview...
            video_info_arr[j]->thumbnail->user_data(static_cast<void*>(&video_metadata[j]->url));

            //Download, resize and update the video thumbnail image...
            std::string thumbn_url = video_metadata[j]->thumbnail_url.substr(0, video_metadata[j]->thumbnail_url.find("?"));
            std::string thumbn_name = "th_" + video_metadata[j]->id + ".jpg";
            download_file(thumbn_url, FLTUBE_TEMPORAL_DIR, thumbn_name);
            int targetWidth = video_info_arr[j]->thumbnail->w();
            Fl_Image* resized_thumbnail = create_resized_image_from_jpg(FLTUBE_TEMPORAL_DIR + thumbn_name, targetWidth);
            if (resized_thumbnail == nullptr) {
                logAtTerminal(_("Something went wrong when generating a resize thumbnail for video with ID=") + video_metadata[j]->id, LogLevel::ERROR);
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
 * Hook: Actions to execute before main window is drawn...
 */
void pre_init() {
    if ( !checkForYTDLP() ) {
        showMessageWindow(_("yt-dlp is not installed on your system or its binary is not at $PATH system variable. See how to install it at https://github.com/yt-dlp/yt-dlp. Or run in a terminal the 'install_yt-dlp' script."));
        logAtTerminal(_("yt-dlp is not installed. Closing app...\n"), LogLevel::ERROR);
        exitApp(FLT_GENERAL_FAILED);
    }

    ///// LOAD CONFIGURATIONS  //////
    configParameters = loadConfFile(CONFIGFILE_PATH.c_str());

    //Init Localization. Use locale path specified at config, or custom config default_locale_path().
    setup_gettext("", getProperty("LOCALE_PATH", default_locale_path().c_str(), configParameters));

    media_player = new MediaPlayerInfo();
    if(existProperty("STREAM_PLAYER_PATH", configParameters)) {
        media_player->binary_path = getProperty("STREAM_PLAYER_PATH", "", configParameters);
        media_player->parameters = getProperty("STREAM_PLAYER_PARAMS", "", configParameters);
    } else {
        media_player->binary_path = DEFAULT_STREAM_PLAYER;
        media_player->parameters = DEFAULT_PLAYER_PARAMS;
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

/** Write a log message at the terminal where the application was launched. */
void logAtTerminal(std::string log_message, LogLevel log_lvl) {
    if (log_lvl == LogLevel::DEBUG && DEBUG_ENABLED == 1) { return; }//if debug mode disabled, cancel debug message...

    std::string logleveltext;
    switch (log_lvl) {
        case LogLevel::INFO:
            logleveltext = "\033[32m\033[1m[INFO]\033[0m ";
            break;
        case LogLevel::WARN:
            logleveltext = "\033[34m\033[1m[WARN]\033[0m ";
            break;
        case LogLevel::ERROR:
            logleveltext = "\033[31m\033[1m[ERROR]\033[0m ";
            break;
        case LogLevel::DEBUG:
            logleveltext = "\033[33m\033[1m[DEBUG]\033[0m ";
            break;
        default:
            logleveltext = "\033[1m[UNKNOWN]\033[0m ";
    }
    printf("\n%s [%s] - - -  %s\n", logleveltext.c_str(), currentDateTime().c_str(), log_message.c_str());
}

/**
 * Create a new group to view video info.
 */
VideoInfo* create_video_group(int posx, int posy) {
    VideoInfo *video_info = new VideoInfo (posx, posy, 600, 90, "");
    video_info->thumbnail->callback((Fl_Callback*)preview_video_cb);
    return video_info;
}

/**
 * Callback for search button: search by YT URL or search term. Input mustn't be empty.'
 */
void doSearch_cb(Fl_Widget*, Fl_Input *input) {
    // Check if there is Internet connectivity before do a search...
    if (! verify_network_connection()) {
        logAtTerminal(_("Your device is offline. Check your internet connection."), LogLevel::WARN);
        showMessageWindow( _("There seems that you don't have access to the Internet. "
                            "Please, verify you network connection before proceed..."));
        return;
    }
    if (input == nullptr) {
        logAtTerminal(_("The Fl_Input search widget inexistent!!!\n"), LogLevel::ERROR);
        return;
    }

    const char* input_text = input->value();
    if (input_text == nullptr || input_text[0] == '\0') {
        showMessageWindow(_("Search input is empty. Please, retry and enter a valid text."));
        return;
    }

    char message[1024];
    snprintf(message, sizeof(message), _("Searching for results for '%s' user input..."), input_text);
    logAtTerminal(std::string(message), LogLevel::DEBUG);
    std::string result;
    if (isUrl(input_text)) {
        if(!isYoutubeURL(input_text)){
            std::string warn_message = _("For now, only Youtube URL's are valid for download. Please, edit your input text or search using a generic term.");
            showMessageWindow(warn_message.c_str());
            logAtTerminal(warn_message, LogLevel::WARN);
            return;
        }
        result = get_videoURL_metadata(input_text);

    } else {
        Pagination_Info page_i(SEARCH_PAGE_SIZE, SEARCH_PAGE_INDEX);
        if (SEARCH_PAGE_INDEX == 0) {
            mainWin->next_results_bttn->activate();
        }
        result = do_ytdlp_search(input_text, YOUTUBE_EXTRACTOR_NAME.c_str(), page_i);
        logAtTerminal(result, LogLevel::DEBUG);
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

/*
 * Print program usage and exit if desire (exitApp = true).
 */
void showUsage(bool exitApp){
    const char* usage = R"([FLTube HELP]:
FLTube is a program developed in FLTK for search and download Youtube videos, as a front-end for the subjacent "yt-dlp" tool.

USAGE:
    fltube [options]

OPTIONS:
  --config=[PATH_TO_FILE]   Please specify the ABSOLUTE path to the custom configuration file, which should be
                              a copy of the original fltube.config. Use this option when you want to apply a
                              development configuration file for development purposes.
  -d, --debug               Enable the debug mode through more verbose log messages.
  -h, --help                Show this text help.
  -v, --version             Show the program version.

)";
    printf("%s", usage);

    if (exitApp) { exit(FLT_OK); }
}

/*
 *  Parse commandline parameters.
 */
void parseOptions(int argc, char **argv){
    if (existsCmdOption(argc, argv, "-h") || existsCmdOption(argc, argv, "--help")) {
        showUsage(true);
    }
    if (existsCmdOption(argc, argv, "-v") || existsCmdOption(argc, argv, "--version")) {
        printf("FLtube v.%s\n", VERSION);
        exit(FLT_OK);
    }
    if (existsCmdOption(argc, argv, "-d") || existsCmdOption(argc, argv, "--debug")) {
        printf(_("DEBUG MODE enabled.\n"));
        DEBUG_ENABLED = 0;
    }
    if (existsCmdOption(argc, argv, "--config")){
        const std::string path = getOptionValue(argc, argv, "--config");
        if (path != ""){
            CONFIGFILE_PATH = path;
            printf(_("Loading custom config file %s.\n"), CONFIGFILE_PATH.c_str());
        } else {
            printf(_("--config parameter with no path specified.\n"));
            exit(FLT_INVALID_CMD_PARAM);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////
///////////////// MAIN     MAIN     MAIN        MAIN        MAIN   //////////////
//////////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv) {
    parseOptions(argc, argv);
    pre_init();
    char message[64];
    snprintf(message, sizeof(message), _("Starting FLTube v.%s\n"), VERSION);
    logAtTerminal(message, LogLevel::INFO);

    char win_title[30] = "FLTube ";
    mainWin = new FLTubeMainWindow(593, 618, strcat(win_title, VERSION));
    mainWin->callback((Fl_Callback*)exitApp);
    mainWin->do_search_bttn->callback((Fl_Callback*)doSearch_cb, (void*)(mainWin->search_term_or_url));
    mainWin->previous_results_bttn->callback((Fl_Callback*)getPreviousSearchResults_cb, (void*)(mainWin->search_term_or_url));
    mainWin->previous_results_bttn->deactivate();
    mainWin->next_results_bttn->callback((Fl_Callback*)getNextSearchResults_cb, (void*)(mainWin->search_term_or_url));
    mainWin->next_results_bttn->deactivate();

    post_init();
    //mainWin->show(argc, argv);    //TODO Avoid passing parameters to the mainWin function to prevent undesirable logs.
    mainWin->show();
    logAtTerminal(_("The FLTube main window has loaded."), LogLevel::DEBUG);
    return Fl::run();

}
