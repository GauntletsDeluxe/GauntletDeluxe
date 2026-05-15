#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/LoadingSpinner.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/Layout.hpp>
#include <functional>

class GDXTagsFiltersPopup : public geode::Popup {
public:
    static GDXTagsFiltersPopup* create(std::function<void(std::string const&)> const& onApply, std::string const& initialTag = "");

private:
    bool init() override;
    void update(float dt) override;
    void onApply(CCObject* sender);
    void onClear(CCObject* sender);
    void onTagCell(CCObject* sender);
    void fetchTags();
    void createTagList(const matjson::Value& tags);
    void rebuildTagList();
    void clearListContent();
    cocos2d::CCNode* createTagCell(const matjson::Value& tag, int index);

    cocos2d::CCMenu* m_tagMenu = nullptr;
    cocos2d::CCLabelBMFont* m_emptyLabel = nullptr;
    geode::LoadingSpinner* m_loadingSpinner = nullptr;
    std::string m_selectedTag;
    int m_selectedIndex = -1;
    matjson::Value m_tags;
    geode::async::TaskHolder<> m_fetchTagsTask;
    std::function<void(std::string const&)> m_onApply;
};
