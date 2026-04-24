#include <Geode/Geode.hpp>
#include <vector>

using namespace geode::prelude;

class GDXGauntletLayer : public CCLayer {
public:
    static GDXGauntletLayer* create();

private:
    bool init() override;
    void update(float dt) override;
    void keyBackClicked() override;
    void onInfo(CCObject* sender);
    void onDot(CCObject* sender);
    void onPrev(CCObject* sender);
    void onNext(CCObject* sender);
    void updateDots();

    BoomScrollLayer* m_scrollLayer = nullptr;
    CCMenu* m_dotsMenu = nullptr;
    std::vector<CCMenuItemSpriteExtra*> m_dots;
    int m_currentPage = -1;
};