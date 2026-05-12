#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include "../include/GDXConstant.hpp"
#include <algorithm>
#include <cmath>
#include <unordered_set>
#include <cue/RepeatingBackground.hpp>
#include <asp/fs.hpp>
#include "Geode/ui/Layout.hpp"
#include "Geode/ui/LazySprite.hpp"
#include "../popup/GDXGauntletCreditsPopup.hpp"
#include "GDXGauntletLevelsLayer.hpp"

using namespace geode::prelude;

namespace {
    static asp::fs::path getCompletedGauntletLevelsPath() {
        auto dir = geode::dirs::getModsSaveDir() / geode::Mod::get()->getID();
        if (auto res = asp::fs::createDirAll(dir); !res) {
            log::warn("Failed to create completed levels save directory: {}", res.unwrapErr().message());
        }
        return dir / "completed_gauntlet_levels.json";
    }

    static std::unordered_set<int> loadCompletedGauntletLevels() {
        std::unordered_set<int> out;
        auto path = getCompletedGauntletLevelsPath();
        if (!asp::fs::isFile(path).unwrapOr(false)) {
            return out;
        }

        auto content = asp::fs::readToString(path);
        if (!content) {
            log::warn("Failed to read completed gauntlet levels file: {}", content.unwrapErr());
            return out;
        }

        auto jsonResult = matjson::parse(content.unwrap());
        if (!jsonResult) {
            log::warn("Failed to parse completed gauntlet levels JSON: {}", jsonResult.unwrapErr());
            return out;
        }

        auto data = std::move(jsonResult).unwrap();
        if (data.isObject() && data["completedLevels"].isArray()) {
            data = data["completedLevels"];
        }
        if (!data.isArray()) {
            return out;
        }

        for (auto const& value : data) {
            if (!value.isNumber()) {
                continue;
            }
            auto maybeId = value.asInt();
            if (maybeId) {
                out.insert(static_cast<int>(maybeId.unwrap()));
            }
        }
        return out;
    }
}

static CCNode* findChildByIDRecursive(CCNode* node, const std::string& id) {
    if (!node) {
        return nullptr;
    }
    if (node->getID() == id) {
        return node;
    }
    auto children = node->getChildren();
    if (!children) {
        return nullptr;
    }
    for (auto i = 0u; i < children->count(); ++i) {
        auto child = static_cast<CCNode*>(children->objectAtIndex(i));
        if (!child) {
            continue;
        }
        if (auto found = findChildByIDRecursive(child, id)) {
            return found;
        }
    }
    return nullptr;
}

static bool isLevelUnlocked(std::size_t index, std::vector<GDXGauntletLevelEntry> const& levels, std::unordered_set<int> const& completedLevels) {
    if (index >= levels.size()) {
        return false;
    }
    if (completedLevels.contains(levels[index].levelId)) {
        return true;
    }
    if (index == 0) {
        return true;
    }
    for (std::size_t j = 0; j < index; ++j) {
        if (!completedLevels.contains(levels[j].levelId)) {
            return false;
        }
    }
    return true;
}

static std::string getStringFromDict(CCDictionary* dict, const char* key) {
    if (!dict) {
        return "";
    }
    auto value = static_cast<CCString*>(dict->objectForKey(key));
    return value ? value->getCString() : std::string();
}

static int getIntFromDict(CCDictionary* dict, const char* key) {
    if (!dict) {
        return 0;
    }
    auto value = static_cast<CCInteger*>(dict->objectForKey(key));
    return value ? value->getValue() : 0;
}

static std::string formatBackgroundName(int bgIndex) {
    int index = std::clamp(bgIndex, 1, 59);
    return fmt::format("game_bg_{:02d}_001.png", index);
}

static LazySprite* createLazySpriteWithFallback(CCNode* parent, const std::string& url, const CCPoint& position, const CCSize& size, int zOrder = 2) {
    auto sprite = LazySprite::create(size, false);
    if (!sprite) {
        return nullptr;
    }

    sprite->setID("gauntlet-image");
    sprite->setAutoResize(true);
    sprite->setPosition(position);

    auto fallbackShadow = CCSprite::createWithSpriteFrameName("GDX_gauntletUnknown.png"_spr);
    if (fallbackShadow) {
        fallbackShadow->setColor({0, 0, 0});
        fallbackShadow->setOpacity(50);
        fallbackShadow->setScaleX(1.f);
        fallbackShadow->setScaleY(1.1f);
        fallbackShadow->setPosition(position);
        fallbackShadow->setPositionY(position.y - 10.f);
        if (parent) {
            parent->addChild(fallbackShadow, zOrder - 2);
        }
    }

    auto fallbackSprite = CCSprite::createWithSpriteFrameName("GDX_gauntletUnknown.png"_spr);
    if (fallbackSprite) {
        fallbackSprite->setPosition(position);
        if (parent) {
            parent->addChild(fallbackSprite, zOrder - 1);
        }
    }

    sprite->setLoadCallback([fallbackSprite, fallbackShadow, url, sprite, position, size, parent, zOrder](geode::Result<> const& result) {
        if (!result) {
            return;
        }
        if (fallbackSprite) {
            fallbackSprite->removeFromParent();
        }
        if (fallbackShadow) {
            fallbackShadow->removeFromParent();
        }
        if (auto texture = sprite->getTexture()) {
            gdx::cacheGauntletTexture(url, texture);
            auto shadow = CCSprite::createWithTexture(texture);
            if (shadow) {
                if (shadow->getContentSize().width > 0 && shadow->getContentSize().height > 0) {
                    shadow->setScaleX(size.width / shadow->getContentSize().width);
                    shadow->setScaleY(size.height / shadow->getContentSize().height);
                }
                shadow->setPosition(position);
                shadow->setColor({0, 0, 0});
                shadow->setOpacity(50);
                if (parent) {
                    parent->addChild(shadow, zOrder - 2);
                }
            }
        }
    });
    sprite->loadFromUrl(url, CCImage::kFmtPng, true);

    if (parent) {
        parent->addChild(sprite, zOrder);
    }
    return sprite;
}

static CCSprite* createRemoteSprite(CCNode* parent, const std::string& url, const CCPoint& position, const CCSize& size, int zOrder = 2) {
    if (auto texture = gdx::findGauntletTexture(url)) {
        if (texture->getName() != 0) {
            auto sprite = CCSprite::createWithTexture(texture);
            if (sprite) {
                sprite->setID("gauntlet-image");
                if (sprite->getContentSize().width > 0 && sprite->getContentSize().height > 0) {
                    const float fitScale = std::min(
                        size.width / sprite->getContentSize().width,
                        size.height / sprite->getContentSize().height);
                    sprite->setScale(fitScale);
                }
                sprite->setPosition(position);
                if (parent) {
                    parent->addChild(sprite, zOrder);
                }

                auto shadow = CCSprite::createWithTexture(texture);
                if (shadow) {
                    if (shadow->getContentSize().width > 0 && shadow->getContentSize().height > 0) {
                        const float fitScale = std::min(
                            size.width / shadow->getContentSize().width,
                            size.height / shadow->getContentSize().height);
                        shadow->setScale(fitScale);
                        shadow->setScaleY(fitScale + 0.1f);
                    }
                    shadow->setPosition(position);
                    shadow->setPositionY(position.y - 10.f);
                    shadow->setColor({0, 0, 0});
                    shadow->setOpacity(50);
                    if (parent) {
                        parent->addChild(shadow, zOrder - 2);
                    }
                }

                return sprite;
            }
        }
    }
    return createLazySpriteWithFallback(parent, url, position, size, zOrder);
}

GDXGauntletLevelsLayer* GDXGauntletLevelsLayer::create(CCArray* levels, const std::string& title, const cocos2d::ccColor3B& color, int gauntletIndex, const matjson::Value& gauntletData) {
    auto ret = new GDXGauntletLevelsLayer();
    if (ret && ret->init(levels, title, color, gauntletIndex, gauntletData)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool GDXGauntletLevelsLayer::init(CCArray* levels, const std::string& title, const cocos2d::ccColor3B& color, int gauntletIndex, const matjson::Value& gauntletData) {
    m_gauntletTitle = title;
    m_backgroundColor = color;
    m_gauntletIndex = gauntletIndex;
    m_gauntletData = gauntletData;
    if (!CCLayer::init()) {
        return false;
    }

    if (levels) {
        for (auto i = 0u; i < levels->count(); ++i) {
            auto raw = levels->objectAtIndex(i);
            auto dict = static_cast<CCDictionary*>(raw);
            if (!dict) {
                continue;
            }

            GDXGauntletLevelEntry entry;
            entry.levelId = getIntFromDict(dict, "levelId");
            entry.levelName = getStringFromDict(dict, "levelName");
            entry.creatorName = getStringFromDict(dict, "creatorName");
            entry.songName = getStringFromDict(dict, "songName");
            entry.songId = getIntFromDict(dict, "songId");
            entry.reward = getIntFromDict(dict, "reward");
            m_levels.push_back(entry);
        }
    }

    addBackButton(this, BackButtonStyle::Green);

    auto bgIndex = m_gauntletData.isObject() ? m_gauntletData["bgIndex"].asInt().unwrapOr(14) : 14;
    auto bgName = formatBackgroundName(bgIndex);
    auto bg = cue::RepeatingBackground::create(bgName.c_str(), 1.0f, cue::RepeatMode::X, CCDirector::sharedDirector()->getWinSize());
    bg->setColor({static_cast<GLubyte>(m_backgroundColor.r), static_cast<GLubyte>(m_backgroundColor.g), static_cast<GLubyte>(m_backgroundColor.b)});
    bg->setSpeed(0.0f);
    this->addChild(bg, -5);

    auto winSize = CCDirector::sharedDirector()->getWinSize();
    auto titleLabel = CCLabelBMFont::create(m_gauntletTitle.c_str(), "goldFont.fnt");
    titleLabel->setScale(0.9f);
    titleLabel->setZOrder(10);
    titleLabel->setAnchorPoint({0.5f, 0.5f});

    auto titleShadow = CCLabelBMFont::create(m_gauntletTitle.c_str(), "goldFont.fnt");
    titleShadow->setScale(0.9f);
    titleShadow->setColor({0, 0, 0});
    titleShadow->setOpacity(50);
    titleShadow->setZOrder(10);
    titleShadow->setAnchorPoint({0.5f, 0.5f});

    this->addChildAtPosition(titleShadow, Anchor::Top, {4, -24}, false);
    this->addChildAtPosition(titleLabel, Anchor::Top, {0, -20}, false);

    auto infoIconSpr = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
    if (infoIconSpr) {
        auto infoBtn = CCMenuItemSpriteExtra::create(infoIconSpr, this, menu_selector(GDXGauntletLevelsLayer::onGauntletInfo));
        auto bottomRightMenu = CCMenu::create(infoBtn, nullptr);
        if (bottomRightMenu) {
            bottomRightMenu->setPosition({winSize.width - 30, 70});
            bottomRightMenu->setContentHeight(100);
            bottomRightMenu->setLayout(ColumnLayout::create()->setGap(5.f)->setAxisAlignment(AxisAlignment::Start));
            this->addChild(bottomRightMenu, 2);
            bottomRightMenu->updateLayout();
        }
    }

    if (m_levels.empty()) {
        auto emptyLabel = CCLabelBMFont::create("No levels found.", "goldFont.fnt");
        emptyLabel->setPosition({winSize.width / 2, winSize.height / 2});
        this->addChild(emptyLabel, 2);
        return true;
    }

    m_levelsMenu = CCMenu::create();
    m_levelsMenu->setPosition({0, 0});
    this->addChild(m_levelsMenu, 1);

    const float buttonY = winSize.height / 2.f;
    const float leftMargin = 80.f;
    const float rightMargin = 80.f;
    const float availableWidth = winSize.width - leftMargin - rightMargin;
    const float stepX = m_levels.size() > 1 ? availableWidth / static_cast<float>(m_levels.size() - 1) : 0.f;
    std::vector<CCPoint> centers;
    bool showPath = true;

    auto completedLevels = loadCompletedGauntletLevels();
    for (auto i = 0u; i < m_levels.size(); ++i) {
        const auto& entry = m_levels[i];
        bool isCompleted = completedLevels.contains(entry.levelId);
        bool isUnlocked = isLevelUnlocked(i, m_levels, completedLevels);

        auto rowNode = CCNodeRGBA::create();
        rowNode->setContentSize({90.f, 90.f});
        rowNode->setScale(0.8f);
        rowNode->setCascadeOpacityEnabled(true);

        auto imageContainer = CCNodeRGBA::create();
        if (imageContainer) {
            imageContainer->setID("gauntlet-container");
            imageContainer->setContentSize({70.f, 90.f});
            imageContainer->setAnchorPoint({0.5f, 0.5f});
            imageContainer->setPosition({rowNode->getContentSize().width / 2.f, rowNode->getContentSize().height / 2.f});
            rowNode->addChild(imageContainer, 2);
        }

        const CCSize iconSize = {70.f, 90.f};
        const CCPoint iconCenter = {iconSize.width / 2.f, iconSize.height / 2.f};
        auto imageUrl = std::string(gdx::BASE_API_URL) + "/gauntlet/gauntlet_" + numToString(m_gauntletIndex) + ".png?v2=true";

        auto imageTarget = imageContainer ? imageContainer : rowNode;
        auto imagePosition = imageContainer
                                 ? ccp(imageTarget->getContentSize().width / 2.f, imageTarget->getContentSize().height / 2.f)
                                 : ccp(rowNode->getContentSize().width / 2.f, rowNode->getContentSize().height / 2.f);
        auto gauntletSprite = createRemoteSprite(imageTarget, imageUrl, imagePosition, iconSize);
        if (!gauntletSprite) {
            continue;
        }

        if (!isUnlocked) {
            gauntletSprite->setColor({120, 120, 120});
            gauntletSprite->setOpacity(200);
            auto lockIcon = CCSprite::createWithSpriteFrameName("GJ_lockGray_001.png");
            if (lockIcon) {
                lockIcon->setID("lock-icon");
                lockIcon->setScale(0.8f);
                lockIcon->setPosition({imageTarget->getContentSize().width / 2.f, imageTarget->getContentSize().height / 2.f});
                imageTarget->addChild(lockIcon, 5);
            }
        }

        auto containerSize = imageTarget->getContentSize();
        auto containerCenter = imageContainer ? imageContainer->getPosition() : ccp(rowNode->getContentSize().width / 2.f, rowNode->getContentSize().height / 2.f);
        auto labelCenterX = containerCenter.x;
        auto nameY = containerCenter.y + containerSize.height / 2.f + 16.f;

        if (completedLevels.contains(entry.levelId)) {
            auto completedIcon = CCSprite::createWithSpriteFrameName("GJ_completesIcon_001.png");
            if (completedIcon) {
                completedIcon->setID("completed-icon");
                completedIcon->setScale(1.5f);
                completedIcon->setPosition({containerSize.width / 2.f, containerSize.height / 2.f});
                imageTarget->addChild(completedIcon, 4);
            }
        }

        auto nameLabel = CCLabelBMFont::create(entry.levelName.c_str(), "bigFont.fnt");
        nameLabel->setID("gauntlet-name");
        nameLabel->setAlignment(kCCTextAlignmentCenter);
        nameLabel->setAnchorPoint({0.5f, 0.f});
        nameLabel->setPosition({labelCenterX, nameY});
        nameLabel->setScale(0.5f);
        nameLabel->setVisible(isUnlocked);
        rowNode->addChild(nameLabel, 2);

        auto nameLabelShadow = CCLabelBMFont::create(entry.levelName.c_str(), "bigFont.fnt");
        nameLabelShadow->setID("gauntlet-name-shadow");
        nameLabelShadow->setAlignment(kCCTextAlignmentCenter);
        nameLabelShadow->setAnchorPoint({0.5f, 0.f});
        nameLabelShadow->setPosition({nameLabel->getPositionX() + 2.f, nameLabel->getPositionY() - 2.f});
        nameLabelShadow->setColor({0, 0, 0});
        nameLabelShadow->setOpacity(50);
        nameLabelShadow->setScale(0.5f);
        nameLabelShadow->setVisible(isUnlocked);
        rowNode->addChild(nameLabelShadow, 1);

        auto creatorLabel = CCLabelBMFont::create(("By " + entry.creatorName).c_str(), "goldFont.fnt");
        creatorLabel->setID("gauntlet-creator");
        creatorLabel->setAlignment(kCCTextAlignmentCenter);
        creatorLabel->setAnchorPoint({0.5f, 1.f});
        creatorLabel->setPosition({labelCenterX, nameLabel->getPositionY()});
        creatorLabel->limitLabelWidth(120.f, 0.5f, 0.35f);
        creatorLabel->setVisible(isUnlocked);
        rowNode->addChild(creatorLabel, 2);

        auto creatorLabelShadow = CCLabelBMFont::create(("By " + entry.creatorName).c_str(), "goldFont.fnt");
        creatorLabelShadow->setID("gauntlet-creator-shadow");
        creatorLabelShadow->setAlignment(kCCTextAlignmentCenter);
        creatorLabelShadow->setAnchorPoint({0.5f, 1.f});
        creatorLabelShadow->setPosition({creatorLabel->getPositionX() + 2.f, creatorLabel->getPositionY() - 2.f});
        creatorLabelShadow->setColor({0, 0, 0});
        creatorLabelShadow->setOpacity(50);
        creatorLabelShadow->limitLabelWidth(120.f, 0.5f, 0.35f);
        creatorLabelShadow->setVisible(isUnlocked);
        rowNode->addChild(creatorLabelShadow, 1);

        auto rewardNode = CCNode::create();
        rewardNode->setID("gauntlet-reward-node");
        rewardNode->setPosition({labelCenterX, containerCenter.y - containerSize.height / 2.f - 10.f});
        rewardNode->setVisible(isUnlocked);
        rowNode->addChild(rewardNode, 2);

        auto rewardLabel = CCLabelBMFont::create((numToString(entry.reward)).c_str(), "bigFont.fnt");
        rewardLabel->setAlignment(kCCTextAlignmentCenter);
        rewardLabel->setAnchorPoint({1.f, 0.5f});
        rewardLabel->setScale(0.6f);
        rewardLabel->limitLabelWidth(100.f, 0.45f, 0.35f);
        rewardLabel->setPosition({-5.f, 0.f});
        rewardNode->addChild(rewardLabel, 2);

        auto rewardLabelShadow = CCLabelBMFont::create((numToString(entry.reward)).c_str(), "bigFont.fnt");
        rewardLabelShadow->setAlignment(kCCTextAlignmentCenter);
        rewardLabelShadow->setAnchorPoint({1.f, 0.5f});
        rewardLabelShadow->setScale(0.6f);
        rewardLabelShadow->setPosition({rewardLabel->getPositionX() + 2.f, rewardLabel->getPositionY() - 2.f});
        rewardLabelShadow->setColor({0, 0, 0});
        rewardLabelShadow->setOpacity(50);
        rewardLabelShadow->limitLabelWidth(100.f, 0.45f, 0.35f);
        rewardNode->addChild(rewardLabelShadow, 1);

        auto levelPointSpr = CCSprite::createWithSpriteFrameName("GDX_levelPoint.png"_spr);
        if (levelPointSpr) {
            levelPointSpr->setScale(0.25f);
            levelPointSpr->setAnchorPoint({0.f, 0.5f});
            levelPointSpr->setPosition({0, 0});
            rewardNode->addChild(levelPointSpr, 2);
        }

        auto levelPointShadow = CCSprite::createWithSpriteFrameName("GDX_levelPoint.png"_spr);
        if (levelPointShadow) {
            levelPointShadow->setColor({0, 0, 0});
            levelPointShadow->setOpacity(50);
            levelPointShadow->setScale(levelPointSpr ? levelPointSpr->getScale() : 0.25f);
            levelPointShadow->setAnchorPoint(levelPointSpr ? levelPointSpr->getAnchorPoint() : ccp(0.f, 0.5f));
            levelPointShadow->setPosition(levelPointSpr ? ccpAdd(levelPointSpr->getPosition(), ccp(2.f, -2.f)) : ccp(2.f, -2.f));
            rewardNode->addChild(levelPointShadow, 1);
        }

        auto rowButton = CCMenuItemSpriteExtra::create(rowNode, this, menu_selector(GDXGauntletLevelsLayer::onLevelClicked));
        rowButton->setTag(entry.levelId);
        if (!isUnlocked) {
            rowButton->setEnabled(false);
            rowButton->setOpacity(180);
        }
        float yOffset = ((i + 1) % 2 == 1) ? -60.f : 60.f;
        rowButton->setPosition({leftMargin + stepX * static_cast<float>(i), buttonY + yOffset});
        if (m_levelsMenu) {
            m_levelsMenu->addChild(rowButton);
        }

        centers.push_back(rowButton->getPosition());
    }

    if (showPath) {
        // draw an outward-curving path between gauntlets
        // i wanna commit die with this path math stuff frfr
        // i kinda forgor what all of these means so sure comment it
        const int dotCount = 5;
        const float baseCurve = 0.25f;
        for (auto j = 0u; j + 1 < centers.size(); ++j) {
            auto a = centers[j];                                       // point a
            auto b = centers[j + 1];                                   // point b
            float dx = b.x - a.x;                                      // vector from a to b
            float dy = b.y - a.y;                                      // vector from a to b
            float midX = (a.x + b.x) * 0.5f;                           // midpoint between a and b
            float midY = (a.y + b.y) * 0.5f;                           // midpoint between a and b
            float perpX = -dy;                                         // perpendicular vector to ab
            float perpY = dx;                                          // perpendicular vector to ab
            float perpLen = std::sqrt(perpX * perpX + perpY * perpY);  // length of the perpendicular vector
            if (perpLen != 0.0f) {                                     // normalize the perpendicular vector
                perpX /= perpLen;
                perpY /= perpLen;
            }
            float curveStrength = std::max(50.f, std::abs(dx) * baseCurve);  // curve strength based on distance, with a minimum value
            float controlX = midX + perpX * curveStrength * 1;               // control point for the quadratic Bezier curve
            float controlY = midY + perpY * curveStrength * 1;
            // i could do different type of curves n stuff but this is fine
            // leetcode questions ah moment am i right???
            for (int k = 1; k <= dotCount; ++k) {
                float t = static_cast<float>(k) / static_cast<float>(dotCount + 1);  // parameter from 0 to 1 along the curve
                float inv = 1.f - t;                                                 // inverse of t for the Bezier formula
                float x = inv * inv * a.x + 2.f * inv * t * controlX + t * t * b.x;  // quadratic Bezier formula for x coordinate
                float y = inv * inv * a.y + 2.f * inv * t * controlY + t * t * b.y;  // quadratic Bezier formula for y coordinate
                auto dot = CCSprite::createWithSpriteFrameName("uiDot_001.png");     // simple dot sprite for the path
                if (!dot) {
                    continue;
                }
                dot->setPosition({x, y});
                m_levelsMenu->addChild(dot, -3);
            }
        }
    }

    this->setKeypadEnabled(true);
    this->scheduleUpdate();

    return true;
}

void GDXGauntletLevelsLayer::onEnter() {
    CCLayer::onEnter();
    this->refreshCompletionIcons();
}

void GDXGauntletLevelsLayer::refreshCompletionIcons() {
    if (!m_levelsMenu) {
        return;
    }

    auto completedLevels = loadCompletedGauntletLevels();
    auto children = m_levelsMenu->getChildren();
    for (auto i = 0u; i < children->count(); ++i) {
        auto child = static_cast<CCNode*>(children->objectAtIndex(i));
        auto button = typeinfo_cast<CCMenuItemSpriteExtra*>(child);
        if (!button) {
            continue;
        }

        auto levelId = button->getTag();
        auto it = std::find_if(m_levels.begin(), m_levels.end(), [levelId](auto const& entry) {
            return entry.levelId == levelId;
        });
        if (it == m_levels.end()) {
            continue;
        }
        auto levelIndex = static_cast<std::size_t>(std::distance(m_levels.begin(), it));
        bool isCompleted = completedLevels.contains(levelId);
        bool isUnlocked = isLevelUnlocked(levelIndex, m_levels, completedLevels);

        CCNode* gauntletContainer = nullptr;
        auto buttonChildren = button->getChildren();
        for (auto j = 0u; j < buttonChildren->count(); ++j) {
            auto inner = static_cast<CCNode*>(buttonChildren->objectAtIndex(j));
            if (inner && inner->getID() == "gauntlet-container") {
                gauntletContainer = inner;
                break;
            }
        }
        if (!gauntletContainer) {
            for (auto j = 0u; j < buttonChildren->count(); ++j) {
                auto inner = static_cast<CCNode*>(buttonChildren->objectAtIndex(j));
                gauntletContainer = typeinfo_cast<CCNodeRGBA*>(inner);
                if (gauntletContainer) {
                    break;
                }
            }
        }
        if (!gauntletContainer) {
            continue;
        }

        auto iconNode = findChildByIDRecursive(gauntletContainer, "gauntlet-image");
        if (auto iconSprite = typeinfo_cast<CCSprite*>(iconNode)) {
            iconSprite->setColor(isUnlocked ? ccColor3B{255, 255, 255} : ccColor3B{120, 120, 120});
            iconSprite->setOpacity(isUnlocked ? 255 : 200);
        } else if (auto lazySprite = typeinfo_cast<LazySprite*>(iconNode)) {
            lazySprite->setColor(isUnlocked ? ccColor3B{255, 255, 255} : ccColor3B{120, 120, 120});
            lazySprite->setOpacity(isUnlocked ? 255 : 200);
        }

        auto lockIcon = findChildByIDRecursive(button, "lock-icon");
        if (!isUnlocked) {
            if (!lockIcon) {
                auto lockSprite = CCSprite::createWithSpriteFrameName("GJ_lockGray_001.png");
                if (lockSprite) {
                    lockSprite->setID("lock-icon");
                    lockSprite->setScale(0.8f);
                    lockSprite->setPosition({gauntletContainer->getContentSize().width / 2.f, gauntletContainer->getContentSize().height / 2.f});
                    gauntletContainer->addChild(lockSprite, 5);
                }
            }
            button->setEnabled(false);
            button->setOpacity(180);
        } else {
            if (lockIcon) {
                lockIcon->removeFromParent();
            }
            button->setEnabled(true);
            button->setOpacity(255);
        }

        if (auto nameLabel = findChildByIDRecursive(button, "gauntlet-name")) {
            nameLabel->setVisible(isUnlocked);
        }
        if (auto nameLabelShadow = findChildByIDRecursive(button, "gauntlet-name-shadow")) {
            nameLabelShadow->setVisible(isUnlocked);
        }
        if (auto creatorLabel = findChildByIDRecursive(button, "gauntlet-creator")) {
            creatorLabel->setVisible(isUnlocked);
        }
        if (auto creatorLabelShadow = findChildByIDRecursive(button, "gauntlet-creator-shadow")) {
            creatorLabelShadow->setVisible(isUnlocked);
        }
        if (auto rewardNode = findChildByIDRecursive(button, "gauntlet-reward-node")) {
            rewardNode->setVisible(isUnlocked);
        }

        // Find existing completed icon
        CCNode* existingIcon = gauntletContainer->getChildByID("completed-icon");

        if (isCompleted) {
            if (!existingIcon) {
                auto completedIcon = CCSprite::createWithSpriteFrameName("GJ_completesIcon_001.png");
                if (completedIcon) {
                    completedIcon->setID("completed-icon");
                    completedIcon->setScale(1.5f);
                    completedIcon->setPosition({gauntletContainer->getContentSize().width / 2.f, gauntletContainer->getContentSize().height / 2.f});
                    gauntletContainer->addChild(completedIcon, 4);
                }
            }
        } else if (existingIcon) {
            existingIcon->removeFromParent();
        }
    }
}

void GDXGauntletLevelsLayer::onLevelClicked(CCObject* sender) {
    auto button = static_cast<CCMenuItemSpriteExtra*>(sender);
    if (!button) {
        return;
    }

    auto levelId = button->getTag();
    if (levelId <= 0) {
        return;
    }

    auto it = std::find_if(m_levels.begin(), m_levels.end(), [levelId](auto const& entry) {
        return entry.levelId == levelId;
    });
    if (it == m_levels.end()) {
        return;
    }

    const auto& entry = *it;
    auto glm = GameLevelManager::get();
    if (!glm) {
        return;
    }

    auto searchObj = GJSearchObject::create(SearchType::Search, numToString(entry.levelId));
    if (!searchObj) {
        return;
    }

    auto key = std::string(searchObj->getKey());
    auto stored = glm->getStoredOnlineLevels(key.c_str());
    if (stored && stored->count() > 0) {
        auto level = static_cast<GJGameLevel*>(stored->objectAtIndex(0));
        if (level && level->m_levelID == entry.levelId) {
            gdx::setPlayingGauntletLevel(true);
            auto scene = LevelInfoLayer::scene(level, false);
            CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(0.5f, scene));
            return;
        }
    }

    if (m_pendingSpinner) {
        m_pendingSpinner->removeFromParent();
        m_pendingSpinner = nullptr;
    }

    m_pendingKey = key;
    m_pendingLevelId = entry.levelId;
    m_pendingTimeout = 10.0f;

    auto spinner = LoadingSpinner::create(36.f);
    if (spinner) {
        spinner->setPosition(button->getPosition());
        spinner->setVisible(true);
        if (m_levelsMenu) {
            m_levelsMenu->addChild(spinner, 10);
        }
        m_pendingSpinner = spinner;
    }

    button->setEnabled(false);
    glm->getOnlineLevels(searchObj);
}

void GDXGauntletLevelsLayer::onGauntletInfo(CCObject* sender) {
    if (!m_gauntletData.isObject()) {
        return;
    }
    GDXGauntletCreditsPopup::create(m_gauntletData)->show();
}

void GDXGauntletLevelsLayer::update(float dt) {
    if (m_pendingKey.empty()) {
        return;
    }

    auto glm = GameLevelManager::get();
    if (!glm) {
        return;
    }

    auto stored = glm->getStoredOnlineLevels(m_pendingKey.c_str());
    if (stored && stored->count() > 0) {
        auto level = static_cast<GJGameLevel*>(stored->objectAtIndex(0));
        if (level && level->m_levelID == m_pendingLevelId) {
            gdx::setPlayingGauntletLevel(true);
            auto scene = LevelInfoLayer::scene(level, false);
            CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(0.5f, scene));

            if (m_pendingSpinner) {
                m_pendingSpinner->removeFromParent();
                m_pendingSpinner = nullptr;
            }
            if (m_levelsMenu) {
                auto children = m_levelsMenu->getChildren();
                for (unsigned int i = 0; i < children->count(); ++i) {
                    auto child = static_cast<CCNode*>(children->objectAtIndex(i));
                    auto btn = typeinfo_cast<CCMenuItemSpriteExtra*>(child);
                    if (btn && btn->getTag() == m_pendingLevelId) {
                        btn->setEnabled(true);
                        break;
                    }
                }
            }
            m_pendingKey.clear();
            m_pendingLevelId = -1;
            m_pendingTimeout = 0.0f;
            return;
        }
    }

    m_pendingTimeout -= dt;
    if (m_pendingTimeout <= 0.0f) {
        if (m_pendingSpinner) {
            m_pendingSpinner->removeFromParent();
            m_pendingSpinner = nullptr;
        }
        if (m_levelsMenu) {
            auto children = m_levelsMenu->getChildren();
            for (unsigned int i = 0; i < children->count(); ++i) {
                auto child = static_cast<CCNode*>(children->objectAtIndex(i));
                auto btn = typeinfo_cast<CCMenuItemSpriteExtra*>(child);
                if (btn && btn->getTag() == m_pendingLevelId) {
                    btn->setEnabled(true);
                    break;
                }
            }
        }
        Notification::create("Level not found", NotificationIcon::Warning)->show();
        m_pendingKey.clear();
        m_pendingLevelId = -1;
        m_pendingTimeout = 0.0f;
    }
}

void GDXGauntletLevelsLayer::keyBackClicked() {
    CCDirector::sharedDirector()->popSceneWithTransition(0.5f, PopTransition::kPopTransitionFade);
}
