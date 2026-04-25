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

    setTitle("Manage Gauntlets");

    auto listSize = CCSizeMake(356.f, 200.f);
    m_list = cue::ListNode::create(listSize);
    m_mainLayer->addChildAtPosition(m_list, Anchor::Center, {0.f, 10.f});

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
    if (!m_list) {
        return;
    }

    m_list->clear();

    if (s_gauntletList.empty()) {
        auto emptyLabel = CCLabelBMFont::create("No gauntlets yet. Press Add to create one.", "chatFont.fnt");
        emptyLabel->setAnchorPoint({0.5f, 0.5f});
        emptyLabel->setPosition({m_list->getListSize().width / 2.f, m_list->getListSize().height / 2.f});
        m_list->addCell(emptyLabel);
        return;
    }

    for (auto const& gauntlet : s_gauntletList) {
        auto entry = CCLabelBMFont::create(("- " + gauntlet.first).c_str(), "chatFont.fnt");
        entry->setAnchorPoint({0.f, 0.5f});
        m_list->addCell(entry);
    }
}

void addGauntletEntry(const gd::string& name, const gd::string& description) {
    s_gauntletList.emplace_back(name, description);
}
