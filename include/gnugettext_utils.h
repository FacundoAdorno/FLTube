
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


#endif // GETTEXT_MACROS_H

void setup_gettext(const std::string& locale, const std::string& locale_path);
