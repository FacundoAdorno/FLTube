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
#include "../include/configuration_manager.h"
#include "../include/userdata_manager.h"

/** Main Fltube window. */
FLTubeMainWindow* mainWin =  (FLTubeMainWindow *)0;

/** Help window. */
HelpFLTubeWindow* helpWin = (HelpFLTubeWindow*) 0;

/** Generic message window.*/
TinyMessageWindow* message_window = (TinyMessageWindow *)0;

// Current page index at search results navigation.
int SEARCH_PAGE_INDEX = 0;

// Count of search results per page. BEWARE: don't modify this value unless you change the view at Fltube_View.cxx file.'
const int SEARCH_PAGE_SIZE = 4;

// Default resolution for video streaming
VCODEC_RESOLUTIONS STREAM_VIDEO_RESOLUTION = R360p;

// Array that holds the search results WIDGETS, in groups of size @SEARCH_PAGE_SIZE...
std::array <VideoInfo*,SEARCH_PAGE_SIZE> video_info_arr{ nullptr, nullptr, nullptr, nullptr };
// Array that holds the search results METADATA, in groups of size @SEARCH_PAGE_SIZE...
std::array <YTDLP_Video_Metadata*,SEARCH_PAGE_SIZE> video_metadata{ nullptr, nullptr, nullptr, nullptr };

const std::string FLTUBE_TEMPORAL_DIR(std::filesystem::temp_directory_path().generic_string() + "/fltube_tmp_files/");

//This FLAG changes over the application usage. If user wants to see all videos of a channel, is set to true (0). Defaults to false.
bool SEARCH_BY_CHANNEL_F = false;

std::string USER_CONFIGFILE_PATH = std::string(getHomePathOr("")) + "/.config/fltube/fltube.conf";

std::string USERDATA_FILE_PATH = std::string(getHomePathOr("")) + "/.local/share/fltube/userdata.txt";

std::string SYSTEM_CONFIGFILE_PATH = "/usr/local/etc/fltube/fltube.conf";

std::string CONFIGFILE_PATH = "";

// Path to fltube resources, like images, sounds, etc. Can be modified at fltube.conf file.
std::string RESOURCES_PATH = "/usr/local/share/fltube/resources";

//If value is 0, enable the debug mode. Defaults to false (or 1).
int DEBUG_ENABLED = 1;

//If value is 0, enable the initial verifications. Defaults to false (or 1).
bool AVOID_INITIAL_CHECKS = 1;

// This variable holds the configured video codec used when download a video.
std::string DOWNLOAD_VIDEO_CODEC;

MediaPlayerInfo* media_player;

ConfigurationManager* config = nullptr;

UserDataManager* userdata = nullptr;

Fl_PNG_Image* live_image = nullptr;

/**
 * Callback for main window close action. By default, exit app with success status code (0).
 */
void exitApp(unsigned short int exitStatusCode = FLT_OK) {
    //Do some cleaning...
    char message[1024];
    snprintf(message, sizeof(message), _("Cleaning temporal files at %s."), FLTUBE_TEMPORAL_DIR.c_str());
    logAtTerminal(std::string(message), LogLevel::INFO);
    std::filesystem::remove_all(FLTUBE_TEMPORAL_DIR);
    if (helpWin != nullptr) delete helpWin;
    if (message_window != nullptr) delete message_window;
    delete mainWin;
    delete userdata;
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

Fl_PNG_Image* load_image(std::string path) {
    if (std::filesystem::exists(path)) {
        return new Fl_PNG_Image(path.c_str());
    } else {
        char message[1024];
        snprintf(message, sizeof(message), _("Resource image at '%s' cannot be loaded..."), path.c_str());
        logAtTerminal(std::string(message), LogLevel::WARN);
    }
    return nullptr;
}

/**
 * Load a PNG image from the resource directory, and return as a Fl_PNG_Image object.
 * If image not exists, a nullptr is returned.
 */
Fl_PNG_Image* load_resource_image(std::string image_filename) {
    std::string resource_image_path = RESOURCES_PATH + "/img/" + image_filename;
    return load_image(resource_image_path);
}

void openURI(const char* uri) {
    char errmsg[512];
    if ( !fl_open_uri(uri, errmsg, sizeof(errmsg)) ) {
        char warnmsg[768];
        sprintf(warnmsg, "Error: %s", errmsg);
        fl_alert("%s", warnmsg);
    }
}

void showFLTubeHelpWindow(Fl_Widget* w) {
    if (helpWin == nullptr) {
        helpWin = new HelpFLTubeWindow(480, 352, _("FLTube Help"));
        helpWin->callback([](Fl_Widget* w) { w->hide(); });
        char version_text[20];
        snprintf(version_text, sizeof(version_text), _("Version %s"), VERSION);
        helpWin->about_version->copy_label(version_text);
        helpWin->hlp_fltube_img->image(load_image("/usr/local/share/icons/fltube/128x128.png"));
        helpWin->hlp_source_code->callback([](Fl_Widget*) {openURI("https://gitlab.com/facuA/fltube");});
        helpWin->hlp_fltk_link->callback([](Fl_Widget*) {openURI("https://www.fltk.org/");});
        helpWin->hlp_ytdlp_link->callback([](Fl_Widget*) {openURI("https://github.com/yt-dlp/yt-dlp");});
        helpWin->hlp_cpp_link->callback([](Fl_Widget*) {openURI("https://isocpp.org/");});
        helpWin->howtouse_txt->buffer(new Fl_Text_Buffer());
        helpWin->howtouse_txt->wrap_mode(Fl_Text_Display::WRAP_AT_COLUMN, 50);
        helpWin->howtouse_txt->buffer()->text(_("FLTube is an application for search & stream Youtube videos. You can search by a search term or by entering the URL of a specific YouTube video.\n\nOnce the results are returned, you can watch any of these videos by clicking on their thumbnail. The stream resolution is 360p by default, but can be changed at fltube.conf configuration file.\n\n"
        "You can also view the latest videos by the creator of a specific video by clicking on their name (a button at the bottom right of each result).\n\nIf a search returns multiple results, they will be paginated, and you can navigate through them using the \"<Previous\" and \"Next>\" buttons at the bottom of the window.\n"));
        helpWin->shortcuts_txt->buffer(new Fl_Text_Buffer());
        helpWin->shortcuts_txt->wrap_mode(Fl_Text_Display::WRAP_AT_COLUMN, 50);
        char help_text_bffr[1024];
        snprintf(help_text_bffr, sizeof(help_text_bffr),
                 _("- Focus at search input: %s\n- Search videos: %s\n"
            "- Get previous results: %s\n- Get next results: %s\n- Open video 1: %s\n- Open video 2: %s\n"
            "- Open video 3: %s\n- Open video 4: %s\n- Search videos of Channel 1: %s\n- Search videos of Channel 2: %s\n"
            "- Search videos of Channel 3: %s\n- Search videos of Channel 4: %s\n- Show this Help Window: %s\n\n"
            "[NOTE] Default shortcuts can be modified at %s.\n"),
            config->getShortcutTextFor(SHORTCUTS::FOCUS_SEARCH).c_str(), "Alt + s | Intro", "Alt + p", "Alt + n",
            config->getShortcutTextFor(SHORTCUTS::FOCUS_VIDEO_1).c_str(), config->getShortcutTextFor(SHORTCUTS::FOCUS_VIDEO_2).c_str(),
            config->getShortcutTextFor(SHORTCUTS::FOCUS_VIDEO_3).c_str(), config->getShortcutTextFor(SHORTCUTS::FOCUS_VIDEO_4).c_str(),
            config->getShortcutTextFor(SHORTCUTS::FOCUS_CHANNEL_1).c_str(), config->getShortcutTextFor(SHORTCUTS::FOCUS_CHANNEL_2).c_str(),
            config->getShortcutTextFor(SHORTCUTS::FOCUS_CHANNEL_3).c_str(), config->getShortcutTextFor(SHORTCUTS::FOCUS_CHANNEL_4).c_str(),
            config->getShortcutTextFor(SHORTCUTS::SHOW_HELP).c_str(), CONFIGFILE_PATH.c_str());
        helpWin->shortcuts_txt->buffer()->text(help_text_bffr);
        helpWin->config_txt->buffer(new Fl_Text_Buffer());
        helpWin->config_txt->wrap_mode(Fl_Text_Display::WRAP_AT_COLUMN, 50);
        snprintf(help_text_bffr, sizeof(help_text_bffr),
                _("The current instance of FLTube is loading the configurations from %s file.\n\n*** EXAMPLES ***\n[*] CHANGE THE DEFAULT STREAM RESOLUTION TO 720p:\n - Uncomment or add this property, and set the desire resolution (240, 360, 480, 720 or 1080).\n      %s = 720\n\n[*] CHANGE THE VIDEO PLAYER TO mpv INSTEAD OF mplayer:\n - Add or uncomment the following properties:\n      %s = mpv\n      %s = --ao=pulse\n      %s = \n The last two params can be empty, but MUST be set uncomment if you change the video player.\n"),
                CONFIGFILE_PATH.c_str(), "STREAM_VIDEO_RESOLUTION", "STREAM_PLAYER_PATH", "STREAM_PLAYER_PARAMS", "STREAM_PLAYER_EXTRA_PARAMS_FOR_LIVE");
        helpWin->config_txt->buffer()->text(help_text_bffr);
        helpWin->authors_txt->buffer(new Fl_Text_Buffer());
        helpWin->authors_txt->wrap_mode(Fl_Text_Display::WRAP_AT_COLUMN, 50);
        snprintf(help_text_bffr, sizeof(help_text_bffr),
                 _("[*_ DEVELOPERS _*]\n  Facundo Gabriel Adorno (Argentina)\n    -Contact: adorno.facundo@gmail.com\n\n"
                     "[*_ CONTRIBUTORS _*]\n  Nicolás Longardi (Uruguay)\n   -Contact: nico@locosporlinux.com\n"));
        helpWin->authors_txt->buffer()->text(help_text_bffr);
    }
    helpWin->show();
    // Loop until the message window is closed...
    while (helpWin->shown()) {
        Fl::wait();
    }
}


//TODO: instead of locking buttons, try to lock main window or open a modal window
//      making notice that you must wait some action to finish..
/**
 * Lock/unlock the following buttons: main search, pagination, preview and search channel's video.
 */
void lock_buttons(bool lock){
    if (lock) {
        mainWin->do_search_bttn->deactivate();
        mainWin->next_results_bttn->deactivate();
        mainWin->previous_results_bttn->deactivate();
        for (int j=0; j < video_info_arr.size(); j++) {
            video_info_arr[j]->thumbnail->deactivate();
            video_info_arr[j]->userUploader->deactivate();
        }
    } else {
        //unlock
        mainWin->do_search_bttn->activate();
        mainWin->next_results_bttn->activate();
        mainWin->previous_results_bttn->activate();
        for (int j=0; j < video_info_arr.size(); j++) {
            video_info_arr[j]->thumbnail->activate();
            video_info_arr[j]->userUploader->activate();
        }
    }
}

/** Callback to preview a video... */
void preview_video_cb(Fl_Button* widget, void* video_url){
    mainWin->cursor(FL_CURSOR_WAIT);
    Fl::check();
    if (! verify_network_connection()) {
        logAtTerminal(_("Your device is offline. Check your internet connection."), LogLevel::WARN);
        showMessageWindow( _("There seems that you don't have access to the Internet. "
        "Please, verify you network connection before proceed..."));
        return;
    }
    std::string* url = static_cast<std::string*>(video_url);
    if (url){
        VideoInfo *vi = static_cast<VideoInfo *>(widget->parent());
        char message[256];
        snprintf(message, sizeof(message), _("Starting streaming preview of video '%s' - (%s)..."), vi->title->label(), url->c_str());
        logAtTerminal(message,LogLevel::INFO);
        stream_video(url->c_str(), vi->is_live_image->visible(), STREAM_VIDEO_RESOLUTION, media_player);
    } else {
        logAtTerminal(_("Cannot get video URL. Review the video metadata enabling app debugging..."), LogLevel::ERROR);
    }
    mainWin->cursor(FL_CURSOR_DEFAULT);
    Fl::check();
}

/**
 * Update the VideoInfo widgets data, based on the data on the video_metadata array.
 */
void update_video_info() {
    YTDLP_Video_Metadata* ytvm = nullptr;
    char text_buffer[128];
    bool is_livestream;
    for (int j=0; j < video_metadata.size(); j++) {
      if (video_metadata[j] != nullptr) {
            is_livestream = (video_metadata[j]->live_status == "is_live");
            video_info_arr[j]->show();
            video_info_arr[j]->title->label(video_metadata[j]->title.c_str());
            video_info_arr[j]->title->tooltip(video_metadata[j]->title.c_str());
            video_info_arr[j]->duration->label(video_metadata[j]->duration.c_str());
            video_info_arr[j]->uploadDate->label(video_metadata[j]->upload_date.c_str());
            video_info_arr[j]->userUploader->label(video_metadata[j]->creators.c_str());
            video_info_arr[j]->userUploader->user_data(static_cast<void*>(&video_metadata[j]->channel_id));
            if (is_livestream) {
                video_info_arr[j]->is_live_image->show();
            } else {
                video_info_arr[j]->is_live_image->hide();
            }
            try {
                snprintf(text_buffer, sizeof(text_buffer), "%s %s", get_metric_abbreviation(std::stoi(video_metadata[j]->viewers_count))->c_str(), (is_livestream) ? _("viewers") : _("views"));
            } catch (const std::invalid_argument& ex) {
                char err_msg[256];
                snprintf(err_msg, sizeof(err_msg), _("Error when getting count of viewers of video: (title: \"%s\", ID: \"%s\")"), video_metadata[j]->title.c_str(), video_metadata[j]->id.c_str());
                logAtTerminal(err_msg, LogLevel::WARN);
                logAtTerminal(ex.what(), LogLevel::DEBUG);
                strcpy(text_buffer, _("Unknown"));
            }
            video_info_arr[j]->views_spectators->copy_label(text_buffer);
            //Setting URL for streaming a video preview...
            video_info_arr[j]->thumbnail->user_data(static_cast<void*>(&video_metadata[j]->url));

            //Download, resize and update the video thumbnail image...
            std::string thumbn_url = video_metadata[j]->thumbnail_url.substr(0, video_metadata[j]->thumbnail_url.find("?"));
            std::string thumbn_name = "th_" + video_metadata[j]->id + ".jpg";
            if (download_file(thumbn_url, FLTUBE_TEMPORAL_DIR, thumbn_name) != FLT_DOWNLOAD_FL_FAILED) {
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
                //If thumbnail cannot be downloaded, then load a generic one...
                Fl_PNG_Image* generic_thumbnail = load_resource_image("no_thumbnail_available.png");
                video_info_arr[j]->thumbnail->image(generic_thumbnail);
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
    ///// LOAD CONFIGURATIONS  //////
    // If not specified custom configuration file through parameter "--config"...
    if (CONFIGFILE_PATH.empty())
        CONFIGFILE_PATH = (std::filesystem::exists(USER_CONFIGFILE_PATH)) ? USER_CONFIGFILE_PATH : SYSTEM_CONFIGFILE_PATH;
    logAtTerminal(_("Loading configurations from ") + CONFIGFILE_PATH, LogLevel::DEBUG);
    config = new ConfigurationManager(CONFIGFILE_PATH.c_str());
    //TODO la ruta tiene que ser otra, ~/.local/share/fltube/userdata.txt
    userdata = new UserDataManager(USERDATA_FILE_PATH, getIntVersion());
    AVOID_INITIAL_CHECKS = config->getBoolProperty("AVOID_INITIAL_VERIFICATIONS", false);

    if ( !AVOID_INITIAL_CHECKS && !isInstalledYTDLP() ) {
        showMessageWindow(_("yt-dlp is not installed on your system or its binary is not at $PATH system variable. See how to install it at https://github.com/yt-dlp/yt-dlp. Or run in a terminal the 'install_yt-dlp' script."));
        logAtTerminal(_("yt-dlp is not installed. Closing app...\n"), LogLevel::ERROR);
        exitApp(FLT_GENERAL_FAILED);
    }
    //Init Localization. Use locale path specified at config, or custom config default_locale_path().
    setup_gettext("", config->getProperty("LOCALE_PATH", default_locale_path().c_str()));

    media_player = new MediaPlayerInfo();
    if(config->existProperty("STREAM_PLAYER_PATH")) {
        media_player->binary_path = config->getProperty("STREAM_PLAYER_PATH", "");
        media_player->parameters = config->getProperty("STREAM_PLAYER_PARAMS", "");
        media_player->extra_live_parameters = config->getProperty("STREAM_PLAYER_EXTRA_PARAMS_FOR_LIVE", "");
    } else {
        media_player->binary_path = DEFAULT_STREAM_PLAYER;
        media_player->parameters = DEFAULT_PLAYER_PARAMS;
        media_player->extra_live_parameters = DEFAULT_PLAYER_EXTRAPARAMS_LIVE;
    }
    if(config->existProperty("RESOURCES_PATH")) {
        RESOURCES_PATH = config->getProperty("RESOURCES_PATH", RESOURCES_PATH.c_str());
    }
    if (config->existProperty("STREAM_VIDEO_RESOLUTION")) {
        int pretended_resolution = config->getIntProperty("STREAM_VIDEO_RESOLUTION", DEFAULT_STREAM_VIDEO_RESOLUTION);
        std::vector<VCODEC_RESOLUTIONS> resolutions = {R240p, R360p, R480p, R720p, R1080p};
        for (auto res: resolutions){
            if (res == pretended_resolution) {
                STREAM_VIDEO_RESOLUTION = res;
                break;
            }
        }
        if (STREAM_VIDEO_RESOLUTION != DEFAULT_STREAM_VIDEO_RESOLUTION)
            logAtTerminal(_("Default streaming resolution (360p) changed at configuration to this new resolution: ") + std::to_string(STREAM_VIDEO_RESOLUTION), LogLevel::DEBUG);
    }
    live_image = load_resource_image("livebutton_18p.png");

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
        video_info_arr[i]->thumbnail->shortcut(config->getShortcutFor("FOCUS_VIDEO_" + std::to_string(i + 1)));
        video_info_arr[i]->userUploader->shortcut(config->getShortcutFor("FOCUS_CHANNEL_" + std::to_string(i + 1)));
        mainWin->add(video_info_arr[i]);
    }

    for (VideoInfo* videoInfo: video_info_arr) {
        videoInfo->hide();
    }

    mainWin->about_bttn->callback((Fl_Callback*)showFLTubeHelpWindow);
    mainWin->about_bttn->shortcut(config->getShortcutFor(SHORTCUTS::SHOW_HELP));

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
    video_info->userUploader->callback((Fl_Callback*)getYTChannelVideo_cb);
    if (live_image != nullptr) video_info->is_live_image->image(live_image);
    return video_info;
}

/**
 * Search by YT URL or search term. Or if "is_a_channel" is set, then return videos from channel URL specified at "input_text".
 */
void doSearch(const char* input_text, bool is_a_channel) {
    //Change cursor to wait symbol, to indicate that the search is in process...
    mainWin->cursor(FL_CURSOR_WAIT);
    Fl::check();
    // Check if there is Internet connectivity before do a search...
    if (! verify_network_connection()) {
        logAtTerminal(_("Your device is offline. Check your internet connection."), LogLevel::WARN);
        showMessageWindow( _("There seems that you don't have access to the Internet. "
                            "Please, verify you network connection before proceed..."));
        return;
    }

    char message[1024];
    snprintf(message, sizeof(message), _("Searching for results for '%s' user input..."), input_text);
    logAtTerminal(std::string(message), LogLevel::DEBUG);
    std::string result;
    if (isUrl(input_text) && !SEARCH_BY_CHANNEL_F) {
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
        result = do_ytdlp_search(input_text, YOUTUBE_EXTRACTOR_NAME.c_str(), page_i, SEARCH_BY_CHANNEL_F);
    }
    logAtTerminal(result, LogLevel::DEBUG);
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
    //Restore cursor to default when search is done.
    mainWin->cursor(FL_CURSOR_DEFAULT);
    Fl::check();
}

/*
 *
 */
void getYTChannelVideo_cb(Fl_Button* bttn, void* channel_id_str){
    if (bttn == nullptr) {
        logAtTerminal(_("The Fl_Button is inexistent when search videos for a channel!!!\n"), LogLevel::ERROR);
        return;
    }
    std::string* channel_id =  static_cast<std::string*>(channel_id_str);
    printf("CHANNEL ID: %s", channel_id->c_str());
    if (channel_id == nullptr || channel_id->empty()) {
        logAtTerminal(_("Channel ID is empty. Please verify the video metadata initilization!!!\n"), LogLevel::ERROR);
        return;
    }
    SEARCH_PAGE_INDEX = 0;
    SEARCH_BY_CHANNEL_F = true;
    char channel_videos_URL[256];
    snprintf(channel_videos_URL, sizeof(channel_videos_URL), "https://www.youtube.com/channel/%s/videos", channel_id->c_str());
    mainWin->search_term_or_url->value(channel_videos_URL);
    lock_buttons(true);
    doSearch(channel_videos_URL, true);
    lock_buttons(false);
    mainWin->previous_results_bttn->deactivate();
}


/*
 * Extract and return the search term value from the search input. Returns nullptr only if text if empty or some error happens...
 */
const char* getSearchValue(Fl_Input *input){
    if (input == nullptr) {
        logAtTerminal(_("The Fl_Input search widget inexistent!!!\n"), LogLevel::ERROR);
        return nullptr;
    }

    const char* input_text = input->value();
    if (input_text == nullptr || input_text[0] == '\0') {
        showMessageWindow(_("Search input is empty. Please, retry and enter a valid text."));
        return nullptr;
    }
    return input_text;
}

/*
 * Callback for search button.
 */
void searchButtonAction_cb(Fl_Widget *wdgt, Fl_Input *input){
    //Reset global pagination index and "deactivate" previos button...
    SEARCH_PAGE_INDEX = 0;
    SEARCH_BY_CHANNEL_F = false;
    const char* input_text = getSearchValue(input);
    if (input_text != nullptr)  {
        lock_buttons(true);
        doSearch(input_text);
        lock_buttons(false);
    }
    mainWin->previous_results_bttn->deactivate();
}

/** Callback for Previous results button... */
void getPreviousSearchResults_cb(Fl_Widget* widget, Fl_Input *input){
    if (SEARCH_PAGE_INDEX>0) {
        SEARCH_PAGE_INDEX--;
    }
    const char* input_text = getSearchValue(input);
    if (input_text != nullptr)  {
        lock_buttons(true);
        doSearch(input_text);
        lock_buttons(false);
    }
    if (SEARCH_PAGE_INDEX == 0) {
        mainWin->previous_results_bttn->deactivate();
    }
}
/** Callback for Next results button.. */
void getNextSearchResults_cb(Fl_Widget* widget, Fl_Input *input){
    //TODO: what to do if there is no more results? It must be controlled in some way...
    SEARCH_PAGE_INDEX++;
    const char* input_text = getSearchValue(input);
    if (input_text != nullptr) {
        lock_buttons(true);
        doSearch(input_text);
        lock_buttons(false);
    }
    mainWin->previous_results_bttn->activate();
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
                              a copy of the original fltube.conf. Use this option when you want to apply a
                              development configuration file for development purposes.
                              By default, the configuration file is searched for in the following locations,
                              in this order: '~/.config/fltube/' and '/usr/local/etc/fltube'.
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
    if (existsCmdOption(argc, argv, "--config")){
        const std::string path = getOptionValue(argc, argv, "--config");
        if (path != "" && std::filesystem::exists(path)){
            CONFIGFILE_PATH = path;
            printf(_("Loading custom config file %s.\n"), CONFIGFILE_PATH.c_str());
        } else {
            if (!std::filesystem::exists(path)) {
                printf(_("Specified path at --config parameter does not exist.\n"));
            } else {
                printf(_("--config parameter with no path specified.\n"));
            }
            exit(FLT_INVALID_CMD_PARAM);
        }
    }
    if (existsCmdOption(argc, argv, "-d") || existsCmdOption(argc, argv, "--debug")) {
        printf(_("DEBUG MODE enabled.\n"));
        DEBUG_ENABLED = 0;
    }
}

// Returns the current version of this software expressed as an Integer (for example: 2.1.4 is expressed as 214).
int getIntVersion() {
    std::string version(VERSION);
    replace_all(version, ".", "");
    try {
        return std::stoi(version);
    } catch (const std::invalid_argument& e) {
        char message[256];
        snprintf(message, sizeof(message), "Error on VERSION number format (%s). The version must be expressed as numbers separated by dots.\n", VERSION);
        logAtTerminal(message, LogLevel::ERROR);
        return -100000;
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
    mainWin = new FLTubeMainWindow(593, 540, strcat(win_title, VERSION));
    mainWin->callback((Fl_Callback*)exitApp);
    mainWin->search_term_or_url->callback((Fl_Callback*)searchButtonAction_cb, (void*)(mainWin->search_term_or_url));
    mainWin->search_term_or_url->when(FL_WHEN_ENTER_KEY);
    mainWin->search_term_or_url->shortcut(config->getShortcutFor(SHORTCUTS::FOCUS_SEARCH));
    mainWin->do_search_bttn->callback((Fl_Callback*)searchButtonAction_cb, (void*)(mainWin->search_term_or_url));
    mainWin->previous_results_bttn->callback((Fl_Callback*)getPreviousSearchResults_cb, (void*)(mainWin->search_term_or_url));
    mainWin->previous_results_bttn->deactivate();
    mainWin->next_results_bttn->callback((Fl_Callback*)getNextSearchResults_cb, (void*)(mainWin->search_term_or_url));
    mainWin->next_results_bttn->deactivate();

    post_init();
    //mainWin->show(argc, argv);    //TODO Avoid passing parameters to the mainWin function to prevent undesirable logs.
    mainWin->icon(load_image("/usr/local/share/icons/fltube/64x64.png"));
    mainWin->show();
    logAtTerminal(_("The FLTube main window has loaded."), LogLevel::DEBUG);
    return Fl::run();

}
