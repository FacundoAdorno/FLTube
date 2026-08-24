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


/*
 * Setup locale used for app ("es", "en"). If nullptr passed, use system locale. This param is used to define
 *      the following env variables: (1) LC_ALL y (2) LANGUAGE (gettext env var).
 * Also, you can specify an alternative locale path.
 */
void setup_gettext(const std::string& locale_ = "", const std::string& locale_path = "") {
    //  Set the app language. If cannot, use DEFAULT_LANGUAGE.
    bool language_is_valid = is_language_available(locale_);
    std::string locale = ((language_is_valid) ? locale_ : accepted_langs.at(DEFAULT_LANGUAGE));
    setenv("LANGUAGE", locale.c_str(), 1);

    // Set the locale (if exists at system). If cannot, use system locale.
    if (locale.empty()) {
        // Use system locale if no locale param is provided...
        setlocale(LC_ALL, "");
    } else {
        // Try to set the provided locale...
        if (setlocale(LC_ALL, locale.c_str()) == nullptr) { setlocale(LC_ALL, ""); }
    }

    if( locale_path.empty() || !std::filesystem::exists(locale_path) ) {
        std::cout << "Locales path: '" << locale_path << "' does not exists!!!" << std::endl;
        bindtextdomain("FLTube", DEFAULT_LOCAL_PATH.c_str());
    } else {
        std::cout << "USING Locales path: '" << locale_path << "'" << std::endl;
        bindtextdomain("FLTube", locale_path.c_str());
    }
    textdomain("FLTube");

    std::cout << "Locale set to: " << get_lang_text(locale) << std::endl;
    std::cout << "Language set to: " << get_lang_text(locale) << std::endl;
}

LANGUAGE_APP get_lang(std::string target_lang) {
    for (auto iter = accepted_langs.begin(); iter != accepted_langs.end(); iter++) {
        if (iter->second == target_lang) return iter->first;
    }
    return LANGUAGE_APP::UNKNOWN;
}

std::string get_lang_text(std::string target_lang) {
    LANGUAGE_APP lang = get_lang(target_lang);
    if (lang != LANGUAGE_APP::UNKNOWN) return accepted_langs_text.at(lang);
    return std::string{};   // Returns empty string if not exists...
}

bool is_language_available(std::string target_lang) {
    return (get_lang(target_lang) != LANGUAGE_APP::UNKNOWN);
}

bool set_new_language(const std::string& locale) {
    if (!is_language_available(locale)) return false;
    setenv("LANGUAGE", locale.c_str(), 1);
    return true;
}



//////     FREE SECTION TO ADD TEXT FOR TRANSLATIONS...     ////////
void free_translations_texts() {
    _("Autos & Vehicles");
    _("Comedy");
    _("Education");
    _("Entertainment");
    _("Film & Animation");
    _("Gaming");
    _("Howto & Style");
    _("Music");
    _("News & Politics");
    _("Nonprofits & Activism");
    _("People & Blogs");
    _("Pets & Animals");
    _("Science & Technology");
    _("Sports");
    _("Travel & Events");
}
