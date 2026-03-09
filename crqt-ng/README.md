# crqt-ng
crqt-ng - cross-platform open source e-book reader using crengine-ng. It is a fork of the [CoolReader](https://github.com/buggins/coolreader) project.

This repository is created from the official CoolReader repository by removing everything that does not belong to the "cr3qt" using the "git-filter-repo" utility.

## Description
crqt-ng is an e-book reader.
In fact, it is a [Qt](https://www.qt.io/) frontend for the [crengine-ng](https://gitlab.com/coolreader-ng/crengine-ng) library.

Supported platforms: Windows, Linux, MacOS. Basically all platforms that are supported by crengine-ng, Qt and cmake.

Supported e-book formats through the use of the crengine-ng library: fb2, fb3 (incomplete), epub (non-DRM), doc, docx, odt, rtf, pdb, mobi (non-DRM), txt, html, Markdown, chm, tcr.

Main functions:
* Ability to display 2 pages at the same time
* Displaying and Navigating Book Contents
* Text search
* Text selection
* Copy the selected text to the clipboard
* Word hyphenation using hyphenation dictionaries
* Most complete support for FB2 - styles, tables, footnotes at the bottom of the page
* Extensive font rendering capabilities: use of ligatures, kerning, hinting option selection, floating punctuation, simultaneous use of several fonts, including fallback fonts
* Detection of the main font incompatible with the language of the book
* Reading books directly from a ZIP archive
* TXT auto reformat, automatic encoding recognition
* Flexible styling with CSS files
* Background pictures, textures, or solid background
* HiDPI support
* Multi-tabbed interface

## Visuals
#### Main window (KDE, Windows, MacOS)

These screenshots use the Noto Serif font.

![Main window in KDE](https://gitlab.com/coolreader-ng/crqt-ng/-/wikis/uploads/1aab10e451b10d8f8c60f813b0c3be21/1101-mainwnd-kde.png "Main window in KDE")

![Main window in Windows](https://gitlab.com/coolreader-ng/crqt-ng/-/wikis/uploads/973e89264d1e1c38bc6113ca83449ef9/1201-mainwnd-win32.png "Main window in Windows")

![Main window in MacOS](https://gitlab.com/coolreader-ng/crqt-ng/-/wikis/uploads/3527a305b8b157f0f2b07ce5e05a894c/1301-mainwnd-macos.png "Main window in MacOS")

## Installation
You can install this program on Linux using flatpak.
To do this, you first need to install and configure flatpak and flathub.
How to do this is explained in this [guide](https://flatpak.org/setup/) or in [this](https://docs.flatpak.org/en/latest/using-flatpak.html).

Then you can install the program from the link <https://dl.flathub.org/repo/appstream/io.gitlab.coolreader_ng.crqt-ng.flatpakref>.

Or using command:
```shell
flatpak install flathub io.gitlab.coolreader_ng.crqt-ng
```
That's all.

Or you can compile from source:
1. To install the program, make sure that all dependencies are installed: Qt framework and crengine-ng library.

   It is best to use your Linux distribution's package manager to install Qt. Otherwise, download the installation package from https://www.qt.io/. When installing using your package manager, remember to install the "-dev" variant of the package, for example for Ubuntu use the following command:
```
$ sudo apt install qtbase5-dev qttools5-dev
```

On windows msys2 and 'pacman' package manager can be used:
```
$ pacman -S mingw-w64-x86_64-qt5
```

To install crengine-ng follow the instructions https://gitlab.com/coolreader-ng/crengine-ng#installation

2. Compile the program using cmake, for example (we are already in the top source directory):
```
$ mkdir ../crqt-ng-build
$ cd ../crqt-ng-build
$ cmake -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE=Release ../crqt-ng
$ make -j10 VERBOSE=1
$ make install

```

## Contributing
See [CONTRIBUTING.md](CONTRIBUTING.md) file.

## Authors and acknowledgment
A list of authors and contributors can be found in the AUTHORS file.

## License
This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.
