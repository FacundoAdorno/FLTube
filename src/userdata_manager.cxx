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
#include <string>

std::string UserDataManager::HISTORY_LIST_NAME = "Navigation History";
std::string UserDataManager::LIKED_LIST_NAME = "Liked";
std::string UserDataManager::VIDEOS_TXT_SEPARATOR = "===VIDEOS===";
std::string UserDataManager::LISTS_TXT_SEPARATOR = "===LISTS===";

UserDataManager::UserDataManager(std::string userdata_filepath, int current_version, std::shared_ptr<TerminalLogger> const& logger_)
    : userdata_filepath(userdata_filepath), userdatafile_software_version(current_version),
        videos(std::make_unique<std::map<std::string, Video*>>()),
        current_software_version(current_version),
        custom_lists(std::make_unique<std::map<std::string, InternalVideoList*>>()),
        logger(logger_)
    {
        //Ensuring create at least the two empty default lists (HISTORY and LIKED).
        (*custom_lists)[UserDataManager::HISTORY_LIST_NAME] = new InternalVideoList(UserDataManager::HISTORY_LIST_NAME, false);
        (*custom_lists)[UserDataManager::LIKED_LIST_NAME] = new InternalVideoList(UserDataManager::LIKED_LIST_NAME, false);

        char message_bffr[1024];
        if (logger_ == nullptr) this->logger = std::make_shared<TerminalLogger>(false);

        if ( loadData(userdata_filepath, current_version) != 0) {
            std::string bkp_filepath = userdata_filepath + ".bkp";
            snprintf(message_bffr, sizeof(message_bffr), _("There was an error when loading data from %s. The file will be rewritted, and previous copied to a backup file %s.\n"), userdata_filepath.c_str(), bkp_filepath.c_str());
            logger->error(message_bffr);
            std::filesystem::copy_file(userdata_filepath, bkp_filepath, std::filesystem::copy_options::overwrite_existing);
            return;
        }
        snprintf(message_bffr, sizeof(message_bffr), _("User Data was loaded correctly from %s .\n"), userdata_filepath.c_str());
        logger->info(message_bffr);

        //NOTE: If userdata_filepath doesn't exists, then this file will be created when close and exiting FLTube.
    }

    int UserDataManager::loadData(std::string filepath, int current_version) {
        char message_bffr[1024];
        //Load userdata file if exists and parse it if exists...
        if ( std::filesystem::exists(filepath)) {
            std::ifstream userdata_file;
            userdata_file.open(filepath, std::ofstream::in);
            if (!userdata_file.is_open()) {
                snprintf(message_bffr, sizeof(message_bffr), _("The user data file at %s doesn't exists. Will be created at program exit...\n"), filepath.c_str());
                logger->warn(message_bffr);
            }

            //Read the first line, the VERSION used to saved the current state of the userdata file
            std::string line, version_str, data;
            std::getline(userdata_file, version_str);
            if (!isNumber(version_str)) {
                snprintf(message_bffr, sizeof(message_bffr), _("Specified version at first line (%s)is not valid. Abort processing...\n"), version_str.c_str());
                logger->error(message_bffr);
                return 1;
            } else {
                int version_i;
                try {
                    version_i = std::stoi(version_str);
                } catch (const std::invalid_argument& e) {
                    fprintf(stderr, _("Error while parsing version number at '%s' userdata file. Abort processing...\n"), filepath.c_str());
                    return 1;   //Exit without continue processing file...
                }
                this->userdatafile_software_version = version_i;
                snprintf(message_bffr, sizeof(message_bffr), "VERSION USED TO SAVE CURRENT USERDATA FILE: %c.%c.%c\n", version_str.at(0), version_str.at(1), version_str.at(2));
                logger->info(message_bffr);
            }
            // TODO determine what to do if version of a saved file is older than current FLTube version...

            // Parsing userdata from loaded file...
            int parse_videos_flag = 0, parse_lists_flag = 0;
            std::string video_id;
            Video* parsed_v;
            while (std::getline(userdata_file, line)) {
                trim(line);
                if (line.size() > 0) {
                    if (line == UserDataManager::VIDEOS_TXT_SEPARATOR) {
                        logger->debug("Processing video info....\n");
                        parse_videos_flag = 1;
                        continue;
                    }
                    if (line == UserDataManager::LISTS_TXT_SEPARATOR) {
                        logger->debug("Processing lists info....\n");
                        parse_lists_flag = 1;
                        parse_videos_flag = 0;
                        continue;
                    }
                    if (parse_videos_flag) {
                        auto video_fields = tokenize(line, FIELD_SEPARATOR);
                        if (video_fields.size() < 7 || video_fields.size() > 7) {
                            // If video data has a bad format, then avoid processing of current line...
                            snprintf(message_bffr, sizeof(message_bffr), _("ABORT VIDEO PROCESSING. Video field size = %lu \n"), video_fields.size());
                            logger->warn(message_bffr);
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
                        InternalVideoList* vl;
                        if (existsVideoList(list_name) && list_name != UserDataManager::HISTORY_LIST_NAME && list_name != UserDataManager::LIKED_LIST_NAME) {
                            snprintf(message_bffr, sizeof(message_bffr),_("List '%s' is duplicated at userdata file. Avoiding to add duplication...\n"), list_name.c_str());
                            logger->warn(message_bffr);
                            continue;
                        }
                        if (list_name != UserDataManager::HISTORY_LIST_NAME && list_name != UserDataManager::LIKED_LIST_NAME)
                            vl = new InternalVideoList(list_name, true);
                        else
                            vl = (*custom_lists)[list_name];
                        for (auto i = 1; i < list_elements.size() ; i++) {
                            if (list_elements.at(i) == "") continue;
                            //Add the the videos to the list, only if they exists at videos general list...
                            auto video_at_list = videos->find(list_elements.at(i));
                            if (video_at_list != videos->end()) {
                                vl->addVideo(video_at_list->second);
                            } else {
                                snprintf(message_bffr, sizeof(message_bffr), _("Video not found: %s.\n"), list_elements.at(i).c_str());
                                logger->debug(message_bffr);
                            }
                        }
                        (*custom_lists)[list_name] = vl;
                        snprintf(message_bffr, sizeof(message_bffr), _("- List '%s' (Count of videos: %d).\n"), list_name.c_str(), vl->getLength());
                        logger->debug(message_bffr);
                    }
                }
            }
            userdata_file.close();
        }
        return 0;
    }

UserDataManager::~UserDataManager(){
    char message_bffr[1024];
    if ( this->saveData(this->userdata_filepath) == 0) {
        snprintf(message_bffr, sizeof(message_bffr), _("Userdata was saved succesfully at '%s' file.\n"), this->userdata_filepath.c_str());
        logger->info(message_bffr);
    } else {
        snprintf(message_bffr, sizeof(message_bffr), _("Userdata file cannot be saved at '%s'!!! Check if you have write permission on directory...\n"), this->userdata_filepath.c_str());
        logger->error(message_bffr);
    }

    for (auto& pair : *videos) {
        delete pair.second;
    }
    videos->clear();

    for (auto& pair : *custom_lists) {
        delete pair.second;
    }
    custom_lists->clear();
}

int UserDataManager::saveData(std::string filepath) {
    std::ofstream outputfile;
    if ( std::filesystem::exists(userdata_filepath) ) {
        //Making a backup of previous file (if exists) before saving new data...
        std::filesystem::copy_file(userdata_filepath, filepath + ".bkp", std::filesystem::copy_options::overwrite_existing);
    }
    //Open file (and create if not exists) for write.
    outputfile.open(filepath, std::ofstream::trunc);
    outputfile << current_software_version << "\n";
    //Write video info...
    outputfile << UserDataManager::VIDEOS_TXT_SEPARATOR << "\n";
    Video* v;
    for (auto it = videos->begin(); it != videos->end() ;it++) {
        //id>title>creator>channel_id>views>duration>thumbnail_url
        v = it->second;
        if (isADanglingVideo(v)) {
            continue;   //Avoid to write any video that is dangling...
        }
        outputfile << v->id << FIELD_SEPARATOR << v->title << FIELD_SEPARATOR << v->creator << FIELD_SEPARATOR
                    << v->channel_id << FIELD_SEPARATOR << v->views << FIELD_SEPARATOR << v->duration << FIELD_SEPARATOR
                    << v->thumbnail_url << "\n";
    }
    //Write video lists info...
    outputfile << UserDataManager::LISTS_TXT_SEPARATOR << "\n";
    InternalVideoList* vl;
    for (auto it = custom_lists->begin(); it != custom_lists->end() ;it++) {
        vl = it->second;
        outputfile << vl->getName() << FIELD_SEPARATOR;
        for (int pos = 0; pos < vl->getLength(); pos++) {
            outputfile << vl->getVideoAt(pos)->id;
            if (pos < vl->getLength() - 1) outputfile << FIELD_SEPARATOR;
        }
        outputfile << "\n";
    }
    outputfile.close();
    return 0;
}

bool UserDataManager::isADanglingVideo(Video* v) {
    bool videoFound = false;
    if ( v != nullptr && existsVideo(v)) {
        for (auto it = custom_lists->begin(); it != custom_lists->end() ; it++) {
            //auto it = videos->find(v->id);
            if (it->second->findVideoById(v->id) != nullptr ) {
                videoFound = true;
                break;
            }
        }
    }
    return !videoFound;
}

bool UserDataManager::createVideoList(std::string name) {
    //TODO finish...
    return false;
}

bool UserDataManager::deleteVideoList(std::string name) {
    //TODO finish...
    return false;
}

bool UserDataManager::existsVideoList(std::string name) {
    auto it = custom_lists->find(name);
    return (it != custom_lists->end());
}

bool UserDataManager::existsVideo(Video* v) {
    if (v == nullptr) {
        logger->error(_("Video is a null pointer... Cannot proceed.\n"));
        return false;
    }
    auto it = videos->find(v->id);
    return (it != videos->end());
}

InternalVideoList* UserDataManager::getInternalVideoList(std::string name) {
    if (existsVideoList(name))
        return custom_lists->find(name)->second;
    else
        return nullptr;
}

VideoList* UserDataManager::getVideoList(std::string name) {
    return getInternalVideoList(name);
}

std::vector<std::string> UserDataManager::getVideoListNames() {
    std::vector<std::string> names = {};
    for (auto it = custom_lists->begin(); it != custom_lists->end() ; it++) {
        names.push_back(it->second->getName());
    }
    return names;
}

VideoList* UserDataManager::getHistoryList() {
    return this->getVideoList(UserDataManager::HISTORY_LIST_NAME);
}

VideoList* UserDataManager::getLikedVideosList() {
    return this->getVideoList(UserDataManager::LIKED_LIST_NAME);
}

int UserDataManager::getVersion() {
    return this->userdatafile_software_version;
}

bool UserDataManager::addVideoInternal(Video* v) {
    if ( !this->existsVideo(v) ) {
        (*videos)[v->id] = v;
        return true;
    }
    return false;
}

bool UserDataManager::addVideo(Video* v, std::string listName) {
    if (v == nullptr || listName.empty()) {
        logger->error(_("Cannot add video because one of the parameters, list or name, is empty...\n"));
        return false;
    }
    if ( !this->existsVideoList(listName) ) {
        char message_bffr[512];
        snprintf(message_bffr, sizeof(message_bffr), _("Cannot add video to an inexisting list '%s'.\n"), listName.c_str());
        logger->error(message_bffr);
        return false;
    }

    if ( !existsVideo(v) ) this->addVideoInternal(v);
    getInternalVideoList(listName)->addVideo(v);
    return true;
}

int UserDataManager::persist() {
    return this->saveData(this->userdata_filepath);
}

int UserDataManager::eraseAllUserData() {
    //TODO finish it...
    return 0;
}


//  ---------------
// VideoList Class methods definitions.
// ----------------

VideoList::VideoList(std::string name, bool canBeManipulated):
    name(name), canBeManiputaledByUser(canBeManipulated) {
        list = std::make_unique<std::vector<Video*>>();
    };

int VideoList::getLength() {
    return list->size();
}

std::string VideoList::getName() {
    return name;
}

Video* VideoList::findVideoById(std::string id) {
    if (!id.empty()) {
        for (int i = 0; i < list->size(); ++i) {
            if (list->at(i)->id == id) return list->at(i);
        }
    }
    return nullptr;
}

Video* VideoList::getVideoAt(int position) {
    if (position >= this->getLength()) return nullptr;
    else return this->list->at(position);
}

//Determine if user can change the content of the list manually or not (for example, History cannot be directly manipulated)...
bool VideoList::isChangeable() {
    return canBeManiputaledByUser;
}

// This method only adds a video to the list if this was not previously added.
void InternalVideoList::addVideo(Video* v) {
    if (v == nullptr) {
        std::cerr << _("Error: Cannot add a null video.\n");
        return;
    }
    if (findVideoById(v->id) != nullptr) {
        return; // Avoid adding a video if already exists at list...
    }
    list->push_back(v);
}

void InternalVideoList::removeVideo(std::string id) {
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
