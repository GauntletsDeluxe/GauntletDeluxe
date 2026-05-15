#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/ui/Layout.hpp>
#include <Geode/ui/Scrollbar.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>

namespace geode {
    class ColorPickPopup;
}

class GDXManageTagsPopup;

class GDXAddTagsPopup : public geode::Popup {
public:
    static GDXAddTagsPopup* create(GDXManageTagsPopup* parent);
    static GDXAddTagsPopup* create(GDXManageTagsPopup* parent, const matjson::Value& tag, int index);

private:
    bool init() override;
    void onSubmit(CCObject* sender);
    void onSelectColor(CCObject* sender);
    void onClose(CCObject* sender) override;
    GDXManageTagsPopup* m_parent = nullptr;
    geode::TextInput* m_nameInput = nullptr;
    geode::TextInput* m_descriptionInput = nullptr;
    CCMenuItemSpriteExtra* m_colorButton = nullptr;
    cocos2d::CCSprite* m_colorSpr = nullptr;
    int m_r = 255;
    int m_g = 255;
    int m_b = 255;
    int m_editIndex = -1;
    matjson::Value m_editTag;
    geode::ColorPickPopup* m_colorPopup = nullptr;
    bool m_unsaved = true;
    geode::async::TaskHolder<> m_requestTask;
};
