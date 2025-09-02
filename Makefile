# Variables
CXX      = $(shell fltk-config --cxx)
DEBUG    = -g
CXXFLAGS = $(shell fltk-config --use-images --cxxflags) -Iinclude `pkg-config --cflags libcurl`
#LDFLAGS  = $(shell fltk-config --use-gl --use-images --ldflags) `pkg-config --libs libcurl`
LDSTATIC  = $(shell fltk-config --use-images --ldstaticflags) `pkg-config --libs libcurl`
LINK     = $(CXX)

BUILD_DIR = build
SRC_DIR = src
INCLUDE_DIR = include
FLUID_PATHFILE = fluid/FLTube_View.fl
SCRIPTS_DIR = scripts
INSTALL_YTDLP_SCRIPTNAME = install_yt-dlp
# Files
TARGET = $(BUILD_DIR)/fltube
SOURCES_LIST = fltube_utils.cxx gnugettext_utils.cxx FLTube_View.cxx FLTube.cxx
SOURCES = $(addprefix $(SRC_DIR)/, $(SOURCES_LIST))
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cxx=$(BUILD_DIR)/%.o)

# Rules
.SUFFIXES: .o .cxx

# Main rule
all: build $(TARGET)

# Rule to link to executable
$(TARGET): $(OBJECTS)
	$(LINK) -o $@ $^ $(LDSTATIC)

# Rule for compile .o (object) files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cxx
	$(CXX) $(CXXFLAGS) $(DEBUG) -c $< -o $@

# Create build directory
build:
	mkdir -p build

# Install program. If $(PREFIX) is specified, all will be installed relative to that directory.
install: all
	mkdir -p $(PREFIX)/usr/local/share/locale/es/LC_MESSAGES
	msgfmt -o locales/es/LC_MESSAGES/FLTube.mo  locales/es/LC_MESSAGES/FLTube.po
	cp locales/es/LC_MESSAGES/FLTube.mo $(PREFIX)/usr/local/share/locale/es/LC_MESSAGES/

	msgfmt -o locales/es/LC_MESSAGES/$(INSTALL_YTDLP_SCRIPTNAME).mo  locales/es/LC_MESSAGES/$(INSTALL_YTDLP_SCRIPTNAME).po
	cp locales/es/LC_MESSAGES/$(INSTALL_YTDLP_SCRIPTNAME).mo $(PREFIX)/usr/local/share/locale/es/LC_MESSAGES/

	mkdir -p $(PREFIX)/etc/fltube
	cp fltube.conf $(PREFIX)/etc/fltube/fltube.conf

	mkdir -p $(PREFIX)/usr/local/bin/
	mv $(TARGET) $(PREFIX)/usr/local/bin/
	cp $(SCRIPTS_DIR)/install_yt-dlp.sh $(PREFIX)/usr/local/bin/


# Extract strings from source and update .po locale files.
po_update:
	xgettext --keyword=_ --keyword=ng_ --language=C++ --from-code=utf-8 --output locales/FLTube.pot src/*.cxx include/*.h
	msgmerge --update locales/es/LC_MESSAGES/FLTube.po  locales/FLTube.pot
	xgettext --keyword=_ --keyword=ng_ --language=Shell --from-code=utf-8 --output locales/$(INSTALL_YTDLP_SCRIPTNAME).pot $(SCRIPTS_DIR)/$(INSTALL_YTDLP_SCRIPTNAME).sh
	msgmerge --update locales/es/LC_MESSAGES/$(INSTALL_YTDLP_SCRIPTNAME).po  locales/$(INSTALL_YTDLP_SCRIPTNAME).pot

# Rule to compile FLUID files
compile_fluid:
	cd `dirname $(FLUID_PATHFILE)` && fluid -o ../$(SRC_DIR)/FLTube_View.cxx -h ../$(INCLUDE_DIR)/FLTube_View.h -c `basename $(FLUID_PATHFILE)`

# Rule to clean objects and directories during development
clean:
	@if [ -z "$(BUILD_DIR)" ]; then \
		echo "Error: BUILD_DIR must be set before make a clean."; \
		exit 1; \
	fi
	rm -f $(BUILD_DIR)/*.o $(TARGET) 2> /dev/null && rm -rf $(BUILD_DIR)/usr/local $(BUILD_DIR)/usr/ $(BUILD_DIR)/etc/fltube && rmdir $(BUILD_DIR)/etc $(BUILD_DIR)

.PHONY: all clean build compile_fluid po_update
