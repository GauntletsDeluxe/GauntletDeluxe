#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/Layout.hpp>

class GDXShowTagsPopup : public geode::Popup {
public:
    static GDXShowTagsPopup* create(std::string const& gauntletName, const matjson::Value& tags);

private:
    bool init(std::string const& gauntletName, const matjson::Value& tags);
    void update(float dt) override;
    void onTagCell(CCObject* sender);
    cocos2d::CCNode* createTagCell(const matjson::Value& tag, int index);

    cocos2d::CCMenu* m_tagMenu = nullptr;
    matjson::Value m_tags;
    std::string m_gauntletName;
};
