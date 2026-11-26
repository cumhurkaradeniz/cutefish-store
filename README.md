# Cutefish Store

Lightweight, fast, and multi-language AppStream & Flathub software store built with C++20 and Cutefish Toolkit.

## Features
- Native AppStream catalog & live screenshot previews
- Flathub / Flatpak integration (list, install, remove, update)
- Automatic system locale support (Turkish / English)
- Debian package (`.deb`) and universal portable `.AppImage` support

## Build
```bash
mkdir build && cd build
cmake ..
make -j\$(nproc)
```
