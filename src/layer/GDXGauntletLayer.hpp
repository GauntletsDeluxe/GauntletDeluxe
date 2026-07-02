#include <Geode/Geode.hpp>
#include <Geode/binding/SetTextPopupDelegate.hpp>
#include <asp/fs.hpp>
#include <unordered_set>
#include <vector>

using namespace geode::prelude;

struct GDXGauntletLevelInfo {
    int levelId = 0;
    std::string levelName;
    std::string creatorName;
    std::string songName;
    int songId = 0;
    int reward = 0;
};

struct GDXGauntletNode {
    int id = 0;
    std::string name;
    std::string description;
    int reward = 0;
    int r = 255;
    int g = 255;
    int b = 255;
    std::vector<GDXGauntletLevelInfo> levelIds;

    static GDXGauntletNode fromJson(const matjson::Value& gauntlet);
};

class GDXGauntletLayer : public CCLayer, public SetTextPopupDelegate {
public:
    static GDXGauntletLayer* create();

private:
    bool init() override;
    void update(float dt) override;
    void onExit() override;
    void keyBackClicked() override;
    void onInfo(CCObject* sender);
    void onLeaderboard(CCObject* sender);
    void onManageGauntlets(CCObject* sender);
    void onSyncAccount(CCObject* sender);
    void onRefreshGauntlets(CCObject* sender);
    void onToggleRecent(CCObject* sender);
    void onToggleLocalMode(CCObject* sender);
    void onDiscord(CCObject* sender);
    void onLink(CCObject* sender);
    void onPrev(CCObject* sender);
    void onNext(CCObject* sender);
    void onCompleteGauntlet(CCObject* sender);
    void onGauntletButtonClick(CCObject* sender);
    void onShowGauntletTags(CCObject* sender);
    void fetchGauntlets();
    void fetchUserData();
    void createGauntletPages(const matjson::Value& gauntlets, bool local = false);
    void updatePageButtons();
    void updateLocalToggleState();
    void updateRecentToggleState();
    void updateSearchButtonState();
    void onEnter() override;
    void onSearchGauntlets(CCObject* sender);
    void onFilterByTag(CCObject* sender);
    void applyTagFilter(std::string const& tag);
    void setTextPopupClosed(SetTextPopup* popup, gd::string text) override;

    CCMenuItemSpriteExtra* createGauntletButton(const matjson::Value& gauntlet, std::size_t index, bool local = false);
    BoomScrollLayer* getActiveScrollLayer() const;
    const matjson::Value& getActiveGauntlets() const;
    BoomScrollLayer* m_scrollLayer = nullptr;
    BoomScrollLayer* m_localScrollLayer = nullptr;
    CCMenu* m_gauntletsMenu = nullptr;
    CCMenu* m_bottomLeftMenu = nullptr;
    CCMenu* m_bottomRightMenu = nullptr;
    CCMenuItemSpriteExtra* m_prevPageBtn = nullptr;
    CCMenuItemSpriteExtra* m_nextPageBtn = nullptr;
    CCMenu* m_pageNavMenu = nullptr;
    CCMenuItemSpriteExtra* m_leaderboardBtn = nullptr;
    CCMenuItemSpriteExtra* m_discordBtn = nullptr;
    CCMenuItemSpriteExtra* m_syncBtn = nullptr;
    CCMenuItemSpriteExtra* m_tagFilterBtn = nullptr;
    CCMenuItemSpriteExtra* m_searchBtn = nullptr;
    CCMenuItemSpriteExtra* m_linkBtn = nullptr;
    CCMenuItemSpriteExtra* m_recentToggle = nullptr;
    CCMenuItemSpriteExtra* m_localToggle = nullptr;
    bool m_recentFilter = false;
    bool m_localMode = false;
    std::string m_tagFilter;
    matjson::Value m_onlineGauntlets;
    matjson::Value m_localGauntlets;
    std::vector<CCMenuItemSpriteExtra*> m_gauntletButtons;
    std::vector<CCMenuItemSpriteExtra*> m_localGauntletButtons;
    LoadingSpinner* m_loadingSpinner = nullptr;
    std::string m_searchQuery;
    CCSprite* m_levelPointsSpr = nullptr;
    CCSprite* m_gauntletPointsSpr = nullptr;
    CCCounterLabel* m_levelPointsCounter = nullptr;
    CCCounterLabel* m_gauntletPointsCounter = nullptr;
    matjson::Value m_gauntlets;
    std::unordered_set<int> m_completedGauntletLevels;
    std::unordered_set<int> m_claimedGauntlets;
    int m_userAccountId = 0;
    std::string m_username;
    int m_gauntletPoints = Mod::get()->getSavedValue<int>("gauntletPoints", 0);
    int m_levelPoints = Mod::get()->getSavedValue<int>("levelPoints", 0);
    int m_currentPage = -1;

    geode::async::TaskHolder<> m_fetchGauntletsTask;
    geode::async::TaskHolder<> m_fetchUserDataTask;
    geode::async::TaskHolder<> m_syncAccountTask;
    geode::async::TaskHolder<> m_completeGauntletTask;
};