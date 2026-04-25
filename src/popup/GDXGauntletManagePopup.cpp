#include "GDXGauntletManagePopup.hpp"
#include "GDXAddGauntletPopup.hpp"
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>

using namespace geode;
using namespace geode::prelude;

static std::vector<std::pair<gd::string, gd::string>> s_gauntletList{
    {"Firestorm", "A fast-paced challenge with lava and spikes."},
    {"Frostbite", "A chill gauntlet with slippery platforms."}};

GDXGauntletManagePopup* GDXGauntletManagePopup::create() {
    auto ret = new GDXGauntletManagePopup();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool GDXGauntletManagePopup::init() {
    if (!Popup::init(480.f, 280.f)) {
        return false;
    }

    this->setTitle("Manage Gauntlets");

    auto panelSize = CCSizeMake(520.f, 360.f);
    m_listLayer = CCLayer::create();
    m_listLayer->setContentSize({panelSize.width - 40.f, panelSize.height - 120.f});
    m_listLayer->setPosition({20.f, 80.f});
    this->m_mainLayer->addChild(m_listLayer);

    auto addBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Add Gauntlet", "goldFont.fnt", "GJ_button_01.png"),
        this,
        menu_selector(GDXGauntletManagePopup::onAdd));
    m_buttonMenu->addChildAtPosition(addBtn, Anchor::Bottom, {0, 25}, false);

    refreshListItems();
    return true;
}

void GDXGauntletManagePopup::onAdd(CCObject* sender) {
    GDXAddGauntletPopup::create(this)->show();
}

void GDXGauntletManagePopup::refreshList() {
    refreshListItems();
}

void GDXGauntletManagePopup::refreshListItems() {
    if (!m_listLayer) {
        return;
    }

    m_listLayer->removeAllChildrenWithCleanup(true);

    if (s_gauntletList.empty()) {
        auto emptyLabel = CCLabelBMFont::create("No gauntlets yet. Press Add to create one.", "chatFont.fnt");
        emptyLabel->setPosition({m_listLayer->getContentSize().width / 2.f, m_listLayer->getContentSize().height / 2.f});
        m_listLayer->addChild(emptyLabel);
        return;
    }

    auto y = m_listLayer->getContentSize().height - 30.f;
    for (auto const& gauntlet : s_gauntletList) {
        auto entry = CCLabelBMFont::create(("- " + gauntlet.first).c_str(), "chatFont.fnt");
        entry->setAnchorPoint({0.f, 0.5f});
        entry->setPosition({10.f, y});
        m_listLayer->addChild(entry);
        y -= 30.f;
        if (y < 30.f) {
            break;
        }
    }
}

void addGauntletEntry(const gd::string& name, const gd::string& description) {
    s_gauntletList.emplace_back(name, description);
}
