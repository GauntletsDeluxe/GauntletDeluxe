#pragma once

#include <Geode/Geode.hpp>
#include <cue/ListNode.hpp>

using namespace geode::prelude;

class GDXLeaderboardLayer : public CCLayer {
public:
    static GDXLeaderboardLayer* create();

private:
    bool init() override;
    void onEnter() override;
    void keyBackClicked() override;
    void fetchLeaderboard();
    void createTabs();
    void updateTabSelection();
    void onTabSelected(CCObject* sender);

    cue::ListNode* m_list = nullptr;
    geode::Scrollbar* m_scrollbar = nullptr;
    LoadingSpinner* m_loadingSpinner = nullptr;
    int m_type = 1;
    TabButton* m_levelPointsTabSprite = nullptr;
    TabButton* m_gauntletPointsTabSprite = nullptr;
};
