#include <Geode/Geode.hpp>
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

class GDXGauntletLayer : public CCLayer {
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
    void onRefreshGauntlets(CCObject* sender);
    void onDot(CCObject* sender);
    void onPrev(CCObject* sender);
    void onNext(CCObject* sender);
    void onGauntletButtonClick(CCObject* sender);
    void onGauntletInfo(CCObject* sender);
    void fetchGauntlets();
    void fetchUserData();
    void createGauntletPages(const matjson::Value& gauntlets);
    void updateDots();
    void onEnter() override;

    CCMenuItemSpriteExtra* createGauntletButton(const matjson::Value& gauntlet, std::size_t index);
    BoomScrollLayer* m_scrollLayer = nullptr;
    CCMenu* m_dotsMenu = nullptr;
    CCMenu* m_gauntletsMenu = nullptr;
    std::vector<CCMenuItemSpriteExtra*> m_dots;
    std::vector<CCMenuItemSpriteExtra*> m_gauntletButtons;
    LoadingSpinner* m_loadingSpinner = nullptr;
    CCCounterLabel* m_levelPointsCounter = nullptr;
    CCCounterLabel* m_gauntletPointsCounter = nullptr;
    matjson::Value m_gauntlets;
    int m_userAccountId = 0;
    std::string m_username;
    int m_gauntletPoints = Mod::get()->getSavedValue<int>("gauntletPoints", 0);
    int m_levelPoints = Mod::get()->getSavedValue<int>("levelPoints", 0);
    int m_currentPage = -1;
};