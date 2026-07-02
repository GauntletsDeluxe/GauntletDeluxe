#include "GDXGauntletLayer.hpp"
#include "GDXGauntletLevelsLayer.hpp"
#include "GDXLeaderboardLayer.hpp"
#include "../popup/GDXGauntletManagePopup.hpp"
#include "../popup/GDXLikeItemPopup.hpp"
#include "../popup/GDXShowTagsPopup.hpp"
#include "../popup/GDXTagsFiltersPopup.hpp"
#include "../popup/GDXLinkDiscordPopup.hpp"
#include <Geode/Enums.hpp>
#include <Geode/binding/UploadActionPopup.hpp>
#include <algorithm>
#include <cctype>
#include <Geode/Geode.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/ui/LoadingSpinner.hpp>
#include <Geode/ui/BasedButton.hpp>
#include <Geode/ui/MDPopup.hpp>
#include <Geode/utils/file.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/binding/SetTextPopup.hpp>
#include <asp/fs.hpp>
#include "../include/GDXConstant.hpp"
#include "Geode/ui/BasedButtonSprite.hpp"
#include "Geode/ui/Layout.hpp"
#include "Geode/ui/LazySprite.hpp"
#include "Geode/ui/NineSlice.hpp"
#include <argon/argon.hpp>

using namespace geode::prelude;

namespace {
    // time to copy paste this encoder i found, it works btw
    static std::string urlEncode(std::string const& value) {
        static const char* hex = "0123456789ABCDEF";
        std::string encoded;
        encoded.reserve(value.size() * 3);
        for (unsigned char c : value) {
            if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                encoded.push_back(c);
            } else {
                encoded.push_back('%');
                encoded.push_back(hex[c >> 4]);
                encoded.push_back(hex[c & 0xF]);
            }
        }
        return encoded;
    }

    static asp::fs::path getCompletedGauntletLevelsPath(bool local) {
        auto dir = geode::dirs::getModsSaveDir() / geode::Mod::get()->getID();
        if (auto res = asp::fs::createDirAll(dir); !res) {
            log::warn("Failed to create completed levels save directory: {}", res.unwrapErr().message());
        }
        return dir / (local ? "completed_gauntlet_levels_local.json" : "completed_gauntlet_levels.json");
    }

    static std::unordered_set<int> loadCompletedGauntletLevels(bool local = false) {
        std::unordered_set<int> out;
        auto path = getCompletedGauntletLevelsPath(local);
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

    static bool saveCompletedGauntletLevels(std::unordered_set<int> const& levels, bool local = false) {
        auto path = getCompletedGauntletLevelsPath(local);
        matjson::Value array = matjson::Value::array();
        for (auto levelId : levels) {
            array.push(levelId);
        }
        auto data = array.dump();
        auto res = asp::fs::write(path, data.c_str(), data.size());
        if (!res) {
            log::warn("Failed to save completed gauntlet levels: {}", res.unwrapErr());
        }
        return static_cast<bool>(res);
    }

    static bool addCompletedGauntletLevel(int levelId, bool local = false) {
        auto levels = loadCompletedGauntletLevels(local);
        levels.insert(levelId);
        return saveCompletedGauntletLevels(levels, local);
    }

    // backward-compatible wrapper
    static bool addCompletedGauntletLevel(int levelId) {
        return addCompletedGauntletLevel(levelId, false);
    }

    static asp::fs::path getCompletedGauntletPath() {
        auto dir = geode::dirs::getModsSaveDir() / geode::Mod::get()->getID();
        if (auto res = asp::fs::createDirAll(dir); !res) {
            log::warn("Failed to create completed gauntlet save directory: {}", res.unwrapErr().message());
        }
        return dir / "completed_gauntlets.json";
    }

    static asp::fs::path getLocalGauntletPath() {
        auto dir = geode::dirs::getModsSaveDir() / geode::Mod::get()->getID();
        if (auto res = asp::fs::createDirAll(dir); !res) {
            log::warn("{}", res.unwrapErr().message());
        }
        return dir / "local_gauntlets.json";
    }

    static matjson::Value loadLocalGauntlets() {
        auto path = getLocalGauntletPath();
        if (!asp::fs::isFile(path).unwrapOr(false)) {
            return matjson::Value::array();
        }

        auto content = asp::fs::readToString(path);
        if (!content) {
            log::warn("Failed to read local gauntlet file: {}", content.unwrapErr());
            return matjson::Value::array();
        }

        auto jsonResult = matjson::parse(content.unwrap());
        if (!jsonResult) {
            log::warn("Failed to parse local gauntlet JSON: {}", jsonResult.unwrapErr());
            return matjson::Value::array();
        }

        auto data = std::move(jsonResult).unwrap();
        if (data.isObject() && data["gauntlets"].isArray()) {
            return data["gauntlets"];
        }
        if (data.isArray()) {
            return data;
        }
        return matjson::Value::array();
    }

    static matjson::Value filterLocalGauntlets(const matjson::Value& gauntlets, const std::string& query) {
        if (!gauntlets.isArray() || query.empty()) {
            return gauntlets;
        }

        std::string lowerQuery = query;
        std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

        matjson::Value filtered = matjson::Value::array();
        for (auto const& gauntlet : gauntlets) {
            auto name = gauntlet["name"].asString().unwrapOr("");
            std::string lowerName = name;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });

            if (lowerName.find(lowerQuery) != std::string::npos) {
                filtered.push(gauntlet);
            }
        }
        return filtered;
    }

    static bool saveLocalGauntlets(matjson::Value const& gauntlets) {
        auto path = getLocalGauntletPath();
        matjson::Value data = matjson::Value::array();
        if (gauntlets.isArray()) {
            for (auto const& item : gauntlets) {
                data.push(item);
            }
        }
        auto result = asp::fs::write(path, data.dump().c_str(), data.dump().size());
        if (!result) {
            log::warn("Failed to save local gauntlets: {}", result.unwrapErr());
        }
        return static_cast<bool>(result);
    }

    static std::unordered_set<int> loadCompletedGauntlets() {
        std::unordered_set<int> out;
        auto path = getCompletedGauntletPath();
        if (!asp::fs::isFile(path).unwrapOr(false)) {
            return out;
        }

        auto content = asp::fs::readToString(path);
        if (!content) {
            log::warn("Failed to read completed gauntlets file: {}", content.unwrapErr());
            return out;
        }

        auto jsonResult = matjson::parse(content.unwrap());
        if (!jsonResult) {
            log::warn("Failed to parse completed gauntlets JSON: {}", jsonResult.unwrapErr());
            return out;
        }

        auto data = std::move(jsonResult).unwrap();
        if (data.isObject() && data["completedGauntlets"].isArray()) {
            data = data["completedGauntlets"];
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

    static bool saveCompletedGauntlets(std::unordered_set<int> const& gauntlets) {
        auto path = getCompletedGauntletPath();
        matjson::Value array = matjson::Value::array();
        for (auto gauntletId : gauntlets) {
            array.push(gauntletId);
        }
        auto data = array.dump();
        auto res = asp::fs::write(path, data.c_str(), data.size());
        if (!res) {
            log::warn("Failed to save completed gauntlets: {}", res.unwrapErr());
        }
        return static_cast<bool>(res);
    }

    static bool addCompletedGauntlet(int gauntletId) {
        auto gauntlets = loadCompletedGauntlets();
        gauntlets.insert(gauntletId);
        return saveCompletedGauntlets(gauntlets);
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
            if (fallbackShadow->getContentSize().width > 0 && fallbackShadow->getContentSize().height > 0) {
                const float fitScale = std::min(
                    size.width / fallbackShadow->getContentSize().width,
                    size.height / fallbackShadow->getContentSize().height);
                fallbackShadow->setScale(fitScale * 1.05f);
                fallbackShadow->setScaleY(fitScale + 0.1f);
            }
            fallbackShadow->setPosition(position);
            fallbackShadow->setPositionY(position.y - 10.f);
            if (parent) {
                parent->addChild(fallbackShadow, zOrder - 2);
            }
        }

        auto fallbackSprite = CCSprite::createWithSpriteFrameName("GDX_gauntletUnknown.png"_spr);
        if (fallbackSprite) {
            if (fallbackSprite->getContentSize().width > 0 && fallbackSprite->getContentSize().height > 0) {
                const float fitScale = std::min(
                    size.width / fallbackSprite->getContentSize().width,
                    size.height / fallbackSprite->getContentSize().height);
                fallbackSprite->setScale(fitScale);
            }
            fallbackSprite->setPosition(position);
            if (parent) {
                parent->addChild(fallbackSprite, zOrder - 1);
            }
        }

        sprite->setLoadCallback([fallbackSprite = geode::Ref<CCSprite>(fallbackSprite), fallbackShadow = geode::Ref<CCSprite>(fallbackShadow), url, sprite = geode::Ref<LazySprite>(sprite), position, size, parent = geode::Ref<CCNode>(parent), zOrder](geode::Result<> const& result) {
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
                if (sprite->getContentSize().width > 0 && sprite->getContentSize().height > 0) {
                    const float fitScale = std::min(
                        size.width / sprite->getContentSize().width,
                        size.height / sprite->getContentSize().height);
                    sprite->setScale(fitScale);
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
            }
        });
        sprite->loadFromUrl(url, CCImage::kFmtPng, true);

        if (parent) {
            parent->addChild(sprite, zOrder);
        }
        return sprite;
    }

    static LazySprite* createLocalSprite(CCNode* parent, const asp::fs::path& path, const CCPoint& position, const CCSize& size, int zOrder = 2) {
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
            if (fallbackShadow->getContentSize().width > 0 && fallbackShadow->getContentSize().height > 0) {
                const float fitScale = std::min(
                    size.width / fallbackShadow->getContentSize().width,
                    size.height / fallbackShadow->getContentSize().height);
                fallbackShadow->setScale(fitScale * 1.05f);
                fallbackShadow->setScaleY(fitScale + 0.1f);
            }
            fallbackShadow->setPosition(position);
            fallbackShadow->setPositionY(position.y - 10.f);
            if (parent) {
                parent->addChild(fallbackShadow, zOrder - 2);
            }
        }

        auto fallbackSprite = CCSprite::createWithSpriteFrameName("GDX_gauntletUnknown.png"_spr);
        if (fallbackSprite) {
            if (fallbackSprite->getContentSize().width > 0 && fallbackSprite->getContentSize().height > 0) {
                const float fitScale = std::min(
                    size.width / fallbackSprite->getContentSize().width,
                    size.height / fallbackSprite->getContentSize().height);
                fallbackSprite->setScale(fitScale);
            }
            fallbackSprite->setPosition(position);
            if (parent) {
                parent->addChild(fallbackSprite, zOrder - 1);
            }
        }

        sprite->setLoadCallback([fallbackSprite = geode::Ref<CCSprite>(fallbackSprite), fallbackShadow = geode::Ref<CCSprite>(fallbackShadow), path, position, size, parent = geode::Ref<CCNode>(parent), zOrder, sprite = geode::Ref<LazySprite>(sprite)](geode::Result<> const& result) {
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
                if (texture->getName() != 0) {
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
                }
            }
        });
        sprite->loadFromFile(path, CCImage::kFmtPng, true);

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
                    if (sprite->getContentSize().width > 0 && sprite->getContentSize().height > 0) {
                        const float scale = std::min(
                            size.width / sprite->getContentSize().width,
                            size.height / sprite->getContentSize().height);
                        sprite->setScale(scale);
                    }
                    sprite->setPosition(position);
                    if (parent) {
                        parent->addChild(sprite, zOrder);
                    }

                    auto shadow = CCSprite::createWithTexture(texture);
                    if (shadow) {
                        if (shadow->getContentSize().width > 0 && shadow->getContentSize().height > 0) {
                            const float scale = std::min(
                                size.width / shadow->getContentSize().width,
                                size.height / shadow->getContentSize().height);
                            shadow->setScale(scale);
                            shadow->setScaleY(scale + 0.1f);
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

    static CCSprite* createRecentToggleSprite(bool on) {
        return CircleButtonSprite::createWithSpriteFrameName(
            "GDX_recentGauntlet.png"_spr,
            1.f,
            on ? CircleBaseColor::Cyan : CircleBaseColor::Gray,
            CircleBaseSize::Small);
    }

    static CCSprite* createLocalToggleSprite(bool on) {
        return CircleButtonSprite::createWithSpriteFrameName(
            "GDX_localGauntlet.png"_spr,
            1.f,
            on ? CircleBaseColor::Cyan : CircleBaseColor::Gray,
            CircleBaseSize::Small);
    }
}

GDXGauntletLayer* GDXGauntletLayer::create() {
    auto ret = new GDXGauntletLayer();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool GDXGauntletLayer::init() {
    if (!CCLayer::init()) return false;

    // ui stuff
    auto director = CCDirector::sharedDirector();
    auto winSize = director->getWinSize();
    addBackButton(this, BackButtonStyle::Green);
    addSideArt(this, SideArt::Bottom, SideArtStyle::LayerGray, false);
    addSideArt(this, SideArt::TopLeft, SideArtStyle::LayerGray, false);
    auto bg = createLayerBG();
    bg->setColor({50, 50, 50});
    this->addChild(bg, -5);

    // search button
    // @geode-ignore(unknown-resource)
    auto searchSpr = CircleButtonSprite::createWithSpriteFrameName(
        "GDX_searchIcon.png"_spr,
        1.f,
        m_searchQuery.empty() ? CircleBaseColor::Green : CircleBaseColor::Cyan,
        CircleBaseSize::Small);
    m_searchBtn = CCMenuItemSpriteExtra::create(searchSpr, this, menu_selector(GDXGauntletLayer::onSearchGauntlets));

    auto tagFilterSpr = CircleButtonSprite::createWithSpriteFrameName("GDX_searchTags.png"_spr, 1.f, CircleBaseColor::Green, CircleBaseSize::Small);
    m_tagFilterBtn = CCMenuItemSpriteExtra::create(tagFilterSpr, this, menu_selector(GDXGauntletLayer::onFilterByTag));

    auto linkIcon = CircleButtonSprite::createWithSpriteFrameName("GDX_linkIcon.png"_spr, 1.f, CircleBaseColor::Green, CircleBaseSize::Small);
    m_linkBtn = CCMenuItemSpriteExtra::create(linkIcon, this, menu_selector(GDXGauntletLayer::onLink));

    // top left menu
    auto topLeftMenu = CCMenu::create(m_searchBtn, m_tagFilterBtn, m_linkBtn, nullptr);
    topLeftMenu->setPosition({10, winSize.height - 45});
    topLeftMenu->setContentWidth(80);
    topLeftMenu->setAnchorPoint({0.f, 1.f});
    topLeftMenu->setLayout(RowLayout::create()->setGap(5.f)->setAxisAlignment(AxisAlignment::Start)->setGrowCrossAxis(true)->setCrossAxisReverse(false));
    this->addChild(topLeftMenu, 2);

    // info button
    auto bottomLeftMenu = CCMenu::create();
    bottomLeftMenu->setPosition({10, 10});
    bottomLeftMenu->setAnchorPoint({0.f, 0.f});
    bottomLeftMenu->setContentHeight(100);
    bottomLeftMenu->setLayout(ColumnLayout::create()->setGap(5.f)->setAxisAlignment(AxisAlignment::Start)->setGrowCrossAxis(true)->setCrossAxisReverse(true));
    this->addChild(bottomLeftMenu, 2);
    m_bottomLeftMenu = bottomLeftMenu;

    auto infoIconSpr = CircleButtonSprite::createWithSpriteFrameName("GDX_infoIcon.png"_spr, 1.f, CircleBaseColor::Green, CircleBaseSize::Small);
    auto infoIconBtn = CCMenuItemSpriteExtra::create(infoIconSpr, this, menu_selector(GDXGauntletLayer::onInfo));
    bottomLeftMenu->addChild(infoIconBtn);

    // leaderboard button
    auto leaderboardIconSpr = CircleButtonSprite::createWithSpriteFrameName("rankIcon_1_001.png", 1.f, CircleBaseColor::Green, CircleBaseSize::Small);
    m_leaderboardBtn = CCMenuItemSpriteExtra::create(leaderboardIconSpr, this, menu_selector(GDXGauntletLayer::onLeaderboard));
    bottomLeftMenu->addChild(m_leaderboardBtn);

    // discord button
    auto discordIconSpr = CircleButtonSprite::createWithSpriteFrameName("GDX_discord.png"_spr, 1.f, CircleBaseColor::Green, CircleBaseSize::Small);
    m_discordBtn = CCMenuItemSpriteExtra::create(discordIconSpr, this, menu_selector(GDXGauntletLayer::onDiscord));
    bottomLeftMenu->addChild(m_discordBtn);

    // @geode-ignore(unknown-resource)
    auto manageLabel = CircleButtonSprite::createWithSpriteFrameName("GDX_pencil.png"_spr, 1.f, CircleBaseColor::Green, CircleBaseSize::Small);
    auto manageBtn = CCMenuItemSpriteExtra::create(manageLabel, this, menu_selector(GDXGauntletLayer::onManageGauntlets));
    bottomLeftMenu->addChild(manageBtn);

    bottomLeftMenu->updateLayout();

    // refresh gauntlet
    // @geode-ignore(unknown-resource)
    auto refreshSpr = CircleButtonSprite::createWithSpriteFrameName("geode.loader/reload.png", 1.f, CircleBaseColor::Green, CircleBaseSize::Small);
    auto refreshBtn = CCMenuItemSpriteExtra::create(
        refreshSpr,
        this,
        menu_selector(GDXGauntletLayer::onRefreshGauntlets));

    // sync account
    // @geode-ignore(unknown-resource)
    auto syncSpr = CircleButtonSprite::createWithSpriteFrameName("geode.loader/update.png", 1.f, CircleBaseColor::Green, CircleBaseSize::Small);
    m_syncBtn = CCMenuItemSpriteExtra::create(syncSpr, this, menu_selector(GDXGauntletLayer::onSyncAccount));

    // recent toggle
    m_recentToggle = CCMenuItemSpriteExtra::create(createRecentToggleSprite(m_recentFilter), this, menu_selector(GDXGauntletLayer::onToggleRecent));
    updateRecentToggleState();

    m_localToggle = CCMenuItemSpriteExtra::create(createLocalToggleSprite(m_localMode), this, menu_selector(GDXGauntletLayer::onToggleLocalMode));
    updateLocalToggleState();
    updateRecentToggleState();

    auto bottomRightMenu = CCMenu::create(refreshBtn, m_syncBtn, m_recentToggle, m_localToggle, nullptr);
    if (bottomRightMenu) {
        bottomRightMenu->setPosition({winSize.width - 10, 10});
        bottomRightMenu->setContentHeight(100);
        bottomRightMenu->setAnchorPoint({1.f, 0.f});
        bottomRightMenu->setLayout(ColumnLayout::create()->setGap(5.f)->setAxisAlignment(AxisAlignment::Start)->setGrowCrossAxis(true));
        this->addChild(bottomRightMenu, 2);
        m_bottomRightMenu = bottomRightMenu;
    }

    if (m_bottomRightMenu) {
        m_bottomRightMenu->updateLayout();
    }

    // title styling
    auto title = CCSprite::createWithSpriteFrameName("GDX_title.png"_spr);
    title->setScale(0.8f);
    this->addChildAtPosition(title, Anchor::Top, {0, -30}, false);

    m_loadingSpinner = LoadingSpinner::create(60.f);
    if (m_loadingSpinner) {
        m_loadingSpinner->setPosition({winSize.width / 2.f, winSize.height / 2.f});
        m_loadingSpinner->setVisible(false);
        this->addChild(m_loadingSpinner, 4);
    }

    // users' gauntlet and level points
    m_levelPointsSpr = CCSprite::createWithSpriteFrameName("GDX_levelPoint.png"_spr);
    if (m_levelPointsSpr) {
        m_levelPointsSpr->setPosition({winSize.width - 20, winSize.height - 20});
        m_levelPointsSpr->setScale(0.3f);
        this->addChild(m_levelPointsSpr, 5);
    }
    m_levelPointsCounter = CCCounterLabel::create(static_cast<int>(m_levelPoints), "bigFont.fnt", FormatterType::Integer);
    if (m_levelPointsCounter) {
        m_levelPointsCounter->setPosition({m_levelPointsSpr ? m_levelPointsSpr->getPositionX() - 15 : winSize.width - 35, m_levelPointsSpr ? m_levelPointsSpr->getPositionY() : winSize.height - 20});
        m_levelPointsCounter->setAnchorPoint({1.0f, 0.5f});
        m_levelPointsCounter->setScale(0.6f);
        this->addChild(m_levelPointsCounter, 5);
    }

    m_gauntletPointsSpr = CCSprite::createWithSpriteFrameName("GDX_gauntletPoint.png"_spr);
    if (m_gauntletPointsSpr) {
        m_gauntletPointsSpr->setPosition({winSize.width - 20, winSize.height - 50});
        m_gauntletPointsSpr->setScale(0.4f);
        this->addChild(m_gauntletPointsSpr, 5);
    }
    m_gauntletPointsCounter = CCCounterLabel::create(static_cast<int>(m_gauntletPoints), "bigFont.fnt", FormatterType::Integer);
    if (m_gauntletPointsCounter) {
        m_gauntletPointsCounter->setPosition({m_gauntletPointsSpr ? m_gauntletPointsSpr->getPositionX() - 15 : winSize.width - 35, m_gauntletPointsSpr ? m_gauntletPointsSpr->getPositionY() : winSize.height - 50});
        m_gauntletPointsCounter->setAnchorPoint({1.0f, 0.5f});
        m_gauntletPointsCounter->setScale(0.6f);
        this->addChild(m_gauntletPointsCounter, 5);
    }

    // gauntlet page container
    createGauntletPages(matjson::Value::array());
    fetchUserData();
    fetchGauntlets();

    this->scheduleUpdate();
    this->setKeypadEnabled(true);

    return true;
}

void GDXGauntletLayer::onEnter() {
    CCLayer::onEnter();
    gdx::setPlayingGauntletLevel(false);
    this->scheduleUpdate();
    fetchUserData();
}

void GDXGauntletLayer::onExit() {
    this->unscheduleUpdate();
    CCLayer::onExit();
}

void GDXGauntletLayer::keyBackClicked() {
    CCDirector::sharedDirector()->popSceneWithTransition(
        0.5f, PopTransition::kPopTransitionFade);
}

void GDXGauntletLayer::onDiscord(CCObject* sender) {
    utils::web::openLinkInBrowser("https://discord.gg/JBVDVcQN6R");
}

void GDXGauntletLayer::onLink(CCObject* sender) {
    GDXLinkDiscordPopup::create()->show();
}

void GDXGauntletLayer::onInfo(CCObject* sender) {
    MDPopup::create(
        "About Gauntlets Deluxe",
        "**Gauntlets Deluxe** is a <cl>community-run mod</c> that adds a <cg>custom gauntlet to Geometry Dash</c>, featuring fan-made <co>gauntlets based on various themes</c>.\n\n"
        "Players can earn points by completing <cp>levels</c> and <cr>gauntlets</c>, and <cl>compete on the leaderboard</c>.\n\n"
        "The mod also allows for <cl>community-hosted creator contests</c>, where players can submit their own gauntlet levels to be featured in the mod.\n\n"
        "You can also create your own <cy>local gauntlets</c> that only you can see and play and create <cg>interesting gauntlets</c> for yourself!\n\n"
        "When you fully <cg>completed a gauntlet</c>, you can submit your <cc>feedback</c> on that gauntlet by <cy>liking</c> or <cr>disliking</c> it!\n\n"
        "\n---\n"
        "### Points System\n"
        "##### <co>*Gauntlet Points and Level Points are only obtainable by playing online Gauntlets. Local Gauntlets do not award points.*</c>\n\n"
        "![GDX](frame:arcticwoof.gauntlets_deluxe/GDX_levelPoint.png?scale=0.15) <cp>**Level Points**</c>: Earned by completing levels in a gauntlet.\n\n"
        "Each level awards a different amount of points based on its difficulty and other factors.\n\n"
        "##### <cy>*If you have already beaten a level before using this mod, you still need to beat it again to earn Level Points.*</c>\n\n"
        "![GDX](frame:arcticwoof.gauntlets_deluxe/GDX_gauntletPoint.png?scale=0.2) <cr>**Gauntlet Points**</c>: Earn points by completing any of these fan-made gauntlets.\n\n"
        "Gauntlet points are based on the number of levels and their difficulty.\n"
        "\n---\n"
        "### <cp>Level Points</c>\n"
        "#### Level Points are awarded based on the following criteria:\n"
        "- **Auto - Easy**: 1 ![Level Points](frame:arcticwoof.gauntlets_deluxe/GDX_levelPoint.png?scale=0.15) <cp>Level Points</c> \n"
        "- **Normal**: 2 ![Level Points](frame:arcticwoof.gauntlets_deluxe/GDX_levelPoint.png?scale=0.15) <cp>Level Points</c>\n"
        "- **Hard** _(4-5 stars)_: 3 ![Level Points](frame:arcticwoof.gauntlets_deluxe/GDX_levelPoint.png?scale=0.15) <cp>Level Points</c>\n"
        "- **Harder** _(6-7 stars)_ - **Insane** _(8-9 stars)_: 4 ![Level Points](frame:arcticwoof.gauntlets_deluxe/GDX_levelPoint.png?scale=0.15) <cp>Level Points</c>\n"
        "- **Easy-Medium Demon**: 5 ![Level Points](frame:arcticwoof.gauntlets_deluxe/GDX_levelPoint.png?scale=0.15) <cp>Level Points</c>\n"
        "- **Hard-Extreme Demon**: 7 ![Level Points](frame:arcticwoof.gauntlets_deluxe/GDX_levelPoint.png?scale=0.15) <cp>Level Points</c>\n"
        "\n---\n"
        "### <cr>Gauntlet Points</c>\n"
        "#### Gauntlet Points are awarded based on the following criteria:\n"
        "- **Auto - Hard Level Mix**: 3 ![Gauntlet Points](frame:arcticwoof.gauntlets_deluxe/GDX_gauntletPoint.png?scale=0.2) <cr>Gauntlet Points</c> \n"
        "- **Hard - Insane Level Mix**: 5 ![Gauntlet Points](frame:arcticwoof.gauntlets_deluxe/GDX_gauntletPoint.png?scale=0.2) <cr>Gauntlet Points</c>\n"
        "- **Easy Demon - Medium Demon Mix**: 10 ![Gauntlet Points](frame:arcticwoof.gauntlets_deluxe/GDX_gauntletPoint.png?scale=0.2) <cr>Gauntlet Points</c>\n"
        "- **Medium - Extreme Demon Mix**: 15 ![Gauntlet Points](frame:arcticwoof.gauntlets_deluxe/GDX_gauntletPoint.png?scale=0.2) <cr>Gauntlet Points</c>\n",
        "OK")
        ->show();
}

void GDXGauntletLayer::onLeaderboard(CCObject* sender) {
    auto scene = CCScene::create();
    scene->addChild(GDXLeaderboardLayer::create());
    CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(0.5f, scene));
}

void GDXGauntletLayer::onManageGauntlets(CCObject* sender) {
    if (m_localMode) {
        GDXGauntletManagePopup::create(true)->show();
        return;
    }

    auto upopup = UploadActionPopup::create(nullptr, "Getting access...");
    upopup->show();

    auto accountData = argon::getGameAccountData();
    auto url = std::string(gdx::baseApiUrl()) + "/getAccess";
    matjson::Value body = matjson::Value::object();
    body["accountId"] = accountData.accountId;

    auto upopupRef = geode::Ref<UploadActionPopup>(upopup);
    m_syncAccountTask.spawn([upopupRef = std::move(upopupRef), url = std::move(url), body = std::move(body), accountData = std::move(accountData)]() mutable -> arc::Future<> {
        auto token = co_await gdx::argonToken(accountData);
        if (token.empty()) {
            co_await geode::async::waitForMainThread([upopupRef = std::move(upopupRef)] {
                if (!upopupRef) return;
                if (upopupRef) upopupRef->showFailMessage("Authentication failed.");
            });
            co_return;
        }

        body["argonToken"] = std::move(token);
        auto response = co_await geode::utils::web::WebRequest()
                            .url(url)
                            .header("Content-Type", "application/json")
                            .bodyJSON(body)
                            .post(url);

        if (response.error() || response.cancelled() || !response.ok()) {
            auto errMsg = gdx::getResponseMessage(response, "Failed to get access.");
            co_await geode::async::waitForMainThread([upopupRef = std::move(upopupRef), errMsg = std::move(errMsg)] {
                if (!upopupRef) return;
                if (upopupRef) upopupRef->showFailMessage(errMsg);
            });
            co_return;
        }

        auto jsonResult = response.json();
        if (!jsonResult) {
            co_await geode::async::waitForMainThread([upopupRef = std::move(upopupRef)] {
                if (!upopupRef) return;
                if (upopupRef) upopupRef->showFailMessage("Failed to get access.");
            });
            co_return;
        }

        auto result = std::move(jsonResult).unwrap();
        bool success = result["success"].asBool().unwrapOr(false);
        if (!result.isObject() || !success) {
            co_await geode::async::waitForMainThread([upopupRef = std::move(upopupRef)] {
                if (!upopupRef) return;
                if (upopupRef) upopupRef->showFailMessage("Failed to get access.");
            });
            co_return;
        }

        // get those roles lol
        bool isContributor = result["isContributor"].asBool().unwrapOr(false);
        bool isManager = result["isManager"].asBool().unwrapOr(false);
        bool isLeaderboardMod = result["isLeaderboardMod"].asBool().unwrapOr(false);

        Mod::get()->setSavedValue("isContributor", isContributor);
        Mod::get()->setSavedValue("isManager", isManager);
        Mod::get()->setSavedValue("isLeaderboardMod", isLeaderboardMod);

        if (!isManager && !isContributor && !isLeaderboardMod) {
            auto errMsg = gdx::getResponseMessage(response, "Permissions Denied");
            co_await geode::async::waitForMainThread([upopupRef = std::move(upopupRef), errMsg = std::move(errMsg)] {
                if (!upopupRef) return;
                if (upopupRef) upopupRef->showFailMessage(errMsg);
            });
            co_return;
        }

        if (gdx::isManager() || gdx::isContributor() || gdx::isLeaderboardMod()) {
            co_await geode::async::waitForMainThread([upopupRef = std::move(upopupRef)]() {
                if (!upopupRef) return;
                if (upopupRef) upopupRef->onClose(nullptr);
                GDXGauntletManagePopup::create()->show();
            });
        }

        co_return; }, []() {});
}

void GDXGauntletLayer::onSyncAccount(CCObject* sender) {
    auto upopup = UploadActionPopup::create(nullptr, "Syncing account...");
    upopup->show();

    auto accountData = argon::getGameAccountData();
    auto url = std::string(gdx::baseApiUrl()) + "/syncUser";
    matjson::Value body = matjson::Value::object();
    body["accountId"] = accountData.accountId;
    body["argonToken"] = "";  // This line is unchanged, but included for context.

    auto self = geode::Ref<GDXGauntletLayer>(this);
    auto upopupRef = geode::Ref<UploadActionPopup>(upopup);
    m_syncAccountTask.spawn([self = std::move(self), upopupRef = std::move(upopupRef), url = std::move(url), body = std::move(body), accountData = std::move(accountData)]() mutable -> arc::Future<> {
        auto token = co_await gdx::argonToken(accountData);
        if (token.empty()) {
            co_await geode::async::waitForMainThread([self = std::move(self), upopupRef = std::move(upopupRef)] {
                if (!upopupRef) return;
                if (upopupRef) upopupRef->showFailMessage("Authentication failed.");
            });
            co_return;
        }

        body["argonToken"] = std::move(token);
        auto response = co_await geode::utils::web::WebRequest()
                            .url(url)
                            .header("Content-Type", "application/json")
                            .bodyJSON(body)
                            .post(url);

        if (response.error() || response.cancelled() || !response.ok()) {
            auto errMsg = gdx::getResponseMessage(response, "Failed to sync account.");
            co_await geode::async::waitForMainThread([self = std::move(self), upopupRef = std::move(upopupRef), errMsg = std::move(errMsg)] {
                if (!upopupRef) return;
                if (upopupRef) upopupRef->showFailMessage(errMsg);
            });
            co_return;
        }

        auto jsonResult = response.json();
        if (!jsonResult) {
            co_await geode::async::waitForMainThread([self = std::move(self), upopupRef = std::move(upopupRef)] {
                if (!upopupRef) return;
                if (upopupRef) upopupRef->showFailMessage("Failed to sync account.");
            });
            co_return;
        }

        auto result = std::move(jsonResult).unwrap();
        bool success = result["success"].asBool().unwrapOr(false);
        if (!success) {
            co_await geode::async::waitForMainThread([self = std::move(self), upopupRef = std::move(upopupRef)] {
                if (!upopupRef) return;
                if (upopupRef) upopupRef->showFailMessage("Failed to sync account.");
            });
            co_return;
        }

        std::unordered_set<int> completedLevels;
        if (result["completedLevels"].isArray()) {
            for (auto const& entry : result["completedLevels"]) {
                if (entry.isNumber()) {
                    auto maybeId = entry.asInt();
                    if (maybeId) {
                        completedLevels.insert(static_cast<int>(maybeId.unwrap()));
                    }
                } else if (entry.isString()) {
                    auto idString = entry.asString().unwrapOr("");
                    auto maybeId = numFromString<int>(idString);
                    if (maybeId) {
                        completedLevels.insert(maybeId.unwrap());
                    }
                }
            }
        }

        std::unordered_set<int> completedGauntlets;
        if (result["completedGauntlets"].isArray()) {
            for (auto const& entry : result["completedGauntlets"]) {
                if (entry.isNumber()) {
                    auto maybeId = entry.asInt();
                    if (maybeId) {
                        completedGauntlets.insert(static_cast<int>(maybeId.unwrap()));
                    }
                } else if (entry.isString()) {
                    auto idString = entry.asString().unwrapOr("");
                    auto maybeId = numFromString<int>(idString);
                    if (maybeId) {
                        completedGauntlets.insert(maybeId.unwrap());
                    }
                }
            }
        }

        auto existingLevels = loadCompletedGauntletLevels(false);
        for (auto levelId : existingLevels) {
            completedLevels.insert(levelId);
        }

        auto existingGauntlets = loadCompletedGauntlets();
        for (auto gauntletId : existingGauntlets) {
            completedGauntlets.insert(gauntletId);
        }

        auto levelsSaved = saveCompletedGauntletLevels(completedLevels, false);
        auto gauntletsSaved = saveCompletedGauntlets(completedGauntlets);

        co_await geode::async::waitForMainThread([self = std::move(self), upopupRef = std::move(upopupRef), levelsSaved, gauntletsSaved]() mutable {
            if (levelsSaved && gauntletsSaved) {
                if (upopupRef) upopupRef->showSuccessMessage("Account synced successfully.");
                if (self) {
                    self->m_completedGauntletLevels = loadCompletedGauntletLevels(false);
                    self->m_claimedGauntlets = loadCompletedGauntlets();
                }
            } else {
                if (upopupRef) upopupRef->showFailMessage("Failed to save sync data.");
            }
        });

        co_return; }, []() {});
}

void GDXGauntletLayer::onRefreshGauntlets(CCObject* sender) {
    if (m_localMode) {
        m_localGauntlets = loadLocalGauntlets();
        if (!m_localGauntlets.isArray() || m_localGauntlets.size() == 0) {
            if (m_localScrollLayer) {
                m_localScrollLayer->removeFromParent();
                m_localScrollLayer = nullptr;
            }
            m_localGauntletButtons.clear();
            Notification::create("Local Gauntlet is empty. Please add one.", NotificationIcon::Info)->show();
            updatePageButtons();
            return;
        }
        auto filtered = filterLocalGauntlets(m_localGauntlets, m_searchQuery);
        createGauntletPages(filtered, true);
        return;
    }

    m_onlineGauntlets = matjson::Value::array();
    createGauntletPages(m_onlineGauntlets, false);
    fetchGauntlets();
}

void GDXGauntletLayer::onToggleRecent(CCObject* sender) {
    m_recentFilter = !m_recentFilter;
    updateRecentToggleState();
    if (!m_localMode) {
        m_gauntlets = matjson::Value::array();
        createGauntletPages(m_gauntlets);
        fetchGauntlets();
    }
}

void GDXGauntletLayer::onSearchGauntlets(CCObject* sender) {
    auto popup = SetTextPopup::create(
        m_searchQuery,
        "Search query",
        128,
        m_localMode ? "Search local gauntlets" : "Search gauntlets",
        "Search",
        true,
        60.0f);
    if (popup) {
        popup->m_delegate = this;
        popup->show();
    }
}

void GDXGauntletLayer::setTextPopupClosed(SetTextPopup* popup, gd::string text) {
    if (!popup) {
        return;
    }

    auto newSearch = std::string(text);
    if (newSearch == m_searchQuery) {
        return;
    }

    m_searchQuery = std::move(newSearch);
    updateSearchButtonState();
    if (m_localMode) {
        m_localGauntlets = loadLocalGauntlets();
        if (!m_localGauntlets.isArray() || m_localGauntlets.size() == 0) {
            if (m_localScrollLayer) {
                m_localScrollLayer->removeFromParent();
                m_localScrollLayer = nullptr;
            }
            m_localGauntletButtons.clear();
            Notification::create("Local Gauntlet is empty. Please add one.", NotificationIcon::Info)->show();
            updatePageButtons();
            return;
        }

        auto filtered = filterLocalGauntlets(m_localGauntlets, m_searchQuery);
        createGauntletPages(filtered, true);
        return;
    }

    m_gauntlets = matjson::Value::array();
    createGauntletPages(m_gauntlets);
    fetchGauntlets();
}

void GDXGauntletLayer::onFilterByTag(CCObject* sender) {
    auto popup = GDXTagsFiltersPopup::create(
        [self = geode::Ref<GDXGauntletLayer>(this)](std::string const& tag) {
            if (self) {
                self->applyTagFilter(tag);
            }
        },
        m_tagFilter);
    if (popup) {
        popup->show();
    }
}

void GDXGauntletLayer::applyTagFilter(std::string const& tag) {
    if (tag == m_tagFilter) {
        return;
    }
    m_tagFilter = tag;
    updateSearchButtonState();
    m_gauntlets = matjson::Value::array();
    createGauntletPages(m_gauntlets);
    fetchGauntlets();
}

void GDXGauntletLayer::onToggleLocalMode(CCObject* sender) {
    m_localMode = !m_localMode;
    m_searchQuery.clear();
    updateSearchButtonState();
    updateLocalToggleState();
    updateRecentToggleState();

    if (m_localMode) {
        m_localGauntlets = loadLocalGauntlets();
        if (!m_localGauntlets.isArray() || m_localGauntlets.size() == 0) {
            if (m_localScrollLayer) {
                m_localScrollLayer->removeFromParent();
                m_localScrollLayer = nullptr;
            }
            m_localGauntletButtons.clear();
            Notification::create("Local Gauntlet is empty. Please add one.", NotificationIcon::Info)->show();
        } else {
            auto filtered = filterLocalGauntlets(m_localGauntlets, m_searchQuery);
            createGauntletPages(filtered, true);
        }

        if (m_localScrollLayer) {
            m_localScrollLayer->setVisible(true);
        }
        if (m_scrollLayer) {
            m_scrollLayer->setVisible(false);
        }
        if (m_leaderboardBtn) {
            m_leaderboardBtn->setVisible(false);
        }
        if (m_discordBtn) {
            m_discordBtn->setVisible(false);
        }
        if (m_syncBtn) {
            m_syncBtn->setVisible(false);
        }
        if (m_bottomLeftMenu) {
            m_bottomLeftMenu->updateLayout();
        }
        if (m_bottomRightMenu) {
            m_bottomRightMenu->updateLayout();
        }
        updatePageButtons();
        return;
    }

    if (m_scrollLayer) {
        m_scrollLayer->setVisible(true);
    }
    if (m_localScrollLayer) {
        m_localScrollLayer->setVisible(false);
    }
    if (m_leaderboardBtn) {
        m_leaderboardBtn->setVisible(true);
    }
    if (m_discordBtn) {
        m_discordBtn->setVisible(true);
    }
    if (m_syncBtn) {
        m_syncBtn->setVisible(true);
    }
    if (m_searchBtn) {
        m_searchBtn->setVisible(true);
    }
    if (m_bottomLeftMenu) {
        m_bottomLeftMenu->updateLayout();
    }
    if (m_bottomRightMenu) {
        m_bottomRightMenu->updateLayout();
    }

    if (m_onlineGauntlets.isArray() && m_onlineGauntlets.size() > 0) {
        updatePageButtons();
    } else {
        m_gauntlets = matjson::Value::array();
        createGauntletPages(m_gauntlets, false);
        fetchGauntlets();
    }
}

void GDXGauntletLayer::onGauntletButtonClick(CCObject* sender) {
    auto button = static_cast<CCMenuItemSpriteExtra*>(sender);
    const auto& activeGauntlets = m_localMode ? m_localGauntlets : m_gauntlets;
    if (!button || !activeGauntlets.isArray()) {
        return;
    }

    auto idx = button->getTag();
    if (idx < 0 || idx >= static_cast<int>(activeGauntlets.size())) {
        return;
    }

    auto gauntlet = activeGauntlets[idx];
    auto levelArray = gauntlet["levelIds"];
    if (!levelArray.isArray()) {
        return;
    }

    auto levels = CCArray::create();
    for (auto i = 0u; i < levelArray.size(); ++i) {
        auto levelValue = levelArray[i];
        auto dict = CCDictionary::create();
        dict->setObject(CCInteger::create(levelValue["levelId"].asInt().unwrapOr(0)), "levelId");
        dict->setObject(CCString::create(levelValue["levelName"].asString().unwrapOr("-").c_str()), "levelName");
        dict->setObject(CCString::create(levelValue["creatorName"].asString().unwrapOr("-").c_str()), "creatorName");
        dict->setObject(CCString::create(levelValue["songName"].asString().unwrapOr("-").c_str()), "songName");
        dict->setObject(CCInteger::create(levelValue["songId"].asInt().unwrapOr(0)), "songId");
        dict->setObject(CCInteger::create(levelValue["reward"].asInt().unwrapOr(0)), "reward");
        dict->setObject(CCInteger::create(levelValue["isFeatured"].asBool().unwrapOr(false) ? 1 : 0), "featured");
        levels->addObject(dict);
    }

    auto scene = CCScene::create();
    auto color = ccColor3B{
        static_cast<GLubyte>(gauntlet["r"].asInt().unwrapOr(255)),
        static_cast<GLubyte>(gauntlet["g"].asInt().unwrapOr(255)),
        static_cast<GLubyte>(gauntlet["b"].asInt().unwrapOr(255)),
    };
    scene->addChild(GDXGauntletLevelsLayer::create(levels, gauntlet["name"].asString().unwrapOr("Gauntlet"), color, gauntlet["index"].asInt().unwrapOr(idx), gauntlet, m_localMode));
    CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(0.5f, scene));
}

void GDXGauntletLayer::onShowGauntletTags(CCObject* sender) {
    auto button = static_cast<CCMenuItemSpriteExtra*>(sender);
    if (!button) {
        return;
    }

    const auto& activeGauntlets = getActiveGauntlets();
    if (!activeGauntlets.isArray()) {
        return;
    }

    auto idx = button->getTag();
    if (idx < 0 || idx >= static_cast<int>(activeGauntlets.size())) {
        return;
    }

    auto gauntlet = activeGauntlets[idx];
    auto tags = gauntlet["tags"];
    auto gauntletName = gauntlet["name"].asString().unwrapOr("Gauntlet");
    auto popup = GDXShowTagsPopup::create(gauntletName, tags);
    if (popup) {
        popup->show();
    }
}

void GDXGauntletLayer::fetchGauntlets() {
    if (m_loadingSpinner) {
        m_loadingSpinner->setVisible(true);
    }

    auto url = std::string(gdx::baseApiUrl()) + "/getGauntlets";
    bool hasQuery = false;
    if (!m_tagFilter.empty()) {
        url += "?tag=" + urlEncode(m_tagFilter);
        hasQuery = true;
    }
    if (m_recentFilter) {
        url += hasQuery ? "&" : "?";
        url += "recent=true";
        hasQuery = true;
    }
    if (!m_searchQuery.empty()) {
        url += hasQuery ? "&" : "?";
        url += "search=" + urlEncode(m_searchQuery);
    }
    auto self = geode::Ref<GDXGauntletLayer>(this);
    m_fetchGauntletsTask.spawn([self = std::move(self), url = std::move(url)]() mutable -> arc::Future<> {
        auto response = co_await geode::utils::web::WebRequest()
                            .get(url);

        if (response.error() || response.cancelled() || !response.ok()) {
            co_await geode::async::waitForMainThread([self = std::move(self)]() {
                if (self && self->m_loadingSpinner) {
                    self->m_loadingSpinner->setVisible(false);
                }
            });
            co_return;
        }

        auto jsonResult = response.json();
        if (!jsonResult) {
            co_await geode::async::waitForMainThread([self = std::move(self)]() {
                if (self && self->m_loadingSpinner) {
                    self->m_loadingSpinner->setVisible(false);
                }
            });
            co_return;
        }

        auto gauntlets = std::move(jsonResult).unwrap();
        co_await geode::async::waitForMainThread([self = std::move(self), gauntlets = std::move(gauntlets)]() mutable {
            if (self && self->isRunning()) {
                if (self->m_loadingSpinner) {
                    self->m_loadingSpinner->setVisible(false);
                }
                if (gauntlets.isArray() && gauntlets.size() == 0) {
                    Notification::create("No gauntlets were found.", NotificationIcon::Info)->show();
                }
                self->m_onlineGauntlets = gauntlets;
                self->createGauntletPages(self->m_onlineGauntlets, false);
            }
        });

        co_return; }, []() {});
}

void GDXGauntletLayer::fetchUserData() {
    auto accountData = argon::getGameAccountData();
    auto url = std::string(gdx::baseApiUrl()) + "/getUser";
    matjson::Value body = matjson::Value::object();
    body["accountId"] = accountData.accountId;
    body["argonToken"] = "";

    auto self = geode::Ref<GDXGauntletLayer>(this);
    m_fetchUserDataTask.spawn([self = std::move(self), url = std::move(url), body = std::move(body), accountData = std::move(accountData)]() mutable -> arc::Future<> {
        auto token = co_await gdx::argonToken(accountData);
        if (token.empty()) {
            co_await geode::async::waitForMainThread([self = std::move(self)]() {});
            co_return;
        }

        body["argonToken"] = std::move(token);

        auto response = co_await geode::utils::web::WebRequest()
                            .url(url)
                            .header("Content-Type", "application/json")
                            .bodyJSON(body)
                            .post(url);

        if (response.error() || response.cancelled() || !response.ok()) {
            co_await geode::async::waitForMainThread([self = std::move(self)]() {});
            co_return;
        }

        auto jsonResult = response.json();
        if (!jsonResult) {
            co_await geode::async::waitForMainThread([self = std::move(self)]() {});
            co_return;
        }

        auto userData = std::move(jsonResult).unwrap();
        if (!userData.isObject() || !userData["success"].asBool().unwrapOr(false)) {
            co_await geode::async::waitForMainThread([self = std::move(self)]() {});
            co_return;
        }

        co_await geode::async::waitForMainThread([self = std::move(self), userData = std::move(userData)]() mutable {
            if (self && self->isRunning()) {
                self->m_userAccountId = userData["accountId"].asInt().unwrapOr(0);
                self->m_username = userData["username"].asString().unwrapOr(" ");
                self->m_gauntletPoints = userData["gauntletPoints"].asInt().unwrapOr(0);
                self->m_levelPoints = userData["levelPoints"].asInt().unwrapOr(0);
                if (self->m_levelPointsCounter) {
                    self->m_levelPointsCounter->setTargetCount(self->m_levelPoints);
                    Mod::get()->setSavedValue<int>("levelPoints", self->m_levelPoints);
                }
                if (self->m_gauntletPointsCounter) {
                    self->m_gauntletPointsCounter->setTargetCount(self->m_gauntletPoints);
                    Mod::get()->setSavedValue<int>("gauntletPoints", self->m_gauntletPoints);
                }

                log::debug("Fetched user data: accountId={}, username={}, gauntletPoints={}, levelPoints={}",
                    self->m_userAccountId,
                    self->m_username,
                    self->m_gauntletPoints,
                    self->m_levelPoints);
            }
        });

        co_return; }, []() {});
}

GDXGauntletNode GDXGauntletNode::fromJson(const matjson::Value& gauntlet) {
    GDXGauntletNode node;
    node.id = gauntlet["id"].asInt().unwrapOr(
        gauntlet["index"].asInt().unwrapOr(0));
    node.name = gauntlet["name"].asString().unwrapOr("Unknown");
    node.description = gauntlet["description"].asString().unwrapOr("");
    node.reward = gauntlet["reward"].asInt().unwrapOr(0);
    node.r = gauntlet["r"].asInt().unwrapOr(255);
    node.g = gauntlet["g"].asInt().unwrapOr(255);
    node.b = gauntlet["b"].asInt().unwrapOr(255);

    auto levelArray = gauntlet["levelIds"];
    if (levelArray.isArray()) {
        for (auto i = 0u; i < levelArray.size(); ++i) {
            auto levelValue = levelArray[i];
            GDXGauntletLevelInfo level;
            level.levelId = levelValue["levelId"].asInt().unwrapOr(0);
            level.levelName = levelValue["levelName"].asString().unwrapOr("");
            level.creatorName = levelValue["creatorName"].asString().unwrapOr("");
            level.songName = levelValue["songName"].asString().unwrapOr("");
            level.songId = levelValue["songId"].asInt().unwrapOr(0);
            level.reward = levelValue["reward"].asInt().unwrapOr(0);
            node.levelIds.push_back(std::move(level));
        }
    }

    return node;
}

BoomScrollLayer* GDXGauntletLayer::getActiveScrollLayer() const {
    return m_localMode ? m_localScrollLayer : m_scrollLayer;
}

const matjson::Value& GDXGauntletLayer::getActiveGauntlets() const {
    return m_localMode ? m_localGauntlets : m_gauntlets;
}

CCMenuItemSpriteExtra* GDXGauntletLayer::createGauntletButton(const matjson::Value& gauntlet, std::size_t index, bool local) {
    auto node = GDXGauntletNode::fromJson(gauntlet);
    int gauntletIndex = node.id;
    std::string gauntletName = node.name;

    auto gauntletBg = NineSlice::create("GJ_squareB_01.png");
    if (!gauntletBg) {
        return nullptr;
    }
    gauntletBg->setContentSize({110, 240});

    if (gauntlet["isFeatured"].asBool().unwrapOr(false)) {
        auto glow = NineSlice::createWithSpriteFrameName("GDX_featuredGlow.png"_spr);
        if (glow) {
            glow->setPosition({gauntletBg->getContentSize().width / 2.f, gauntletBg->getContentSize().height / 2.f});
            glow->setContentSize(gauntletBg->getContentSize() + CCSize{5.f, 5.f});
            glow->setColor({255, 215, 0});
            glow->setScale(1.0f);
            glow->setOpacity(255);

            auto tintToWhite = CCTintTo::create(1.2f, 255, 255, 255);
            auto tintToGold = CCTintTo::create(1.2f, 255, 215, 0);
            auto fadeToDim = CCFadeTo::create(1.2f, 100);
            auto fadeToBright = CCFadeTo::create(1.2f, 255);
            auto pulseUp = CCSpawn::create(tintToWhite, fadeToDim, nullptr);
            auto pulseDown = CCSpawn::create(tintToGold, fadeToBright, nullptr);
            auto pulseSequence = CCSequence::create(pulseUp, pulseDown, nullptr);
            glow->runAction(CCRepeatForever::create(pulseSequence));

            gauntletBg->addChild(glow, -2);

            // if (auto blendNode = typeinfo_cast<CCBlendProtocol*>(glow)) {
            //     blendNode->setBlendFunc({GL_SRC_ALPHA, GL_ONE});
            // }

            // particle system
            const std::string& pString = gdx::featuredParticle;
            ParticleStruct pStruct;
            GameToolbox::particleStringToStruct(pString, pStruct);
            CCParticleSystemQuad* particle = GameToolbox::particleFromStruct(pStruct, nullptr, false);
            if (particle) {
                gauntletBg->addChildAtPosition(particle, Anchor::Center, {0, 0}, false);
                particle->setZOrder(5);
                particle->resetSystem();
                particle->update(0.15f);
            }
        }
    }

    std::string formattedName = gauntletName;
    std::replace(formattedName.begin(), formattedName.end(), ' ', '\n');

    auto nameLabel = CCLabelBMFont::create(formattedName.c_str(), "bigFont.fnt");
    nameLabel->setAlignment(kCCTextAlignmentCenter);
    nameLabel->setPosition({gauntletBg->getContentSize().width / 2, gauntletBg->getContentSize().height - 15});
    nameLabel->setAnchorPoint({0.5f, 1.0f});
    nameLabel->limitLabelWidth(gauntletBg->getContentSize().width - 30.f, 0.5f, 0.3f);

    auto nameLabelShadow = CCLabelBMFont::create(formattedName.c_str(), "bigFont.fnt");
    nameLabelShadow->setAlignment(kCCTextAlignmentCenter);
    nameLabelShadow->setPosition({nameLabel->getPositionX() + 2, nameLabel->getPositionY() - 2});
    nameLabelShadow->setAnchorPoint({0.5f, 1.0f});
    nameLabelShadow->setColor({0, 0, 0});
    nameLabelShadow->setOpacity(50);
    nameLabelShadow->limitLabelWidth(gauntletBg->getContentSize().width - 30.f, 0.5f, 0.3f);

    gauntletBg->addChild(nameLabel, 3);
    gauntletBg->addChild(nameLabelShadow, 2);

    gauntletBg->setColor({static_cast<GLubyte>(node.r), static_cast<GLubyte>(node.g), static_cast<GLubyte>(node.b)});

    auto const imageCenter = ccp(gauntletBg->getContentSize().width / 2, gauntletBg->getContentSize().height / 2 + 10);
    auto imageUrl = std::string(gdx::baseApiUrl()) + "/gauntlet/gauntlet_" + numToString(node.id) + ".png?v2=true";

    auto imageContainer = CCNodeRGBA::create();
    if (imageContainer) {
        imageContainer->setContentSize({65.f, 90.f});
        imageContainer->setAnchorPoint({0.5f, 0.5f});
        imageContainer->setPosition(imageCenter);
        gauntletBg->addChild(imageContainer, 3);
    }

    auto imageTarget = imageContainer ? imageContainer : gauntletBg;
    auto imageSize = imageContainer ? imageContainer->getContentSize() : CCSize{90.f, 140.f};
    auto imagePosition = imageContainer ? ccp(imageSize.width / 2.f, imageSize.height / 2.f) : imageCenter;

    CCSprite* gauntletImage = nullptr;
    if (local && gauntlet["spritePath"].isString()) {
        auto spritePath = gauntlet["spritePath"].asString().unwrapOr("");
        if (!spritePath.empty()) {
            asp::fs::path path(spritePath);
            if (asp::fs::isFile(path).unwrapOr(false)) {
                auto localSprite = createLocalSprite(imageTarget, path, imagePosition, imageSize, 0);
                gauntletImage = localSprite;
            }
        }
    }
    if (!gauntletImage) {
        gauntletImage = createRemoteSprite(imageTarget, imageUrl, imagePosition, imageSize, 0);
    }

    auto completedCount = 0;
    for (auto const& level : node.levelIds) {
        if (m_completedGauntletLevels.contains(level.levelId)) {
            ++completedCount;
        }
    }

    bool hasCompletedAllLevels = !node.levelIds.empty() && completedCount == static_cast<int>(node.levelIds.size());
    bool isClaimedGauntlet = hasCompletedAllLevels && m_claimedGauntlets.contains(gauntletIndex);
    CCMenuItemSpriteExtra* rewardBtn = nullptr;

    if (isClaimedGauntlet) {
        auto completedIconShadow = CCSprite::createWithSpriteFrameName("GJ_completesIcon_001.png");
        if (completedIconShadow) {
            completedIconShadow->setColor({0, 0, 0});
            completedIconShadow->setOpacity(50);
            completedIconShadow->setScale(1.1f);
            completedIconShadow->setPosition({gauntletBg->getContentSize().width / 2.f + 2.f, 48.f});
            gauntletBg->addChild(completedIconShadow, 3);
        }

        auto completedIcon = CCSprite::createWithSpriteFrameName("GJ_completesIcon_001.png");
        if (completedIcon) {
            completedIcon->setScale(1.1f);
            completedIcon->setPosition({gauntletBg->getContentSize().width / 2.f, 50.f});
            gauntletBg->addChild(completedIcon, 4);
        }
    } else if (hasCompletedAllLevels && !local) {
        auto rewardBtnSpr = CCSprite::createWithSpriteFrameName("GJ_rewardBtn_001.png");
        rewardBtnSpr->setScale(0.8f);
        if (rewardBtnSpr) {
            rewardBtn = CCMenuItemSpriteExtra::create(rewardBtnSpr, this, menu_selector(GDXGauntletLayer::onCompleteGauntlet));
            rewardBtn->setTag(gauntletIndex);
            rewardBtn->setPosition({gauntletBg->getContentSize().width / 2.f, 50.f});
            auto rewardMenu = CCMenu::create(rewardBtn, nullptr);
            if (rewardMenu) {
                rewardMenu->setPosition({0.f, 0.f});
                gauntletBg->addChild(rewardMenu, 3);
            }
        }
    } else if (!local) {
        auto rewardLabel = CCLabelBMFont::create(numToString(node.reward).c_str(), "bigFont.fnt");
        rewardLabel->setAlignment(kCCTextAlignmentLeft);
        rewardLabel->setAnchorPoint({1.f, 0.5f});
        rewardLabel->setScale(0.5f);
        rewardLabel->limitLabelWidth(50.f, 0.5f, 0.35f);
        rewardLabel->setPosition({gauntletBg->getContentSize().width / 2.f, 50.f});
        gauntletBg->addChild(rewardLabel, 3);

        auto rewardLabelShadow = CCLabelBMFont::create(numToString(node.reward).c_str(), "bigFont.fnt");
        rewardLabelShadow->setAlignment(kCCTextAlignmentLeft);
        rewardLabelShadow->setAnchorPoint({1.f, 0.5f});
        rewardLabelShadow->limitLabelWidth(50.f, 0.5f, 0.35f);
        rewardLabelShadow->setPosition({rewardLabel->getPositionX() + 2.f, rewardLabel->getPositionY() - 2.f});
        rewardLabelShadow->setColor({0, 0, 0});
        rewardLabelShadow->setOpacity(50);
        gauntletBg->addChild(rewardLabelShadow, 2);

        auto rewardIcon = CCSprite::createWithSpriteFrameName("GDX_gauntletPoint.png"_spr);
        if (rewardIcon) {
            rewardIcon->setScale(0.3f);
            rewardIcon->setAnchorPoint({0.f, 0.5f});
            rewardIcon->setPosition({rewardLabel->getPositionX() + 5, 50.f});
            gauntletBg->addChild(rewardIcon, 3);
        }

        auto rewardIconShadow = CCSprite::createWithSpriteFrameName("GDX_gauntletPoint.png"_spr);
        if (rewardIconShadow) {
            rewardIconShadow->setColor({0, 0, 0});
            rewardIconShadow->setOpacity(50);
            rewardIconShadow->setScale(0.3f);
            rewardIconShadow->setAnchorPoint({0.f, 0.5f});
            rewardIconShadow->setPosition({rewardLabel->getPositionX() + 5, 48.f});
            gauntletBg->addChild(rewardIconShadow, 2);
        }

        auto rewardTextLabel = CCLabelBMFont::create("Reward", "goldFont.fnt");
        rewardTextLabel->setAlignment(kCCTextAlignmentCenter);
        rewardTextLabel->setAnchorPoint({0.5f, 0.5f});
        rewardTextLabel->setPosition({gauntletBg->getContentSize().width / 2.f, 70.f});
        rewardTextLabel->setScale(0.4f);
        gauntletBg->addChild(rewardTextLabel, 3);

        auto rewardTextLabelShadow = CCLabelBMFont::create("Reward", "goldFont.fnt");
        rewardTextLabelShadow->setAlignment(kCCTextAlignmentCenter);
        rewardTextLabelShadow->setAnchorPoint({0.5f, 0.5f});
        rewardTextLabelShadow->setPosition({rewardTextLabel->getPositionX() + 2.f, rewardTextLabel->getPositionY() - 2.f});
        rewardTextLabelShadow->setColor({0, 0, 0});
        rewardTextLabelShadow->setOpacity(50);
        rewardTextLabelShadow->setScale(0.4f);
        gauntletBg->addChild(rewardTextLabelShadow, 2);
    }

    if (gauntlet["tags"].isArray() && gauntlet["tags"].size() > 0) {
        auto tagsArray = gauntlet["tags"];
        auto firstTag = tagsArray[0];
        std::string firstTagName = firstTag["name"].asString().unwrapOr("Tag");
        if (firstTagName.empty()) {
            firstTagName = "Tag";
        }

        // @geode-ignore(unknown-resource)
        auto tagCell = NineSlice::createWithSpriteFrameName("geode.loader/tab-bg.png");
        if (tagCell) {
            auto r = static_cast<GLubyte>(firstTag["r"].asInt().unwrapOr(255));
            auto g = static_cast<GLubyte>(firstTag["g"].asInt().unwrapOr(255));
            auto b = static_cast<GLubyte>(firstTag["b"].asInt().unwrapOr(255));
            tagCell->setColor({r, g, b});
            tagCell->setOpacity(255);
            tagCell->setScale(0.5f);

            auto tagLabel = CCLabelBMFont::create(firstTagName.c_str(), "bigFont.fnt");
            if (tagLabel) {
                tagLabel->setScale(0.4f);
                tagLabel->setColor({255, 255, 255});
                tagLabel->limitLabelWidth(90.f, 0.5f, 0.35f);
                tagCell->setContentSize({tagLabel->getScaledContentSize().width + 10.f, tagLabel->getScaledContentSize().height + 10.f});
                tagCell->addChildAtPosition(tagLabel, Anchor::Center, {0.f, 0.f}, false);
            }

            CCLabelBMFont* extraLabel = nullptr;
            int extraCount = static_cast<int>(tagsArray.size()) - 1;
            if (extraCount > 0) {
                extraLabel = CCLabelBMFont::create(fmt::format("+{}", extraCount).c_str(), "goldFont.fnt");
                if (extraLabel) {
                    extraLabel->setScale(0.7f);
                    extraLabel->setColor({255, 255, 255});
                    extraLabel->setAnchorPoint({0.f, 0.5f});
                    tagCell->addChildAtPosition(extraLabel, Anchor::Right, {2.f, 0.f}, false);
                }
            }

            auto tagButton = CCMenuItemSpriteExtra::create(tagCell, this, menu_selector(GDXGauntletLayer::onShowGauntletTags));
            if (tagButton) {
                tagButton->setTag(static_cast<int>(index));
                tagButton->setPosition({gauntletBg->getContentSize().width / 2.f, 18.f});
                auto tagMenu = CCMenu::create(tagButton, nullptr);
                if (tagMenu) {
                    tagMenu->setPosition({0.f, 0.f});
                    gauntletBg->addChild(tagMenu, 4);
                }
            }
        }
    }

    auto completionLabelShadow = CCLabelBMFont::create(fmt::format("{}/{}", completedCount, node.levelIds.size()).c_str(), "bigFont.fnt");
    completionLabelShadow->setScale(0.4f);
    completionLabelShadow->setAnchorPoint({0.5f, 0.5f});
    completionLabelShadow->setPosition({imageCenter.x + 2.f, imageCenter.y - 42.f});
    completionLabelShadow->setColor({0, 0, 0});
    completionLabelShadow->setOpacity(50);
    gauntletBg->addChild(completionLabelShadow, 2);

    auto completionLabel = CCLabelBMFont::create(fmt::format("{}/{}", completedCount, node.levelIds.size()).c_str(), "bigFont.fnt");
    completionLabel->setScale(0.4f);
    completionLabel->setAnchorPoint({0.5f, 0.5f});
    completionLabel->setPosition({imageCenter.x, imageCenter.y - 40.f});
    gauntletBg->addChild(completionLabel, 3);

    // show predominant feedback (likes or dislikes) under the completion label
    if (!this->m_localMode) {
        auto feedback = gauntlet["feedback"];
        int likes = feedback["likes"].asInt().unwrapOr(0);
        int dislikes = feedback["dislikes"].asInt().unwrapOr(0);
        int displayValue = likes >= dislikes ? likes : dislikes;
        const char* iconName = likes >= dislikes ? "GJ_likesIcon_001.png" : "GJ_dislikesIcon_001.png";

        auto feedbackIcon = CCSprite::createWithSpriteFrameName(iconName);
        if (feedbackIcon) {
            feedbackIcon->setScale(0.4f);
            feedbackIcon->setAnchorPoint({1.f, 0.5f});
            feedbackIcon->setPosition({imageCenter.x - 1.f, imageCenter.y - 51.f});
            gauntletBg->addChild(feedbackIcon, 3);
        }

        auto feedbackLabel = CCLabelBMFont::create(numToString(displayValue).c_str(), "bigFont.fnt");
        if (feedbackLabel) {
            feedbackLabel->limitLabelWidth(80.f, 0.35f, 0.25f);
            feedbackLabel->setAnchorPoint({0.f, 0.5f});
            feedbackLabel->setPosition({imageCenter.x + 1.f, imageCenter.y - 51.f});
            gauntletBg->addChild(feedbackLabel, 3);
        }
    }

    auto button = CCMenuItemSpriteExtra::create(gauntletBg, this, menu_selector(GDXGauntletLayer::onGauntletButtonClick));
    if (!button) {
        return nullptr;
    }
    button->setTag(static_cast<int>(index));
    m_gauntletButtons.push_back(button);
    button->m_scaleMultiplier = 1.05f;
    button->setVisible(true);
    return button;
}

void GDXGauntletLayer::onCompleteGauntlet(CCObject* sender) {
    auto button = static_cast<CCMenuItemSpriteExtra*>(sender);
    if (!button) {
        return;
    }

    CCPoint buttonPos = button->getPosition();

    auto gauntletIndex = button->getTag();
    auto accountData = argon::getGameAccountData();
    auto url = std::string(gdx::baseApiUrl()) + "/completeGauntlet";
    matjson::Value body = matjson::Value::object();
    body["accountId"] = accountData.accountId;
    body["gauntletIndex"] = gauntletIndex;

    auto rewardSpinner = LoadingSpinner::create(35.f);
    if (rewardSpinner) {
        rewardSpinner->setPosition(buttonPos);
        rewardSpinner->setVisible(true);
        if (auto parent = button->getParent()) {
            parent->addChild(rewardSpinner, 4);
        } else {
            this->addChild(rewardSpinner, 4);
        }
        button->setVisible(false);
    }

    auto self = geode::Ref<GDXGauntletLayer>(this);
    m_completeGauntletTask.spawn([self = std::move(self), button, buttonPos, rewardSpinner, url = std::move(url), body = std::move(body), accountData = std::move(accountData), gauntletIndex]() mutable -> arc::Future<> {
        auto token = co_await gdx::argonToken(accountData);
        if (token.empty()) {
            co_await geode::async::waitForMainThread([self = std::move(self), button, rewardSpinner]() {
                if (button) {
                    button->setVisible(true);
                }
                if (rewardSpinner) {
                    rewardSpinner->removeFromParent();
                }
                Notification::create("Authentication failed", NotificationIcon::Warning)->show();
            });
            co_return;
        }

        body["argonToken"] = std::move(token);
        auto response = co_await geode::utils::web::WebRequest()
                            .url(url)
                            .header("Content-Type", "application/json")
                            .bodyJSON(body)
                            .post(url);

        if (response.error() || response.cancelled() || !response.ok()) {
            auto errMsg = gdx::getResponseMessage(response, "Failed to claim gauntlet");
            co_await geode::async::waitForMainThread([self = std::move(self), button, rewardSpinner, errMsg = std::move(errMsg)]() {
                if (button) {
                    button->setVisible(true);
                }
                if (rewardSpinner) {
                    rewardSpinner->removeFromParent();
                }
                Notification::create(errMsg, NotificationIcon::Warning)->show();
            });
            co_return;
        }

        auto jsonResult = response.json();
        if (!jsonResult) {
            auto errMsg = gdx::getResponseMessage(response, "Failed to claim gauntlet");
            co_await geode::async::waitForMainThread([self = std::move(self), button, rewardSpinner, errMsg = std::move(errMsg)]() {
                if (button) {
                    button->setVisible(true);
                }
                if (rewardSpinner) {
                    rewardSpinner->removeFromParent();
                }
                Notification::create(errMsg, NotificationIcon::Warning)->show();
            });
            co_return;
        }

        auto result = std::move(jsonResult).unwrap();
        bool success = result["success"].asBool().unwrapOr(false);
        auto newGauntletPoints = result["gauntletPoints"].asInt().unwrapOr(self->m_gauntletPoints);
        auto errMsg = gdx::getResponseMessage(response, "Failed to claim gauntlet");

        co_await geode::async::waitForMainThread([self = std::move(self), button, buttonPos, rewardSpinner, success, newGauntletPoints, gauntletIndex, errMsg = std::move(errMsg)]() mutable {
            if (rewardSpinner) {
                rewardSpinner->removeFromParent();
            }
            if (!success) {
                if (button) {
                    button->setVisible(true);
                }
                Notification::create(errMsg, NotificationIcon::Warning)->show();
                return;
            }

            if (self && self->isRunning()) {
                self->m_gauntletPoints = static_cast<int>(newGauntletPoints);
                if (self->m_gauntletPointsCounter) {
                    self->m_gauntletPointsCounter->setTargetCount(self->m_gauntletPoints);
                    Mod::get()->setSavedValue<int>("gauntletPoints", self->m_gauntletPoints);
                }

                self->m_claimedGauntlets.insert(gauntletIndex);
                addCompletedGauntlet(gauntletIndex);
            }

            if (button && button->getParent()) {
                auto completedIconShadow = CCSprite::createWithSpriteFrameName("GJ_completesIcon_001.png");
                if (completedIconShadow) {
                    completedIconShadow->setColor({0, 0, 0});
                    completedIconShadow->setOpacity(50);
                    completedIconShadow->setScale(1.1f);
                    completedIconShadow->setPosition({buttonPos.x + 2.f, buttonPos.y - 2.f});
                    button->getParent()->addChild(completedIconShadow, 3);
                }

                auto completedIcon = CCSprite::createWithSpriteFrameName("GJ_completesIcon_001.png");
                if (completedIcon) {
                    completedIcon->setScale(1.1f);
                    completedIcon->setPosition(buttonPos);
                    button->getParent()->addChild(completedIcon, 4);
                }
                auto circleWave = CCCircleWave::create(10.f, 110.f, 0.5f, false);
                circleWave->setPosition(buttonPos);
                circleWave->m_color = ccColor3B({255, 100, 100});
                button->getParent()->addChild(circleWave, 3);
            }

            Notification::create("Gauntlet Completed!", NotificationIcon::Success)->show();
            // @geode-ignore(unknown-resource)
            FMODAudioEngine::sharedEngine()->playEffect("gold02.ogg");

            if (self && !self->m_localMode) {
                const auto& activeGauntlets = self->getActiveGauntlets();
                if (activeGauntlets.isArray()) {
                    for (auto i = 0u; i < activeGauntlets.size(); ++i) {
                        auto gauntlet = activeGauntlets[i];
                        auto currentIndex = gauntlet["id"].asInt().unwrapOr(gauntlet["index"].asInt().unwrapOr(-1));
                        if (currentIndex == gauntletIndex) {
                            GDXLikeItemPopup::create(gauntlet)->show();
                            break;
                        }
                    }
                }
            }
        });

        co_return; }, []() {});
}

void GDXGauntletLayer::createGauntletPages(const matjson::Value& gauntlets, bool local) {
    if (local) {
        m_localGauntlets = gauntlets;
    } else {
        m_onlineGauntlets = gauntlets;
        m_gauntlets = gauntlets;
    }

    m_completedGauntletLevels = loadCompletedGauntletLevels(local);
    m_claimedGauntlets = loadCompletedGauntlets();
    auto& targetScroll = local ? m_localScrollLayer : m_scrollLayer;
    auto& targetButtons = local ? m_localGauntletButtons : m_gauntletButtons;

    if (targetScroll) {
        targetScroll->removeFromParent();
        targetScroll = nullptr;
    } else {
        for (auto button : targetButtons) {
            if (button && button->getParent()) {
                button->removeFromParent();
            }
        }
    }
    targetButtons.clear();

    if (!local) {
        if (m_prevPageBtn != nullptr) {
            m_prevPageBtn->removeFromParent();
            m_prevPageBtn = nullptr;
        }

        if (m_nextPageBtn != nullptr) {
            m_nextPageBtn->removeFromParent();
            m_nextPageBtn = nullptr;
        }
        if (m_pageNavMenu != nullptr) {
            m_pageNavMenu->removeFromParent();
            m_pageNavMenu = nullptr;
        }
    }

    const auto winSize = CCDirector::sharedDirector()->getWinSize();

    if (!gauntlets.isArray()) {
        return;
    }

    const auto gauntletCount = gauntlets.size();
    if (gauntletCount == 0) {
        return;
    }

    auto pages = CCArray::create();

    CCLayer* page = nullptr;
    CCMenu* pageMenu = nullptr;
    for (auto i = 0u; i < gauntletCount; ++i) {
        if (i % 3 == 0) {
            page = CCLayer::create();
            page->setContentSize(winSize);
            pageMenu = CCMenu::create();
            pageMenu->setLayout(RowLayout::create()->setGap(0.f)->setAxisAlignment(AxisAlignment::Center));
            pageMenu->setPosition({page->getContentSize().width / 2, page->getContentSize().height / 2 + 35.f});
            page->addChild(pageMenu);
            pages->addObject(page);
        }

        auto gauntletButton = createGauntletButton(gauntlets[i], i, local);
        if (pageMenu && gauntletButton) {
            pageMenu->addChild(gauntletButton);
            targetButtons.push_back(gauntletButton);
        }

        if (pageMenu && ((i % 3 == 2) || (i + 1 == gauntletCount))) {
            pageMenu->updateLayout();
        }
    }

    if (pages->count() == 0) {
        return;
    }

    targetScroll = BoomScrollLayer::create(pages, 0, false);
    if (!targetScroll) {
        return;
    }

    targetScroll->setPosition({0, -45});
    targetScroll->setVisible(local ? m_localMode : !m_localMode);
    this->addChild(targetScroll);
    m_currentPage = 0;

    auto pageCount = pages->count();
    if (pageCount > 1) {
        if (!m_prevPageBtn) {
            auto prevSpr = CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
            if (prevSpr) {
                prevSpr->setFlipX(true);
            }
            m_prevPageBtn = CCMenuItemSpriteExtra::create(
                prevSpr,
                this,
                menu_selector(GDXGauntletLayer::onPrev));
            if (m_prevPageBtn) {
                m_prevPageBtn->setPosition({25.f, winSize.height / 2.f});
            }
        }

        if (!m_nextPageBtn) {
            auto nextSpr = CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
            m_nextPageBtn = CCMenuItemSpriteExtra::create(
                nextSpr,
                this,
                menu_selector(GDXGauntletLayer::onNext));
            if (m_nextPageBtn) {
                m_nextPageBtn->setPosition({winSize.width - 25.f, winSize.height / 2.f});
            }
        }

        if (!m_pageNavMenu) {
            m_pageNavMenu = CCMenu::create();
            if (m_pageNavMenu) {
                m_pageNavMenu->setPosition({0, 0});
                this->addChild(m_pageNavMenu, 2);
            }
        }

        if (m_pageNavMenu) {
            if (m_prevPageBtn && !m_prevPageBtn->getParent()) {
                m_pageNavMenu->addChild(m_prevPageBtn);
            }
            if (m_nextPageBtn && !m_nextPageBtn->getParent()) {
                m_pageNavMenu->addChild(m_nextPageBtn);
            }
        }

        updatePageButtons();
    } else {
        if (!m_prevPageBtn) {
            auto prevSpr = CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
            if (prevSpr) {
                prevSpr->setFlipX(true);
            }
            m_prevPageBtn = CCMenuItemSpriteExtra::create(
                prevSpr,
                this,
                menu_selector(GDXGauntletLayer::onPrev));
            if (m_prevPageBtn) {
                m_prevPageBtn->setPosition({25.f, winSize.height / 2.f});
            }
        }

        if (!m_nextPageBtn) {
            auto nextSpr = CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
            m_nextPageBtn = CCMenuItemSpriteExtra::create(
                nextSpr,
                this,
                menu_selector(GDXGauntletLayer::onNext));
            if (m_nextPageBtn) {
                m_nextPageBtn->setPosition({winSize.width - 25.f, winSize.height / 2.f});
            }
        }

        if (!m_pageNavMenu) {
            m_pageNavMenu = CCMenu::create();
            if (m_pageNavMenu) {
                m_pageNavMenu->setPosition({0, 0});
                this->addChild(m_pageNavMenu, 2);
            }
        }

        if (m_pageNavMenu) {
            if (m_prevPageBtn && !m_prevPageBtn->getParent()) {
                m_pageNavMenu->addChild(m_prevPageBtn);
            }
            if (m_nextPageBtn && !m_nextPageBtn->getParent()) {
                m_pageNavMenu->addChild(m_nextPageBtn);
            }
        }

        updatePageButtons();
    }
}

void GDXGauntletLayer::update(float dt) {
    auto layer = getActiveScrollLayer();
    if (!layer) {
        return;
    }

    auto page = layer->pageNumberForPosition(layer->getPosition());
    if (page < 0 || page >= layer->getTotalPages()) {
        page = layer->m_page;
    }
    if (page != m_currentPage) {
        m_currentPage = page;
        updatePageButtons();
    }
}

void GDXGauntletLayer::onPrev(CCObject* sender) {
    auto layer = getActiveScrollLayer();
    if (!layer) {
        return;
    }

    auto total = layer->getTotalPages();
    if (total <= 0) {
        return;
    }

    auto page = layer->m_page;
    page = (page - 1 + total) % total;

    layer->moveToPage(page);
    layer->updatePages();
    updatePageButtons();
}

void GDXGauntletLayer::onNext(CCObject* sender) {
    auto layer = getActiveScrollLayer();
    if (!layer) {
        return;
    }

    auto total = layer->getTotalPages();
    if (total <= 0) {
        return;
    }

    auto page = layer->m_page;
    page = (page + 1) % total;

    layer->moveToPage(page);
    layer->updatePages();
    updatePageButtons();
}

void GDXGauntletLayer::updatePageButtons() {
    auto layer = getActiveScrollLayer();
    if (!layer) {
        return;
    }

    auto page = layer->pageNumberForPosition(layer->getPosition());
    if (page < 0 || page >= layer->getTotalPages()) {
        page = layer->m_page;
    }
    auto totalPages = layer->getTotalPages();
    if (totalPages <= 1) {
        if (m_prevPageBtn) {
            m_prevPageBtn->setVisible(false);
        }
        if (m_nextPageBtn) {
            m_nextPageBtn->setVisible(false);
        }
        if (m_pageNavMenu) {
            m_pageNavMenu->setVisible(false);
        }
        return;
    }

    if (m_prevPageBtn) {
        m_prevPageBtn->setVisible(true);
    }
    if (m_nextPageBtn) {
        m_nextPageBtn->setVisible(true);
    }
    if (m_pageNavMenu) {
        m_pageNavMenu->setVisible(true);
    }
}

void GDXGauntletLayer::updateLocalToggleState() {
    if (m_localToggle) {
        auto sprite = createLocalToggleSprite(m_localMode);
        if (sprite) {
            m_localToggle->setSprite(sprite);
            m_localToggle->updateSprite();
        }
    }
    if (m_levelPointsSpr) {
        m_levelPointsSpr->setVisible(!m_localMode);
    }
    if (m_gauntletPointsSpr) {
        m_gauntletPointsSpr->setVisible(!m_localMode);
    }
    if (m_levelPointsCounter) {
        m_levelPointsCounter->setVisible(!m_localMode);
    }
    if (m_gauntletPointsCounter) {
        m_gauntletPointsCounter->setVisible(!m_localMode);
    }
    if (m_tagFilterBtn) {
        m_tagFilterBtn->setVisible(!m_localMode);
    }
}

void GDXGauntletLayer::updateSearchButtonState() {
    if (m_searchBtn) {
        auto sprite = CircleButtonSprite::createWithSpriteFrameName(
            "GDX_searchIcon.png"_spr,
            1.f,
            m_searchQuery.empty() ? CircleBaseColor::Green : CircleBaseColor::Cyan,
            CircleBaseSize::Small);
        if (sprite) {
            m_searchBtn->setSprite(sprite);
            m_searchBtn->updateSprite();
        }
    }

    if (m_tagFilterBtn) {
        auto tagSprite = CircleButtonSprite::createWithSpriteFrameName(
            "GDX_searchTags.png"_spr,
            1.f,
            m_tagFilter.empty() ? CircleBaseColor::Green : CircleBaseColor::Cyan,
            CircleBaseSize::Small);
        if (tagSprite) {
            m_tagFilterBtn->setSprite(tagSprite);
            m_tagFilterBtn->updateSprite();
        }
    }
}

void GDXGauntletLayer::updateRecentToggleState() {
    if (m_recentToggle) {
        auto sprite = createRecentToggleSprite(m_recentFilter);
        if (sprite) {
            m_recentToggle->setSprite(sprite);
            m_recentToggle->updateSprite();
        }
        m_recentToggle->setVisible(!m_localMode);
    }
}
