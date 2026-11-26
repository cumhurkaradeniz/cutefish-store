#pragma once
#include <appstream.h>
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <regex>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <mutex>
#include <vector>

namespace fs = std::filesystem;

inline bool isTurkishLocale() {
    const char* lang = std::getenv("LC_ALL");
    if (!lang || !*lang) lang = std::getenv("LC_MESSAGES");
    if (!lang || !*lang) lang = std::getenv("LANG");
    return lang && std::string(lang).rfind("tr", 0) == 0;
}

inline std::string trText(const std::string& tr, const std::string& en) {
    return isTurkishLocale() ? tr : en;
}

struct StoreApp {
    std::string title, summary, description, developer, license, category, icon, pkgName, screenshotUrl;
    bool installed = false, isUpdate = false, isFlatpak = false;
    int popularity = 0;
};

inline std::string stripHtml(const char* raw) {
    if (!raw) return "";
    std::string s = std::regex_replace(std::string(raw), std::regex("<[^>]*>"), " ");
    s = std::regex_replace(s, std::regex("\\s+"), " ");
    if (!s.empty() && s.front() == ' ') s.erase(0, 1);
    return s;
}

inline std::string autoTranslateIfTurkish(const std::string& text) {
    static std::unordered_map<std::string, std::string> trCache;
    static std::mutex trMtx;
    { std::lock_guard<std::mutex> lk(trMtx); if (trCache.count(text)) return trCache[text]; }
    if (!isTurkishLocale() || text.empty()) return text;
    for (const char* tr : {"ş", "ğ", "ü", "ö", "ç", "ı", "İ", "Ş", "Ğ", "Ü", "Ö", "Ç"})
        if (text.find(tr) != std::string::npos) return text;
    std::string clean;
    for (char ch : text) clean += (ch == '"' || ch == '\'' || ch == '$' || ch == '`') ? ' ' : ch;
    if (clean.size() > 400) clean = clean.substr(0, 400);
    std::string cmd = "trans -b :tr \"" + clean + "\" 2>/dev/null";
    FILE* p = popen(cmd.c_str(), "r");
    if (!p) return text;
    std::array<char, 1024> buf{}; std::string out;
    while (fgets(buf.data(), buf.size(), p)) out += buf.data();
    pclose(p);
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
    std::string res = out.empty() ? text : out; { std::lock_guard<std::mutex> lk(trMtx); trCache[text] = res; } return res;
}
inline std::string resolveAppIcon(const std::string& rawIcon) {
    if (rawIcon.empty()) return "preferences-system";
    if (fs::exists(rawIcon)) return rawIcon;
    static std::unordered_map<std::string, std::string> iconMap;
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        for (const auto& base : {"/usr/share/icons/Crule", "/usr/share/icons/Cutefish", "/usr/share/icons/hicolor", "/usr/share/icons", "/usr/share/pixmaps", "/var/lib/swcatalog/icons", "/var/cache/swcatalog/icons", "/var/lib/swcatalog/icons", "/var/lib/app-info/icons"}) {
            std::error_code ec;
            if (!fs::exists(base, ec)) continue;
            for (const auto& entry : fs::recursive_directory_iterator(base, fs::directory_options::skip_permission_denied, ec)) {
                if (!entry.is_regular_file(ec)) continue;
                auto ext = entry.path().extension().string();
                if (ext == ".png" || ext == ".svg" || ext == ".xpm") {
                    std::string stem = entry.path().stem().string();
                    std::string full = entry.path().string();
                    if (!iconMap.count(stem) || (ext == ".png" && (full.find("128") != std::string::npos || full.find("256") != std::string::npos || full.find("64") != std::string::npos || full.find("/pixmaps/") != std::string::npos))) {
                        iconMap[stem] = full;
                    }
                    auto uPos = stem.find('_');
                    if (uPos != std::string::npos) {
                        std::string shortStem = stem.substr(uPos + 1);
                        if (!iconMap.count(shortStem) || ext == ".png") iconMap[shortStem] = full;
                    }
                }
            }
        }
    }
    std::string stem = fs::path(rawIcon).stem().string();
    if (iconMap.count(stem)) return iconMap[stem];
    auto pos = stem.find('_');
    if (pos != std::string::npos) {
        std::string clean = stem.substr(pos + 1);
        if (iconMap.count(clean)) return iconMap[clean];
    }
    return "preferences-system";
}

inline std::unordered_set<std::string> getInstalledPackages() {
    std::unordered_set<std::string> set;
    FILE* pipe = popen("dpkg-query -W -f='${Package}\n' 2>/dev/null", "r");
    if (!pipe) return set;
    std::array<char, 256> buf{};
    while (fgets(buf.data(), buf.size(), pipe)) {
        std::string s(buf.data());
        if (!s.empty() && s.back() == '\n') s.pop_back();
        set.insert(s);
    }
    pclose(pipe);
    return set;
}

inline std::vector<StoreApp> getUpgradablePackages() {
    std::vector<StoreApp> list;
    FILE* pipe = popen("apt list --upgradable 2>/dev/null", "r");
    if (!pipe) return list;
    std::array<char, 512> buf{};
    while (fgets(buf.data(), buf.size(), pipe)) {
        std::string line(buf.data());
        auto slash = line.find('/');
        if (slash == std::string::npos) continue;
        std::string pkg = line.substr(0, slash), ver = line.substr(slash + 1);
        if (!ver.empty() && ver.back() == '\n') ver.pop_back();
        list.push_back({pkg, ver, trText("Sistem paketi güncellemesi mevcut.", "System package update available."), "Debian", "Open Source", "Guncellemeler", "view-refresh", pkg, "", true, true, 0});
    }
    pclose(pipe);
    if (FILE* fp = popen("flatpak remote-ls --updates --columns=name,application 2>/dev/null", "r")) {
        std::array<char, 512> b{};
        while (fgets(b.data(), b.size(), fp)) {
            std::string line(b.data());
            while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();
            auto t = line.find('\t');
            if (t != std::string::npos) {
                std::string n = line.substr(0, t), id = line.substr(t + 1);
                list.push_back({n + " (Flathub)", "Flathub Update", trText("Flathub uygulaması için yeni sürüm mevcut.", "New Flathub update available."), "Flathub", "Flatpak", "Guncellemeler", "view-refresh", id, "", true, true, true, 0});
            }
        }
        pclose(fp);
    }
    return list;
}

inline void appendFlatpakApps(std::vector<StoreApp>& apps) {
    if (std::system("command -v flatpak >/dev/null 2>&1") != 0) return;
    std::unordered_set<std::string> instFp;
    if (FILE* p = popen("flatpak list --app --columns=application 2>/dev/null", "r")) {
        std::array<char, 256> b{};
        while (fgets(b.data(), b.size(), p)) {
            std::string s(b.data());
            while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
            if (!s.empty()) instFp.insert(s);
        }
        pclose(p);
    }
    if (FILE* p = popen("flatpak remote-ls --app --columns=name,application,description flathub 2>/dev/null | head -n 400", "r")) {
        std::array<char, 1024> b{};
        while (fgets(b.data(), b.size(), p)) {
            std::string line(b.data());
            while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();
            auto t1 = line.find('\t');
            if (t1 == std::string::npos) continue;
            auto t2 = line.find('\t', t1 + 1);
            std::string name = line.substr(0, t1);
            std::string appId = (t2 == std::string::npos) ? line.substr(t1 + 1) : line.substr(t1 + 1, t2 - t1 - 1);
            std::string desc = (t2 == std::string::npos) ? trText("Flathub Uygulaması", "Flathub Application") : line.substr(t2 + 1);
            if (name.empty() || appId.empty()) continue;
            apps.push_back({name + " (Flathub)", desc, desc, "Flathub", "Flatpak", "Kesfet", resolveAppIcon(appId), appId, "", instFp.count(appId) > 0, false, true, 10});
        }
        pclose(p);
    }
}

inline void appendSystemInstalledApps(std::vector<StoreApp>& apps) {
    std::unordered_set<std::string> existingPkgs, existingTitles;
    for (const auto& a : apps) {
        if (!a.pkgName.empty()) existingPkgs.insert(a.pkgName);
        if (!a.title.empty()) existingTitles.insert(a.title);
    }
    const char* cmd = "python3 /usr/share/cutefish-store/scan_apps.py 2>/dev/null";

    if (FILE* p = popen(cmd, "r")) {
        char buf[2048];
        while (fgets(buf, sizeof(buf), p)) {
            std::string line(buf);
            while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();
            size_t p1 = line.find('\t');
            size_t p2 = (p1 != std::string::npos) ? line.find('\t', p1 + 1) : std::string::npos;
            size_t p3 = (p2 != std::string::npos) ? line.find('\t', p2 + 1) : std::string::npos;
            if (p3 == std::string::npos) continue;
            std::string title = line.substr(0, p1);
            std::string summary = line.substr(p1 + 1, p2 - p1 - 1);
            std::string icon = line.substr(p2 + 1, p3 - p2 - 1);
            std::string pkg = line.substr(p3 + 1);
            if (existingPkgs.count(pkg) || existingTitles.count(title)) continue;
            existingPkgs.insert(pkg);
            existingTitles.insert(title);
            apps.push_back({title, summary, summary, trText("Sistem Uygulaması", "System Application"), "Open Source", "Kesfet", resolveAppIcon(icon), pkg, "", true, false, false, 0});
        }
        pclose(p);
    }
}

inline std::vector<StoreApp> loadSystemCatalog() {
    std::vector<StoreApp> apps;
    auto installedSet = getInstalledPackages();
    AsPool* pool = as_pool_new();
    as_pool_set_locale(pool, isTurkishLocale() ? "tr_TR" : "en_US");
    as_pool_load(pool, nullptr, nullptr);
    AsComponentBox* cbox = as_pool_get_components(pool);
    guint len = as_component_box_len(cbox);
    static const std::unordered_set<std::string> popular = {"vlc", "gimp", "obs-studio", "blender", "inkscape", "libreoffice-writer", "qbittorrent", "telegram-desktop", "chromium", "firefox-esr", "audacity", "kdenlive", "krita"};

    for (guint i = 0; i < len; ++i) {
        AsComponent* comp = as_component_box_index(cbox, i);
        if (as_component_get_kind(comp) != AS_COMPONENT_KIND_DESKTOP_APP) continue;
        const gchar* name = as_component_get_name(comp);
        const gchar* summary = as_component_get_summary(comp);
        gchar** pkgs = as_component_get_pkgnames(comp);
        if (!name || !summary || !pkgs || !(*pkgs)) continue;

        std::string pkg = *pkgs;
        std::string desc = stripHtml(as_component_get_description(comp));
        if (desc.empty()) desc = summary;
        AsDeveloper* devObj = as_component_get_developer(comp);
        std::string dev = (devObj && as_developer_get_name(devObj)) ? as_developer_get_name(devObj) : trText("Açık Kaynak Topluluğu", "Open Source Community");
        const gchar* licPtr = as_component_get_project_license(comp);
        std::string lic = licPtr ? licPtr : "Open Source";

        std::string cat = "Kesfet";
        if (as_component_has_category(comp, "Game")) cat = "Oyun";
        else if (as_component_has_category(comp, "Office") || as_component_has_category(comp, "Graphics") || as_component_has_category(comp, "Development")) cat = "Uretkenlik";

        std::string iconName = "preferences-system";
        GPtrArray* icons = as_component_get_icons(comp);
        if (icons && icons->len > 0) {
            for (guint j = 0; j < icons->len; ++j) {
                AsIcon* ic = static_cast<AsIcon*>(g_ptr_array_index(icons, j));
                if (as_icon_get_kind(ic) == AS_ICON_KIND_CACHED && as_icon_get_filename(ic)) { iconName = as_icon_get_filename(ic); break; }
                if (as_icon_get_name(ic)) iconName = as_icon_get_name(ic);
            }
        }
        std::string shotUrl;
        GPtrArray* shots = as_component_get_screenshots_all(comp);
        if (shots && shots->len > 0) {
            AsScreenshot* sc = static_cast<AsScreenshot*>(g_ptr_array_index(shots, 0));
            GPtrArray* imgs = as_screenshot_get_images(sc);
            if (imgs && imgs->len > 0) {
                AsImage* im = static_cast<AsImage*>(g_ptr_array_index(imgs, 0));
                if (as_image_get_url(im)) shotUrl = as_image_get_url(im);
            }
        }
        apps.push_back({name, summary, desc, dev, lic, cat, resolveAppIcon(iconName), pkg, shotUrl, installedSet.count(pkg) > 0, false, false, popular.count(pkg) ? 100 : 0});
    }
    std::sort(apps.begin(), apps.end(), [](const StoreApp& a, const StoreApp& b) {
        if (a.popularity != b.popularity) return a.popularity > b.popularity;
        return a.title < b.title;
    });
    g_object_unref(cbox); g_object_unref(pool);
    appendFlatpakApps(apps);
    appendSystemInstalledApps(apps);
    return apps;
}
