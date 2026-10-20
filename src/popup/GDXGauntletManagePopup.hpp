#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/Layout.hpp>
#include <Geode/ui/TextInput.hpp>
#include <cue/ListNode.hpp>

using namespace geode::prelude;

class GDXAddGauntletPopup;

class GDXGauntletManagePopup : public Popup {
public:
    static GDXGauntletManagePopup* create();
    static GDXGauntletManagePopup* create(bool localMode);
    void refreshList();

private:
    bool init() override;
    void update(float dt) override;
    void onManageAssets(CCObject* sender);
    void onUserPanel(CCObject* sender);
    void onAdd(CCObject* sender);
    void onImport(CCObject* sender);
    void onShowTags(CCObject* sender);
    void onManageTags(CCObject* sender);
    void onDelete(CCObject* sender);
    void onEdit(CCObject* sender);
    void refreshListItems();
    void fetchGauntlets();
    void createGauntletList(const matjson::Value& gauntlets);
    void rebuildGauntletList();
    void clearListContent();
    void deleteGauntletAtIndex(int index);
    cocos2d::CCNode* createGauntletCell(const matjson::Value& gauntlet, int index);
    cue::ListNode* m_list = nullptr;
    TextInput* m_searchInput = nullptr;
    Scrollbar* m_scrollbar = nullptr;
    cocos2d::CCLabelBMFont* m_emptyLabel = nullptr;
    matjson::Value m_gauntlets;
    matjson::Value m_allGauntlets;
    std::string m_searchFilter;
    bool m_localMode = false;
    async::TaskHolder<> m_manageAssetsTask;
    async::TaskHolder<> m_fetchGauntletsTask;
    async::TaskHolder<> m_deleteGauntletTask;
};
