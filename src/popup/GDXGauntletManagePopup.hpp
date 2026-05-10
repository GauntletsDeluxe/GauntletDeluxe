#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/Layout.hpp>
#include <cue/ListNode.hpp>

class GDXAddGauntletPopup;

class GDXGauntletManagePopup : public geode::Popup {
public:
    static GDXGauntletManagePopup* create();
    void refreshList();

private:
    bool init() override;
    void update(float dt) override;
    void onManageAssets(CCObject* sender);
    void onUserPanel(CCObject* sender);
    void onAdd(CCObject* sender);
    void onDelete(CCObject* sender);
    void onEdit(CCObject* sender);
    void refreshListItems();
    void fetchGauntlets();
    void createGauntletList(const matjson::Value& gauntlets);
    void deleteGauntletAtIndex(int index);
    cocos2d::CCNode* createGauntletCell(const matjson::Value& gauntlet, int index);
    cue::ListNode* m_list = nullptr;
    geode::Scrollbar* m_scrollbar = nullptr;
    matjson::Value m_gauntlets;
    cocos2d::CCMenu* m_menu = nullptr;
    geode::async::TaskHolder<> m_manageAssetsTask;
    geode::async::TaskHolder<> m_fetchGauntletsTask;
    geode::async::TaskHolder<> m_deleteGauntletTask;
};
