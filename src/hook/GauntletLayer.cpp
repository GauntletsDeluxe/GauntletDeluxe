#include <Geode/Geode.hpp>
#include <Geode/modify/GauntletSelectLayer.hpp>
#include "../layer/GDXGauntletLayer.hpp"

using namespace geode::prelude;

class $modify(GDXHookGauntletSelectLayer, GauntletSelectLayer) {
    bool init(int unused) {
        if (!GauntletSelectLayer::init(unused)) return false;

        // layout gauntlet in vanilla gd
        if (auto topRight = static_cast<CCMenu*>(this->getChildByID("top-right-menu"))) {
            auto gauntletSpr = CCSprite::createWithSpriteFrameName("GDX_gauntletDeluxe.png"_spr);
            auto gauntletBtnSpr = AccountButtonSprite::create(gauntletSpr, AccountBaseColor::Gray, AccountBaseSize::Normal);
            auto gauntletBtn = CCMenuItemSpriteExtra::create(
                gauntletBtnSpr, this, menu_selector(GDXHookGauntletSelectLayer::onGauntletButtonClick));
            gauntletBtn->setID("rated-layouts-gauntlets-button"_spr);
            topRight->addChild(gauntletBtn);
            topRight->updateLayout();
        }
        return true;
    }

    void onGauntletButtonClick(CCObject*) {
        auto scene = CCScene::create();
        scene->addChild(GDXGauntletLayer::create());
        CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(0.5f, scene));
    }
};