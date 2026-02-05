# Variables
CXX      = $(shell fltk-config --cxx)
DEBUG    = -g
CXXFLAGS = $(shell fltk-config --use-images --cxxflags) -fexceptions -Iinclude `pkg-config --cflags libcurl` `pkg-config --cflags libarchive`
#LDFLAGS  = $(shell fltk-config --use-gl --use-images --ldflags) `pkg-config --libs libcurl`
LDSTATIC  = $(shell fltk-config --use-images --ldstaticflags) -fexceptions `pkg-config --libs libcurl` `pkg-config --libs libarchive`
LINK     = $(CXX)
SHELL 	:= /bin/bash

ARCH_CPU_T  != uname -m | grep -q "x86_64" && echo "amd64" || echo "i386"
# If cannot calculate architecture for some reason, defaults to i386 architecture...
ARCH_CPU_T  ?= i386
ARCH_CPU    ?= $(ARCH_CPU_T)

ARCH_BITS_T != uname -m | grep -q "x86_64" && echo "64" || echo "32"
# If cannot calculate architecture, defaults to 32 bits...
ARCH_BITS_T ?= 32
ARCH_BITS ?= $(ARCH_BITS_T)

BUILD_DIR = build
SRC_DIR = src
INCLUDE_DIR = include
FLUID_PATHFILE = fluid/FLTube_View.fl
SCRIPTS_DIR = scripts
INSTALL_YTDLP_SCRIPTNAME = install_yt-dlp
LOCALE_INSTALL_DIR = /usr/local/share/locale
# Files
TARGET = $(BUILD_DIR)/fltube
SOURCES_LIST = fltube_utils.cxx gnugettext_utils.cxx FLTube_View.cxx FLTube.cxx configuration_manager.cxx userdata_manager.cxx fltk_animation.cxx
SOURCES = $(addprefix $(SRC_DIR)/, $(SOURCES_LIST))
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cxx=$(BUILD_DIR)/%.o)
FLTUBE_VERSION=2.1.0
DEB_PACKAGE_NAME=fltube_$(FLTUBE_VERSION)-$(ARCH_CPU).deb
TCZ_PACKAGE_NAME=fltube_$(FLTUBE_VERSION)_tcz.tar.gz


# Rules
.SUFFIXES: .o .cxx

# Main rule
all: build $(TARGET)

# Rule to link to executable
$(TARGET): $(OBJECTS)
	$(LINK) -m$(ARCH_BITS) -o $@ $^ $(LDSTATIC)

# Rule for compile .o (object) files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cxx
	$(CXX) -m$(ARCH_BITS) $(CXXFLAGS) $(DEBUG) -c $< -o $@

# Create build directory
build:
	mkdir -p build

# Install program. If $(PREFIX) is specified, all will be installed relative to that directory.
install: all
	mkdir -p $(PREFIX)$(LOCALE_INSTALL_DIR)/es/LC_MESSAGES
	msgfmt -o locales/es/LC_MESSAGES/FLTube.mo  locales/es/LC_MESSAGES/FLTube.po
	cp locales/es/LC_MESSAGES/FLTube.mo $(PREFIX)$(LOCALE_INSTALL_DIR)/es/LC_MESSAGES/

	msgfmt -o locales/es/LC_MESSAGES/$(INSTALL_YTDLP_SCRIPTNAME).mo  locales/es/LC_MESSAGES/$(INSTALL_YTDLP_SCRIPTNAME).po
	cp locales/es/LC_MESSAGES/$(INSTALL_YTDLP_SCRIPTNAME).mo $(PREFIX)$(LOCALE_INSTALL_DIR)/es/LC_MESSAGES/

	mkdir -p $(PREFIX)/usr/local/etc/fltube
	cp fltube.conf $(PREFIX)/usr/local/etc/fltube/fltube.conf

	mkdir -p $(PREFIX)/usr/local/bin/
	mv $(TARGET) $(PREFIX)/usr/local/bin/
	cp $(SCRIPTS_DIR)/install_yt-dlp.sh $(PREFIX)/usr/local/bin/

	mkdir -p $(PREFIX)/usr/local/share/icons/fltube/ $(PREFIX)/usr/local/share/{applications,fltube/resources/img}
	cp resources/desktop/fltube.desktop $(PREFIX)/usr/local/share/applications/
	cp resources/icons/*.png $(PREFIX)/usr/local/share/icons/fltube/
	cp resources/img/*.png $(PREFIX)/usr/local/share/fltube/resources/img/

uninstall:
	rm $(PREFIX)/usr/local/etc/fltube/fltube.conf $(PREFIX)/usr/local/bin/fltube $(PREFIX)/usr/local/bin/install_yt-dlp.sh $(PREFIX)/usr/local/share/applications/fltube.desktop $(PREFIX)/usr/local/share/icons/fltube/*.png $(PREFIX)/usr/local/share/locale/es/LC_MESSAGES/FLTube.mo $(PREFIX)/usr/local/share/locale/es/LC_MESSAGES/install_yt-dlp.mo $(PREFIX)/usr/local/share/fltube/resources/img/*.png
	rmdir $(PREFIX)/usr/local/share/icons/fltube/ $(PREFIX)/usr/local/etc/fltube $(PREFIX)/usr/local/share/{fltube/resources/img,fltube/resources,fltube}

# Extract strings from source and update .po locale files.
po_update:
	xgettext --keyword=_ --keyword=ng_ --language=C++ --from-code=utf-8 --output locales/FLTube.pot src/*.cxx include/*.h
	msgmerge --update locales/es/LC_MESSAGES/FLTube.po  locales/FLTube.pot
	xgettext --keyword=_ --keyword=ng_ --language=Shell --from-code=utf-8 --output locales/$(INSTALL_YTDLP_SCRIPTNAME).pot $(SCRIPTS_DIR)/$(INSTALL_YTDLP_SCRIPTNAME).sh
	msgmerge --update locales/es/LC_MESSAGES/$(INSTALL_YTDLP_SCRIPTNAME).po  locales/$(INSTALL_YTDLP_SCRIPTNAME).pot


DEB_BLD_DIR=/tmp/fltube_deb_build
deb_package: install
	rm -rf $(DEB_BLD_DIR)
	mkdir -p $(DEB_BLD_DIR)/DEBIAN
	mkdir -p $(DEB_BLD_DIR)/tmp
	cp -R $(PREFIX)/usr $(DEB_BLD_DIR)
	sed  's/-REPLACE_ARCH-/$(ARCH_CPU)/g' packaging/debian/control.template | sed 's/-REPLACE_VERSION-/$(FLTUBE_VERSION)/g' > $(DEB_BLD_DIR)/DEBIAN/control
	cp packaging/debian/postinst $(DEB_BLD_DIR)/DEBIAN/
	cp packaging/debian/postrm $(DEB_BLD_DIR)/DEBIAN/
	cp packaging/common/yt-dlp_config $(DEB_BLD_DIR)/tmp/
	chmod +x $(DEB_BLD_DIR)/DEBIAN/postinst
	chmod +x $(DEB_BLD_DIR)/DEBIAN/postrm
	chmod +x $(DEB_BLD_DIR)/usr/local/bin/*
	dpkg-deb --root-owner-group --build $(DEB_BLD_DIR) $(DEB_BLD_DIR)/$(DEB_PACKAGE_NAME)
	@printf "\033[32mPackage built at: $(DEB_BLD_DIR)/$(DEB_PACKAGE_NAME)\033[0m. Install using 'apt install [path_to_pkg]'...\n"


TCZ_BLD_DIR=/tmp/fltube_tcz_build
tcz_package: install
	rm -rf $(TCZ_BLD_DIR) && mkdir -p $(TCZ_BLD_DIR)/build
	cp -R $(PREFIX)/usr $(TCZ_BLD_DIR)/build
	cp -R packaging/tinycore/tce.installed $(TCZ_BLD_DIR)/build/usr/local/
	mkdir $(TCZ_BLD_DIR)/build/tmp/ && cp packaging/common/yt-dlp_config $(TCZ_BLD_DIR)/build/tmp/
# 	Setting permissions according to Chapter 16 of book "Into the Core" (http://tinycorelinux.net/corebook.pdf).
	chown -R tc:staff $(TCZ_BLD_DIR)/build/usr/local/tce.installed/* && chmod +x $(TCZ_BLD_DIR)/build/usr/local/tce.installed/*
#	chown root:staff $(TCZ_BLD_DIR)/usr/local/tce.installed && chmod 755 $(TCZ_BLD_DIR)/usr/local/tce.installed
	mksquashfs $(TCZ_BLD_DIR)/build $(TCZ_BLD_DIR)/fltube.tcz -progress
	cp packaging/tinycore/fltube.tcz.dep $(TCZ_BLD_DIR)
	cp packaging/tinycore/install.sh $(TCZ_BLD_DIR) && chmod +x $(TCZ_BLD_DIR)/install.sh
#	Package tne install_yt-dlp.sh script for installation of yt-dlp and QuickJS during tcz installation process.
	cp scripts/install_yt-dlp.sh $(TCZ_BLD_DIR) && chmod +x $(TCZ_BLD_DIR)/install_yt-dlp.sh
	sed  's/-REPLACE_FLTUBE_VERSION-/$(FLTUBE_VERSION)/g' packaging/tinycore/fltube.tcz.info_TEMPLATE > $(TCZ_BLD_DIR)/fltube.tcz.info
	CURDIR=`pwd`
	cd $(TCZ_BLD_DIR)/build && find usr/ -not -type d > $(TCZ_BLD_DIR)/fltube.tcz.list
	echo "tmp/yt-dlp_config" >> $(TCZ_BLD_DIR)/fltube.tcz.list
	cd $(TCZ_BLD_DIR) && md5sum fltube.tcz > $(TCZ_BLD_DIR)/fltube.tcz.md5.txt
	tar czf $(TCZ_BLD_DIR)/$(TCZ_PACKAGE_NAME) -C $(TCZ_BLD_DIR) fltube.tcz fltube.tcz.dep fltube.tcz.info fltube.tcz.list fltube.tcz.md5.txt install.sh install_yt-dlp.sh
	cd $(CURDIR)
	@printf "\033[32mPackage built at: $(TCZ_BLD_DIR)/$(TCZ_PACKAGE_NAME)\033[0m...\n"


# Rule to compile FLUID files
compile_fluid:
	cd `dirname $(FLUID_PATHFILE)` && fluid -o ../$(SRC_DIR)/FLTube_View.cxx -h ../$(INCLUDE_DIR)/FLTube_View.h -c `basename $(FLUID_PATHFILE)`

# Rule to clean objects and directories during development
clean:
	@if [ -z "$(BUILD_DIR)" ]; then \
		echo "Error: BUILD_DIR must be set before make a clean."; \
		exit 1; \
	fi
	rm -f $(BUILD_DIR)/*.o $(TARGET) 2> /dev/null && rm -rf $(BUILD_DIR)/usr/local $(BUILD_DIR)/usr/ $(BUILD_DIR)/usr/local/etc/fltube $(BUILD_DIR)/usr/local/etc && rmdir $(BUILD_DIR)

.PHONY: all clean build compile_fluid po_update install uninstall deb_package tcz_package
