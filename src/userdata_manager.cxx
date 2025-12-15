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

#include "../include/userdata_manager.h"
#include "../include/fltube_utils.h"

std::string UserDataManager::HISTORY_LIST_NAME = "Navigation History";
std::string UserDataManager::LIKED_LIST_NAME = "Liked";
std::string UserDataManager::VIDEOS_TXT_SEPARATOR = "===VIDEOS===";
std::string UserDataManager::LISTS_TXT_SEPARATOR = "===LISTS===";

UserDataManager::UserDataManager(std::string userdata_filepath, int current_version)
    : userdata_filepath(userdata_filepath), software_version(current_version),
        videos(std::make_unique<std::map<std::string, Video*>>()),
        custom_lists(std::make_unique<std::map<std::string, VideoList*>>())
    {
        //Ensuring create at least the two empty default lists (HISTORY and LIKED).
        (*custom_lists)[UserDataManager::HISTORY_LIST_NAME] = new VideoList(UserDataManager::HISTORY_LIST_NAME, false);
        (*custom_lists)[UserDataManager::LIKED_LIST_NAME] = new VideoList(UserDataManager::LIKED_LIST_NAME, false);

        //Load userdata file if exists and parse it if exists...
        if ( std::filesystem::exists(userdata_filepath)) {
            std::ifstream userdata_file;
            userdata_file.open(userdata_filepath, std::ofstream::in);
            if (!userdata_file.is_open())
                std::cerr << "The user data file at " << userdata_filepath << " doesn't exists. Will be created at program exit...\n";

            //Read the first line, the VERSION used to saved the current state of the userdata file
            std::string line, version_str, data;
            int version_i;
            std::getline(userdata_file, version_str);
            try {
                version_i = std::stoi(version_str);
            } catch (const std::invalid_argument& e) {
                fprintf(stderr, _("Error while parsing version number at '%s' userdata file. Aborting the processing...\n"), userdata_filepath.c_str());
                //Exit without continue processing file...
                return;
            }
            std::cout << "VERSION USED TO SAVE CURRENT USERDATA FILE: " << version_str.at(0) << "."
                << version_str.at(1) << "." << version_str.substr(2) <<"\n";
            // TODO determine what to do if version of a saved file is older than current FLTube version...

            // Parsing userdata from loaded file...
            int parse_videos_flag = 0, parse_lists_flag = 0;
            std::string video_id;
            Video* parsed_v;
            while (std::getline(userdata_file, line)) {
                trim(line);
                if (line.size() > 0) {
                    if (line == UserDataManager::VIDEOS_TXT_SEPARATOR) {
                        std::cout << "Processing video info....\n";
                        parse_videos_flag = 1;
                        continue;
                    }
                    if (line == UserDataManager::LISTS_TXT_SEPARATOR) {
                        std::cout << "Processing lists info....\n";
                        parse_lists_flag = 1;
                        parse_videos_flag = 0;
                        continue;
                    }
                    if (parse_videos_flag) {
                        auto video_fields = tokenize(line,'>');
                        if (video_fields.size() < 7 || video_fields.size() > 7) {
                            // If video data has a bad format, then avoid processing of current line...
                            printf("ABORT VIDEO PROCESSING. Video field size = %lu \n", video_fields.size());
                            continue;
                        }
                        video_id = video_fields.at(0);
                        if (videos->find(video_id) != videos->end()) {
                            // If video already exists at videos map, then avoid adding it again.
                            continue;
                        }
                        parsed_v = new Video();
                        parsed_v->id = video_fields.at(0);              // Pos0=Id
                        parsed_v->title = video_fields.at(1);           // Pos1=Title
                        parsed_v->creator = video_fields.at(2);         // Pos2=Creator
                        parsed_v->channel_id = video_fields.at(3);      // Pos3=Channel_id
                        parsed_v->views = video_fields.at(4);           // Pos4=CountOfViews
                        parsed_v->duration = video_fields.at(5);        // Pos5=Duration
                        parsed_v->thumbnail_url = video_fields.at(6);   // Pos6=Thumbnail_url
                        (*videos)[parsed_v->id] = parsed_v;             //Add the parsed video to the vector of videos.

                    }
                    // Processing VideoLists data saved at userfile...
                    if (parse_lists_flag) {
                        auto list_elements = tokenize(line,'>');
                        if (list_elements.empty() || list_elements.at(0).empty())
                            continue;   // A list must have at least a name, otherwise avoid current line processing...
                        std::string list_name = list_elements.at(0);
                        VideoList* vl;
                        if (list_name != UserDataManager::HISTORY_LIST_NAME && list_name != UserDataManager::LIKED_LIST_NAME)
                            vl = new VideoList(list_name, true);
                        else
                           vl = (*custom_lists)[list_name];
                        for (auto i = 1; i < list_elements.size() ; i++) {
                            if (list_elements.at(i) == "") continue;
                            //Add the the videos to the list, only if they exists at videos general list...
                            auto video_at_list = videos->find(list_elements.at(i));
                            if (video_at_list != videos->end()) {
                                vl->addVideo(video_at_list->second);
                            } else {
                               std::cout << "Video not found: " << list_elements.at(i) << "\n";
                            }
                        }
                        (*custom_lists)[list_name] = vl;

                        std::cout << "- List '" << list_name << "' (Count of videos: " << vl->getLength() << ").\n";
                    }
                }
            }
            userdata_file.close();
        }

        // printf("Video {%s, %s, %s, %s, %s, %s, %s}\n", v->id.c_str(), v->title.c_str(), v->creator.c_str(), v->channel_id.c_str(), v->views.c_str(), v->duration.c_str(), v->thumbnail_url.c_str());

        //NOTE: If userdata_filepath doesn't exists, then this file will be created when close and exiting FLTube.
    };


//  ---------------
// VideoList Class methods definitions.
// ----------------

VideoList::VideoList(std::string name, bool canBeManipulated):
    name(name), canBeManiputaledByUser(canBeManipulated) {
        list = std::make_unique<std::vector<Video*>>();
    };

void VideoList::addVideo(Video* v) {
    if (v == nullptr) {
        std::cout << "Error: Cannot add a null video.\n";
        return;
    }
    if (list != nullptr) list->push_back(v);
}

void VideoList::removeVideo(std::string id) {
    if (id.empty()) return;
    Video* v;
    for (int i = 0; i < list->size(); i++) {
        v = list->at(i);
        if (v->id == id) {
            list->erase(list->begin() + i);
            break;
        }
    }
}

int VideoList::getLength() {
    return list->size();
}

std::string VideoList::getName() {
    return name;
}
//Determine if user can change the content of the list manually or not (for example, History cannot be directly manipulated)...
bool VideoList::isChangeable() {
    return canBeManiputaledByUser;
}
