
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
#ifndef GETTEXT_MACROS_H
#define GETTEXT_MACROS_H

#define _(String) gettext(String)
#define ng_(SINGULAR, PLURAL, COUNT) ngettext(SINGULAR, PLURAL, COUNT)

#include <libintl.h>
#include <locale.h>
#include <iostream>
#include <sys/stat.h>
#include <string>
#include <filesystem>
#include <map>

enum class LANGUAGE_APP {UNKNOWN, SYSTEM, EN, ES, PT_BR};
const std::map<LANGUAGE_APP, std::string> accepted_langs = {
    {LANGUAGE_APP::SYSTEM,""},  // This corresponds is the locale configured at current operating system.
    {LANGUAGE_APP::EN,"en"},
    {LANGUAGE_APP::ES,"es"},
    {LANGUAGE_APP::PT_BR,"pt_BR"},
};

const std::map<LANGUAGE_APP, std::string> accepted_langs_text = {
    {LANGUAGE_APP::SYSTEM,_("System Locale")},  // This corresponds is the locale configured at current operating system.
    {LANGUAGE_APP::EN,_("English")},
    {LANGUAGE_APP::ES,_("Spanish")},
    {LANGUAGE_APP::PT_BR,_("Brazilian Portuguese")},
};

const std::string DEFAULT_LOCAL_PATH("/usr/local/share/locale");
const LANGUAGE_APP DEFAULT_LANGUAGE = LANGUAGE_APP::SYSTEM;

void setup_gettext(const std::string& locale, const std::string& locale_path);

/* Returns true if target_lang is available for its use from UI. */
bool is_language_available(std::string target_lang);

/* Returns the LANGUAGE_APP corresponding to target_lang. If there is no match, returns LANGUAGE_APP::UNKNONW. */
LANGUAGE_APP get_lang(std::string target_lang);

std::string get_lang_text(std::string target_lang);

/* Update the LANGUAGE enviroment variable to a new language, if it is a valid language for FLTube. */
bool set_new_language(const std::string& locale);

#endif // GETTEXT_MACROS_H
