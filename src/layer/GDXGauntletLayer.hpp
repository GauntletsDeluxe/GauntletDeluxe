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
    void keyBackClicked() override;
    void onInfo(CCObject* sender);
    void onManageGauntlets(CCObject* sender);
    void onDot(CCObject* sender);
    void onPrev(CCObject* sender);
    void onNext(CCObject* sender);
    void onGauntletButtonClick(CCObject* sender);
    void fetchGauntlets();
    void createGauntletPages(const matjson::Value& gauntlets);
    void updateDots();

    CCMenuItemSpriteExtra* createGauntletButton(const matjson::Value& gauntlet);
    BoomScrollLayer* m_scrollLayer = nullptr;
    CCMenu* m_dotsMenu = nullptr;
    CCMenu* m_gauntletsMenu = nullptr;
    std::vector<CCMenuItemSpriteExtra*> m_dots;
    std::vector<CCMenuItemSpriteExtra*> m_gauntletButtons;
    int m_currentPage = -1;
};