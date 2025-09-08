# FLTube

FLTube is an application for search & stream Youtube videos. Written for [FLTK](https://www.fltk.org/) and powered by [yt-dlp](https://github.com/yt-dlp/yt-dlp).

Although initially developed for [Spirit-OS](https://spirit-os.sourceforge.io/) distribution, but can be compiled for other modern distributions with a C++ compiler.

### Features
- A very lighweight app thanks to **C++** and **FLTK** toolkit *(support for FLTK 1.3.x or above)*.
- Powered by **yt-dlp**, a feature-rich command-line audio/video downloader developed in Python.
- With capabilities to search for videos on YouTube using search terms or a specific YouTube URL.
- Localized in English and Spanish, with the potential for future translation into other languages. Localization is done using [GNU gettext](https://www.gnu.org/software/gettext/).
- Designed for low screen resolutions on small screens.
- For now, the app only was tested on GNU/Linux.

![FLtube on Spirit OS!](https://i.postimg.cc/5yKTKdCG/fltube-screenshot-3.png "Fltube on Spirit OS")

![FLtube on Debian 13 Trixie!](https://i.postimg.cc/pdM76Bm2/fltube-screenshot-4.png "Fltube on Debian 13 Trixie")


## Installation

Once downloaded the source code, you can compile it using [Make](https://www.gnu.org/software/make/). 
> NOTE: By default, the installation directories are under `/usr/local`, so to find the binaries correctly after installation, you must ensure that `/usr/local/bin` is on your `$PATH` environment variable.

### On SpiritOS (or any TinyCore-based distro)

Install app dependencies and compile using *make*.

```bash
$ tce-load -wi make.tcz fluid.tcz pkg-config.tcz gettext.tcz \ 
    curl-dev.tcz gcc.tcz glibc_base-dev.tcz tcc.tcz python3.9.tcz mplayer-cli.tcz 

$ sudo make install
```

On some distribution more minimalist, like [FLinux](https://flinux-distro.sourceforge.io/), you must install some extra packages: 
```bash
$ tce-load -wi ffmpeg4.tcz libEGL.tcz bash.tcz
``` 

### On a Debian-based distro

Install app dependencies and compile using *make*.
```bash
$ sudo apt install libfltk1.3-dev pkg-config libcurl4-openssl-dev g++ python3 gettext wget mplayer\
    ffmpeg libpng-dev zlib1g-dev libjpeg-dev libxrender-dev libxcursor-dev libxfixes-dev libxext-dev \
    libxft-dev libfontconfig1-dev libxinerama-dev
    
$ sudo make install

$ fltube        ## To execute app.
```

#### WARNING

The app doesn't have an **uninstall** task yet, so if not sure what you are doing, you can install it in a sort of *"development mode"*, instead of running `make install`:

```bash
$ make PREFIX=./build install

$ ./build/usr/local/bin/fltube   ##To execute app.

$ make clean                    ## For clean ./build directory
```

## *yt-dlp* installation

Is recommended to use the latest version of **yt-dlp**, so we encouraged to use the installation script provided by this app.

Once FLTube is installed, run this command on a terminal to install *yt-dlp*:
```bash
$ install_yt-dlp.sh
```
By default, **yt-dlp** will be installed at `$HOME/.local/bin`, but you can change the target installation directory using the `-p` parameter. Run `install_yt-dlp.sh -h` for more help.

## [!!] TODO [!!]
    
    Complete this README!

## Clarifications

This application is based on an initial concept created by Nicolas Longardi for a package called FLTube, developed for the Spirit-OS distribution. https://gitlab.com/tinydesktoplinux/xpkg/-/tree/f5e29a272fbd27d91c9b17b775ee79f3e3ea420e/fltube

    
## License

This project is licensed under the GNU General Public License v3. For more details, see the LICENSE file.

