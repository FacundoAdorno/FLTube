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

#ifndef USERDATA_MANAGER_H
#define USERDATA_MANAGER_H


#include <map>
#include <vector>
#include <memory>
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include "../include/fltube_utils.h"


//TODO Delete this struct and use in its place the YTDLP_Video_Metadata struct...
struct Video {
    std::string id;
    std::string title;
    std::string creator;
    std::string channel_id;
    std::string views;
    std::string duration;
    std::string thumbnail_url;

    //Empty constructor
    Video() {}

    Video(std::string id, std::string title, std::string creator, std::string channel_id, std::string views,
          std::string duration, std::string thumbnail_url):
          id(id), title(title), creator(creator), channel_id(channel_id), views(views),
          duration(duration), thumbnail_url(thumbnail_url) {}
};

// A VideoList is only a view of videos in a List, cannot be modified directly.
class VideoList {
    protected:
        std::string name;
        bool canBeManiputaledByUser;    //History cannot be directly manipulated by user, except removing a video.
        std::unique_ptr<std::vector<Video*>> list;
    public:
        VideoList(std::string name, bool canBeManipulated);

        int getLength();

        std::string getName();

        Video* getVideoAt(int position);

        // Search a video by its id. If does not exists at this list, a nullptr is returned.
        Video* findVideoById(std::string id);

        // Returns true if the video ID exists in the list, false otherwise.
        bool existAtList(std::string id);

        //Print on terminal all videos from begining of videolist.
        void printElementsOnTerminal() {
            for (auto it=list->begin(); it != list->end() ; it++ ) {
                Video* v = static_cast<Video*>(*it);
                std::cout << "Video: " << v->id << "\n";
            }
        }

        //Determine if user can change the content of the list manually or not (for example, History cannot be directly manipulated)...
        bool isChangeable();

        // Return a list with YTDLP_Video_Metadata objects.
        std::vector<YTDLP_Video_Metadata*> getYTDLPVideoList() {
            std::vector<YTDLP_Video_Metadata*> result = {};
            for (int i = 0; i < list->size() ; ++i) {

                Video* v = list->at(i);
                result.push_back(toYTDLPVideo(v));
            }
            return result;
        };

        YTDLP_Video_Metadata* toYTDLPVideo(Video* v) {
            YTDLP_Video_Metadata* ytdlp_v = new YTDLP_Video_Metadata();
            ytdlp_v->id = v->id;
            ytdlp_v->title = v->title;
            ytdlp_v->creators = v->creator;
            ytdlp_v->channel_id = v->channel_id;
            ytdlp_v->viewers_count = v->views;
            ytdlp_v->duration = v->duration;
            ytdlp_v->thumbnail_url = v->thumbnail_url;
            ytdlp_v->url = YOUTUBE_URL_PREFIX + v->id;
            ytdlp_v->upload_date = "-";
            ytdlp_v->live_status = "-";
            return ytdlp_v;
        }
};

// A InternalVideoList can be modified directly (add, remove), only must be used by UserDataManager.class.
class InternalVideoList: public VideoList {
    public:
        InternalVideoList(std::string name, bool canBeManipulated)
            :VideoList(name, canBeManipulated) {};
        void addVideo(Video* v);
        void removeVideo(std::string id);
};

/**
 *  Clase diseñada para guardar y leer diversa información sobre los videos con los que el
 * usuario interactúa: historial, listas personalizadas, etc. Esta información se persiste
 * en el directorio de usuario en un archivo de texto plano.
 *
 *  La primer línea determina la versión de FLTube en la que estos datos fueron persistidos
 * (por ejemplo: 203 representa a la versión 2.0.3 de FLTube).
 * *
 *  ===VIDEOS=== [separador]
 *  Las siguientes líneas representan la información de los videos almacenados.
 *  Formato de cada línea:
 *      id>title>creator>channel_id>views>duration>thumbnail_url
 *
 *  ===LISTS=== [separador]
 *  Siguen las listas de IDs de videos. Existen 2 listas creadas por defecto: la @UserDataManager::HISTORY_LIST_NAME y la @UserDataManager::LIKED_LIST_NAME.
 */
class UserDataManager {
    //TODO this must be a base class, and create a subclass TXTUserDataManager for this case
    protected:
        // Original version used when save the userdata file processed when instantate this manager class.
        int userdatafile_software_version;
        // The current version on which this program is running (@FLTube->VERSION). Used when saving a new userdata file.
        int current_software_version;
        // Path to file containing user video data.
        std::string userdata_filepath;
        // All videos that an user has interacted (played it, liked it, saved it in a custom list, etc.)...
        std::unique_ptr<std::map<std::string, Video*>> videos;
        // Custom lists have an unique name and a vector of Video:id's.
        std::unique_ptr<std::map<std::string, InternalVideoList*>> custom_lists;

        std::shared_ptr<TerminalLogger> logger;


        /** Loads from @filepath the videos saved and the differents lists of videos (History, Likes, etc.).
         * Returns 0 if all was OK.
         * HOOK METHOD (can be reimplemented if new forms of saving data is implemented)...
         */
        int loadData(std::string filepath, int current_version);

        /*
         * Check if a video is dangling, it means, that is not added in any videolist.
         */
        bool isADanglingVideo(Video* v);

        /* Save at @filepath the videos and the differents lists created (History, Likes, etc.).
         * Returns 0 if all was OK.
         * HOOK METHOD (can be reimplemented if new forms of saving data is implemented)...
         */
        int saveData(std::string filepath);

        /*
         * Add a video to the general list of videos hold by this class.
         * Return true if video was added correctly and not exist previously at list.
         */
        bool addVideoInternal(Video* v);

        InternalVideoList* getInternalVideoList(std::string name);

    public:

        static std::string HISTORY_LIST_NAME;
        static std::string LIKED_LIST_NAME;

        static std::string VIDEOS_TXT_SEPARATOR;
        static std::string LISTS_TXT_SEPARATOR;

        static const char FIELD_SEPARATOR = '>';

        /**
         * Constructor. Parameters definition:
         * - (1) @filepath: system path where userdata file will be created/loaded/saved.
         * - (2) @current_version: version of FLTube used to save the userdata file.
         */
        UserDataManager(std::string userdata_filepath, int current_version, std::shared_ptr<TerminalLogger> const& logger);
        ~UserDataManager();



        /** Erase all data saved at userdata filepath, besides delete all elements at memory.
         * Returns 0 if all was OK.
         */
        int eraseAllUserData();

        bool existsVideoList(std::string name);

        // If list doesnt exists, return nullptr.
        VideoList* getVideoList(std::string name);

        // Return the name of every existing video list (the defaults and created by user).
        std::vector<std::string> getVideoListNames();

        // If video list already exists, return false.
        bool createVideoList(std::string name);

        // Delete a video list if exists (except for @HISTORY_LIST_NAME and @LIKED_LIST_NAME). Return true if all was OK.
        bool deleteVideoList(std::string name);

        VideoList* getHistoryList();

        VideoList* getLikedVideosList();

        // Return true if Video was saved previously saved in ANY existing VideoList.
        bool existsVideo(Video* v);

        // Add a video from an existing list.
        bool addVideo(Video* v, std::string listName);

        /** Remove a video from an existing list, searching by its ID.
         * If neither other list holds that video, it is completely deleted
         * of this instance internal data structures.
         */
        bool removeVideoFromList(std::string id, std::string listName);

        // Returns the FLTube version used to save a specific file
        int getVersion();

        // For saving data in the permanent storage.
        int persist();
};

#endif //USERDATA_MANAGER_H
