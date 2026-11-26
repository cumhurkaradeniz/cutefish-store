#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include "ui/application.h"
#include "ui/window.h"
#include "ui/window_header.h"
#include "ui/layout.h"
#include "ui/label.h"
#include "ui/icon_image.h"
#include "ui/image_view.h"
#include "ui/scroll_view.h"
#include "ui/search_field.h"
#include "ui/progress_bar.h"
#include "ui/painter.h"
#include "ui/toast.h"
#include "cutefish/packagekit/installer.h"
#include "cutefish/packagekit/updates.h"
#include "catalog.h"

using namespace cutefish;

static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

class SidebarItem : public ui::View {
public:
    bool active = false; std::function<void()> onClick; ui::IconImage* ico; ui::Label* lbl;
    SidebarItem(std::string t, std::string ic) {
        ico = addChild<ui::IconImage>(std::move(ic), 18); ico->setFallbackName("preferences-system");
        lbl = addChild<ui::Label>(std::move(t)); lbl->setFontSize(13.0);
    }
    void setActive(bool a) { active = a; lbl->setWeight(a ? 620 : 450); lbl->setColor(a ? ui::Color{0, 122, 255, 255} : ui::Color{90, 90, 95, 255}); requestPaint(); }
    bool acceptsPointer() const override { return true; }
    ui::SizeF sizeHint() const override { return {195.0, 36.0}; }
protected:
    void layout() override { ico->setGeometry({12, 9, 18, 18}); lbl->setGeometry({38, 8, geometry().width - 44, 20}); }
    void pointerRelease(const ui::PointerEvent&) override { if (onClick) onClick(); }
    void paint(ui::Painter& p) override { if (active) p.fillRoundedRect(localRect(), 8.0, ui::Color{0, 122, 255, 32}); }
};

class AppCard : public ui::View {
public:
    ui::IconImage* icon; ui::Label *titleLbl, *descLbl, *actionLbl; StoreApp current; bool emptySlot = false;
    std::function<void(const StoreApp&)> onSelect;
    AppCard() {
        icon = addChild<ui::IconImage>("preferences-system", 44); icon->setFallbackName("preferences-system");
        titleLbl = addChild<ui::Label>(""); titleLbl->setWeight(600); titleLbl->setFontSize(13.5);
        descLbl = addChild<ui::Label>(""); descLbl->setFontSize(11.5);
        actionLbl = addChild<ui::Label>(""); actionLbl->setWeight(600); actionLbl->setFontSize(12.0);
    }
    void setApp(const StoreApp& app) {
        emptySlot = false; current = app;
        icon->setVisible(true); titleLbl->setVisible(true); descLbl->setVisible(true); actionLbl->setVisible(true);
        icon->setName(app.icon); icon->requestPaint(); titleLbl->setText(app.title); descLbl->setText(app.summary);
        if (app.isUpdate) {
            actionLbl->setText(trText("Güncelle", "Update"));
            actionLbl->setColor(ui::Color{234, 88, 12, 255});
        } else {
            actionLbl->setText(app.installed ? trText("Kurulu", "Installed") : trText("Yükle", "Install"));
            actionLbl->setColor(app.installed ? ui::Color{22, 163, 74, 255} : ui::Color{0, 122, 255, 255});
        }
        requestPaint();
    }
    void setEmpty() { emptySlot = true; icon->setVisible(false); titleLbl->setVisible(false); descLbl->setVisible(false); actionLbl->setVisible(false); requestPaint(); }
    bool acceptsPointer() const override { return !emptySlot; }
    ui::SizeF sizeHint() const override { return {320.0, 78.0}; }
protected:
    void layout() override {
        double w = geometry().width; icon->setGeometry({14, 17, 44, 44});
        titleLbl->setGeometry({70, 16, w - 160, 20}); descLbl->setGeometry({70, 38, w - 160, 22}); actionLbl->setGeometry({w - 78, 28, 64, 22});
    }
    void pointerRelease(const ui::PointerEvent&) override { if (!emptySlot && onSelect) onSelect(current); }
    void paint(ui::Painter& p) override {
        if (emptySlot) return;
        p.fillRoundedRect(localRect(), 12.0, ui::Color{128, 128, 128, 18}); p.strokeRoundedRect(localRect(), 12.0, ui::Color{128, 128, 128, 40}, 1);
        ui::Color badgeBg = current.isUpdate ? ui::Color{234, 88, 12, 28} : (current.installed ? ui::Color{22, 163, 74, 28} : ui::Color{0, 122, 255, 28});
        p.fillRoundedRect({geometry().width - 86, 24, 72, 28}, 14.0, badgeBg);
    }
};
class PillAction : public ui::View {
public:
    ui::Color bg{0, 122, 255, 255}; ui::Label* lbl; std::function<void()> onClick;
    PillAction(std::string t) { lbl = addChild<ui::Label>(std::move(t)); lbl->setWeight(600); lbl->setFontSize(13.0); lbl->setColor(ui::Color{255, 255, 255, 255}); }
    void setStyle(std::string t, ui::Color c) { bg = c; lbl->setText(std::move(t)); requestLayout(); requestPaint(); }
    bool acceptsPointer() const override { return true; }
    ui::SizeF sizeHint() const override { return {130.0, 36.0}; }
protected:
    void layout() override { auto s = lbl->sizeHint(); lbl->setGeometry({(geometry().width - s.width) / 2.0, (geometry().height - s.height) / 2.0, s.width, s.height}); }
    void pointerRelease(const ui::PointerEvent&) override { if (onClick) onClick(); }
    void paint(ui::Painter& p) override { p.fillRoundedRect(localRect(), 18.0, bg); }
};

int main(int argc, char** argv) {
    ui::Application app(argc, argv);
    ui::Window window; window.setTitle(trText("Mağaza", "Store"));
    window.resize({1040, 680}); window.setMinimumSize({840, 560});
    ui::Toast toast(&window); packagekit::Installer installer; packagekit::Updates updater;
    auto allApps = loadSystemCatalog(); auto upgradableApps = getUpgradablePackages(); StoreApp selectedApp;

    auto root = std::make_unique<ui::Column>(); root->setMargins({12, 8, 12, 12}); root->setSpacing(10);
    auto* header = root->addChild<ui::WindowHeader>(40); header->setLayoutHints({.fillWidth = true}); header->setButtonsCentered(true);
    auto* bodyRow = root->addChild<ui::Row>(); bodyRow->setLayoutHints({.fillWidth = true, .fillHeight = true}); bodyRow->setSpacing(18);
    auto* sidebar = bodyRow->addChild<ui::Column>(); sidebar->setLayoutHints({.fillHeight = true, .preferredWidth = 205}); sidebar->setSpacing(6);
    auto* search = sidebar->addChild<ui::SearchField>(trText("Ara (" + std::to_string(allApps.size()) + " uygulama)...", "Search (" + std::to_string(allApps.size()) + " apps)..."));
    search->setLayoutHints({.fillWidth = true});

    struct TabDef { std::string id, label, icon; };
    std::vector<TabDef> tabs = {
        {"Kesfet", trText("Keşfet", "Discover"), "cf-cat-discover"},
        {"Uretkenlik", trText("Üretkenlik", "Productivity"), "cf-cat-productivity"},
        {"Oyun", trText("Oyun", "Games"), "cf-cat-games"},
        {"Yuklu", trText("Yüklü", "Installed"), "cf-cat-installed"},
        {"Guncellemeler", trText("Güncellemeler", "Updates") + (upgradableApps.empty() ? "" : " (" + std::to_string(upgradableApps.size()) + ")"), "cf-cat-updates"},
        {"Hakkinda", trText("Hakkında", "About"), "cf-cat-about"}
    };
    std::vector<SidebarItem*> sideItems;
    for (const auto& t : tabs) sideItems.push_back(sidebar->addChild<SidebarItem>(t.label, t.icon));
    sidebar->addStretch();

    auto* content = bodyRow->addChild<ui::Column>(); content->setLayoutHints({.fillWidth = true, .fillHeight = true}); content->setSpacing(14);
    auto* topBar = content->addChild<ui::Row>(); topBar->setLayoutHints({.fillWidth = true}); topBar->setSpacing(10);
    auto* pageTitle = topBar->addChild<ui::Label>(tabs[0].label); pageTitle->setWeight(650); pageTitle->setFontSize(20.0);
    topBar->addStretch();
    auto* updateAllBtn = topBar->addChild<PillAction>(trText("Tümünü Güncelle", "Update All"));
    updateAllBtn->setStyle(trText("Tümünü Güncelle", "Update All"), ui::Color{22, 163, 74, 255}); updateAllBtn->setVisible(false);
    auto* prevBtn = topBar->addChild<PillAction>(""); prevBtn->setVisible(false);
    auto* pageInfo = topBar->addChild<ui::Label>(""); pageInfo->setVisible(false);
    auto* nextBtn = topBar->addChild<PillAction>(""); nextBtn->setVisible(false);
    // HAKKINDA PANELİ (Anonim & Çok Dilli)
    auto* aboutBox = content->addChild<ui::Column>(); aboutBox->setLayoutHints({.fillWidth = true}); aboutBox->setSpacing(12); aboutBox->setVisible(false);
    auto* abTitle = aboutBox->addChild<ui::Label>(trText("Mağaza", "Store")); abTitle->setWeight(650); abTitle->setFontSize(18.0);
    auto* abVer = aboutBox->addChild<ui::Label>(trText("Sürüm 0.2.0  ·  BSD-3-Clause Açık Kaynak Lisansı", "Version 0.2.0  ·  BSD-3-Clause Open Source License")); abVer->setFontSize(12.5);
    auto* abDesc = aboutBox->addChild<ui::Label>(trText(
        "C++20 ve Cutefish Toolkit ile geliştirilmiş hafif, hızlı ve çok dilli uygulama merkezi. Sistem depolarındaki AppStream veritabanını kullanarak binlerce uygulamayı düşük bellek tüketimiyle listeler, kurar ve günceller.",
        "Lightweight, fast, and multi-language software center built with C++20 and Cutefish Toolkit. Lists, installs, and updates thousands of applications via AppStream with minimal memory usage."));
    abDesc->setFontSize(13.0); abDesc->setWrap(true);

    auto* detailBox = content->addChild<ui::Column>(); detailBox->setLayoutHints({.fillWidth = true}); detailBox->setSpacing(14); detailBox->setVisible(false);
    auto* backBtn = detailBox->addChild<PillAction>(trText("← Geri Dön", "← Back")); backBtn->setStyle(trText("← Geri Dön", "← Back"), ui::Color{110, 115, 125, 255});
    auto* detHead = detailBox->addChild<ui::Row>(); detHead->setLayoutHints({.fillWidth = true}); detHead->setSpacing(16);
    auto* detIcon = detHead->addChild<ui::IconImage>("preferences-system", 64); detIcon->setLayoutHints({.preferredWidth = 64, .preferredHeight = 64});
    auto* detInfo = detHead->addChild<ui::Column>(); detInfo->setSpacing(4);
    auto* detTitle = detInfo->addChild<ui::Label>(""); detTitle->setWeight(650); detTitle->setFontSize(18.0);
    auto* detMeta = detInfo->addChild<ui::Label>(""); detMeta->setFontSize(12.0); detHead->addStretch();
    auto* detActionBtn = detHead->addChild<PillAction>(trText("Yükle", "Install"));
    auto* detSummary = detailBox->addChild<ui::Label>(""); detSummary->setWeight(560); detSummary->setFontSize(13.5);
    auto* detShot = detailBox->addChild<ui::ImageView>();
    detShot->setLayoutHints({.fillWidth = true, .preferredHeight = 260});
    detShot->onStatusChanged = [detShot, detailBox](ui::ImageView::Status st) { if (st == ui::ImageView::Status::Error) { detShot->setVisible(false); detailBox->requestLayout(); } else if (st == ui::ImageView::Status::Ready) { detShot->requestPaint(); detailBox->requestPaint(); } };
    detShot->setFillMode(ui::ImageView::FillMode::PreserveAspectFit); detShot->setCornerRadius(10.0); detShot->setVisible(false);
    auto* detDesc = detailBox->addChild<ui::Label>(""); detDesc->setFontSize(12.5); detDesc->setWrap(true);
    auto* detBar = detailBox->addChild<ui::ProgressBar>(); detBar->setLayoutHints({.fillWidth = true}); detBar->setVisible(false);

    std::vector<ui::Row*> gridRows; std::vector<AppCard*> cards;
    auto* scroll = content->addChild<ui::ScrollView>(); scroll->setLayoutHints({.fillWidth = true, .fillHeight = true});
    auto* scrollCol = scroll->setContent<ui::Column>(); scrollCol->setSpacing(12);
    for (int r = 0; r < 30; ++r) {
        auto* gr = scrollCol->addChild<ui::Row>(); gr->setLayoutHints({.fillWidth = true}); gr->setSpacing(14); gridRows.push_back(gr);
        for (int c = 0; c < 2; ++c) { auto* card = gr->addChild<AppCard>(); card->setLayoutHints({.fillWidth = true}); cards.push_back(card); }
    }

    auto openDetail = [&](const StoreApp& item) {
        selectedApp = item; scroll->setVisible(false);
        prevBtn->setVisible(false); pageInfo->setVisible(false); nextBtn->setVisible(false); updateAllBtn->setVisible(false);
        aboutBox->setVisible(false); detailBox->setVisible(true); pageTitle->setText(trText("Uygulama Detayı", "Application Details"));
        detIcon->setName(item.icon); detIcon->requestPaint(); detTitle->setText(item.title);
        detMeta->setText(trText("Paket: ", "Package: ") + item.pkgName + "  ·  " + trText("Geliştirici: ", "Developer: ") + item.developer + "  ·  " + trText("Lisans: ", "License: ") + item.license);
        detSummary->setText(item.summary); detDesc->setText(item.description);
        detShot->setVisible(true);
        detShot->setSource("");
        detailBox->requestLayout();

        // 1. Arka planda Türkçe çeviri (kasılma olmadan)
        std::thread([detSummary, detDesc, item]() {
            auto sTr = autoTranslateIfTurkish(item.summary);
            auto dTr = autoTranslateIfTurkish(item.description);
            detSummary->setText(sTr); detDesc->setText(dTr);
            detSummary->requestLayout(); detDesc->requestLayout();
        }).detach();

        // 2. Arka planda ekran görüntüsü indirme
        std::thread([detShot, detailBox, pkg = item.pkgName]() {
            std::string tmp = "/tmp/cf_shot_" + pkg + ".png";
            std::string cmd = "cf-store-helper shot '" + pkg + "' '" + tmp + "'";
            if (std::system(cmd.c_str()) == 0) {
                detShot->setSource(tmp);
                detShot->requestLayout();
                detShot->requestPaint();
                detailBox->requestPaint();
            } else {
                detShot->setVisible(false);
                detailBox->requestLayout();
            }
        }).detach();

                if (item.isUpdate) detActionBtn->setStyle(trText("Güncelle", "Update"), ui::Color{234, 88, 12, 255});
        else detActionBtn->setStyle(item.installed ? trText("Kaldır", "Remove") : trText("Yükle", "Install"), item.installed ? ui::Color{220, 38, 38, 255} : ui::Color{0, 122, 255, 255});
    };
    for (auto* c : cards) c->onSelect = openDetail;

    detActionBtn->onClick = [&]() {
        if (selectedApp.isFlatpak) {
            detBar->setVisible(true); detBar->setIndeterminate(true);
            bool removing = (selectedApp.installed && !selectedApp.isUpdate);
            toast.show(selectedApp.title + (removing ? trText(" kaldırılıyor...", " removing...") : trText(" kuruluyor...", " installing...")));
            std::thread([&, removing]() {
                std::string cmd = removing
                    ? ("flatpak uninstall --user -y " + selectedApp.pkgName)
                    : ("flatpak install --user -y flathub " + selectedApp.pkgName + " || flatpak update --user -y " + selectedApp.pkgName);
                int r = std::system(cmd.c_str());
                detBar->setVisible(false);
                if (r == 0) {
                    toast.show(trText("İşlem tamamlandı!", "Completed!"));
                    detActionBtn->setStyle(removing ? trText("Yükle", "Install") : trText("Kaldır", "Remove"),
                                           removing ? ui::Color{0, 122, 255, 255} : ui::Color{220, 38, 38, 255});
                } else toast.show(trText("Flatpak işlemi başarısız oldu.", "Flatpak operation failed."));
            }).detach();
        } else if (selectedApp.isUpdate || !selectedApp.installed) {
            detBar->setVisible(true); detBar->setIndeterminate(true); installer.install(selectedApp.pkgName);
        } else {
            toast.show(selectedApp.title + trText(" kaldırılıyor...", " removing..."));
            std::thread([&]() {
                std::string rmCmd = "if command -v apt >/dev/null; then pkexec apt autoremove --purge -y " + selectedApp.pkgName +
                                    "; elif command -v pacman >/dev/null; then pkexec pacman -Rns --noconfirm " + selectedApp.pkgName +
                                    "; elif command -v dnf >/dev/null; then pkexec dnf remove -y " + selectedApp.pkgName + "; fi";
                int r = std::system(rmCmd.c_str());
                toast.show(r == 0 ? selectedApp.title + trText(" kaldırıldı!", " removed!") : trText("İşlem iptal edildi.", "Cancelled."));
            }).detach();
        }
    };
    updateAllBtn->onClick = [&]() {
        toast.show(trText("Sistem ve Flathub güncellemeleri başlatıldı...", "Updating system & Flathub..."));
        updater.update(true);
        std::thread([]() { std::system("flatpak update --user -y >/dev/null 2>&1"); }).detach();
    };

    auto sub = installer.changed.connect([&]() {
        using S = packagekit::Installer::State;
        if (installer.state() == S::Downloading || installer.state() == S::Installing) { detBar->setVisible(true); detBar->setIndeterminate(false); detBar->setValue(installer.progress() / 100.0); }
        else if (installer.state() == S::Installed) { detBar->setVisible(false); toast.show(selectedApp.title + trText(" başarıyla kuruldu!", " installed successfully!")); detActionBtn->setStyle(trText("Kaldır", "Remove"), ui::Color{220, 38, 38, 255}); }
    });

    std::vector<StoreApp> activeList; size_t pageIdx = 0, currentTab = 0;
    auto renderPage = [&](const std::string& title) {
        detailBox->setVisible(false); aboutBox->setVisible(false); scroll->setVisible(true); pageTitle->setText(title);
        updateAllBtn->setVisible(tabs[currentTab].id == "Guncellemeler" && search->text().empty() && !activeList.empty());
        size_t total = std::max<size_t>(1, (activeList.size() + 59) / 60); if (pageIdx >= total) pageIdx = 0;
        prevBtn->setVisible(false); pageInfo->setVisible(false); nextBtn->setVisible(false);
        pageInfo->setText(std::to_string(pageIdx + 1) + " / " + std::to_string(total));
        size_t start = pageIdx * 60;
        for (size_t i = 0; i < cards.size(); ++i) { if (start + i < activeList.size()) cards[i]->setApp(activeList[start + i]); else cards[i]->setEmpty(); }
    };

    auto selectTab = [&](size_t idx) {
        currentTab = idx; pageIdx = 0; for (size_t i = 0; i < sideItems.size(); ++i) sideItems[i]->setActive(i == idx);
        activeList.clear();
        if (tabs[idx].id == "Hakkinda") {
            detailBox->setVisible(false); scroll->setVisible(false);
            prevBtn->setVisible(false); pageInfo->setVisible(false); nextBtn->setVisible(false); updateAllBtn->setVisible(false);
            pageTitle->setText(tabs[idx].label); aboutBox->setVisible(true); return;
        } else if (tabs[idx].id == "Guncellemeler") { upgradableApps = getUpgradablePackages(); activeList = upgradableApps; if (activeList.empty()) toast.show(trText("Sisteminiz tamamen güncel!", "Your system is up to date!")); }
        else { for (const auto& a : allApps) { if (tabs[idx].id == "Yuklu" && a.installed) activeList.push_back(a); else if (tabs[idx].id != "Yuklu" && (tabs[idx].id == "Kesfet" || a.category == tabs[idx].id)) activeList.push_back(a); } }
        renderPage(tabs[idx].label);
    };
    for (size_t i = 0; i < sideItems.size(); ++i) sideItems[i]->onClick = [&, i]() { selectTab(i); };
    backBtn->onClick = [&]() { selectTab(currentTab); };
    prevBtn->onClick = [&]() { if (pageIdx > 0) { --pageIdx; renderPage(pageTitle->text()); } };
    nextBtn->onClick = [&]() { if ((pageIdx + 1) * 60 < activeList.size()) { ++pageIdx; renderPage(pageTitle->text()); } };
    search->onTextChanged = [&]() {
        std::string q = toLower(search->text()); if (q.empty()) { selectTab(currentTab); return; }
        activeList.clear(); pageIdx = 0;
        for (const auto& a : allApps) if (toLower(a.title).find(q) != std::string::npos || toLower(a.summary).find(q) != std::string::npos) activeList.push_back(a);
        renderPage(trText("Arama: ", "Search: ") + search->text());
    };
    selectTab(0); window.setContentView(std::move(root)); window.show(); return app.run();
}
