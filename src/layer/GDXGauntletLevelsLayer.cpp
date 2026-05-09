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

static bool isLazyImageLoaded(LazySprite* image) {
    if (!image) {
        return false;
    }

    auto texture = image->getTexture();
    return texture && texture->getName() != 0;
}

static void removeLoadedGauntletFallbacks(CCMenu* menu) {
    if (!menu) {
        return;
    }

    auto children = menu->getChildren();
    if (!children) {
        return;
    }

    for (auto i = 0u; i < children->count(); ++i) {
        auto child = static_cast<CCNode*>(children->objectAtIndex(i));
        auto btn = typeinfo_cast<CCMenuItemSpriteExtra*>(child);
        if (!btn) {
            continue;
        }

        auto buttonChildren = btn->getChildren();
        if (!buttonChildren) {
            continue;
        }

        LazySprite* lazyImage = nullptr;
        CCNode* fallback = nullptr;
        CCNode* fallbackShadow = nullptr;
        for (auto j = 0u; j < buttonChildren->count(); ++j) {
            auto inner = static_cast<CCNode*>(buttonChildren->objectAtIndex(j));
            if (!inner) {
                continue;
            }

            if (inner->getID() == "gauntlet-image") {
                lazyImage = typeinfo_cast<LazySprite*>(inner);
            } else if (inner->getID() == "gauntlet-fallback") {
                fallback = inner;
            } else if (inner->getID() == "gauntlet-fallback-shadow") {
                fallbackShadow = inner;
            }
        }

        if (lazyImage && isLazyImageLoaded(lazyImage)) {
            if (fallback) {
                fallback->removeFromParent();
            }
            if (fallbackShadow) {
                fallbackShadow->removeFromParent();
            }
        }
    }
}

GDXGauntletLevelsLayer* GDXGauntletLevelsLayer::create(CCArray* levels, const std::string& title, const cocos2d::ccColor3B& color, int gauntletIndex) {
    auto ret = new GDXGauntletLevelsLayer();
    if (ret && ret->init(levels, title, color, gauntletIndex)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool GDXGauntletLevelsLayer::init(CCArray* levels, const std::string& title, const cocos2d::ccColor3B& color, int gauntletIndex) {
    m_gauntletTitle = title;
    m_backgroundColor = color;
    m_gauntletIndex = gauntletIndex;
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

    auto bg = cue::RepeatingBackground::create("game_bg_14_001.png", 1.0f, cue::RepeatMode::X, CCDirector::sharedDirector()->getWinSize());
    bg->setColor({static_cast<GLubyte>(m_backgroundColor.r), static_cast<GLubyte>(m_backgroundColor.g), static_cast<GLubyte>(m_backgroundColor.b)});
    bg->setSpeed(0.0f);
    this->addChild(bg, -5);

    auto winSize = CCDirector::sharedDirector()->getWinSize();
    auto titleLabel = CCLabelBMFont::create(m_gauntletTitle.c_str(), "goldFont.fnt");
    titleLabel->setScale(0.9f);
    titleLabel->setAnchorPoint({0.5f, 0.5f});

    auto titleShadow = CCLabelBMFont::create(m_gauntletTitle.c_str(), "goldFont.fnt");
    titleShadow->setScale(0.9f);
    titleShadow->setColor({0, 0, 0});
    titleShadow->setOpacity(140);
    titleShadow->setAnchorPoint({0.5f, 0.5f});

    this->addChildAtPosition(titleShadow, Anchor::Top, {4, -24}, false);
    this->addChildAtPosition(titleLabel, Anchor::Top, {0, -20}, false);

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
        auto gauntletSprite = LazySprite::create({90.f, 90.f}, false);
        if (!gauntletSprite) {
            continue;
        }

        auto rowNode = CCNodeRGBA::create();
        rowNode->setContentSize({90.f, 90.f});
        rowNode->setScale(0.8f);
        rowNode->setCascadeOpacityEnabled(true);

        const float iconScale = 0.75f;
        gauntletSprite->setScale(iconScale);
        gauntletSprite->setID("gauntlet-image");
        gauntletSprite->setPosition({gauntletSprite->getContentSize().width / 2.f, gauntletSprite->getContentSize().height / 2.f});
        rowNode->addChild(gauntletSprite);

        auto fallbackSprite = CCSprite::createWithSpriteFrameName("GDX_gauntletUnknown.png"_spr);
        auto fallbackShadow = CCSprite::createWithSpriteFrameName("GDX_gauntletUnknown.png"_spr);
        if (fallbackShadow) {
            fallbackShadow->setColor({0, 0, 0});
            fallbackShadow->setOpacity(120);
            fallbackShadow->setScaleX(1.f);
            fallbackShadow->setScaleY(1.1f);
            fallbackShadow->setPosition({gauntletSprite->getContentSize().width / 2.f, gauntletSprite->getContentSize().height / 2.f - 5.f});
            fallbackShadow->setID("gauntlet-fallback-shadow");
            rowNode->addChild(fallbackShadow, -1);
        }
        if (fallbackSprite) {
            fallbackSprite->setID("gauntlet-fallback");
            fallbackSprite->setScale(1.f);
            fallbackSprite->setPosition({gauntletSprite->getContentSize().width / 2.f, gauntletSprite->getContentSize().height / 2.f});
            rowNode->addChild(fallbackSprite, 0);
        }

        auto imageUrl = std::string(gdx::BASE_API_URL) + "/gauntlet/gauntlet_" + numToString(m_gauntletIndex) + ".png";
        gauntletSprite->setAutoResize(true);

        auto gauntletSpriteShadow = LazySprite::create({90.f, 90.f}, false);
        if (gauntletSpriteShadow) {
            gauntletSpriteShadow->setVisible(false);
            gauntletSpriteShadow->setAutoResize(true);
            gauntletSpriteShadow->setPosition({gauntletSprite->getContentSize().width / 2.f, gauntletSprite->getContentSize().height / 2.f - 5.f});
            gauntletSpriteShadow->setLoadCallback([gauntletSpriteShadow](geode::Result<> const& result) {
                if (result && gauntletSpriteShadow) {
                    gauntletSpriteShadow->setVisible(true);
                    gauntletSpriteShadow->setColor({0, 0, 0});
                    gauntletSpriteShadow->setOpacity(120);
                }
            });
            gauntletSpriteShadow->loadFromUrl(imageUrl, CCImage::kFmtPng, true);
            rowNode->addChild(gauntletSpriteShadow, -2);
        }

        gauntletSprite->setLoadCallback([fallbackSprite, fallbackShadow](geode::Result<> const& result) {
            if (result) {
                if (fallbackSprite) {
                    fallbackSprite->removeFromParent();
                }
                if (fallbackShadow) {
                    fallbackShadow->removeFromParent();
                }
            }
        });
        gauntletSprite->loadFromUrl(imageUrl, CCImage::kFmtPng, true);

        if (completedLevels.contains(entry.levelId)) {
            auto completedIcon = CCSprite::createWithSpriteFrameName("GJ_completesIcon_001.png");
            if (completedIcon) {
                completedIcon->setID("completed-icon");
                completedIcon->setScale(1.5f);
                completedIcon->setPosition({gauntletSprite->getContentSize().width / 2.f, gauntletSprite->getContentSize().height / 2.f});
                rowNode->addChild(completedIcon, 4);
            }
        }

        auto nameLabel = CCLabelBMFont::create(entry.levelName.c_str(), "bigFont.fnt");
        nameLabel->setAlignment(kCCTextAlignmentCenter);
        nameLabel->setAnchorPoint({0.5f, 0.f});
        nameLabel->setPosition({gauntletSprite->getContentSize().width / 2.f, gauntletSprite->getContentSize().height + 16.f});
        nameLabel->setScale(0.5f);
        rowNode->addChild(nameLabel, 2);

        auto nameLabelShadow = CCLabelBMFont::create(entry.levelName.c_str(), "bigFont.fnt");
        nameLabelShadow->setAlignment(kCCTextAlignmentCenter);
        nameLabelShadow->setAnchorPoint({0.5f, 0.f});
        nameLabelShadow->setPosition({nameLabel->getPositionX() + 2.f, nameLabel->getPositionY() - 2.f});
        nameLabelShadow->setColor({0, 0, 0});
        nameLabelShadow->setOpacity(120);
        nameLabelShadow->setScale(0.5f);
        rowNode->addChild(nameLabelShadow, 1);

        auto creatorLabel = CCLabelBMFont::create(("By " + entry.creatorName).c_str(), "goldFont.fnt");
        creatorLabel->setAlignment(kCCTextAlignmentCenter);
        creatorLabel->setAnchorPoint({0.5f, 1.f});
        creatorLabel->setPosition({gauntletSprite->getContentSize().width / 2.f, nameLabel->getPositionY()});
        creatorLabel->limitLabelWidth(120.f, 0.5f, 0.35f);
        rowNode->addChild(creatorLabel, 2);

        auto creatorLabelShadow = CCLabelBMFont::create(("By " + entry.creatorName).c_str(), "goldFont.fnt");
        creatorLabelShadow->setAlignment(kCCTextAlignmentCenter);
        creatorLabelShadow->setAnchorPoint({0.5f, 1.f});
        creatorLabelShadow->setPosition({creatorLabel->getPositionX() + 2.f, creatorLabel->getPositionY() - 2.f});
        creatorLabelShadow->setColor({0, 0, 0});
        creatorLabelShadow->setOpacity(120);
        creatorLabelShadow->limitLabelWidth(120.f, 0.5f, 0.35f);
        rowNode->addChild(creatorLabelShadow, 1);

        auto rewardNode = CCNode::create();
        rewardNode->setPosition({gauntletSprite->getContentSize().width / 2.f, -10});
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
        rewardLabelShadow->setOpacity(120);
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
            levelPointShadow->setOpacity(120);
            levelPointShadow->setScale(levelPointSpr ? levelPointSpr->getScale() : 0.25f);
            levelPointShadow->setAnchorPoint(levelPointSpr ? levelPointSpr->getAnchorPoint() : ccp(0.f, 0.5f));
            levelPointShadow->setPosition(levelPointSpr ? ccpAdd(levelPointSpr->getPosition(), ccp(2.f, -2.f)) : ccp(2.f, -2.f));
            rewardNode->addChild(levelPointShadow, 1);
        }

        auto rowButton = CCMenuItemSpriteExtra::create(rowNode, this, menu_selector(GDXGauntletLevelsLayer::onLevelClicked));
        rowButton->setTag(entry.levelId);
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
                this->addChild(dot, 0);
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
        bool isCompleted = completedLevels.contains(levelId);

        cocos2d::CCSprite* gauntletSprite = nullptr;
        auto buttonChildren = button->getChildren();
        for (auto j = 0u; j < buttonChildren->count(); ++j) {
            auto inner = static_cast<CCNode*>(buttonChildren->objectAtIndex(j));
            auto sprite = typeinfo_cast<CCSprite*>(inner);
            if (sprite) {
                gauntletSprite = sprite;
                break;
            }
        }
        if (!gauntletSprite) {
            continue;
        }

        auto existingIcon = gauntletSprite->getChildByID("completed-icon");
        if (isCompleted) {
            if (!existingIcon) {
                auto completedIcon = CCSprite::createWithSpriteFrameName("GJ_completesIcon_001.png");
                if (completedIcon) {
                    completedIcon->setID("completed-icon");
                    completedIcon->setScale(1.5f);
                    completedIcon->setPosition({gauntletSprite->getContentSize().width / 2.f, gauntletSprite->getContentSize().height / 2.f});
                    gauntletSprite->addChild(completedIcon, 4);
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
            m_levelsMenu->addChild(spinner);
        }
        m_pendingSpinner = spinner;
    }

    button->setEnabled(false);
    glm->getOnlineLevels(searchObj);
}

void GDXGauntletLevelsLayer::update(float dt) {
    removeLoadedGauntletFallbacks(m_levelsMenu);
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
