# Cutefish Store

A lightweight, fast, and native software center built specifically for **Cutefish Desktop** using **C++20**, **Cutefish Toolkit**, and **AppStream**.

![Cutefish Store Main Window](screenshots/main-window.png)

## ✨ Features
- **Ultra-Lightweight:** Runs with ~15–20 MB RAM and zero heavy background daemons.
- **AppStream & Flathub Integration:** Browse, search, install, remove, and update thousands of native repository packages and Flathub applications.
- **Live Screenshots & Auto-Localization:** Fetches application screenshots on demand and adapts automatically to the system language (English / Turkish).
- **Universal Packaging:** Available as a Debian package (`.deb`) and a portable single-file `.AppImage`.

## 📦 Installation

### Debian / Ubuntu / CutefishOS (`.deb`)
Download the latest `.deb` package from the [Releases](https://://github.com) page and install it:
```bash
sudo apt install ./cutefish-store_0.3.0_amd64.deb
```

### Universal Portable (`AppImage`)
Download `Cutefish-Store-x86_64.AppImage` from [Releases](https://://github.com), make it executable, and run:
```bash
chmod +x Cutefish-Store-x86_64.AppImage
./Cutefish-Store-x86_64.AppImage
```

## 🛠️ Building from Source
```bash
sudo apt install cmake g++ libappstream-dev pkg-config
mkdir build && cd build
cmake ..
make -j\$(nproc)
```
