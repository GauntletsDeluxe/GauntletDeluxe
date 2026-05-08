#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <algorithm>
#include <cmath>
#include <cue/RepeatingBackground.hpp>
#include "Geode/ui/Layout.hpp"
#include "GDXGauntletLevelsLayer.hpp"

using namespace geode::prelude;

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

GDXGauntletLevelsLayer* GDXGauntletLevelsLayer::create(CCArray* levels, const std::string& title, const cocos2d::ccColor3B& color) {
    auto ret = new GDXGauntletLevelsLayer();
    if (ret && ret->init(levels, title, color)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool GDXGauntletLevelsLayer::init(CCArray* levels, const std::string& title, const cocos2d::ccColor3B& color) {
    m_gauntletTitle = title;
    m_backgroundColor = color;
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

    for (auto i = 0u; i < m_levels.size(); ++i) {
        const auto& entry = m_levels[i];
        auto gauntletSprite = CCSprite::createWithSpriteFrameName("GDX_gauntletUnknown.png"_spr);
        if (!gauntletSprite) {
            continue;
        }

        const float iconScale = 0.75f;
        gauntletSprite->setScale(iconScale);

        auto spriteShadow = CCSprite::createWithSpriteFrameName("GDX_gauntletUnknown.png"_spr);
        if (spriteShadow) {
            spriteShadow->setColor({0, 0, 0});
            spriteShadow->setOpacity(120);
            spriteShadow->setScaleX(1.f);
            spriteShadow->setScaleY(1.1f);
            spriteShadow->setPosition(gauntletSprite->getContentSize() / 2.f - CCPoint{0, 5.f});
            gauntletSprite->addChild(spriteShadow, -1);
        }

        auto nameLabel = CCLabelBMFont::create(entry.levelName.c_str(), "bigFont.fnt");
        nameLabel->setAlignment(kCCTextAlignmentCenter);
        nameLabel->setAnchorPoint({0.5f, 0.f});
        nameLabel->setPosition({gauntletSprite->getContentSize().width / 2.f, gauntletSprite->getContentSize().height + 16.f});
        nameLabel->setScale(0.5f);
        gauntletSprite->addChild(nameLabel, 2);

        auto nameLabelShadow = CCLabelBMFont::create(entry.levelName.c_str(), "bigFont.fnt");
        nameLabelShadow->setAlignment(kCCTextAlignmentCenter);
        nameLabelShadow->setAnchorPoint({0.5f, 0.f});
        nameLabelShadow->setPosition({nameLabel->getPositionX() + 2.f, nameLabel->getPositionY() - 2.f});
        nameLabelShadow->setColor({0, 0, 0});
        nameLabelShadow->setOpacity(120);
        nameLabelShadow->setScale(0.5f);
        gauntletSprite->addChild(nameLabelShadow, 1);

        auto creatorLabel = CCLabelBMFont::create(("By " + entry.creatorName).c_str(), "goldFont.fnt");
        creatorLabel->setAlignment(kCCTextAlignmentCenter);
        creatorLabel->setAnchorPoint({0.5f, 1.f});
        creatorLabel->setPosition({gauntletSprite->getContentSize().width / 2.f, nameLabel->getPositionY()});
        creatorLabel->limitLabelWidth(120.f, 0.5f, 0.35f);
        gauntletSprite->addChild(creatorLabel, 2);

        auto creatorLabelShadow = CCLabelBMFont::create(("By " + entry.creatorName).c_str(), "goldFont.fnt");
        creatorLabelShadow->setAlignment(kCCTextAlignmentCenter);
        creatorLabelShadow->setAnchorPoint({0.5f, 1.f});
        creatorLabelShadow->setPosition({creatorLabel->getPositionX() + 2.f, creatorLabel->getPositionY() - 2.f});
        creatorLabelShadow->setColor({0, 0, 0});
        creatorLabelShadow->setOpacity(120);
        creatorLabelShadow->limitLabelWidth(120.f, 0.5f, 0.35f);
        gauntletSprite->addChild(creatorLabelShadow, 1);

        auto rewardNode = CCNode::create();
        rewardNode->setPosition({gauntletSprite->getContentSize().width / 2.f, -10});
        gauntletSprite->addChild(rewardNode, 2);

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

        auto rowButton = CCMenuItemSpriteExtra::create(gauntletSprite, this, menu_selector(GDXGauntletLevelsLayer::onLevelClicked));
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
        const int dotCount = 5;
        const float baseCurve = 0.25f;
        for (auto j = 0u; j + 1 < centers.size(); ++j) {
            auto a = centers[j];
            auto b = centers[j + 1];
            float dx = b.x - a.x;
            float dy = b.y - a.y;
            float midX = (a.x + b.x) * 0.5f;
            float midY = (a.y + b.y) * 0.5f;
            float perpX = -dy;
            float perpY = dx;
            float perpLen = std::sqrt(perpX * perpX + perpY * perpY);
            if (perpLen != 0.0f) {
                perpX /= perpLen;
                perpY /= perpLen;
            }
            float curveStrength = std::max(50.f, std::abs(dx) * baseCurve);
            float sign = (j % 2 == 0) ? -1.f : 1.f;
            float controlX = midX + perpX * curveStrength * sign;
            float controlY = midY + perpY * curveStrength * sign;

            for (int k = 1; k <= dotCount; ++k) {
                float t = static_cast<float>(k) / static_cast<float>(dotCount + 1);
                float inv = 1.f - t;
                float x = inv * inv * a.x + 2.f * inv * t * controlX + t * t * b.x;
                float y = inv * inv * a.y + 2.f * inv * t * controlY + t * t * b.y;
                auto dot = CCSprite::createWithSpriteFrameName("uiDot_001.png");
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
