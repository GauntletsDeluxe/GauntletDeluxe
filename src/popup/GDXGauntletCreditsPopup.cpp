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

static CCSprite* createCachedGauntletSprite(CCNode* parent, const std::string& url, const CCPoint& position, const CCSize& size, int zOrder = 2) {
    if (!parent) {
        return nullptr;
    }

    if (auto texture = gdx::findGauntletTexture(url)) {
        auto sprite = CCSprite::createWithTexture(texture);
        if (!sprite) {
            return nullptr;
        }
        sprite->setID("gauntlet-image");
        if (sprite->getContentSize().width > 0 && sprite->getContentSize().height > 0) {
            const float fitScale = std::min(size.width / sprite->getContentSize().width, size.height / sprite->getContentSize().height);
            sprite->setScale(fitScale);
        }
        sprite->setPosition(position);
        parent->addChild(sprite, zOrder);
        return sprite;
    }

    auto sprite = LazySprite::create(size, false);
    if (!sprite) {
        return nullptr;
    }
    sprite->setID("gauntlet-image");
    sprite->setAutoResize(true);
    sprite->setPosition(position);
    sprite->setLoadCallback([sprite, url](geode::Result<> const& result) {
        if (!result) {
            return;
        }
        if (auto texture = sprite->getTexture()) {
            gdx::cacheGauntletTexture(url, texture);
        }
    });
    sprite->loadFromUrl(url, CCImage::kFmtPng, true);
    parent->addChild(sprite, zOrder);
    return sprite;
}

static cocos2d::CCNode* createGauntletCreditsTagCell(const matjson::Value& tag) {
    std::string tagName = tag["name"].asString().unwrapOr("Tag");
    if (tagName.empty()) {
        tagName = "Tag";
    }

    // @geode-ignore(unknown-resource)
    auto tagCell = NineSlice::createWithSpriteFrameName("geode.loader/tab-bg.png");
    if (!tagCell) {
        return nullptr;
    }

    auto r = static_cast<GLubyte>(tag["r"].asInt().unwrapOr(255));
    auto g = static_cast<GLubyte>(tag["g"].asInt().unwrapOr(255));
    auto b = static_cast<GLubyte>(tag["b"].asInt().unwrapOr(255));
    tagCell->setColor({r, g, b});
    tagCell->setOpacity(255);

    auto tagLabel = CCLabelBMFont::create(tagName.c_str(), "bigFont.fnt");
    if (tagLabel) {
        tagLabel->setScale(0.4f);
        tagLabel->setAnchorPoint({0.5f, 0.5f});
        tagLabel->setColor({255, 255, 255});
        tagCell->setContentSize({tagLabel->getScaledContentSize().width + 10.f, tagLabel->getScaledContentSize().height + 10.f});
        tagCell->addChildAtPosition(tagLabel, Anchor::Center, {0.f, 0.f}, false);
    }

    return tagCell;
}

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
    m_gauntlet = gauntlet;
    auto spriteBy = GDXUserInfo::fromJson(gauntlet["spriteBy"]);
    auto suggestedBy = GDXUserInfo::fromJson(gauntlet["suggestedBy"]);

    const float contentWidth = m_mainLayer->getContentWidth();

    auto creditsBg = NineSlice::create("square02_small.png");
    if (creditsBg) {
        creditsBg->setInsets({10, 10, 10, 10});
        creditsBg->setAnchorPoint({0.f, .5f});
        creditsBg->setContentSize({320.f, 140.f});
        creditsBg->setOpacity(100);
        m_mainLayer->addChildAtPosition(creditsBg, Anchor::Left, {20.f, -20.f}, false);
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

    auto spriteByName = spriteBy.username.empty() ? "Unknown" : spriteBy.username.c_str();
    auto spriteByLabel = geode::Button::createWithLabel(spriteByName, "goldFont.fnt", [spriteBy](geode::Button* sender) {
        ProfilePage::create(spriteBy.accountId, false)->show();
    });
    if (spriteByLabel) {
        if (spriteBy.username.empty()) {
            spriteByLabel->setEnabled(false);
            spriteByLabel->setColor({255, 115, 255});
        }
        spriteByLabel->setAnchorPoint({0.f, 0.5f});
        spriteByLabel->setScale(.7f);
        spriteByLabel->setPosition({50.f, creditsBg->getContentSize().height - 45.f});
        creditsBg->addChild(spriteByLabel);
    }

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

    auto suggestedByName = suggestedBy.username.empty() ? "Unknown" : suggestedBy.username.c_str();
    auto suggestedByLabel = geode::Button::createWithLabel(suggestedByName, "goldFont.fnt", [suggestedBy](geode::Button* sender) {
        ProfilePage::create(suggestedBy.accountId, false)->show();
    });
    if (suggestedByLabel) {
        if (suggestedBy.username.empty()) {
            suggestedByLabel->setEnabled(false);
            suggestedByLabel->setColor({255, 115, 255});
        }
        suggestedByLabel->setAnchorPoint({0.f, 0.5f});
        suggestedByLabel->setScale(.7f);
        suggestedByLabel->setPosition({50.f, creditsBg->getContentSize().height - 110.f});
        creditsBg->addChild(suggestedByLabel);
    }

    // gauntlet sprite on the right
    auto gauntletId = gauntlet["id"].asInt().unwrapOr(gauntlet["index"].asInt().unwrapOr(0));
    auto imageUrl = std::string(gdx::baseApiUrl()) + "/gauntlet/gauntlet_" + numToString(gauntletId) + ".png?v2=true";
    auto gauntletSprite = createCachedGauntletSprite(m_mainLayer, imageUrl, {430.f, 102.f}, {120.f, 240.f}, 2);
    if (gauntletSprite) {
        if (auto lazySprite = typeinfo_cast<LazySprite*>(gauntletSprite)) {
            lazySprite->setZOrder(2);
        }
        if (!typeinfo_cast<LazySprite*>(gauntletSprite)) {
            gauntletSprite->setZOrder(2);
        }
    }

    auto gauntletBg = NineSlice::create("square02_small.png");
    if (gauntletBg) {
        gauntletBg->setContentSize({150, 180});
        gauntletBg->setOpacity(100);
        m_mainLayer->addChildAtPosition(gauntletBg, Anchor::Right, {-100.f, -40.f}, false);
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

    auto rateNode = CCNode::create();
    if (rateNode) {
        rateNode->setID("gauntlet-rate-node");
        rateNode->setAnchorPoint({0.5f, 0.5f});
        rateNode->setScale(0.8f);
        m_mainLayer->addChildAtPosition(rateNode, Anchor::TopRight, {-73.f, -25.f}, false);

        auto nodeBg = NineSlice::create("square02_small.png");
        if (nodeBg) {
            nodeBg->setContentSize({130.f, 30.f});
            nodeBg->setOpacity(100);
            rateNode->addChildAtPosition(nodeBg, Anchor::Center, {0.f, 0.f}, false);
        }

        auto feedback = gauntlet["feedback"];
        int likes = feedback["likes"].asInt().unwrapOr(0);
        int dislikes = feedback["dislikes"].asInt().unwrapOr(0);

        auto likesIconSpr = CCSprite::createWithSpriteFrameName("GJ_likesIcon_001.png");
        if (likesIconSpr) {
            likesIconSpr->setPosition({-55.f, 6.f});
            likesIconSpr->setScale(0.5f);
            likesIconSpr->setAnchorPoint({0.f, 0.5f});
            rateNode->addChild(likesIconSpr);
        }

        auto likesLabel = CCLabelBMFont::create(GameToolbox::pointsToString(likes).c_str(), "bigFont.fnt");
        if (likesLabel) {
            likesLabel->limitLabelWidth(50.f, 0.4f, 0.1f);
            likesLabel->setAnchorPoint({0.f, 0.5f});
            likesLabel->setPosition({-55.f, -6.f});
            rateNode->addChild(likesLabel);
        }

        auto dislikesIconSpr = CCSprite::createWithSpriteFrameName("GJ_dislikesIcon_001.png");
        if (dislikesIconSpr) {
            dislikesIconSpr->setPosition({10.f, 8.f});
            dislikesIconSpr->setAnchorPoint({0.f, 0.5f});
            dislikesIconSpr->setScale(0.5f);
            rateNode->addChild(dislikesIconSpr);
        }

        auto dislikesLabel = CCLabelBMFont::create(GameToolbox::pointsToString(dislikes).c_str(), "bigFont.fnt");
        if (dislikesLabel) {
            dislikesLabel->limitLabelWidth(50.f, 0.4f, 0.1f);
            dislikesLabel->setAnchorPoint({0.f, 0.5f});
            dislikesLabel->setPosition({10.f, -6.f});
            rateNode->addChild(dislikesLabel);
        }
    }

    auto tags = gauntlet["tags"];
    if (tags.isArray() && tags.size() > 0) {
        auto tagMenu = CCMenu::create();
        if (tagMenu) {
            tagMenu->setLayout(RowLayout::create()
                    ->setGap(5.f)
                    ->setAxisAlignment(AxisAlignment::Center)
                    ->setGrowCrossAxis(true)
                    ->setAxisReverse(true)
                    ->setCrossAxisOverflow(false));
            tagMenu->setContentSize({310.f, 25.f});
            tagMenu->setPosition({0.f, 0.f});
            tagMenu->setZOrder(2);

            for (auto i = 0u; i < tags.size(); ++i) {
                if (auto tagCell = createGauntletCreditsTagCell(tags[i])) {
                    auto tagButton = CCMenuItemSpriteExtra::create(tagCell, this, menu_selector(GDXGauntletCreditsPopup::onTagCell));
                    if (tagButton) {
                        tagButton->setTag(static_cast<int>(i));
                        tagMenu->addChild(tagButton);
                    }
                }
            }

            tagMenu->updateLayout();
            m_mainLayer->addChildAtPosition(tagMenu, Anchor::BottomLeft, {180.f, 30.f}, false);
        }
        auto tagsBg = NineSlice::create("square02_small.png");
        if (tagsBg) {
            tagsBg->setContentSize({tagMenu->getContentSize().width + 10.f, tagMenu->getContentSize().height + 10.f});
            tagsBg->setOpacity(100);
            m_mainLayer->addChildAtPosition(tagsBg, Anchor::BottomLeft, {180.f, 30.f}, false);
        }
    }

    return true;
}

void GDXGauntletCreditsPopup::onTagCell(CCObject* sender) {
    auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
    if (!btn) {
        return;
    }

    auto index = btn->getTag();
    auto tags = m_gauntlet["tags"];
    if (!tags.isArray() || index < 0 || static_cast<size_t>(index) >= tags.size()) {
        return;
    }

    auto tag = tags[static_cast<size_t>(index)];
    std::string name = tag["name"].asString().unwrapOr("Tag");
    if (name.empty()) {
        name = "Tag";
    }
    std::string description = tag["description"].asString().unwrapOr("No description available.");
    if (description.empty()) {
        description = "No description available.";
    }

    MDPopup::create(name.c_str(), description.c_str(), "OK")->show();
}
