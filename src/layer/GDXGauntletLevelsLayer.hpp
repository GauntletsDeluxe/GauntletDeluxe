#include <Geode/Geode.hpp>
#include <vector>

using namespace geode::prelude;

struct GDXGauntletLevelEntry {
    int levelId = 0;
    std::string levelName;
    std::string creatorName;
    std::string songName;
    int songId = 0;
    int reward = 0;
};

class GDXGauntletLevelsLayer : public CCLayer {
public:
    static GDXGauntletLevelsLayer* create(CCArray* levels, const std::string& title, const cocos2d::ccColor3B& color, int gauntletIndex);

private:
    bool init(CCArray* levels, const std::string& title, const cocos2d::ccColor3B& color, int gauntletIndex);
    void onEnter() override;
    void keyBackClicked() override;
    void update(float dt) override;
    void onLevelClicked(CCObject* sender);
    void refreshCompletionIcons();

    std::vector<GDXGauntletLevelEntry> m_levels;
    std::string m_gauntletTitle;
    cocos2d::ccColor3B m_backgroundColor;
    int m_gauntletIndex = -1;

    CCMenu* m_levelsMenu = nullptr;
    LoadingSpinner* m_pendingSpinner = nullptr;
    std::string m_pendingKey;
    int m_pendingLevelId = -1;
    float m_pendingTimeout = 0.0f;
};