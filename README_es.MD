# FLTube

Fltube es una aplicación para buscar y reproducir videos desde Youtube. Escrito para [FLTK](https://www.fltk.org/) y potenciado por [yt-dlp](https://github.com/yt-dlp/yt-dlp).

Aunque inicialmente fue desarrollado para la distribución [Spirit-OS](https://spirit-os.sourceforge.io/), puede ser compilado en otras distribuciones modernas que tengan un compilador C++.

### Características
- Aplicación ligera gracias a **C++** y la librería **FLTK** *(soporta FLTK 1.3.x o superior)*.
- Potenciado por **yt-dlp**, una herramienta para descargar videos/audios por línea de comandos, muy rica en características, y desarrollada en Python.
- Con capacidad de búsqueda de video en YouTube usando términos de búsqueda o una URL de YouTube específica.
- Capacidad para recordar los videos vistos y que gustaron, así como funciones para explorar esos videos a través de un "Historial de navegación" y una lista de videos "Me gusta".
- Configuraciones disponibles para cambiar la resolución de transmisión de video: **240p**, **360p** (predeterminado), **480p**, **720p** y **1080p**.
- Localizado al Inglés y Español, pero con potencial futuro a traducirse en otros idiomas. La localización se realiza mediante el uso de [GNU gettext](https://www.gnu.org/software/gettext/).
- Diseñado para bajas resoluciones de pantalla en pantallas pequeñas.
- Por ahora, la aplicación sólo fue probado en GNU/Linux.

![FLtube en Spirit OS!](https://i.postimg.cc/44QyBXH8/fltube-screenshot-5.png "Fltube en Spirit OS")

![FLtube en Debian 13 Trixie!](https://fltube.sourceforge.io/img/screenshot_2.png "Fltube en Debian 13 Trixie")

![FLtube showing Navigation History list!](https://fltube.sourceforge.io/img/screenshot_6.png "FLtube showing 'Navigation History' list")


------------

## Instalar desde un paquete

Se pueden descargar e instalar paquetes para 32 y 64 bits de .deb, así como un paquete de instalación para distribuciones basados en TinyCore (.tcz).
- Descargar desde el [sitio oficial de FLTube](https://fltube.sourceforge.io/).
- Descargar desde los [releases en Gitlab](https://gitlab.com/facuA/fltube/-/releases).

## Compilar e instar desde el código fuente.

Una vez descargado el código fuente (y siempre recomendable mantenerlo versionado con Git), puedes compilarlo utilizando [Make](https://www.gnu.org/software/make/).
> NOTA: Por defecto, los directorios de instalación están bajo `/usr/local`, así que para encontrar los binarios del programa correctamente luego de la instalación, debes asegurarte que `/usr/local/bin` esté en la variable `$PATH` del sistema.

### En FLinux, SpiritOS, o cualquier otra distribución basada en TinyCore

Instalar las dependencias y compilar usando *make*.
```bash
$ tce-load -wi make.tcz fluid.tcz pkg-config.tcz gettext.tcz \ 
    curl-dev.tcz gcc.tcz glibc_base-dev.tcz tcc.tcz mplayer-cli.tcz 
    
# Descarga e instala Python 3.11.
$ wget -q -O /tmp/python3.11.tcz  https://gitlab.com/-/project/74160365/uploads/f286ad3d76aabc21492b31853c76bd0c/python3.11.tcz
$ tce-load -i python3.11.tcz

$ sudo make install

$ fltube        ## Para ejecutar la aplicación.
```

En algunas distribuciones más minimalistas, como [FLinux](https://flinux-distro.sourceforge.io/), debes instalar algunos paquetes extra:
```bash
$ tce-load -wi ffmpeg4.tcz libEGL.tcz bash.tcz squashfs-tools.tcz
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

### Desinstalación

Para desinstalar la aplicación, puedes ejecutar `make uninstall`.

## Modo Desarrollo

Puedes instalar la aplicación en una especie de **modo de desarrollo**, así que en lugar de ejecutar `make install`, puedes hacer lo siguiente:

```bash
$ make PREFIX=./build install    ## Compila los archivos en el directorio relativo "./build"...

$ ./build/usr/local/bin/fltube   ## Ruta relativa para ejecutar la aplicación.
```
Ejecutando *make* de esta manera te permitirá eliminar la aplicación más fácilmente de tu sistema (ya que está todo concentrado en un directorio relativo), además de permitirte realizar tus propias modificaciones al código fuente y probarlas en un entorno de desarrollo controlado.

```bash
$ make clean                    ## Finalmente, ejecutar para eliminar archivos del directorio ./build.
```

### Contribuciones

Si quieres contribuir (reportar problemas, corregir errores, mejorar el código, añadir nuevas funciones o traducir a tu idioma), puedes abrir una hilo en https://gitlab.com/facuA/fltube/-/issues.

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

