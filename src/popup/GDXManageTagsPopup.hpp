#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/Layout.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/ui/Scrollbar.hpp>
#include <cue/ListNode.hpp>

class GDXManageTagsPopup : public geode::Popup {
public:
    static GDXManageTagsPopup* create();
    void refreshTags();

private:
    bool init() override;
    void update(float dt) override;
    void onAddTag(CCObject* sender);
    void onManageTagsWeb(CCObject* sender);
    void onEditTag(CCObject* sender);
    void onDeleteTag(CCObject* sender);
    void refreshListItems();
    void fetchTags();
    void createTagList(const matjson::Value& tags);
    void rebuildTagList();
    void clearListContent();
    void deleteTagAtIndex(int index);
    cocos2d::CCNode* createTagCell(const matjson::Value& tag, int index);

    cue::ListNode* m_list = nullptr;
    geode::Scrollbar* m_scrollbar = nullptr;
    geode::TextInput* m_searchInput = nullptr;
    cocos2d::CCLabelBMFont* m_emptyLabel = nullptr;
    matjson::Value m_tags;
    matjson::Value m_allTags;
    std::string m_searchFilter;
    geode::async::TaskHolder<> m_fetchTagsTask;
    geode::async::TaskHolder<> m_deleteTagTask;
};
