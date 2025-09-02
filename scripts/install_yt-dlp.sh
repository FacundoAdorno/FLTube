#!/bin/bash

#
#  Copyright (C) 2025 - FLtube
#
#  This program is free software: you can redistribute it and/or modify it
#  under the terms of the GNU General Public License, version 3, as published
#  by the Free Software Foundation.
#
#  This program is distributed in the hope that it will be useful, but WITHOUT
#  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
#  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
#  more details.
#

yt_dlp_DOWNLOAD_URL="https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp"
yt_dlp_INSTALL_DIR="$HOME/.local/bin"
## Global booleans flags. Keep in mind this bash convention: 0 is true, and 1 is false.
YES_TO_ALL=1    #Defaults to false
ALTERNATIVE_PATH=1    #Defaults to false

#Configure GNU Gettext variables
export TEXTDOMAIN=install_yt-dlp
#export TEXTDOMAINDIR=/usr/local/share/

# Wildcard variable for localized text in gettext.
LOC_TEXT=

_() {
  gettext "$@"
}

USAGE=$(_ "[ == HELP == ]
    Use this script to INSTALL or UPDATE the 'yt-dlp' tool in your system.
    Advice that this software require that Python 3.X been installed on your
    system (more info at: %s).

USAGE:
    install_yt-dp [options]

OPTIONS:
  -p    Alternative PATH for binary installation/update. It must exists in case of installation.
        Default directory is '%s'.
  -y    Confirm all questions. Avoid interaction with script and accept all.
  -h,   Show this usage help.")

errorExit() {
  local message=$1; local params=("${@:2}")
  printf "\033[31m[ERROR]\033[0m `date +'%F %T'` -- $message\n" $params; exit 1;
}

normalExit() {
  local message=$1; local params=("${@:2}")
  printf "\033[32m[INFO]\033[0m `date +'%F %T'` -- $message\n" $params; exit 0;
}

showInfo() {
  local info=$1; local params=("${@:2}")
  printf "\033[34m[INFO]\033[0m `date +'%F %T'` -- $info\n" $params;
}

## Returns an exit code of "0" if "true", or "1" if "false".
questionAndWait() {
  if [ $YES_TO_ALL -eq 0 ]; then return 0; fi  ## Avoid to question if YES_TO_ALL is true

  local question="$1"
  local params=("${@:2}")
  printf "\033[33m== [QUESTION\!] ==\033[0m $question \033[1m [Y/N] \033[0m\n" $params; read decision;
  if [ $decision == 'Y' -o $decision == 'y' ]; then return 0; else return 1; fi
}

updateYTDLP(){
  $yt_dlp_INSTALL_DIR/yt-dlp -U
}

download_ytdlp() {
  if [ $ALTERNATIVE_PATH -eq 0 -a ! -d $yt_dlp_INSTALL_DIR -a $YES_TO_ALL -ne 0 ]; then
    LOC_TEXT=$(_ "'%s' does not exists. Do you want to create it?")
    questionAndWait "$LOC_TEXT" $yt_dlp_INSTALL_DIR
    if [ $? -eq 0 ]; then mkdir -p $yt_dlp_INSTALL_DIR && LOC_TEXT=$(_ "Directory '%s' created.") && showInfo "$LOC_TEXT" "$yt_dlp_INSTALL_DIR"
      else
        LOC_TEXT=$(_ "You must create '%s' before procced. Aborting installation...")
        errorExit "$LOC_TEXT" "$yt_dlp_INSTALL_DIR"
    fi
  else
    if [ ! -d $yt_dlp_INSTALL_DIR ]; then mkdir -p $yt_dlp_INSTALL_DIR && LOC_TEXT=$(_ "Directory '%s' created.") && showInfo "$LOC_TEXT" "$yt_dlp_INSTALL_DIR"; fi
  fi
  LOC_TEXT=$(_ "Starting 'yt-dlp' DOWNLOAD...")
  showInfo "$LOC_TEXT"
  wget $yt_dlp_DOWNLOAD_URL -O $yt_dlp_INSTALL_DIR/yt-dlp
  chmod a+rx $yt_dlp_INSTALL_DIR/yt-dlp
  LOC_TEXT=$(_ "'yt-dlp' installed at %s/yt-dlp directory.")
  showInfo "$LOC_TEXT" "$yt_dlp_INSTALL_DIR"
}

#Parse parameters
while getopts p:yh opts; do
   case ${opts} in
      p) ### Remove any trailing slash on specified path
         yt_dlp_INSTALL_DIR=`echo $OPTARG | sed 's/\/*$//g'`
         ALTERNATIVE_PATH=0;;
      y) YES_TO_ALL=0;;
      h) printf "$USAGE\n" "https://github.com/yt-dlp/yt-dlp?tab=readme-ov-file#dependencies" "$yt_dlp_INSTALL_DIR"
         exit 0;;
   esac
done


##############################
## ## ## MAIN PROGRAM ## ## ##
##############################
YT_DLP_GLOBAL_SYSTEM_VERSION=`yt-dlp --version 2> /dev/null`
if [ $? -eq 0 ]; then
  LOC_TEXT=$(_ "'yt-dlp' is installed globally at your system %s.
    Maybe is installed as a system package. Is recommended to use this script for download the latest 'yt-dlp' version.
    Do you want to continue and install another 'yt-dlp' binary on your system?")
  questionAndWait "$LOC_TEXT" "$YT_DLP_GLOBAL_SYSTEM_VERSION"
  if [ $? -eq 1 ]; then errorExit "Installation aborted by user choice..."; fi
  #else continue with the installation...
fi


YT_DLP_VERSION=`$yt_dlp_INSTALL_DIR/yt-dlp --version 2> /dev/null`
#YT_DLP_EXIT_CODE="$?"
if [ $? -ne 0 ]; then
  if [ $YES_TO_ALL -eq 0 ]; then
    download_ytdlp
  else
    LOC_TEXT=$(_ "'yt-dlp' is not in your system. Do you want to install at '%s'?")
    questionAndWait "$LOC_TEXT" "$yt_dlp_INSTALL_DIR"
    if [ $? -eq 0 ]; then
      download_ytdlp
    else
      LOC_TEXT=$(_ "Installation aborted by user choice...")
      normalExit $LOC_TEXT
    fi
  fi
else
  ## if yt-dlp exists, do you want to update it?
  if [ $YES_TO_ALL -eq 0 ]; then
    updateYTDLP
  else
    LOC_TEXT=$(_ "Do you want to update 'yt-dlp@%s' to any NEW VERSION (if any)?")
    questionAndWait "$LOC_TEXT" "$YT_DLP_VERSION"
    if [ $? -eq 0 ]; then
      updateYTDLP
    else
      LOC_TEXT=$(_ "yt-dlp update process aborted by user choice...")
      normalExit "$LOC_TEXT"
    fi
  fi
fi
