#include "GDXAddGauntletPopup.hpp"
#include "GDXGauntletManagePopup.hpp"
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/ui/Layout.hpp>

extern void addGauntletEntry(const gd::string& name, const gd::string& description);

using namespace geode;
using namespace geode::prelude;

GDXAddGauntletPopup* GDXAddGauntletPopup::create(GDXGauntletManagePopup* owner) {
    auto ret = new GDXAddGauntletPopup();
    ret->m_owner = owner;
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool GDXAddGauntletPopup::init() {
    if (!Popup::init(480.f, 280.f)) {
        return false;
    }

    this->setTitle("Add New Gauntlet");

    m_nameInput = TextInput::create(380.f, "Gauntlet Name", "chatFont.fnt");
    m_nameInput->setPosition({240.f, 180.f});
    m_nameInput->setLabel("Name");
    this->m_mainLayer->addChild(m_nameInput);

    m_descriptionInput = TextInput::create(380.f, "Gauntlet description", "chatFont.fnt");
    m_descriptionInput->setPosition({240.f, 120.f});
    m_descriptionInput->setLabel("Description");
    this->m_mainLayer->addChild(m_descriptionInput);

    auto saveBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Save", "goldFont.fnt", "GJ_button_01.png"),
        this,
        menu_selector(GDXAddGauntletPopup::onSave));
    m_buttonMenu->addChildAtPosition(saveBtn, Anchor::Bottom, {0, 20}, false);

    return true;
}

void GDXAddGauntletPopup::onSave(CCObject* sender) {
    if (!m_owner || !m_nameInput || !m_descriptionInput) {
        return;
    }

    auto name = m_nameInput->getString();
    auto description = m_descriptionInput->getString();
    if (name.empty()) {
        return;
    }

    addGauntletEntry(name, description);
    m_owner->refreshList();
    this->removeFromParent();
}