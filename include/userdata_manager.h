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

#ifndef USERDATA_MANAGER_H
#define USERDATA_MANAGER_H


#include <map>
#include <vector>
#include <memory>
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>


//TODO Delete this struct and use in its place the YTDLP_Video_Metadata struct...
struct Video {
    std::string id;
    std::string title;
    std::string creator;
    std::string channel_id;
    std::string views;
    std::string duration;
    std::string thumbnail_url;
};

class VideoList {
    protected:
        std::string name;
        bool canBeManiputaledByUser;    //History cannot be directly manipulated by user, except removing a video.
        std::unique_ptr<std::vector<Video*>> list;
    public:
        VideoList(std::string name, bool canBeManipulated);
        void addVideo(Video* v);
        void removeVideo(std::string id);
        int getLength();
        std::string getName();
        //Determine if user can change the content of the list manually or not (for example, History cannot be directly manipulated)...
        bool isChangeable();
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
    protected:
        int software_version;
        // Path to file containing user video data.
        std::string userdata_filepath;
        // All videos that an user has interacted (played it, liked it, saved it in a custom list, etc.)...
        std::unique_ptr<std::map<std::string, Video*>> videos;
        // Custom lists have an unique name and a vector of Video:id's.
        std::unique_ptr<std::map<std::string, VideoList*>> custom_lists;


        /** Loads the videos saved and the differents lists of videos (History, Likes, etc.).
         * Returns 0 if all was OK.
         */
        int loadData(std::string filepath);

        bool existsVideoList(std::string name);

        // Return true if Video was saved previously saved in any VideoList.
        bool existsVideo(Video v);
    public:

        static std::string HISTORY_LIST_NAME;
        static std::string LIKED_LIST_NAME;

        static std::string VIDEOS_TXT_SEPARATOR;
        static std::string LISTS_TXT_SEPARATOR;

        /**
         * Constructor. Parameters definition:
         * - (1) @filepath: system path where userdata file will be created/loaded/saved.
         * - (2) @current_version: version of FLTube used to save the userdata file.
         */
        UserDataManager(std::string userdata_filepath, int current_version);
        ~UserDataManager();

        /** Save at filepath the videos and the differents lists created (History, Likes, etc.).
         * Returns 0 if all was OK.
         */
        int saveData(std::string filepath);

        /** Erase all data saved at userdata filepath, besides delete all elements at memory.
         * Returns 0 if all was OK.
         */
        int eraseAllUserData();

        // If list doesnt exists, return nullptr.
        VideoList* getVideoList(std::string name);

        // If video list already exists, return false.
        bool createVideoList(std::string name);

        VideoList* getHistoryList();

        VideoList* getLikedVideosList();

        bool addVideo(Video v, std::string name);

        // Returns the FLTube version used to save a specific file
        int getVersion(std::string filepath);
};

#endif //USERDATA_MANAGER_H
