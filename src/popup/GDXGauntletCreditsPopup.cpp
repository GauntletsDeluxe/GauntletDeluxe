#include "GDXGauntletCreditsPopup.hpp"
#include "../include/GDXConstant.hpp"
#include "Geode/cocos/label_nodes/CCLabelBMFont.h"
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/ui/Layout.hpp>
#include <Geode/ui/LazySprite.hpp>
#include <Geode/ui/Button.hpp>
#include <cue/PlayerIcon.hpp>

using namespace geode;
using namespace geode::prelude;

GDXUserInfo GDXUserInfo::fromJson(const matjson::Value& value) {
    GDXUserInfo info;
    if (!value.isObject()) {
        return info;
    }

    info.accountId = value["accountId"].asInt().unwrapOr(0);
    info.username = value["username"].asString().unwrapOr("Unknown");
    info.icon = value["icon"].asInt().unwrapOr(0);
    info.iconColor1 = value["iconColor1"].asInt().unwrapOr(0);
    info.iconColor2 = value["iconColor2"].asInt().unwrapOr(0);
    info.iconColor3 = value["iconColor3"].asInt().unwrapOr(-1);
    return info;
}

GDXGauntletCreditsPopup* GDXGauntletCreditsPopup::create(const matjson::Value& gauntlet) {
    auto ret = new GDXGauntletCreditsPopup();
    if (ret && ret->init(gauntlet)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool GDXGauntletCreditsPopup::init(const matjson::Value& gauntlet) {
    if (!Popup::init(530.f, 285.f, "square01_001.png")) {
        return false;
    }

    auto title = gauntlet["name"].asString().unwrapOr("Gauntlet Credits");
    this->setTitle(title.c_str());
    m_title->setPositionY(m_title->getPositionY() - 5.f);

    auto description = gauntlet["description"].asString().unwrapOr("No description available.");
    auto spriteBy = GDXUserInfo::fromJson(gauntlet["spriteBy"]);
    auto suggestedBy = GDXUserInfo::fromJson(gauntlet["suggestedBy"]);

    const float contentWidth = m_mainLayer->getContentWidth();

    auto creditsBg = NineSlice::create("square02_small.png");
    if (creditsBg) {
        creditsBg->setInsets({10, 10, 10, 10});
        creditsBg->setAnchorPoint({0.f, .5f});
        creditsBg->setContentSize({320.f, 160.f});
        creditsBg->setOpacity(100);
        m_mainLayer->addChildAtPosition(creditsBg, Anchor::Left, {20.f, -40.f}, false);
    }

    // gauntlet sprite creator label
    auto spriteLabel = CCLabelBMFont::create("Gauntlet Sprite:", "bigFont.fnt");
    spriteLabel->setScale(.5f);
    spriteLabel->setAlignment(CCTextAlignment::kCCTextAlignmentLeft);
    spriteLabel->setAnchorPoint({0.f, 0.5f});
    spriteLabel->setPosition({10.f, creditsBg->getContentSize().height - 15.f});
    creditsBg->addChild(spriteLabel);

    auto spriteIcon = SimplePlayer::create(spriteBy.icon);
    if (spriteIcon) {
        auto gm = GameManager::sharedState();
        spriteIcon->updatePlayerFrame(spriteBy.icon, IconType::Cube);
        spriteIcon->setColors(gm->colorForIdx(spriteBy.iconColor1), gm->colorForIdx(spriteBy.iconColor2));
        if (spriteBy.iconColor3 >= 0) {
            spriteIcon->setGlowOutline(gm->colorForIdx(spriteBy.iconColor3));
        }
        spriteIcon->setAnchorPoint({0.f, 0.5f});
        spriteIcon->setPosition({25.f, creditsBg->getContentSize().height - 45.f});
        creditsBg->addChild(spriteIcon);
    }

    auto spriteByLabel = geode::Button::createWithLabel(spriteBy.username.c_str(), "goldFont.fnt", [spriteBy](geode::Button* sender) {
        ProfilePage::create(spriteBy.accountId, false)->show();
    });
    spriteByLabel->setAnchorPoint({0.f, 0.5f});
    spriteByLabel->setScale(.7f);
    spriteByLabel->setPosition({50.f, creditsBg->getContentSize().height - 45.f});
    creditsBg->addChild(spriteByLabel);

    // gauntlet suggester label
    auto suggestedLabel = CCLabelBMFont::create("Gauntlet Suggester:", "bigFont.fnt");
    suggestedLabel->setScale(.5f);
    suggestedLabel->setAlignment(CCTextAlignment::kCCTextAlignmentLeft);
    suggestedLabel->setAnchorPoint({0.f, 0.5f});
    suggestedLabel->setPosition({10.f, creditsBg->getContentSize().height - 80.f});
    creditsBg->addChild(suggestedLabel);

    auto suggestedIcon = SimplePlayer::create(suggestedBy.icon);
    if (suggestedIcon) {
        auto gm = GameManager::sharedState();
        suggestedIcon->updatePlayerFrame(suggestedBy.icon, IconType::Cube);
        suggestedIcon->setColors(gm->colorForIdx(suggestedBy.iconColor1), gm->colorForIdx(suggestedBy.iconColor2));
        if (suggestedBy.iconColor3 >= 0) {
            suggestedIcon->setGlowOutline(gm->colorForIdx(suggestedBy.iconColor3));
        }
        suggestedIcon->setAnchorPoint({0.f, 0.5f});
        suggestedIcon->setPosition({25.f, creditsBg->getContentSize().height - 110.f});
        creditsBg->addChild(suggestedIcon);
    }

    auto suggestedByLabel = geode::Button::createWithLabel(suggestedBy.username.c_str(), "goldFont.fnt", [suggestedBy](geode::Button* sender) {
        ProfilePage::create(suggestedBy.accountId, false)->show();
    });
    suggestedByLabel->setAnchorPoint({0.f, 0.5f});
    suggestedByLabel->setScale(.7f);
    suggestedByLabel->setPosition({50.f, creditsBg->getContentSize().height - 110.f});
    creditsBg->addChild(suggestedByLabel);

    // gauntlet sprite on the right
    auto gauntletId = gauntlet["id"].asInt().unwrapOr(gauntlet["index"].asInt().unwrapOr(0));
    auto imageUrl = std::string(gdx::baseApiUrl()) + "/gauntlet/gauntlet_" + numToString(gauntletId) + ".png?v2=true";
    auto gauntletSprite = LazySprite::create({120.f, 240.f}, false);
    if (gauntletSprite) {
        gauntletSprite->setAutoResize(true);
        gauntletSprite->setZOrder(2);
        m_mainLayer->addChildAtPosition(gauntletSprite, Anchor::Right, {-100.f, -40.f}, false);
        gauntletSprite->loadFromUrl(imageUrl, CCImage::kFmtPng, true);
    }

    // description below the credits area
    auto descriptionArea = SimpleTextArea::create(description.c_str(), "chatFont.fnt", .8f, m_mainLayer->getContentWidth() - 60.f);
    descriptionArea->setAlignment(kCCTextAlignmentLeft);
    descriptionArea->setMaxLines(2);
    descriptionArea->setAnchorPoint({0.5f, 1.0f});
    descriptionArea->setZOrder(1);
    m_mainLayer->addChildAtPosition(descriptionArea, Anchor::Top, {0.f, -50.f}, false);

    auto descriptionBg = NineSlice::create("square02_small.png");
    if (descriptionBg) {
        descriptionBg->setContentSize({descriptionArea->getContentSize().width + 20.f, descriptionArea->getContentSize().height + 20.f});
        descriptionBg->setInsets({10, 10, 10, 10});
        descriptionBg->setAnchorPoint({0.5f, 1.0f});
        descriptionBg->setOpacity(100);
        m_mainLayer->addChildAtPosition(descriptionBg, Anchor::Top, {0.f, -40.f}, false);
    }

    return true;
}

