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

#include "../include/gnugettext_utils.h"
#include "../include/FLTube.h"
#include "../include/FLTube_View.h"
#include "../include/fltube_utils.h"
#include "../include/ytdlp_helper.h"
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

// Pointer to the video currently selected for stream...
VideoInfo* video_selected_for_stream = nullptr;

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

//If value is true, enable the debug mode. Defaults to false.
bool DEBUG_ENABLED = false;

//If value is 0, enable the initial verifications. Defaults to false (or 1).
bool AVOID_INITIAL_CHECKS = 1;

// This variable holds the configured video codec used when download a video.
std::string DOWNLOAD_VIDEO_CODEC;

const std::string TAB_SEARCH_NAME = "SEARCHVIDEOS_TAB";
const std::string TAB_VIDEOLIST_NAME = "VIDEOLISTS_TABS";

MediaPlayerInfo* media_player;

ConfigurationManager* config = nullptr;

UserDataManager* userdata = nullptr;

YtDlp_Helper* ytdlp = nullptr;

Fl_PNG_Image* live_image = nullptr;

Fl_PNG_Image* already_viewed_image = nullptr;

Fl_PNG_Image* like_icon_image = nullptr;
Fl_PNG_Image* like_red_icon_image = nullptr;

Fl_PNG_Image* playicon_image = nullptr;

/* Keep the current displayed cursor. FLTK doesn't have a way to know this. */
Fl_Cursor current_displayed_cursor = FL_CURSOR_DEFAULT;

std::shared_ptr<TerminalLogger> logger;

std::atomic<bool> ytdlp_action_in_progress(false);

/**
 * Callback for main window close action. By default, exit app with success status code (0).
 */
void exitApp(unsigned short int exitStatusCode = FLT_OK) {
    if (ytdlp_action_in_progress) {
        logger->info(_("The main window was closed by user demand when a video was in streaming."));
    }
    //Do some cleaning...
    char message[1024];
    snprintf(message, sizeof(message), _("Cleaning temporal files at %s."), FLTUBE_TEMPORAL_DIR.c_str());
    logger->info(std::string(message));
    std::filesystem::remove_all(FLTUBE_TEMPORAL_DIR);
    if (helpWin != nullptr) delete helpWin;
    if (message_window != nullptr) delete message_window;
    delete userdata;
    delete mainWin;
    //Exiting the app...
    logger->info(_("Closing FLtube... Bye!\n"));
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
        logger->warn(std::string(message));
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

/* Change the current window cursor to any valid Fl_Cursor. If no parameter passed, change to FL_CURSOR_DEFAULT. */
void change_cursor(Fl_Cursor new_cursor = FL_CURSOR_DEFAULT) {
    if (current_displayed_cursor != new_cursor) {
        mainWin->cursor(new_cursor);
        current_displayed_cursor = new_cursor;
        Fl::check();
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
        "You can also view the latest videos by the creator of a specific video by clicking on their name (a button at the bottom right of each result).\n\nIf a search returns multiple results, they will be paginated, and you can navigate through them using the \"<Previous\" and \"Next>\" buttons at the bottom of the window.\n\n"
        "Every time you watch a video, it is added to an existing list called \"Navigation History\". This list can be consulted using the user interface at the \"My Lists\" tab.\n\nBesides, you can mark a video as \"Liked\" pressing a button with heart shape. A video list called \"Liked\" will be generated to in order to consult it in the future.\n"));
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
                     "[*_ CONTRIBUTORS _*]\n  Nicolas Longardi (Uruguay)\n   -Contact: nico@locosporlinux.com\n"));
        helpWin->authors_txt->buffer()->text(help_text_bffr);
    }
    helpWin->show();
    // Loop until the message window is closed...
    while (helpWin->shown()) {
        Fl::wait();
    }
}


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
            video_info_arr[j]->like_icon_bttn->deactivate();
        }
    } else {
        //unlock
        mainWin->do_search_bttn->activate();
        mainWin->next_results_bttn->activate();
        mainWin->previous_results_bttn->activate();
        for (int j=0; j < video_info_arr.size(); j++) {
            video_info_arr[j]->thumbnail->activate();
            video_info_arr[j]->userUploader->activate();
            video_info_arr[j]->like_icon_bttn->activate();
        }
    }
    Fl::check();
}

/** Callback to preview a video... */
void preview_video_cb(Fl_Button* widget, void* video_url){
    if (ytdlp_action_in_progress)
        return;
    if (! verify_network_connection()) {
        logger->warn(_("Your device is offline. Check your internet connection."));
        showMessageWindow( _("There seems that you don't have access to the Internet. "
        "Please, verify you network connection before proceed..."));
        return;
    }
    std::string* url = static_cast<std::string*>(video_url);
    if (url){
        VideoInfo *vi = static_cast<VideoInfo *>(widget->parent());
        video_selected_for_stream = vi;
        video_selected_for_stream->thumbnail_overlay->image(playicon_image);
        video_selected_for_stream->thumbnail_overlay->show();
        //Registering view of current video at History List...
        std::string id = *url;
        replace_all(id, std::string(YOUTUBE_URL_PREFIX), "");
        for (YTDLP_Video_Metadata* ytv : video_metadata) {
            if (ytv->id == id) {
                Video* v = new Video(id, ytv->title, ytv->creators, ytv->channel_id, ytv->viewers_count, ytv->duration, ytv->thumbnail_url);
                userdata->addVideo(v, UserDataManager::HISTORY_LIST_NAME);
                break;
            }
        }
        //Stream video section...
        char message[256];
        snprintf(message, sizeof(message), _("Starting streaming preview of video '%s' - (%s)..."), vi->title->label(), url->c_str());
        logger->info(message);
        auto stream_lambda = [&](std::string* url, bool is_a_live, VCODEC_RESOLUTIONS v_resolution, const MediaPlayerInfo* mp, bool use_alternative_method) {
            ytdlp_action_in_progress = true;
            //TODO 'ytdlp' variable must be protected when using in other thread????????
            ytdlp->is_live(is_a_live);
            ytdlp->stream(url->c_str());
            ytdlp_action_in_progress = false;

        };
        // TODO: in the future, a ThreadPool of one or more threads could be implemented for optimization. More info at https://www.geeksforgeeks.org/cpp/thread-pool-in-cpp/.
        std::thread worker(stream_lambda, url, vi->is_live_image->visible(), STREAM_VIDEO_RESOLUTION, media_player, (STREAM_VIDEO_RESOLUTION != VCODEC_RESOLUTIONS::R360p));

        worker.detach();
    } else {
        logger->error(_("Cannot get video URL. Review the video metadata enabling app debugging..."));
    }
}

/**
 * Hide VideoInfo widgets.
 */
void clear_video_info() {
    for (int j=0; j < video_info_arr.size(); j++) {
        video_info_arr[j]->hide();
    }
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
            // Update History icon...
            if (userdata->getHistoryList()->findVideoById(video_metadata[j]->id) != nullptr) {
                video_info_arr[j]->already_viewed_icon->show();
            } else {
                video_info_arr[j]->already_viewed_icon->hide();
            }
            // Update Liked icon...
            if (userdata->getLikedVideosList()->existAtList(video_metadata[j]->id)) {
                video_info_arr[j]->like_icon_bttn->image(like_red_icon_image);
            } else {
                video_info_arr[j]->like_icon_bttn->image(like_icon_image);
            }
            video_info_arr[j]->like_icon_bttn->redraw();
            try {
                snprintf(text_buffer, sizeof(text_buffer), "%s %s", YtDlp_Helper::get_metric_abbreviation(std::stoi(video_metadata[j]->viewers_count))->c_str(), (is_livestream) ? _("viewers") : _("views"));
            } catch (const std::invalid_argument& ex) {
                char err_msg[256];
                snprintf(err_msg, sizeof(err_msg), _("Error when getting count of viewers of video: (title: \"%s\", ID: \"%s\")"), video_metadata[j]->title.c_str(), video_metadata[j]->id.c_str());
                logger->warn(err_msg);
                logger->debug(ex.what());
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
                    logger->error(_("Something went wrong when generating a resize thumbnail for video with ID=") + video_metadata[j]->id);
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
 * Update the elements in the video_metadata array using as input the videos in the selected list at @TAB_VIDEOLIST_NAME section.
 * This must be called for show the videos in a saved list (History, Liked, etc...).
 * Returns @true if there is more results to process at video list. Returns @false in otherwise.
 */
bool updateVideoMetadataFromVideoList() {
    Fl::check();
    std::string selected_list = mainWin->videolist_selector->mvalue()->label();
    VideoList* vlist = userdata->getVideoList(selected_list);
    int next_video_position = (SEARCH_PAGE_INDEX * SEARCH_PAGE_SIZE);
    for (int i=0; i < SEARCH_PAGE_SIZE; i++) {
        if (next_video_position < vlist->getLength()) {
            video_metadata[i] = vlist->toYTDLPVideo(vlist->getVideoAt(next_video_position));
        } else {
            video_metadata[i] = nullptr;
        }
        next_video_position++;
    }

    update_video_info();

    if (next_video_position < vlist->getLength())
        mainWin->next_results_bttn->activate();
    if (vlist->getLength() < next_video_position + 1){
        mainWin->next_results_bttn->deactivate();     // If the list doesn't have more elements to show, deactivate next button...'
        return false;
    }

    return true;
}

void getVideosAtList_cb(Fl_Choice* w, void* a){
    SEARCH_PAGE_INDEX = 0;
    mainWin->previous_results_bttn->deactivate();
    updateVideoMetadataFromVideoList();
};

void selectCentralTab_cb(Fl_Choice* w, void* a){
    char* tabname = static_cast<char*>(mainWin->central_tabs->value()->user_data());
    if (tabname == TAB_VIDEOLIST_NAME) {
        mainWin->videolist_selector->value(mainWin->videolist_selector->find_index(UserDataManager::HISTORY_LIST_NAME.c_str()));
        mainWin->videolist_selector->set_changed();
        mainWin->videolist_selector->do_callback();
    }
    if (tabname == TAB_SEARCH_NAME) {
        mainWin->search_term_or_url->value("");
        mainWin->previous_results_bttn->deactivate();
        mainWin->next_results_bttn->deactivate();
        clear_video_info();
    }
}

/*
 * Return the name of the current active tab at @central_tabs widget.
 */
std::string getActiveTabName() {
    char* tabname = static_cast<char*>(mainWin->central_tabs->value()->user_data());
    return std::string(tabname);
}

void markLikedVideo_cb(Fl_Widget *wdg) {
    VideoInfo* vi = static_cast<VideoInfo*>(wdg->parent());
    //Registering view of current video at History List...
    std::string id = *static_cast<std::string*>(vi->thumbnail->user_data());
    replace_all(id, std::string(YOUTUBE_URL_PREFIX), "");
    for (YTDLP_Video_Metadata* ytv : video_metadata) {
        if (ytv->id == id) {
            if ( userdata->getLikedVideosList()->existAtList(ytv->id)) {
                userdata->removeVideoFromList(ytv->id, UserDataManager::LIKED_LIST_NAME);
                vi->like_icon_bttn->image(like_icon_image);
            } else {
                Video* v = new Video(id, ytv->title, ytv->creators, ytv->channel_id, ytv->viewers_count, ytv->duration, ytv->thumbnail_url);
                userdata->addVideo(v, UserDataManager::LIKED_LIST_NAME);
                vi->like_icon_bttn->image(like_red_icon_image);
            }
            break;
        }
    }
}

/**
 * Hook: Actions to execute before main window is drawn...
 */
void pre_init() {
    logger = std::make_shared<TerminalLogger>(DEBUG_ENABLED);
    ///// LOAD CONFIGURATIONS  //////
    // If not specified custom configuration file through parameter "--config"...
    if (CONFIGFILE_PATH.empty())
        CONFIGFILE_PATH = (std::filesystem::exists(USER_CONFIGFILE_PATH)) ? USER_CONFIGFILE_PATH : SYSTEM_CONFIGFILE_PATH;
    logger->debug(_("Loading configurations from ") + CONFIGFILE_PATH);
    config = new ConfigurationManager(CONFIGFILE_PATH.c_str(), logger);
    userdata = new UserDataManager(USERDATA_FILE_PATH, getIntVersion(), logger);
    AVOID_INITIAL_CHECKS = config->getBoolProperty("AVOID_INITIAL_VERIFICATIONS", false);

    if ( !AVOID_INITIAL_CHECKS && !isInstalledYTDLP() ) {
        showMessageWindow(_("yt-dlp is not installed on your system or its binary is not at $PATH system variable. See how to install it at https://github.com/yt-dlp/yt-dlp. Or run in a terminal the 'install_yt-dlp' script."));
        logger->error(_("yt-dlp is not installed. Closing app...\n"));
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
            logger->debug(_("Default streaming resolution (360p) changed at configuration to this new resolution: ") + std::to_string(STREAM_VIDEO_RESOLUTION));
    }

    live_image = load_resource_image("livebutton_18p.png");
    already_viewed_image = load_resource_image("clock_18p.png");
    like_icon_image = load_resource_image("heart_18p.png");
    like_red_icon_image = load_resource_image("heart_18p_red.png");
    playicon_image = load_resource_image("playicon.png");

    //Create temporal directory and change current working directory to that dir.
    std::filesystem::create_directory(FLTUBE_TEMPORAL_DIR);
    std::filesystem::current_path(FLTUBE_TEMPORAL_DIR);

    bool enable_alt_stream = config->getBoolProperty("ENABLE_ALTERNATIVE_STREAM_METHOD", true);
    ytdlp = new YtDlp_Helper(STREAM_VIDEO_RESOLUTION, media_player, enable_alt_stream, logger, FLTUBE_TEMPORAL_DIR);

    // Add a custom FLTK event dispatcher
    Fl::event_dispatch([](int event, Fl_Window* w) -> int{
        if (ytdlp_action_in_progress) {
            // If streaming is in progress, then turn cursor to default if changed...
            if (current_displayed_cursor != FL_CURSOR_WAIT) lock_buttons(true);
            change_cursor(FL_CURSOR_WAIT);
            switch (event) {
                case FL_PUSH:
                case FL_RELEASE:
                case FL_DRAG:
                case FL_KEYDOWN:
                case FL_KEYUP:
                case FL_MOUSEWHEEL:
                    return 1;       // Ignore this events when streaming in course...
            }
        } else {
            if (current_displayed_cursor != FL_CURSOR_DEFAULT) {
                lock_buttons(false);
                if (video_selected_for_stream->thumbnail_overlay->visible()) {
                    video_selected_for_stream->thumbnail_overlay->image(nullptr);
                    video_selected_for_stream->thumbnail_overlay->hide();
                    video_selected_for_stream = nullptr;
                }
            }
            change_cursor();
        }
        return Fl::handle_(event, w);      // Otherwise, let default FLTK Handler handle the event...
    });

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

    std::vector<std::string> videoLists = userdata->getVideoListNames();
    std::sort(videoLists.begin(),videoLists.end()); //Sort lists alphabetically
    for (int i=0; i < videoLists.size() ; i++) {
        mainWin->videolist_selector->add(videoLists.at(i).c_str());
    }

    mainWin->videolist_selector->value(mainWin->videolist_selector->find_index(UserDataManager::HISTORY_LIST_NAME.c_str()));
    mainWin->videolist_selector->when(FL_WHEN_CHANGED);
    mainWin->videolist_selector->callback((Fl_Callback*)getVideosAtList_cb);

    mainWin->central_tabs->when(FL_WHEN_RELEASE_ALWAYS);
    mainWin->central_tabs->callback((Fl_Callback*)selectCentralTab_cb);

    // Redraw the window to show the new button
    mainWin->redraw();
}

/**
 * Create a new group to view video info.
 */
VideoInfo* create_video_group(int posx, int posy) {
    VideoInfo *video_info = new VideoInfo (posx, posy, 600, 90, "");
    video_info->thumbnail->callback((Fl_Callback*)preview_video_cb);
    video_info->userUploader->callback((Fl_Callback*)getYTChannelVideo_cb);
    if (live_image != nullptr) video_info->is_live_image->image(live_image);
    if (already_viewed_image != nullptr) video_info->already_viewed_icon->image(already_viewed_image);
    if (like_icon_image != nullptr) video_info->like_icon_bttn->image(like_icon_image);
    video_info->like_icon_bttn->callback((Fl_Callback*)markLikedVideo_cb);
    return video_info;
}

/**
 * Search by YT URL or search term. Or if "is_a_channel" is set, then return videos from channel URL specified at "input_text".
 */
void doSearch(const char* input_text) {
    //Change cursor to wait symbol, to indicate that the search is in process...
    ytdlp_action_in_progress = true;
    change_cursor(FL_CURSOR_WAIT);
    // Check if there is Internet connectivity before do a search...
    if (! verify_network_connection()) {
        logger->warn(_("Your device is offline. Check your internet connection."));
        showMessageWindow( _("There seems that you don't have access to the Internet. "
                            "Please, verify you network connection before proceed..."));
        return;
    }

    char message[1024];
    snprintf(message, sizeof(message), _("Searching for results for '%s' user input..."), input_text);
    logger->debug(std::string(message));
    std::string result;
    Pagination_Info page_i;
    if (isUrl(input_text) && !SEARCH_BY_CHANNEL_F) {
        if(!YtDlp_Helper::isYoutubeURL(input_text)){
            std::string warn_message = _("For now, only Youtube URL's are valid for download. Please, edit your input text or search using a generic term.");
            showMessageWindow(warn_message.c_str());
            logger->warn(warn_message);
            return;
        }
        ytdlp->set_search_type(SEARCH_BY_TYPE::VIDEO_URL);
        page_i = Pagination_Info();

    } else {
        page_i = Pagination_Info(SEARCH_PAGE_SIZE, SEARCH_PAGE_INDEX);
        if (SEARCH_PAGE_INDEX == 0) {
            mainWin->next_results_bttn->activate();
        }
        ytdlp->set_search_type( (SEARCH_BY_CHANNEL_F) ? SEARCH_BY_TYPE::CHANNEL_URL : SEARCH_BY_TYPE::TERM);
    }
    result = ytdlp->search(input_text, page_i);
    logger->debug(result);
    // Read input lines until an empty line is encountered
    std::istringstream result_sstream(result);
    std::string line;
    for (int i=0; i < SEARCH_PAGE_SIZE; i++) {
        getline(result_sstream, line);
        if (!line.empty()) {
            //Set the video metadata at the array...
            video_metadata[i] = YtDlp_Helper::parse_metadata(line.c_str());
        } else if(video_metadata[i] != nullptr){
            //Or release the unused pointers to free memory...
            delete video_metadata[i];
            video_metadata[i] = nullptr;
        }
    }
    update_video_info();
    //Restore cursor to default when search is done.
    ytdlp_action_in_progress = false;
    change_cursor();
}

/*
 *  Callback to get videos from an specific Youtube channel.
 */
void getYTChannelVideo_cb(Fl_Button* bttn, void* channel_id_str){
    if (bttn == nullptr) {
        logger->error(_("The Fl_Button is inexistent when search videos for a channel!!!\n"));
        return;
    }
    std::string* channel_id =  static_cast<std::string*>(channel_id_str);
    printf("CHANNEL ID: %s", channel_id->c_str());
    if (channel_id == nullptr || channel_id->empty()) {
        logger->error(_("Channel ID is empty. Please verify the video metadata initilization!!!\n"));
        return;
    }
    if (getActiveTabName() == TAB_VIDEOLIST_NAME) {
        // If "My Lists" tab is open, then change to the main searchbox tab...
        mainWin->central_tabs->value(mainWin->searchbox_tab);
    }
    SEARCH_PAGE_INDEX = 0;
    SEARCH_BY_CHANNEL_F = true;
    char channel_videos_URL[256];
    snprintf(channel_videos_URL, sizeof(channel_videos_URL), "https://www.youtube.com/channel/%s/videos", channel_id->c_str());
    mainWin->search_term_or_url->value(channel_videos_URL);
    lock_buttons(true);
    doSearch(channel_videos_URL);
    lock_buttons(false);
    mainWin->previous_results_bttn->deactivate();
}


/*
 * Extract and return the search term value from the search input. Returns nullptr only if text if empty or some error happens...
 */
const char* getSearchValue(Fl_Input *input){
    if (input == nullptr) {
        logger->error(_("The Fl_Input search widget inexistent!!!\n"));
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
    if ( getActiveTabName() == TAB_SEARCH_NAME ) {
        const char* input_text = getSearchValue(input);
        if (input_text != nullptr)  {
            lock_buttons(true);
            doSearch(input_text);
            lock_buttons(false);
        }
    }
    if (getActiveTabName() == TAB_VIDEOLIST_NAME) {
        lock_buttons(true);
        updateVideoMetadataFromVideoList();
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
    if (getActiveTabName() == TAB_SEARCH_NAME) {
        const char* input_text = getSearchValue(input);
        if (input_text != nullptr) {
            lock_buttons(true);
            doSearch(input_text);
            lock_buttons(false);
        }
    }
    if (getActiveTabName() == TAB_VIDEOLIST_NAME) {
        lock_buttons(true);
        bool areMoreVideos = updateVideoMetadataFromVideoList();
        lock_buttons(false);
        if (!areMoreVideos) mainWin->next_results_bttn->deactivate();
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
        DEBUG_ENABLED = true;
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
        logger->error(message);
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
    logger->info(message);

    char win_title[30] = "FLTube ";
    mainWin = new FLTubeMainWindow(593, 540, strcat(win_title, VERSION));
    mainWin->callback((Fl_Callback*)exitApp);
    mainWin->search_term_or_url->when(FL_WHEN_ENTER_KEY);
    mainWin->search_term_or_url->callback((Fl_Callback*)searchButtonAction_cb, (void*)(mainWin->search_term_or_url));
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
    logger->debug(_("The FLTube main window has loaded."));
    return Fl::run();

}
