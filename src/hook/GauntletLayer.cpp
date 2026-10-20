#include <Geode/Geode.hpp>
#include <Geode/modify/GauntletSelectLayer.hpp>
#include "../layer/GDXGauntletLayer.hpp"

using namespace geode::prelude;

class $modify(GDXHookGauntletSelectLayer, GauntletSelectLayer) {
    bool init(int unused) {
        if (!GauntletSelectLayer::init(unused)) return false;

        if (auto topRight = static_cast<CCMenu*>(this->getChildByID("top-right-menu"))) {
            
            auto gauntletSpr = CCSprite::createWithSpriteFrameName("GDX_gauntletDeluxe.png"_spr);

            auto color = Loader::get()->getLoadedMod("dasshu.better-gauntlets")
                ? CircleBaseColor::Green
                : CircleBaseColor::Gray;

            auto gauntletBtnSpr = CircleButtonSprite::create(
                gauntletSpr,
                color,
                CircleBaseSize::Medium
            );
            gauntletBtnSpr->setScale(0.75);

            auto gauntletBtn = CCMenuItemSpriteExtra::create(
                gauntletBtnSpr,
                this,
                menu_selector(GDXHookGauntletSelectLayer::onGDXButtonClick)
            );
            gauntletBtn->setID("gauntlets-deluxe-button"_spr);
            topRight->addChild(gauntletBtn);
            topRight->updateLayout();
        }
        return true;
    }

    void onGDXButtonClick(CCObject*) {
        auto scene = CCScene::create();
        scene->addChild(GDXGauntletLayer::create());
        CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(0.5f, scene));
    }
};