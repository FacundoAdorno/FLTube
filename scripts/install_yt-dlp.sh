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
yt_dlp_INSTALL_DIR="$HOME/.local/bin/"
YES_TO_ALL=0
QUIET=0

USAGE=$(cat <<-ENDHELP
[ == HELP == ] Use this script to INSTALL or UPDATE the 'yt-dlp' tool in your system.
               Advice that this software require that Python 3.X been installed on your
               system (more info at: https://github.com/yt-dlp/yt-dlp?tab=readme-ov-file#dependencies).

USAGE:
    install_yt-dp [options]

OPTIONS:
  -y    Confirm all questions. Avoid interaction with script and accept all.
  -h,   Show this usage help.
ENDHELP
)

download_ytdlp() {
  wget $yt_dlp_DOWNLOAD_URL -O $yt_dlp_INSTALL_DIR/yt-dlp
  chmod a+rx $yt_dlp_INSTALL_DIR/yt-dlp
  echo "[INFO] 'yt-dlp' installed at $yt_dlp_INSTALL_DIR/yt-dlp directory."
}

#Parse parameters
while getopts yh opts; do
   case ${opts} in
      y) YES_TO_ALL=1;;
      h) echo "$USAGE" && exit 0;;
   esac
done


YT_DLP_VERSION=`yt-dlp --version 2> /dev/null`
#YT_DLP_EXIT_CODE="$?"
if [ $? -ne 0 ]; then
  if [ $YES_TO_ALL -eq 1 ]; then
    download_ytdlp
  else
    echo "yt-dlp is not in your system. Do you want to install at $yt_dlp_INSTALL_DIR ? [Y/N]"
    read decision
    if [ $decision == 'Y' -o $decision == 'y' ]; then
      download_ytdlp
    else
      echo "[INFO] Installation aborted by user choice..."
    fi
  fi
else
  ## if yt-dlp exists, do you want to update it?
  if [ $YES_TO_ALL -eq 1 ]; then
    yt-dlp -U
  else
    echo "Do you want to update 'yt-dlp@$YT_DLP_VERSION' to any NEW VERSION (if any)? [Y/N]"
    read decision
    if [ $decision == 'Y' -o $decision == 'y' ]; then
      yt-dlp -U
    else
      echo "[INFO] yt-dlp update process aborted by user choice..."
    fi
  fi
fi

echo "Done!"
