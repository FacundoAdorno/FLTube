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


std::string default_locale_path(){
    return std::string("/usr/local/share/locale");
}

/*
 * Setup locale used for app ("es", "en"). If nullptr passed, use system locale. Also, you can specify an alternative locale path.
 */
void setup_gettext(const std::string& locale = "", const std::string& locale_path = "") {
    // Establecer el locale
    if (locale.empty()) {
        // Si no se proporciona un locale, usar el sistema
        setlocale(LC_ALL, "");
    } else {
        // Intentar establecer el locale proporcionado
        if (setlocale(LC_ALL, locale.c_str()) == nullptr) {
            std::cerr << "Warning: Locale '" << locale << "' not available. Falling back to system locale." << std::endl;
            setlocale(LC_ALL, "");
        }
    }

    if( locale_path.empty() || !std::filesystem::exists(locale_path) ) {
        std::cout << "Locales path: '" << locale_path << "' does not exists!!!" << std::endl;
        bindtextdomain("FLTube", default_locale_path().c_str());
    } else {
        std::cout << "USING Locales path: '" << locale_path << "'" << std::endl;
        bindtextdomain("FLTube", locale_path.c_str());
    }
    textdomain("FLTube");

    std::cout << "Locale set to: " << (locale.empty() ? "system locale" : locale) << std::endl;
}

