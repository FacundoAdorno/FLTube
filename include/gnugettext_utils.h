
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


std::string default_locale_path();
void setup_gettext(const std::string& locale, const std::string& locale_path);

#endif // GETTEXT_MACROS_H
