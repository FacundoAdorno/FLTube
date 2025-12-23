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
#include "../include/configuration_manager.h"
#include "../include/fltube_utils.h"


ConfigurationManager::ConfigurationManager(std::string path_to_conf, std::shared_ptr<TerminalLogger> const& logger_) : logger(logger_)
{
  configurations = std::make_unique<std::map<std::string, std::string>>();
  shortcuts = new KeyboardShortcuts();
  if (logger_ == nullptr) this->logger = std::make_shared<TerminalLogger>(false);
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
                (*configurations)[config_key] = config_value;
            }
        }
    }

    shortcuts->overwriteDefaults(this);
}

ConfigurationManager::~ConfigurationManager(){
    delete shortcuts;
}

bool ConfigurationManager::existProperty(const char* config_name) {
    return (config_name && configurations && configurations->find(config_name) != configurations->end());
}

std::string ConfigurationManager::getProperty(const char* config_name, const char* default_value){
    if (!config_name || std::strlen(config_name) == 0) {
        return "";
    }
    else if (this->existProperty(config_name)) {
        return configurations->find(config_name)->second;
    }
    return std::string((default_value) ? default_value : "");
}

int ConfigurationManager::getIntProperty(const char *config_name, int default_value) {

    char default_value_bff[20];
    snprintf(default_value_bff, sizeof(default_value_bff), "%d", default_value);
    std::string prop = this->getProperty(config_name, default_value_bff);

    try {
        return std::stoi(prop);
    } catch (const std::invalid_argument& e) {
        logger->error("Invalid configuration at property '" + std::string(config_name) + "' (CURRENT VALUE: " + prop + "). Please, modify it to a valid integer value.\n");
    } catch (const std::out_of_range& e) {
        logger->error("Invalid configuration at property '" + std::string(config_name) + "' (CURRENT VALUE: " + prop + "). Value out of range: " + std::string(e.what()) + ".\n");
    }

    return default_value;
}

bool ConfigurationManager::getBoolProperty(const char* config_name, bool default_value) {
    std::string p = getProperty(config_name, "");
    if (!p.empty()) {
        if (p == "True" || p == "true") return true;
        else if (p == "False" || p == "false") return false;
    }
    // If configuration is not defined, or value not equal to "True", "true", "False", or "false"...
    return default_value;
}

int ConfigurationManager::getShortcutFor(SHORTCUTS s){
    return this->shortcuts->getShortcut(s);
}

int ConfigurationManager::getShortcutFor(std::string s){
    return this->shortcuts->getShortcut(s);
}

std::string ConfigurationManager::getShortcutTextFor(SHORTCUTS s) {
    return this->shortcuts->getShortcutText(s);
}

// KeyboardShortcuts methods implementation. //

int KeyboardShortcuts::getShortcut(SHORTCUTS s){
    if (this->keybindings->find(s) != this->keybindings->end()) {
        std::pair shortcut = this->keybindings->find(s)->second;
        return shortcut.first;
    }
    // If no shortcut can be returned, return 0.
    return this->NO_SHORTCUT;
}

int KeyboardShortcuts::getShortcut(std::string shtct) {
    return this->getShortcut(this->toShortcut(shtct));
}

std::string KeyboardShortcuts::getShortcutText(SHORTCUTS s) {
    if (this->keybindings->find(s) != this->keybindings->end()) {
        std::pair shortcut = this->keybindings->find(s)->second;
        return shortcut.second;
    }
    // If no shortcut can be returned, return 0.
    return "Unknown";
}

SHORTCUTS KeyboardShortcuts::toShortcut(std::string shtct) {
    for (auto iter = shortcuts_str->begin(); iter != shortcuts_str->end(); ++iter) {
        if (iter->second == shtct) return iter->first;
    }
    return SHORTCUTS::UNDEFINED;
}

void KeyboardShortcuts::setShortcut(SHORTCUTS s, int value, std::string keybinding_text) {
    std::pair<int, std::string>& shortcut = this->keybindings->at(s);
    shortcut.first = value;
    shortcut.second = keybinding_text;
}

int KeyboardShortcuts::isWellDefined(std::string shortcut_def) {

    if (shortcut_def.empty()) return KeyboardShortcuts::INVALID_SHORTCUT;
    std::smatch m;
    std::regex r1,r2,r3;
    std::string otherValidKeysTemplate = "^([F|f](1[0-2]|[1-9]))|([a-zA-Z])|([0-9])|([[:punct:]])$";
    int k1, k2, k3;
    std::vector<std::string> keys = tokenize(shortcut_def, '+');
    // The count of keys in a shortcut is 1 to 3, otherwise is invalid.
    if (keys.size() == 0 || keys.size() > 3) return -1;

    if (keys.size() == 1) {
        //Shortcuts with 1 key - only functions keys [F1, F2, ..., F12] accepted.
        r1 = std::regex("^[F|f](1[0-2]|[1-9])$");
        std::regex_search(keys[0], m, r1);
        if (!m.empty()) {
            // printf("Value of FL_F + %s in hexadecimal is: 0x%x\n", m[1].str().c_str(), FL_F + stoi(m[1].str()));
            return FL_F + stoi(m[1].str());
        }
    } else if (keys.size() == 2){
        //Shortcuts with 2 keys - (1stkey)=[Ctrl | Alt] + (2ndKey)=[OtherValidKey]
        r1 = std::regex("^(Ctrl|Alt)$");
        r2 = std::regex(otherValidKeysTemplate);
        std::regex_search(keys[0], m, r1);
        if (!m.empty()) {
            if(m[1].str() == "Ctrl") k1 = FL_CTRL;
            else k1 = FL_ALT;
        } else {
            // The two keys must be setted...
            return KeyboardShortcuts::INVALID_SHORTCUT;
        }

        std::regex_search(keys[1], m, r2);
        if (!m.empty()) {
            if (m[1].length() > 0) {
                k2 = FL_F + stoi(m[2].str());
                return k1 + k2;
            } else {
                return k1 + m[0].str()[0];
            }
        } else { return KeyboardShortcuts::INVALID_SHORTCUT; }
    } else {
        //Shortcuts with 3 keys - (1stkey)=[Ctrl | Alt] + (2ndKey)=[Shift | Alt ] + (3rdKey)=[OtherValidKey]
        r1 = std::regex("^(Ctrl|Alt)$");
        r2 = std::regex("^(Shift|Alt)$");
        r3 = std::regex(otherValidKeysTemplate);
        std::regex_search(keys[0], m, r1);
        if (!m.empty()) {
            if(m[1].str() == "Ctrl") k1 = FL_CTRL;
            else k1 = FL_ALT;
        } else {
            // The two keys must be setted...
            return KeyboardShortcuts::INVALID_SHORTCUT;
        }

        std::regex_search(keys[1], m, r2);
        if (!m.empty()) {
            if(m[1].str() == "Shift") k2 = FL_SHIFT;
            else if (k1 != FL_ALT) {
                k2 = FL_ALT;
            } else {
                // Cannot be two "Alt" key simultaneously...
                return KeyboardShortcuts::INVALID_SHORTCUT;
            }
        } else {
            // The two keys must be setted...
            return KeyboardShortcuts::INVALID_SHORTCUT;
        }

        std::regex_search(keys[2], m, r3);
        if (!m.empty()) {
            if (m[1].length() > 0) {
                k3 = FL_F + stoi(m[2].str());
                return k1 + k2 + k3;
            } else {
                return k1 + k2 + m[0].str()[0];
            }
        } else { return KeyboardShortcuts::INVALID_SHORTCUT; }
    }
    return KeyboardShortcuts::INVALID_SHORTCUT;
};

void KeyboardShortcuts::overwriteDefaults(ConfigurationManager* config) {
    std::string shtc_value;
    int shtc_value_int;
    for ( auto iter = this->shortcuts_str->begin(); iter != shortcuts_str->end(); ++iter) {
        shtc_value = config->getProperty(iter->second, "");
        if (shtc_value != "") {
            shtc_value_int = KeyboardShortcuts::isWellDefined(shtc_value);
            // printf("Value 1: %d - Value 2: %d", FL_CTRL + FL_F + 1 ,shtc_value_int);
            // If the shortcut is well defined, then overwrite the default with user custom shortcut...
            if(shtc_value_int > 0) {
              if (!isUsed(shtc_value_int)) this->setShortcut(iter->first, shtc_value_int, shtc_value);
              else printf(_("Keybinding for shortcut '%s' already in use. Please, change it for an unused combination of keys.\n"), iter->second);
            }
            else printf(_("Invalid keybinding for shortcut '%s'. Please, change it for a valid combination of keys.\n"), iter->second);
        }
    }
}

bool KeyboardShortcuts::isUsed(int shortcut){
    for ( auto iter = this->keybindings->begin(); iter != this->keybindings->end(); ++iter) {
        if (iter->second.first == shortcut) return true;
    }
    return false;
}
