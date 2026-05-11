#include "GDXGauntletLayer.hpp"
#include "GDXGauntletLevelsLayer.hpp"
#include "GDXLeaderboardLayer.hpp"
#include "../popup/GDXGauntletManagePopup.hpp"
#include <Geode/Enums.hpp>
#include <Geode/binding/UploadActionPopup.hpp>
#include <algorithm>
#include <Geode/Geode.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/ui/LoadingSpinner.hpp>
#include <Geode/ui/BasedButton.hpp>
#include <Geode/ui/MDPopup.hpp>
#include <Geode/utils/file.hpp>
#include <Geode/utils/web.hpp>
#include <asp/fs.hpp>
#include "../include/GDXConstant.hpp"
#include "Geode/ui/BasedButtonSprite.hpp"
#include "Geode/ui/Layout.hpp"
#include "Geode/ui/LazySprite.hpp"
#include <argon/argon.hpp>

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

    static bool saveCompletedGauntletLevels(std::unordered_set<int> const& levels) {
        auto path = getCompletedGauntletLevelsPath();
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

    static bool addCompletedGauntletLevel(int levelId) {
        auto levels = loadCompletedGauntletLevels();
        levels.insert(levelId);
        return saveCompletedGauntletLevels(levels);
    }

    static asp::fs::path getCompletedGauntletPath() {
        auto dir = geode::dirs::getModsSaveDir() / geode::Mod::get()->getID();
        if (auto res = asp::fs::createDirAll(dir); !res) {
            log::warn("Failed to create completed gauntlet save directory: {}", res.unwrapErr().message());
        }
        return dir / "completed_gauntlets.json";
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

    // info button
    auto bottomMenu = CCMenu::create();
    bottomMenu->setPosition({30, 70});
    bottomMenu->setContentHeight(100);
    bottomMenu->setLayout(ColumnLayout::create()->setGap(5.f)->setAxisAlignment(AxisAlignment::Start));
    this->addChild(bottomMenu, 2);

    auto infoIconSpr = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
    auto infoIconBtn = CCMenuItemSpriteExtra::create(infoIconSpr, this, menu_selector(GDXGauntletLayer::onInfo));
    bottomMenu->addChild(infoIconBtn);

    // leaderboard button
    auto leaderboardIconSpr = CCSprite::createWithSpriteFrameName("GJ_levelLeaderboardBtn_001.png");
    leaderboardIconSpr->setScale(0.6f);
    auto leaderboardIconBtn = CCMenuItemSpriteExtra::create(leaderboardIconSpr, this, menu_selector(GDXGauntletLayer::onLeaderboard));
    bottomMenu->addChild(leaderboardIconBtn);

    // discord button
    auto discordIconSpr = CircleButtonSprite::createWithSpriteFrameName("GDX_discord.png"_spr, 1.f, CircleBaseColor::Green, CircleBaseSize::Small);
    discordIconSpr->setScale(0.7f);
    auto discordIconBtn = CCMenuItemSpriteExtra::create(discordIconSpr, this, menu_selector(GDXGauntletLayer::onDiscord));
    discordIconBtn->setPosition({0.f, -35.f});
    bottomMenu->addChild(discordIconBtn);

    // @geode-ignore(unknown-resource)
    auto manageLabel = CircleButtonSprite::createWithSpriteFrameName("GDX_pencil.png"_spr, 1.f, CircleBaseColor::Green, CircleBaseSize::Small);
    manageLabel->setScale(0.7f);
    auto manageBtn = CCMenuItemSpriteExtra::create(manageLabel, this, menu_selector(GDXGauntletLayer::onManageGauntlets));
    manageBtn->setPosition({0.f, -70.f});
    bottomMenu->addChild(manageBtn);

    bottomMenu->updateLayout();

    // refresh gauntlet
    auto refreshSpr = CCSprite::createWithSpriteFrameName("GJ_updateBtn_001.png");
    refreshSpr->setScale(0.75f);
    auto refreshBtn = CCMenuItemSpriteExtra::create(
        refreshSpr,
        this,
        menu_selector(GDXGauntletLayer::onRefreshGauntlets));

    // sync account
    // @geode-ignore(unknown-resource)
    auto syncSpr = CircleButtonSprite::createWithSpriteFrameName("geode.loader/update.png", 1.f, CircleBaseColor::Green, CircleBaseSize::Small);
    auto syncBtn = CCMenuItemSpriteExtra::create(syncSpr, this, menu_selector(GDXGauntletLayer::onSyncAccount));
    auto refreshMenu = CCMenu::create(refreshBtn, syncBtn, nullptr);
    if (refreshMenu) {
        refreshMenu->setPosition({winSize.width - 30, 70});
        refreshMenu->setContentHeight(100);
        refreshMenu->setLayout(ColumnLayout::create()->setGap(5.f)->setAxisAlignment(AxisAlignment::Start));
        this->addChild(refreshMenu, 2);
    }

    refreshMenu->updateLayout();

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
    auto levelPointsSpr = CCSprite::createWithSpriteFrameName("GDX_levelPoint.png"_spr);
    levelPointsSpr->setPosition({winSize.width - 20, winSize.height - 20});
    levelPointsSpr->setScale(0.3f);
    this->addChild(levelPointsSpr, 5);
    m_levelPointsCounter = CCCounterLabel::create(static_cast<int>(m_levelPoints), "bigFont.fnt", FormatterType::Integer);
    if (m_levelPointsCounter) {
        m_levelPointsCounter->setPosition({levelPointsSpr->getPositionX() - 15, levelPointsSpr->getPositionY()});
        m_levelPointsCounter->setAnchorPoint({1.0f, 0.5f});
        m_levelPointsCounter->setScale(0.6f);
        this->addChild(m_levelPointsCounter, 5);
    }

    auto gauntletPointsSpr = CCSprite::createWithSpriteFrameName("GDX_gauntletPoint.png"_spr);
    gauntletPointsSpr->setPosition({winSize.width - 20, winSize.height - 50});
    gauntletPointsSpr->setScale(0.4f);
    this->addChild(gauntletPointsSpr, 5);
    m_gauntletPointsCounter = CCCounterLabel::create(static_cast<int>(m_gauntletPoints), "bigFont.fnt", FormatterType::Integer);
    if (m_gauntletPointsCounter) {
        m_gauntletPointsCounter->setPosition({gauntletPointsSpr->getPositionX() - 15, gauntletPointsSpr->getPositionY()});
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

void GDXGauntletLayer::onInfo(CCObject* sender) {
    MDPopup::create(
        "About Gauntlet Deluxe",
        "**Gauntlet Deluxe** is a <cl>community-run mod</c> that adds a <cg>custom gauntlet to Geometry Dash</c>, featuring fan-made <co>gauntlets based on various themes</c>.\n\n"
        "Players can earn points by completing <cp>levels</c> and <cr>gauntlets</c>, and <cl>compete on the leaderboard</c>.\n\n"
        "The mod also allows for <cl>community-hosted creator contests</c>, where players can submit their own gauntlet levels to be featured in the mod.\n\n"
        "\n---\n"
        "### Points System\n"
        "![GDX](frame:arcticwoof.gauntlets_deluxe/GDX_levelPoint.png?scale=0.15) <cp>**Level Points**</c>: Earned by completing levels in a gauntlet.\n\n"
        "Each level awards a different amount of points based on its difficulty and other factors.\n\n"
        "##### <cy>*If you have already beaten the level before using this mod, you still need to rebeat them to earn Level Points.*</c>\n\n"
        "![GDX](frame:arcticwoof.gauntlets_deluxe/GDX_gauntletPoint.png?scale=0.2) <cr>**Gauntlet Points**</c>: Earn points by completing any of these fan-made gauntlets.\n\n"
        "Gauntlet points are based on the amount of levels and their difficulty.\n"
        "\n---\n"
        "### <cp>Level Points</c>\n"
        "#### Level Points are measured in the following criteria:\n"
        "- **Auto - Easy**: 1 ![Level Points](frame:arcticwoof.gauntlets_deluxe/GDX_levelPoint.png?scale=0.15) <cp>Level Points</c> \n"
        "- **Normal**: 2 ![Level Points](frame:arcticwoof.gauntlets_deluxe/GDX_levelPoint.png?scale=0.15) <cp>Level Points</c>\n"
        "- **Hard** _(4-5 stars)_: 3 ![Level Points](frame:arcticwoof.gauntlets_deluxe/GDX_levelPoint.png?scale=0.15) <cp>Level Points</c>\n"
        "- **Harder** _(6-7 stars)_ - **Insane** _(8-9 stars)_: 4 ![Level Points](frame:arcticwoof.gauntlets_deluxe/GDX_levelPoint.png?scale=0.15) <cp>Level Points</c>\n"
        "- **Easy-Medium Demon**: 5 ![Level Points](frame:arcticwoof.gauntlets_deluxe/GDX_levelPoint.png?scale=0.15) <cp>Level Points</c>\n"
        "- **Hard-Extreme Demon**: 7 ![Level Points](frame:arcticwoof.gauntlets_deluxe/GDX_levelPoint.png?scale=0.15) <cp>Level Points</c>\n"
        "\n---\n"
        "### <cr>Gauntlet Points</c>\n"
        "#### Gauntlet Points are measured in the following criteria:\n"
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
    auto upopup = UploadActionPopup::create(nullptr, "Getting access...");
    upopup->show();
    auto accountData = argon::getGameAccountData();
    auto url = std::string(gdx::BASE_API_URL) + "/getAccess";
    matjson::Value body = matjson::Value::object();
    body["accountId"] = accountData.accountId;

    m_syncAccountTask.spawn([upopup, url = std::move(url), body = std::move(body), accountData = std::move(accountData)]() mutable -> arc::Future<> {
        auto token = co_await gdx::argonToken(accountData);
        if (token.empty()) {
            co_await geode::async::waitForMainThread([upopup] {
                upopup->showFailMessage("Authentication failed.");
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
            co_await geode::async::waitForMainThread([upopup, response] {
                upopup->showFailMessage(gdx::getResponseMessage(response, "Failed to get access."));
            });
            co_return;
        }

        auto jsonResult = response.json();
        if (!jsonResult) {
            co_await geode::async::waitForMainThread([upopup] {
                upopup->showFailMessage("Failed to get access.");
            });
            co_return;
        }

        auto result = std::move(jsonResult).unwrap();
        bool success = result["success"].asBool().unwrapOr(false);
        if (!result.isObject() || !success) {
            co_await geode::async::waitForMainThread([upopup] {
                upopup->showFailMessage("Failed to get access.");
            });
            co_return;
        }

        // get those roles lol
        bool isMod = result["isMod"].asBool().unwrapOr(false);
        bool isManager = result["isManager"].asBool().unwrapOr(false);

        Mod::get()->setSavedValue("isMod", isMod);
        Mod::get()->setSavedValue("isManager", isManager);

        if (!isManager && !isMod) {
            co_await geode::async::waitForMainThread([upopup, response] {
                upopup->showFailMessage(gdx::getResponseMessage(response, "Permissions Denied"));
            });
            co_return;
        }

        if (gdx::isManager() || gdx::isMod()) {
            co_await geode::async::waitForMainThread([upopup]() {
                upopup->onClose(nullptr);
                GDXGauntletManagePopup::create()->show();
            });
        }

        co_return; }, []() {});
}

void GDXGauntletLayer::onSyncAccount(CCObject* sender) {
    auto upopup = UploadActionPopup::create(nullptr, "Syncing account...");
    upopup->show();

    auto accountData = argon::getGameAccountData();
    auto url = std::string(gdx::BASE_API_URL) + "/syncUser";
    matjson::Value body = matjson::Value::object();
    body["accountId"] = accountData.accountId;
    body["argonToken"] = "";

    auto self = geode::Ref<GDXGauntletLayer>(this);
    m_syncAccountTask.spawn([self = std::move(self), upopup, url = std::move(url), body = std::move(body), accountData = std::move(accountData)]() mutable -> arc::Future<> {
        auto token = co_await gdx::argonToken(accountData);
        if (token.empty()) {
            co_await geode::async::waitForMainThread([upopup] {
                upopup->showFailMessage("Authentication failed.");
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
            co_await geode::async::waitForMainThread([upopup, errMsg = std::move(errMsg)] {
                upopup->showFailMessage(errMsg);
            });
            co_return;
        }

        auto jsonResult = response.json();
        if (!jsonResult) {
            co_await geode::async::waitForMainThread([upopup] {
                upopup->showFailMessage("Failed to sync account.");
            });
            co_return;
        }

        auto result = std::move(jsonResult).unwrap();
        bool success = result["success"].asBool().unwrapOr(false);
        if (!success) {
            co_await geode::async::waitForMainThread([upopup] {
                upopup->showFailMessage("Failed to sync account.");
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

        auto levelsSaved = saveCompletedGauntletLevels(completedLevels);
        auto gauntletsSaved = saveCompletedGauntlets(completedGauntlets);

        co_await geode::async::waitForMainThread([self, upopup, levelsSaved, gauntletsSaved]() mutable {
            if (levelsSaved && gauntletsSaved) {
                upopup->showSuccessMessage("Account synced successfully.");
                self->m_completedGauntletLevels = loadCompletedGauntletLevels();
                self->m_claimedGauntlets = loadCompletedGauntlets();
            } else {
                upopup->showFailMessage("Failed to save sync data.");
            }
        });

        co_return; }, []() {});
}

void GDXGauntletLayer::onRefreshGauntlets(CCObject* sender) {
    m_gauntlets = matjson::Value::array();
    createGauntletPages(m_gauntlets);
    fetchGauntlets();
}

void GDXGauntletLayer::onGauntletButtonClick(CCObject* sender) {
    auto button = static_cast<CCMenuItemSpriteExtra*>(sender);
    if (!button || !m_gauntlets.isArray()) {
        return;
    }

    auto idx = button->getTag();
    if (idx < 0 || idx >= static_cast<int>(m_gauntlets.size())) {
        return;
    }

    auto gauntlet = m_gauntlets[idx];
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
        levels->addObject(dict);
    }

    auto scene = CCScene::create();
    auto color = ccColor3B{
        static_cast<GLubyte>(gauntlet["r"].asInt().unwrapOr(255)),
        static_cast<GLubyte>(gauntlet["g"].asInt().unwrapOr(255)),
        static_cast<GLubyte>(gauntlet["b"].asInt().unwrapOr(255)),
    };
    scene->addChild(GDXGauntletLevelsLayer::create(levels, gauntlet["name"].asString().unwrapOr("Gauntlet"), color, gauntlet["index"].asInt().unwrapOr(idx)));
    CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(0.5f, scene));
}

void GDXGauntletLayer::fetchGauntlets() {
    if (m_loadingSpinner) {
        m_loadingSpinner->setVisible(true);
    }

    auto url = std::string(gdx::BASE_API_URL) + "/getGauntlets";
    auto self = geode::Ref<GDXGauntletLayer>(this);
    m_fetchGauntletsTask.spawn([self = std::move(self), url = std::move(url)]() -> arc::Future<> {
        auto response = co_await geode::utils::web::WebRequest()
                            .get(url);

        if (response.error() || response.cancelled() || !response.ok()) {
            co_await geode::async::waitForMainThread([self]() {
                if (self->m_loadingSpinner) {
                    self->m_loadingSpinner->setVisible(false);
                }
            });
            co_return;
        }

        auto jsonResult = response.json();
        if (!jsonResult) {
            co_await geode::async::waitForMainThread([self]() {
                if (self->m_loadingSpinner) {
                    self->m_loadingSpinner->setVisible(false);
                }
            });
            co_return;
        }

        auto gauntlets = std::move(jsonResult).unwrap();
        co_await geode::async::waitForMainThread([self, gauntlets = std::move(gauntlets)]() mutable {
            if (self->m_loadingSpinner) {
                self->m_loadingSpinner->setVisible(false);
            }
            self->createGauntletPages(gauntlets);
        });

        co_return; }, []() {});
}

void GDXGauntletLayer::fetchUserData() {
    auto accountData = argon::getGameAccountData();
    auto url = std::string(gdx::BASE_API_URL) + "/getUser";
    matjson::Value body = matjson::Value::object();
    body["accountId"] = accountData.accountId;
    body["argonToken"] = "";

    auto self = geode::Ref<GDXGauntletLayer>(this);
    m_fetchUserDataTask.spawn([self = std::move(self), url = std::move(url), body = std::move(body), accountData = std::move(accountData)]() mutable -> arc::Future<> {
        auto token = co_await gdx::argonToken(accountData);
        if (token.empty()) {
            co_return;
        }

        body["argonToken"] = std::move(token);

        auto response = co_await geode::utils::web::WebRequest()
                            .url(url)
                            .header("Content-Type", "application/json")
                            .bodyJSON(body)
                            .post(url);

        if (response.error() || response.cancelled() || !response.ok()) {
            co_return;
        }

        auto jsonResult = response.json();
        if (!jsonResult) {
            co_return;
        }

        auto userData = std::move(jsonResult).unwrap();
        if (!userData.isObject() || !userData["success"].asBool().unwrapOr(false)) {
            co_return;
        }

        co_await geode::async::waitForMainThread([self, userData = std::move(userData)]() mutable {
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

CCMenuItemSpriteExtra* GDXGauntletLayer::createGauntletButton(const matjson::Value& gauntlet, std::size_t index) {
    auto node = GDXGauntletNode::fromJson(gauntlet);
    int gauntletIndex = node.id;
    std::string gauntletName = node.name;

    auto gauntletBg = NineSlice::create("GJ_squareB_01.png");
    if (!gauntletBg) {
        return nullptr;
    }
    gauntletBg->setContentSize({110, 240});

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
    auto imageUrl = std::string(gdx::BASE_API_URL) + "/gauntlet/gauntlet_" + numToString(node.id) + ".png";

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
    auto gauntletImage = createRemoteSprite(imageTarget, imageUrl, imagePosition, imageSize, 0);

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
    } else if (hasCompletedAllLevels) {
        auto rewardBtnSpr = CCSprite::createWithSpriteFrameName("GJ_rewardBtn_001.png");
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
    } else {
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

    auto infoMenu = CCMenu::create();
    infoMenu->setPosition({0.f, 0.f});
    gauntletBg->addChild(infoMenu, 4);

    auto infoIconSpr = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
    infoIconSpr->setScale(0.65f);
    if (infoIconSpr) {
        auto infoBtn = CCMenuItemSpriteExtra::create(infoIconSpr, this, menu_selector(GDXGauntletLayer::onGauntletInfo));
        infoBtn->setTag(static_cast<int>(index));
        infoBtn->setPosition({gauntletBg->getContentSize().width - 5.f, gauntletBg->getContentSize().height - 5.f});
        infoMenu->addChild(infoBtn);
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
    CCPoint buttonPos = button->getPosition();
    if (!button) {
        return;
    }

    auto gauntletIndex = button->getTag();
    auto accountData = argon::getGameAccountData();
    auto url = std::string(gdx::BASE_API_URL) + "/completeGauntlet";
    matjson::Value body = matjson::Value::object();
    body["accountId"] = accountData.accountId;
    body["gauntletIndex"] = gauntletIndex;

    auto rewardSpinner = LoadingSpinner::create(45.f);
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
            co_await geode::async::waitForMainThread([self, button, buttonPos, rewardSpinner]() {
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
            co_await geode::async::waitForMainThread([self, button, buttonPos, rewardSpinner, response]() {
                if (button) {
                    button->setVisible(true);
                }
                if (rewardSpinner) {
                    rewardSpinner->removeFromParent();
                }
                Notification::create(gdx::getResponseMessage(response, "Failed to claim gauntlet"), NotificationIcon::Warning)->show();
            });
            co_return;
        }

        auto jsonResult = response.json();
        if (!jsonResult) {
            co_await geode::async::waitForMainThread([self, button, buttonPos, rewardSpinner, response]() {
                if (button) {
                    button->setVisible(true);
                }
                if (rewardSpinner) {
                    rewardSpinner->removeFromParent();
                }
                Notification::create(gdx::getResponseMessage(response, "Failed to claim gauntlet"), NotificationIcon::Warning)->show();
            });
            co_return;
        }

        auto result = std::move(jsonResult).unwrap();
        bool success = result["success"].asBool().unwrapOr(false);
        auto newGauntletPoints = result["gauntletPoints"].asInt().unwrapOr(self->m_gauntletPoints);

        co_await geode::async::waitForMainThread([self, button, buttonPos, rewardSpinner, success, newGauntletPoints, response, gauntletIndex]() mutable {
            if (rewardSpinner) {
                rewardSpinner->removeFromParent();
            }
            if (!success) {
                if (button) {
                    button->setVisible(true);
                }
                Notification::create(gdx::getResponseMessage(response, "Failed to claim gauntlet"), NotificationIcon::Warning)->show();
                return;
            }

            self->m_gauntletPoints = static_cast<int>(newGauntletPoints);
            if (self->m_gauntletPointsCounter) {
                self->m_gauntletPointsCounter->setTargetCount(self->m_gauntletPoints);
                Mod::get()->setSavedValue<int>("gauntletPoints", self->m_gauntletPoints);
            }

            self->m_claimedGauntlets.insert(gauntletIndex);
            addCompletedGauntlet(gauntletIndex);

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
            }

            Notification::create("Gauntlet Completed!", NotificationIcon::Success)->show();
            // @geode-ignore(unknown-resource)
            FMODAudioEngine::sharedEngine()->playEffect("gold02.ogg");
            auto circleWave = CCCircleWave::create(10.f, 110.f, 0.5f, false);
            circleWave->setPosition(buttonPos);
            circleWave->m_color = ccColor3B({255, 100, 100});
            button->getParent()->addChild(circleWave, 3);
        });

        co_return; }, []() {});
}

void GDXGauntletLayer::onGauntletInfo(CCObject* sender) {
    auto button = static_cast<CCMenuItemSpriteExtra*>(sender);
    if (!button || !m_gauntlets.isArray()) {
        return;
    }

    auto idx = button->getTag();
    if (idx < 0 || idx >= static_cast<int>(m_gauntlets.size())) {
        return;
    }

    auto gauntlet = m_gauntlets[idx];
    auto title = gauntlet["name"].asString().unwrapOr("Gauntlet");
    auto description = gauntlet["description"].asString().unwrapOr("No description available.");

    MDPopup::create(title.c_str(), description.c_str(), "OK")->show();
}

void GDXGauntletLayer::createGauntletPages(const matjson::Value& gauntlets) {
    m_gauntlets = gauntlets;

    m_completedGauntletLevels = loadCompletedGauntletLevels();
    m_claimedGauntlets = loadCompletedGauntlets();
    if (m_scrollLayer) {
        m_scrollLayer->removeFromParent();
        m_scrollLayer = nullptr;
    }
    for (auto button : m_gauntletButtons) {
        if (button) {
            button->removeFromParent();
        }
    }
    m_gauntletButtons.clear();

    if (m_prevPageBtn != nullptr) {
        m_prevPageBtn->removeFromParent();
        m_prevPageBtn = nullptr;
    }

    if (m_nextPageBtn != nullptr) {
        m_nextPageBtn->removeFromParent();
        m_nextPageBtn = nullptr;
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

        auto gauntletButton = createGauntletButton(gauntlets[i], i);
        if (pageMenu && gauntletButton) {
            pageMenu->addChild(gauntletButton);
        }

        if (pageMenu && ((i % 3 == 2) || (i + 1 == gauntletCount))) {
            pageMenu->updateLayout();
        }
    }

    if (pages->count() == 0) {
        return;
    }

    m_scrollLayer = BoomScrollLayer::create(pages, 0, false);
    if (!m_scrollLayer) {
        return;
    }

    m_scrollLayer->setPosition({0, -45});
    this->addChild(m_scrollLayer);
    m_currentPage = 0;

    auto pageCount = pages->count();
    if (pageCount > 1) {
        auto prevSpr = CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
        auto nextSpr = CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
        if (prevSpr) {
            prevSpr->setFlipX(true);
        }

        m_prevPageBtn = CCMenuItemSpriteExtra::create(
            prevSpr,
            this,
            menu_selector(GDXGauntletLayer::onPrev));
        m_nextPageBtn = CCMenuItemSpriteExtra::create(
            nextSpr,
            this,
            menu_selector(GDXGauntletLayer::onNext));

        if (m_prevPageBtn) {
            m_prevPageBtn->setPosition({25.f, winSize.height / 2.f});
        }
        if (m_nextPageBtn) {
            m_nextPageBtn->setPosition({winSize.width - 25.f, winSize.height / 2.f});
        }

        auto navMenu = CCMenu::create();
        if (navMenu) {
            navMenu->setPosition({0, 0});
            if (m_prevPageBtn) {
                navMenu->addChild(m_prevPageBtn);
            }
            if (m_nextPageBtn) {
                navMenu->addChild(m_nextPageBtn);
            }
            this->addChild(navMenu, 2);
        }

        updatePageButtons();
    }
}

void GDXGauntletLayer::update(float dt) {
    if (!m_scrollLayer) {
        return;
    }

    auto page = m_scrollLayer->pageNumberForPosition(m_scrollLayer->getPosition());
    if (page < 0 || page >= m_scrollLayer->getTotalPages()) {
        page = m_scrollLayer->m_page;
    }
    if (page != m_currentPage) {
        m_currentPage = page;
        updatePageButtons();
    }
}

void GDXGauntletLayer::onPrev(CCObject* sender) {
    if (!m_scrollLayer) {
        return;
    }

    auto total = m_scrollLayer->getTotalPages();
    if (total <= 0) {
        return;
    }

    auto page = m_scrollLayer->m_page;
    page = (page - 1 + total) % total;

    m_scrollLayer->moveToPage(page);
    m_scrollLayer->updatePages();
    // m_scrollLayer->repositionPagesLooped();
    updatePageButtons();
}

void GDXGauntletLayer::onNext(CCObject* sender) {
    if (!m_scrollLayer) {
        return;
    }

    auto total = m_scrollLayer->getTotalPages();
    if (total <= 0) {
        return;
    }

    auto page = m_scrollLayer->m_page;
    page = (page + 1) % total;

    m_scrollLayer->moveToPage(page);
    m_scrollLayer->updatePages();
    // m_scrollLayer->repositionPagesLooped();
    updatePageButtons();
}

void GDXGauntletLayer::updatePageButtons() {
    if (!m_scrollLayer) {
        return;
    }

    auto page = m_scrollLayer->pageNumberForPosition(m_scrollLayer->getPosition());
    if (page < 0 || page >= m_scrollLayer->getTotalPages()) {
        page = m_scrollLayer->m_page;
    }
    auto totalPages = m_scrollLayer->getTotalPages();
    if (totalPages <= 0) {
        return;
    }

    const bool prevEnabled = page > 0;
    const bool nextEnabled = page < totalPages - 1;

    if (m_prevPageBtn) {
        m_prevPageBtn->setEnabled(prevEnabled);
        m_prevPageBtn->setOpacity(prevEnabled ? 255 : 100);
    }
    if (m_nextPageBtn) {
        m_nextPageBtn->setEnabled(nextEnabled);
        m_nextPageBtn->setOpacity(nextEnabled ? 255 : 100);
    }
}
