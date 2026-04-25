#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/Layout.hpp>

class GDXAddGauntletPopup;

class GDXGauntletManagePopup : public geode::Popup {
public:
    static GDXGauntletManagePopup* create();
    void refreshList();

private:
    bool init() override;
    void onAdd(CCObject* sender);
    void refreshListItems();

    cocos2d::CCLayer* m_listLayer = nullptr;
    cocos2d::CCMenu* m_menu = nullptr;
};
