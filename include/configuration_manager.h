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
#ifndef CONFIGURATION_MANAGER_H
#define CONFIGURATION_MANAGER_H

#include <FL/Enumerations.H>
#include <cstring>
#include <memory>
#include <string>
#include <map>
#include <regex>

class KeyboardShortcuts; // Forward declaration

enum class SHORTCUTS {
    FOCUS_SEARCH=1, FOCUS_VIDEO_1, FOCUS_VIDEO_2, FOCUS_VIDEO_3, FOCUS_VIDEO_4,
    FOCUS_CHANNEL_1, FOCUS_CHANNEL_2, FOCUS_CHANNEL_3, FOCUS_CHANNEL_4, SHOW_HELP,
    UNDEFINED
};

/**
 * Class used for parse and load configurations defined at a .conf file.
 * The properties must follow the next format:
 *          config_key = value.
 * Comments starts with "#".
*/
class ConfigurationManager {
protected:
    std::string filepath;
    std::unique_ptr<std::map<std::string, std::string>> configurations;
    KeyboardShortcuts* shortcuts;

public:
    ConfigurationManager(std::string path_to_conf);
    ~ConfigurationManager();
    /*
     *   Returns true if the configuration is set in the configuration file.
     */
    bool existProperty(const char* config_name);
    /*
     * Return the value of a property configuration loaded with @loadConfFile() method.
     * If no property is found, returns @default_value or empty string ("") if no default value is specified..
     */
    std::string getProperty(const char* config_name, const char* default_value);
    /** Return the value of a INTEGER property configuration loaded with @loadConfFile() method.
     * If no property is found, returns @default_value.
     */
    int getIntProperty(const char *config_name, int default_value);

    /**
     * Return the specific keybindings for some shortcut name. As underlying implementation, a KeyboardShortcuts object is used.
     */
    int getShortcutFor(SHORTCUTS s);
    int getShortcutFor(std::string s);
};

/** Define the program shortcuts, and overwrite the defaults based on the fltube configuration file.
 *  * Valid combinations:
 *      - 1 Key: only functions keys [F1, F2, ..., F12].
 *
 *      - 2 keys:
 *          * (1stkey)=[Ctrl | Alt] + (2ndKey)=[OtherValidKey]
 *
 *      - 3 keys:
 *          * (1stkey)=[Ctrl | Alt] + (2ndKey)=[Shift | Alt ] + (3rdKey)=[OtherValidKey]
 *
 *      Where "OtherValidKey" are:
 *        * functions keys [F1, F2, ..., F12],
 *        * numbers,
 *        * alphabet charecters [a-zA-Z], or
 *        * puntuaction characters [!"#$%&'()*+,-./:;<=>?@[\]^_`{|}~.].
 */
class KeyboardShortcuts {
    protected:
        const char* CTRL_S = "Ctrl";
        const char* ALT_S = "Alt";
        const char* SHIFT_S = "Shift";
        /** 0 for shortcuts in FLTK means 'no shortcut for specific widget'. */
        const int  NO_SHORTCUT = 0;
        std::unique_ptr<std::map<SHORTCUTS, int>> keybindings;
        /** Holds the names of the properties at fltube.conf corresponding to each shortcut. */
        std::unique_ptr<std::map<SHORTCUTS, char[32]>> shortcuts_str;
        /** Check if keybinding is being used for some SHORTCUT. */
        bool isUsed(int shortcut);
        /**  Return the equivalent element in SHORTCUTS enum, if exists. If not, returns UNDEFINED. */
        SHORTCUTS toShortcut(std::string shortcut_s);


    public:
        static const int INVALID_SHORTCUT = -1;
        /** Constructor that set the defaults keybindings for the widgets shortcuts. */
        KeyboardShortcuts()
            : keybindings(std::make_unique<std::map<SHORTCUTS, int>>()),
              shortcuts_str(std::make_unique<std::map<SHORTCUTS, char[32]>>()){

            (*keybindings)[SHORTCUTS::FOCUS_SEARCH] = FL_CTRL + 'l';
            (*keybindings)[SHORTCUTS::FOCUS_VIDEO_1] = FL_CTRL + '1';
            (*keybindings)[SHORTCUTS::FOCUS_VIDEO_2] = FL_CTRL + '2';
            (*keybindings)[SHORTCUTS::FOCUS_VIDEO_3] = FL_CTRL + '3';
            (*keybindings)[SHORTCUTS::FOCUS_VIDEO_4] = FL_CTRL + '4';
            (*keybindings)[SHORTCUTS::FOCUS_CHANNEL_1] = FL_CTRL + FL_SHIFT + '1';
            (*keybindings)[SHORTCUTS::FOCUS_CHANNEL_2] = FL_CTRL + FL_SHIFT + '2';
            (*keybindings)[SHORTCUTS::FOCUS_CHANNEL_3] = FL_CTRL + FL_SHIFT + '3';
            (*keybindings)[SHORTCUTS::FOCUS_CHANNEL_4] = FL_CTRL + FL_SHIFT + '4';
            (*keybindings)[SHORTCUTS::SHOW_HELP] = FL_CTRL + '?';

            std::strcpy((*shortcuts_str)[SHORTCUTS::FOCUS_SEARCH], "FOCUS_SEARCH");
            std::strcpy((*shortcuts_str)[SHORTCUTS::FOCUS_VIDEO_1], "FOCUS_VIDEO_1");
            std::strcpy((*shortcuts_str)[SHORTCUTS::FOCUS_VIDEO_2], "FOCUS_VIDEO_2");
            std::strcpy((*shortcuts_str)[SHORTCUTS::FOCUS_VIDEO_3], "FOCUS_VIDEO_3");
            std::strcpy((*shortcuts_str)[SHORTCUTS::FOCUS_VIDEO_4], "FOCUS_VIDEO_4");
            std::strcpy((*shortcuts_str)[SHORTCUTS::FOCUS_CHANNEL_1], "FOCUS_CHANNEL_1");
            std::strcpy((*shortcuts_str)[SHORTCUTS::FOCUS_CHANNEL_2], "FOCUS_CHANNEL_2");
            std::strcpy((*shortcuts_str)[SHORTCUTS::FOCUS_CHANNEL_3], "FOCUS_CHANNEL_3");
            std::strcpy((*shortcuts_str)[SHORTCUTS::FOCUS_CHANNEL_4], "FOCUS_CHANNEL_4");
            std::strcpy((*shortcuts_str)[SHORTCUTS::SHOW_HELP], "SHOW_HELP");
            };

        ~KeyboardShortcuts() {
            delete CTRL_S; delete ALT_S; delete SHIFT_S;
        };

        /** OVerwrite the defaults keybindings if defined in a ConfigurationManager. */
        void overwriteDefaults(ConfigurationManager* config);
        /** Set an individual shortcut. */
        void setShortcut(SHORTCUTS s , int value);
        /** Return the configured keybinding for an specific shortcut.
         *  If not defined, returns 0 (NO_SHORTCUT). */
        int getShortcut(SHORTCUTS s);
        int getShortcut(std::string s);
        /** Check if a shortcut configuration is well defined according to the shortcut rules.
         *      For example: 'Ctrl + a' is well defined, not 'a + Ctrl' or 'Crl + a'...
         *  If the shortcut is valid, then return the corresponding "int" value, otherwise return -1 if invalid.
         */
        static int isWellDefined(std::string shortcut_def);
};

#endif //CONFIGURATION_MANAGER_H
