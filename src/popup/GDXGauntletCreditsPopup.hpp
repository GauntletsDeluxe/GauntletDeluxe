#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>

using namespace geode::prelude;

struct GDXUserInfo {
    int accountId = 0;
    std::string username;
    int icon = 0;
    int iconColor1 = 0;
    int iconColor2 = 0;
    int iconColor3 = -1;

    static GDXUserInfo fromJson(const matjson::Value& value);
};

class GDXGauntletCreditsPopup : public geode::Popup {
public:
    static GDXGauntletCreditsPopup* create(const matjson::Value& gauntlet);

private:
    bool init(const matjson::Value& gauntlet);
    void onTagCell(CCObject* sender);

    matjson::Value m_gauntlet;
};
