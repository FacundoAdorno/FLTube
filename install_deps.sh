#!/bin/sh

## Use this script to install all system packages required for compilation of FLTube from source.

# Packages required for Compilation at Debians-based systems.
COMPILATION_DEPS_DEBIAN="libfltk1.3-dev pkg-config libcurl4-openssl-dev g++ python3 gettext wget mplayer ffmpeg libpng-dev zlib1g-dev libjpeg-dev libxrender-dev libxcursor-dev libxfixes-dev libxext-dev libxft-dev libfontconfig1-dev libxinerama-dev libarchive-dev"
# Packages required for Compilation at TinyCoreLinux-based systems.
COMPILATION_DEPS_TINYCORE="make.tcz fluid.tcz pkg-config.tcz gettext.tcz curl-dev.tcz gcc.tcz glibc_base-dev.tcz tcc.tcz  mplayer-cli.tcz ffmpeg4.tcz libEGL.tcz bash.tcz squashfs-tools.tcz libarchive3-dev.tcz"

case "$1" in
    --debian)
        printf "Installing system packages required for compilation at Debian. \033[1m sudo password is required to proceed. Please enter password...\033[0m\n"
        sudo apt install $COMPILATION_DEPS_DEBIAN
        exit 0;
        ;;
    --tinycore)
        echo "Installing system packages required for compilation at TinyCore systems."
        tce-load -wi $COMPILATION_DEPS_TINYCORE
        exit 0;
        ;;
    *)
        echo "USAGE: ./install_deps.sh [--debian | --tinycore]"
        echo "ABOUT:"
        echo "  Use this script to install all system packages required for FLTube compilation and installation from source."
        exit 1
        ;;
esac
