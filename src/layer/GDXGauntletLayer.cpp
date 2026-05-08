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
#include <Geode/utils/web.hpp>
#include "../include/GDXConstant.hpp"
#include "Geode/ui/Layout.hpp"
#include "Geode/ui/MDPopup.hpp"
#include <argon/argon.hpp>

using namespace geode::prelude;

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

    // @geode-ignore(unknown-resource)
    auto manageLabel = CircleButtonSprite::createWithSpriteFrameName("GDX_pencil.png"_spr, 1.f, CircleBaseColor::Green, CircleBaseSize::Small);
    manageLabel->setScale(0.7f);
    auto manageBtn = CCMenuItemSpriteExtra::create(manageLabel, this, menu_selector(GDXGauntletLayer::onManageGauntlets));
    manageBtn->setPosition({0.f, -35.f});
    bottomMenu->addChild(manageBtn);

    bottomMenu->updateLayout();

    auto refreshSpr = CCSprite::createWithSpriteFrameName("GJ_updateBtn_001.png");
    refreshSpr->setScale(0.75f);
    auto refreshBtn = CCMenuItemSpriteExtra::create(
        refreshSpr,
        this,
        menu_selector(GDXGauntletLayer::onRefreshGauntlets));

    auto refreshMenu = CCMenu::create(refreshBtn, nullptr);
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
    this->addChild(levelPointsSpr);
    m_levelPointsCounter = CCCounterLabel::create(static_cast<int>(m_levelPoints), "bigFont.fnt", FormatterType::Integer);
    if (m_levelPointsCounter) {
        m_levelPointsCounter->setPosition({levelPointsSpr->getPositionX() - 15, levelPointsSpr->getPositionY()});
        m_levelPointsCounter->setAnchorPoint({1.0f, 0.5f});
        m_levelPointsCounter->setScale(0.6f);
        this->addChild(m_levelPointsCounter);
    }

    auto gauntletPointsSpr = CCSprite::createWithSpriteFrameName("GDX_gauntletPoint.png"_spr);
    gauntletPointsSpr->setPosition({winSize.width - 20, winSize.height - 50});
    gauntletPointsSpr->setScale(0.4f);
    this->addChild(gauntletPointsSpr);
    m_gauntletPointsCounter = CCCounterLabel::create(static_cast<int>(m_gauntletPoints), "bigFont.fnt", FormatterType::Integer);
    if (m_gauntletPointsCounter) {
        m_gauntletPointsCounter->setPosition({gauntletPointsSpr->getPositionX() - 15, gauntletPointsSpr->getPositionY()});
        m_gauntletPointsCounter->setAnchorPoint({1.0f, 0.5f});
        m_gauntletPointsCounter->setScale(0.6f);
        this->addChild(m_gauntletPointsCounter);
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

void GDXGauntletLayer::onInfo(CCObject* sender) {
    FLAlertLayer::create(
        "Gauntlet Deluxe",
        "This is a work in progress mod that adds a new gauntlet mode to Geometry Dash. "
        "The mod is currently in early development, so expect bugs and missing features. "
        "If you want to help out, join the Discord server linked on the mod page.",
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

    async::spawn([upopup, url = std::move(url), body = std::move(body), accountData = std::move(accountData)]() mutable -> arc::Future<> {
        auto token = co_await gdx::argonToken(accountData);
        if (token.empty()) {
            geode::queueInMainThread([upopup] {
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
            geode::queueInMainThread([upopup, response] {
                upopup->showFailMessage(gdx::getResponseMessage(response, "Failed to get access."));
            });
            co_return;
        }

        auto jsonResult = response.json();
        if (!jsonResult) {
            geode::queueInMainThread([upopup] {
                upopup->showFailMessage("Failed to get access.");
            });
            co_return;
        }

        auto result = std::move(jsonResult).unwrap();
        bool success = result["success"].asBool().unwrapOr(false);
        if (!result.isObject() || !success) {
            geode::queueInMainThread([upopup] {
                upopup->showFailMessage("Failed to get access.");
            });
            co_return;
        }

        // get those roles lol
        bool isMod = result["isMod"].asBool().unwrapOr(false);
        bool isManager = result["isManager"].asBool().unwrapOr(false);

        if (isMod) Mod::get()->setSavedValue("isMod", isMod);
        if (isManager) Mod::get()->setSavedValue("isManager", isManager);

        if (!isManager && !isMod) {
            geode::queueInMainThread([upopup, response] {
                upopup->showFailMessage(gdx::getResponseMessage(response, "Permissions Denied"));
            });
            co_return;
        }

        if (gdx::isManager() || gdx::isMod()) {
            geode::queueInMainThread([upopup]() {
                upopup->onClose(nullptr);
                GDXGauntletManagePopup::create()->show();
            });
        }

        co_return;
    });
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
    scene->addChild(GDXGauntletLevelsLayer::create(levels, gauntlet["name"].asString().unwrapOr("Gauntlet"), color));
    CCDirector::sharedDirector()->pushScene(CCTransitionFade::create(0.5f, scene));
}

void GDXGauntletLayer::fetchGauntlets() {
    if (m_loadingSpinner) {
        m_loadingSpinner->setVisible(true);
    }

    auto url = std::string(gdx::BASE_API_URL) + "/getGauntlets";
    auto self = geode::Ref<GDXGauntletLayer>(this);
    async::spawn([self = std::move(self), url = std::move(url)]() -> arc::Future<> {
        auto response = co_await geode::utils::web::WebRequest()
                            .get(url);

        if (response.error() || response.cancelled() || !response.ok()) {
            geode::queueInMainThread([self]() {
                if (self->m_loadingSpinner) {
                    self->m_loadingSpinner->setVisible(false);
                }
            });
            co_return;
        }

        auto jsonResult = response.json();
        if (!jsonResult) {
            geode::queueInMainThread([self]() {
                if (self->m_loadingSpinner) {
                    self->m_loadingSpinner->setVisible(false);
                }
            });
            co_return;
        }

        auto gauntlets = std::move(jsonResult).unwrap();
        geode::queueInMainThread([self, gauntlets = std::move(gauntlets)]() mutable {
            if (self->m_loadingSpinner) {
                self->m_loadingSpinner->setVisible(false);
            }
            self->createGauntletPages(gauntlets);
        });

        co_return;
    });
}

void GDXGauntletLayer::fetchUserData() {
    auto accountData = argon::getGameAccountData();
    auto url = std::string(gdx::BASE_API_URL) + "/getUser";
    matjson::Value body = matjson::Value::object();
    body["accountId"] = accountData.accountId;
    body["argonToken"] = std::string(accountData.gjp2);

    auto self = geode::Ref<GDXGauntletLayer>(this);
    async::spawn([self = std::move(self), url = std::move(url), body = std::move(body), accountData = std::move(accountData)]() mutable -> arc::Future<> {
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

        geode::queueInMainThread([self, userData = std::move(userData)]() mutable {
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

        co_return;
    });
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
    nameLabelShadow->setOpacity(60);
    nameLabelShadow->limitLabelWidth(gauntletBg->getContentSize().width - 30.f, 0.5f, 0.3f);

    gauntletBg->addChild(nameLabel, 3);
    gauntletBg->addChild(nameLabelShadow, 2);

    gauntletBg->setColor({static_cast<GLubyte>(node.r), static_cast<GLubyte>(node.g), static_cast<GLubyte>(node.b)});

    std::string spriteName = fmt::format("GDX_gauntletUnknown.png"_spr, gauntletIndex);
    auto gauntletSprite = CCSprite::createWithSpriteFrameName(spriteName.c_str());
    auto gauntletSpriteShadow = CCSprite::createWithSpriteFrameName(spriteName.c_str());
    gauntletSpriteShadow->setColor({0, 0, 0});
    gauntletSpriteShadow->setOpacity(50);
    gauntletSpriteShadow->setScaleY(1.2f);

    gauntletBg->addChild(gauntletSprite, 3);
    gauntletBg->addChild(gauntletSpriteShadow, 2);
    gauntletSprite->setPosition(gauntletBg->getContentSize() / 2);
    gauntletSpriteShadow->setPosition({gauntletSprite->getPositionX(), gauntletSprite->getPositionY() - 6});

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
    rewardLabelShadow->setOpacity(60);
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
        rewardIconShadow->setOpacity(60);
        rewardIconShadow->setScale(0.3f);
        rewardIconShadow->setAnchorPoint({0.f, 0.5f});
        rewardIconShadow->setPosition({rewardLabel->getPositionX() + 5, 48.f});
        gauntletBg->addChild(rewardIconShadow, 2);
    }

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
    button->setTag(static_cast<int>(index));
    m_gauntletButtons.push_back(button);
    button->m_scaleMultiplier = 1.05f;
    button->setVisible(true);
    return button;
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

    const auto winSize = CCDirector::sharedDirector()->getWinSize();
    const float cardWidth = 110.f;

    auto pages = CCArray::create();
    pages->retain();

    CCLayer* page = nullptr;
    CCMenu* pageMenu = nullptr;
    for (auto i = 0u; i < gauntlets.size(); ++i) {
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
        auto slotIndex = i % 3;
        if (pageMenu) {
            pageMenu->addChild(gauntletButton);
            pageMenu->updateLayout();
        }
    }

    m_scrollLayer = BoomScrollLayer::create(pages, 0, false);
    m_scrollLayer->setPosition({0, -45});
    this->addChild(m_scrollLayer);

    auto pageCount = pages->count();
    if (pageCount > 2) {
        auto navMenu = CCMenu::create();
        navMenu->setPosition({0, 0});
        this->addChild(navMenu, 3);

        auto prevBtnSpr = CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
        prevBtnSpr->setFlipX(true);
        auto prevBtn = CCMenuItemSpriteExtra::create(
            prevBtnSpr,
            this,
            menu_selector(GDXGauntletLayer::onPrev));
        prevBtn->setPosition({winSize.width / 2.f - 100.f, 60.f});
        navMenu->addChild(prevBtn);

        auto nextBtn = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("navArrowBtn_001.png"),
            this,
            menu_selector(GDXGauntletLayer::onNext));
        nextBtn->setPosition({winSize.width / 2.f + 100.f, 60.f});
        navMenu->addChild(nextBtn);
    }

    pages->release();
}

void GDXGauntletLayer::update(float dt) {
    if (!m_scrollLayer) {
        return;
    }

    auto page = m_scrollLayer->m_page;
    if (page != m_currentPage) {
        m_currentPage = page;
        updateDots();
    }
}

void GDXGauntletLayer::onDot(CCObject* sender) {
    auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
    auto it = std::find(m_dots.begin(), m_dots.end(), btn);
    if (it == m_dots.end() || !m_scrollLayer) {
        return;
    }

    auto idx = static_cast<int>(it - m_dots.begin());
    m_scrollLayer->moveToPage(idx);
    m_scrollLayer->updatePages();
    m_scrollLayer->repositionPagesLooped();
    updateDots();
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
    m_scrollLayer->repositionPagesLooped();
    updateDots();
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
    m_scrollLayer->repositionPagesLooped();
    updateDots();
}

void GDXGauntletLayer::updateDots() {
    if (m_dots.empty() || !m_scrollLayer) {
        return;
    }

    auto page = m_scrollLayer->m_page;
    if (page < 0) {
        page = static_cast<int>(m_dots.size()) + page;
    }
    if (page >= static_cast<int>(m_dots.size())) {
        page %= static_cast<int>(m_dots.size());
    }

    auto sfc = CCSpriteFrameCache::sharedSpriteFrameCache();
    for (auto i = 0u; i < m_dots.size(); ++i) {
        auto btn = m_dots[i];
        auto spr = static_cast<CCSprite*>(btn->getNormalImage());
        if (!spr) {
            continue;
        }

        auto frame = (static_cast<int>(i) == page)
                         ? sfc->spriteFrameByName("gj_navDotBtn_on_001.png")
                         : sfc->spriteFrameByName("gj_navDotBtn_off_001.png");
        spr->setDisplayFrame(frame);
    }
}