#include <Geode/Geode.hpp>
#include <Geode/modify/GauntletSelectLayer.hpp>
#include <Geode/ui/Button.hpp>
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

            auto gauntletBtn = Button::createWithNode(gauntletBtnSpr, [this](Button* sender) { pushSceneWithLayer(GDXGauntletLayer::create()); });
            gauntletBtn->setID("gauntlets-deluxe-button"_spr);
            topRight->addChild(gauntletBtn);
            topRight->updateLayout();
        }
        return true;
    }
};