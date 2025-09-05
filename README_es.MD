# FLTube

Fltube es una aplicación para buscar y reproducir videos desde Youtube, con opciones de descarga de videos hasta una resolución de 1080p. Escrito para [FLTK](https://www.fltk.org/) y potenciado por [yt-dlp](https://github.com/yt-dlp/yt-dlp).

Aunque inicialmente fue desarrollado para la distribución [Spirit-OS](https://spirit-os.sourceforge.io/), puede ser compilado en otras distribuciones modernas que tengan un compilador C++.

### Características
- Aplicación ligera gracias a **C++** y la librería **FLTK** *(soporta FLTK 1.3.x o superior)*.
- Potenciado por **yt-dlp**, una herramienta para descargar videos/audios por línea de comandos, muy rica en características, y desarrollada en Python.
- Con capacidad de búsqueda de video en YouTube usando términos de búsqueda o una URL de YouTube específica. Además, se puede descargar los videos en cualquier resolución disponible desde los 240p hasta los 1080p.
- Localizado al Inglés y Español, pero con potencial futuro a traducirse en otros idiomas. La localización se realiza mediante el uso de [GNU gettext](https://www.gnu.org/software/gettext/).
- Todavía está en **fase alpha**, pero es utilizable. Por ahora, sólo fue probado en GNU/Linux.

![FLtube on Spirit OS!](https://i.postimg.cc/zXyrVV4S/fltube-screenshot-1.png "Fltube on Spirit OS")

![FLtube on Debian 13 Trixie!](https://i.postimg.cc/pdVFvCgX/fltube-demo-3.png "Fltube on Debian 13 Trixie")


=====================================================

## Installation

Una vez descargado el código fuente (y siempre recomendable mantenerlo versionado con Git), puedes compilarlo utilizando [Make](https://www.gnu.org/software/make/).
> NOTA: Por defecto, los directorios de instalación están bajo `/usr/local`, así que para encontrar los binarios del programa correctamente luego de la instalación, debes asegurarte que `/usr/local/bin` esté en la variable `$PATH` del sistema.

### En SpiritOS (o cualquier otra distribución basada en TinyCore)

Instalar las dependencias y compilar usando *make*.
```bash
$ tce-load -wi make.tcz fluid.tcz pkg-config.tcz gettext.tcz \ 
    curl-dev.tcz gcc.tcz glibc_base-dev.tcz tcc.tcz python3.9.tcz mplayer-cli.tcz 

$ sudo make install

$ fltube        ## Para ejecutar la aplicación.
```

En algunas distribuciones más minimalistas, como [FLinux](https://flinux-distro.sourceforge.io/), debes instalar algunos paquetes extra:
```bash
$ tce-load -wi ffmpeg4.tcz libEGL.tcz bash.tcz
``` 

### En distribuciones basadas en Debian

Instalar las dependencias y compilar usando *make*.
```bash
$ sudo apt install libfltk1.3-dev pkg-config libcurl4-openssl-dev g++ python3 gettext wget mplayer\
    ffmpeg libpng-dev zlib1g-dev libjpeg-dev libxrender-dev libxcursor-dev libxfixes-dev libxext-dev \
    libxft-dev libfontconfig1-dev libxinerama-dev
    
$ sudo make install

$ fltube        ## Para ejecutar la aplicación.
```

#### ADVERTENCIA

La aplicación aún no tiene una tarea de **desinstalación**, así que no estás seguro de lo que estás haciendo, puedes instalarla en una especie de **modo de desarrollo**, en lugar de ejecutar `make install`:

```bash
$ make PREFIX=./build install    ## Instala los archivos en el directorio relativo "./build"...

$ ./build/usr/local/bin/fltube   ## Para ejecutar la aplicación.

$ make clean                    ## Para eliminar archivos del directorio ./build.
```

## Instalación de *yt-dlp*

Es recomendable utilizar la última versión de **yt-dlp**, por lo que animamos utilizar el script de instalación distribuido en esta aplicación.

Una vez que FLTube es instalado, ejecuta este comando en una terminal para instalar *yt-dlp*:
```bash
$ install_yt-dlp.sh
```

Por defecto, **yt-dlp** se instalará en `$HOME/.local/bin`, pero puedes cambiar el directorio de instalación utilizando el parámetro `-p`. Ejecuta `install_yt-dlp.sh -h` para mostrar más ayuda sobre ésto.


## [!!] TODO [!!]
    
    Completar este README!

## Aclaraciones

Esta aplicación está basada en un concepto inicial desarrollado por Nicolás Longardi para un paquete llamado *FLTube*, desarrollado para la distribución Spirit-OS. https://gitlab.com/tinydesktoplinux/xpkg/-/tree/f5e29a272fbd27d91c9b17b775ee79f3e3ea420e/fltube

    
## Licencia

Este proyecto está licenciado bajo la Licencia Pública General GNU v3. Para más detalles, consulte el archivo LICENSE.

