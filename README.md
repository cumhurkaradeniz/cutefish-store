# Cutefish Store (v1.3.1)

CutefishOS ve Debian tabanlı Linux dağıtımları için geliştirilmiş; **C++ / SDL2** tabanlı hızlı, hafif ve modern uygulama mağazası.

## v1.3.1 Güncelleme Notları
* **Tam Sistem Uygulama Taraması (`scan_apps.py`):** Yüklü uygulamalar sekmesi artık yalnızca katalogdaki sınırlı uygulamaları değil, sistemde kurulu olan tüm APT, Flatpak ve masaüstü (`.desktop`) uygulamalarını eksiksiz listeler.
* **Gelişmiş İkon Çözümleyici (`resolveAppIcon`):** Hem **Keşfet** (2100+ uygulama) hem de **Yüklü** sekmesindeki uygulamaların ikonları sistem tema dizinleri (`Crule`, `Cutefish`, `hicolor`, `pixmaps`) ve AppStream önbelleğinden (`/var/lib/swcatalog/icons`) otomatik eşleştirilir.
* **Otomatik AppStream İkon Önbelleği:** `.deb` paketi kurulumunda APT yapılandırması (`/etc/apt/apt.conf.d/99appstream-icons`) ve `appstreamcli refresh` otomatik etkinleştirilerek yeni kurulumlarda uygulama ikonlarının eksiksiz gelmesi sağlanır.

## Kurulum (`cutefish-store_1.3.1_amd64.deb`)
```bash
sudo apt install ./cutefish-store_1.3.1_amd64.deb
```

## Kaynak Koddan Derleme
```bash
mkdir -p build && cd build
cmake ..
make -j\$(nproc)
sudo cp cutefish-store /usr/bin/cutefish-store
```
