#include "GDXGauntletLayer.hpp"
#include "GDXGauntletLevelsLayer.hpp"
#include "GDXLeaderboardLayer.hpp"
#include "GDXScrollLayer.hpp"
#include "../popup/GDXGauntletManagePopup.hpp"
#include "../popup/GDXLikeItemPopup.hpp"
#include "../popup/GDXShowTagsPopup.hpp"
#include "../popup/GDXTagsFiltersPopup.hpp"
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
#include <Geode/binding/SetTextPopup.hpp>
#include <asp/fs.hpp>
#include "../include/GDXConstant.hpp"
#include "Geode/ui/BasedButtonSprite.hpp"
#include "Geode/ui/Layout.hpp"
#include "Geode/ui/NineSlice.hpp"
#include <dankmeme.argon/argon.hpp>

using namespace geode::prelude;
using namespace gdx;

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

    // empty state label
    m_emptyStateLabel = Label::create("No gauntlets found.", "goldFont.fnt");
    m_emptyStateLabel->setScale(0.6f);
    m_emptyStateLabel->setPosition({winSize.width / 2.f, winSize.height / 2.f});
    m_emptyStateLabel->setVisible(false);
    m_emptyStateLabel->setAlignment(Label::Alignment::Center);
    this->addChild(m_emptyStateLabel, 1);

    // search button
    // @geode-ignore(unknown-resource)
    auto searchSpr = CircleButtonSprite::createWithSpriteFrameName(
        "GDX_searchIcon.png"_spr,
        1.2f,
        m_searchQuery.empty() ? CircleBaseColor::Green : CircleBaseColor::Cyan,
        CircleBaseSize::Small);
    m_searchBtn = Button::createWithNode(searchSpr, [this](Button* sender) { this->onSearchGauntlets(sender); });

    auto tagFilterSpr = CircleButtonSprite::createWithSpriteFrameName("GDX_searchTags.png"_spr, 1.2f, CircleBaseColor::Green, CircleBaseSize::Small);
    m_tagFilterBtn = Button::createWithNode(tagFilterSpr, [this](Button* sender) { this->onFilterByTag(sender); });

    // top left menu
    auto topLeftMenu = CCMenu::create();
    topLeftMenu->addChild(m_searchBtn);
    topLeftMenu->addChild(m_tagFilterBtn);
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

    auto infoIconSpr = CircleButtonSprite::createWithSpriteFrameName("GDX_infoIcon.png"_spr, 1.2f, CircleBaseColor::Green, CircleBaseSize::Small);
    auto infoIconBtn = Button::createWithNode(infoIconSpr, [this](Button* sender) { this->onInfo(sender); });
    bottomLeftMenu->addChild(infoIconBtn);

    // leaderboard button
    auto leaderboardIconSpr = CircleButtonSprite::createWithSpriteFrameName("GDX_leaderboardIcon.png"_spr, 1.2f, CircleBaseColor::Green, CircleBaseSize::Small);
    m_leaderboardBtn = Button::createWithNode(leaderboardIconSpr, [this](Button* sender) { pushSceneWithLayer(GDXLeaderboardLayer::create()); });
    bottomLeftMenu->addChild(m_leaderboardBtn);

    // discord button
    auto discordIconSpr = CircleButtonSprite::createWithSpriteFrameName("GDX_discord.png"_spr, 1.2f, CircleBaseColor::Green, CircleBaseSize::Small);
    m_discordBtn = Button::createWithNode(discordIconSpr, [this](Button* sender) { this->onDiscord(sender); });
    bottomLeftMenu->addChild(m_discordBtn);

    // @geode-ignore(unknown-resource)
    auto manageLabel = CircleButtonSprite::createWithSpriteFrameName(m_localMode ? "GDX_editButton.png"_spr : "GDX_editButton_02.png"_spr, 1.2f, CircleBaseColor::Green, CircleBaseSize::Small);
    auto manageBtn = Button::createWithNode(manageLabel, [this](Button* sender) { this->onManageGauntlets(sender); });
    bottomLeftMenu->addChild(manageBtn);

    bottomLeftMenu->updateLayout();

    // refresh gauntlet
    // @geode-ignore(unknown-resource)
    auto refreshSpr = CircleButtonSprite::createWithSpriteFrameName("GDX_refreshIcon.png"_spr, 1.2f, CircleBaseColor::Green, CircleBaseSize::Small);
    auto refreshBtn = Button::createWithNode(refreshSpr, [this](Button* sender) { this->onRefreshGauntlets(sender); });

    // recent toggle
    m_recentToggle = Button::createWithNode(createRecentToggleSprite(m_recentFilter), [this](Button* sender) { this->onToggleRecent(sender); });
    updateRecentToggleState();

    // favorite toggle
    m_favoriteToggle = Button::createWithNode(createFavoriteToggleSprite(m_favoriteFilter), [this](Button* sender) { this->onToggleFavorite(sender); });
    updateFavoriteToggleState();

    m_localToggle = Button::createWithNode(createLocalToggleSprite(m_localMode), [this](Button* sender) { this->onToggleLocalMode(sender); });
    updateLocalToggleState();
    updateRecentToggleState();
    updateFavoriteToggleState();

    auto bottomRightMenu = CCMenu::create();
    for (auto btn : {refreshBtn, m_localToggle, m_recentToggle, m_favoriteToggle}) bottomRightMenu->addChild(btn);
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
    this->addChildAtPosition(title, Anchor::Top, {0, -25}, false);

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

    if (m_favoriteFilter && !m_localMode && m_appliedFavorites != gdx::loadFavoriteGauntlets()) {
        showOnlineGauntlets();
    }
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
    auto communityUrl = getMod()->getMetadata().getLinks().getCommunityURL();
    if (communityUrl.has_value()) {
        utils::web::openLinkInBrowser(communityUrl.value());
    }
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

    auto upopupRef = Ref<UploadActionPopup>(upopup);
    m_syncAccountTask.spawn([upopupRef = std::move(upopupRef), url = std::move(url), body = std::move(body), accountData = std::move(accountData)]() mutable -> arc::Future<> {
        auto token = co_await gdx::argonToken(accountData);
        if (token.empty()) {
            co_await async::waitForMainThread([upopupRef = std::move(upopupRef)] {
                if (!upopupRef) return;
                if (upopupRef) upopupRef->showFailMessage("Authentication failed.");
            });
            co_return;
        }

        body["argonToken"] = std::move(token);
        auto response = co_await utils::web::WebRequest()
                            .url(url)
                            .header("Content-Type", "application/json")
                            .bodyJSON(body)
                            .post(url);

        if (response.error() || response.cancelled() || !response.ok()) {
            auto errMsg = gdx::getResponseMessage(response, "Failed to get access.");
            co_await async::waitForMainThread([upopupRef = std::move(upopupRef), errMsg = std::move(errMsg)] {
                if (!upopupRef) return;
                if (upopupRef) upopupRef->showFailMessage(errMsg);
            });
            co_return;
        }

        auto jsonResult = response.json();
        if (!jsonResult) {
            co_await async::waitForMainThread([upopupRef = std::move(upopupRef)] {
                if (!upopupRef) return;
                if (upopupRef) upopupRef->showFailMessage("Failed to get access.");
            });
            co_return;
        }

        auto result = std::move(jsonResult).unwrap();
        bool success = result["success"].asBool().unwrapOr(false);
        if (!result.isObject() || !success) {
            co_await async::waitForMainThread([upopupRef = std::move(upopupRef)] {
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
            co_await async::waitForMainThread([upopupRef = std::move(upopupRef), errMsg = std::move(errMsg)] {
                if (!upopupRef) return;
                if (upopupRef) upopupRef->showFailMessage(errMsg);
            });
            co_return;
        }

        if (gdx::isManager() || gdx::isContributor() || gdx::isLeaderboardMod()) {
            co_await async::waitForMainThread([upopupRef = std::move(upopupRef)]() {
                if (!upopupRef) return;
                if (upopupRef) upopupRef->onClose(nullptr);
                GDXGauntletManagePopup::create()->show();
            });
        }

        co_return; }, []() {});
}

void GDXGauntletLayer::onRefreshGauntlets(CCObject* sender) {
    if (m_emptyStateLabel) {
        m_emptyStateLabel->setVisible(false);
    }

    if (m_localMode) {
        m_localGauntlets = loadLocalGauntlets();
        if (!m_localGauntlets.isArray() || m_localGauntlets.size() == 0) {
            if (m_localScrollLayer) {
                m_localScrollLayer->removeFromParent();
                m_localScrollLayer = nullptr;
            }
            m_localGauntletButtons.clear();
            //Notification::create("Local Gauntlet is empty. Please add one.", NotificationIcon::Info)->show();
            updateEmptyStateLabel();
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

void GDXGauntletLayer::onToggleFavorite(CCObject* sender) {
    m_favoriteFilter = !m_favoriteFilter;
    updateFavoriteToggleState();
    if (!m_localMode) {
        showOnlineGauntlets();
    }
}

// builds the online pages from the last response, keeping only favorites while the favorite filter is on
void GDXGauntletLayer::showOnlineGauntlets() {
    if (!m_fetchedGauntlets.isArray()) {
        return;
    }

    m_appliedFavorites = m_favoriteFilter ? gdx::loadFavoriteGauntlets() : std::unordered_set<int>{};
    auto shown = m_favoriteFilter ? filterFavoriteGauntlets(m_fetchedGauntlets, m_appliedFavorites) : m_fetchedGauntlets;

    m_onlineGauntlets = shown;
    createGauntletPages(m_onlineGauntlets, false);
}

// the one place that decides the empty label: it follows the mode being shown, so the other mode can't leave it behind
void GDXGauntletLayer::updateEmptyStateLabel() {
    if (!m_emptyStateLabel) {
        return;
    }

    bool show = false;
    if (m_localMode) {
        show = !m_localGauntlets.isArray() || m_localGauntlets.size() == 0;
        if (show) {
            m_emptyStateLabel->setText("Local Gauntlet is empty.\nPlease add one.");
        }
    } else {
        // not while the request is still out, an empty list then just means nothing has arrived yet
        show = m_onlineLoaded && (!m_onlineGauntlets.isArray() || m_onlineGauntlets.size() == 0);
        if (show) {
            m_emptyStateLabel->setText(m_favoriteFilter ? "You have no favorite gauntlets here." : "No gauntlets were found.");
        }
    }
    m_emptyStateLabel->setVisible(show);
}

void GDXGauntletLayer::onSearchGauntlets(CCObject* sender) {
    auto popup = SetTextPopup::create(
        m_searchQuery,
        "Search query",
        128,
        m_localMode ? "Search local gauntlets" : "Search gauntlets",
        "Search",
        true,
        60.0f,
        false,
        0,
        0);
    if (popup) {
        popup->m_delegate = this;
        popup->show();
    }
}

void GDXGauntletLayer::setTextPopupClosed(SetTextPopup* popup, std::string text) {
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
            //Notification::create("Local Gauntlet is empty. Please add one.", NotificationIcon::Info)->show();
            updateEmptyStateLabel();
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
        [self = Ref<GDXGauntletLayer>(this)](std::string const& tag) {
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
    updateFavoriteToggleState();

    if (m_localMode) {
        m_localGauntlets = loadLocalGauntlets();
        if (!m_localGauntlets.isArray() || m_localGauntlets.size() == 0) {
            if (m_localScrollLayer) {
                m_localScrollLayer->removeFromParent();
                m_localScrollLayer = nullptr;
            }
            m_localGauntletButtons.clear();
            //Notification::create("Local Gauntlet is empty. Please add one.", NotificationIcon::Info)->show();
            updateEmptyStateLabel();
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
    if (m_searchBtn) {
        m_searchBtn->setVisible(true);
    }
    if (m_bottomLeftMenu) {
        m_bottomLeftMenu->updateLayout();
    }
    if (m_bottomRightMenu) {
        m_bottomRightMenu->updateLayout();
    }

    // the label from the other mode must not stay behind
    updateEmptyStateLabel();

    if (m_onlineGauntlets.isArray() && m_onlineGauntlets.size() > 0) {
        updatePageButtons();
    } else {
        m_gauntlets = matjson::Value::array();
        createGauntletPages(m_gauntlets, false);
        fetchGauntlets();
    }
}

void GDXGauntletLayer::onGauntletButtonClick(CCObject* sender) {
    auto button = static_cast<Button*>(sender);
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

    auto color = ccColor3B{
        static_cast<GLubyte>(gauntlet["r"].asInt().unwrapOr(255)),
        static_cast<GLubyte>(gauntlet["g"].asInt().unwrapOr(255)),
        static_cast<GLubyte>(gauntlet["b"].asInt().unwrapOr(255)),
    };
    pushSceneWithLayer(GDXGauntletLevelsLayer::create(levels, gauntlet["name"].asString().unwrapOr("Gauntlet"), color, gauntlet["index"].asInt().unwrapOr(idx), gauntlet, m_localMode));
}

void GDXGauntletLayer::onShowGauntletTags(CCObject* sender) {
    auto button = static_cast<Button*>(sender);
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

    m_onlineLoaded = false;
    updateEmptyStateLabel();

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
    auto self = Ref<GDXGauntletLayer>(this);
    m_fetchGauntletsTask.spawn([self = std::move(self), url = std::move(url)]() mutable -> arc::Future<> {
        auto response = co_await utils::web::WebRequest()
                            .get(url);

        if (response.error() || response.cancelled() || !response.ok()) {
            co_await async::waitForMainThread([self = std::move(self)]() {
                if (self && self->m_loadingSpinner) {
                    self->m_loadingSpinner->setVisible(false);
                }
            });
            co_return;
        }

        auto jsonResult = response.json();
        if (!jsonResult) {
            co_await async::waitForMainThread([self = std::move(self)]() {
                if (self && self->m_loadingSpinner) {
                    self->m_loadingSpinner->setVisible(false);
                }
            });
            co_return;
        }

        auto gauntlets = std::move(jsonResult).unwrap();
        co_await async::waitForMainThread([self = std::move(self), gauntlets = std::move(gauntlets)]() mutable {
            if (self && self->isRunning()) {
                if (self->m_loadingSpinner) {
                    self->m_loadingSpinner->setVisible(false);
                }
                self->m_onlineLoaded = true;
                self->m_fetchedGauntlets = gauntlets;
                self->showOnlineGauntlets();
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

    auto self = Ref<GDXGauntletLayer>(this);
    m_fetchUserDataTask.spawn([self = std::move(self), url = std::move(url), body = std::move(body), accountData = std::move(accountData)]() mutable -> arc::Future<> {
        auto token = co_await gdx::argonToken(accountData);
        if (token.empty()) {
            co_await async::waitForMainThread([self = std::move(self)]() {});
            co_return;
        }

        body["argonToken"] = std::move(token);

        auto response = co_await utils::web::WebRequest()
                            .url(url)
                            .header("Content-Type", "application/json")
                            .bodyJSON(body)
                            .post(url);

        if (response.error() || response.cancelled() || !response.ok()) {
            co_await async::waitForMainThread([self = std::move(self)]() {});
            co_return;
        }

        auto jsonResult = response.json();
        if (!jsonResult) {
            co_await async::waitForMainThread([self = std::move(self)]() {});
            co_return;
        }

        auto userData = std::move(jsonResult).unwrap();
        if (!userData.isObject() || !userData["success"].asBool().unwrapOr(false)) {
            co_await async::waitForMainThread([self = std::move(self)]() {});
            co_return;
        }

        co_await async::waitForMainThread([self = std::move(self), userData = std::move(userData)]() mutable {
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

GDXScrollLayer* GDXGauntletLayer::getActiveScrollLayer() const {
    return m_localMode ? m_localScrollLayer : m_scrollLayer;
}

const matjson::Value& GDXGauntletLayer::getActiveGauntlets() const {
    return m_localMode ? m_localGauntlets : m_gauntlets;
}

Button* GDXGauntletLayer::createGauntletButton(const matjson::Value& gauntlet, std::size_t index, bool local) {
    auto node = GDXGauntletNode::fromJson(gauntlet);
    int gauntletIndex = node.id;
    std::string gauntletName = node.name;

    auto gauntletBg = NineSlice::createWithSpriteFrameName("GDX_gauntletSquare_01.png"_spr);
    if (!gauntletBg) {
        return nullptr;
    }
    gauntletBg->setContentSize({110, 240});

    if (gauntlet["isFeatured"].asBool().unwrapOr(false)) {
        auto glow = NineSlice::createWithSpriteFrameName("GDX_featuredGlow.png"_spr);
        if (glow) {
            glow->setPosition({gauntletBg->getContentSize().width / 2.f, gauntletBg->getContentSize().height / 2.f});
            glow->setContentSize(gauntletBg->getContentSize() + CCSize{15.f, 15.f});
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
                for (int i = 0; i < 5; ++i) {
                    particle->update(0.15f);
                }
            }
        }
    }

    std::string formattedName = gauntletName;
    std::replace(formattedName.begin(), formattedName.end(), ' ', '\n');

    auto nameLabel = Label::create(formattedName, "bigFont.fnt");
    nameLabel->setAlignment(Label::Alignment::Center);
    nameLabel->setPosition({gauntletBg->getContentSize().width / 2, gauntletBg->getContentSize().height - 15});
    nameLabel->setAnchorPoint({0.5f, 1.0f});
    nameLabel->setLimitLabelWidth(gauntletBg->getContentSize().width - 30.f, 0.5f, 0.3f);

    auto nameLabelShadow = Label::create(formattedName, "bigFont.fnt");
    nameLabelShadow->setAlignment(Label::Alignment::Center);
    nameLabelShadow->setPosition({nameLabel->getPositionX() + 2, nameLabel->getPositionY() - 2});
    nameLabelShadow->setAnchorPoint({0.5f, 1.0f});
    nameLabelShadow->setColor({0, 0, 0});
    nameLabelShadow->setOpacity(50);
    nameLabelShadow->setLimitLabelWidth(gauntletBg->getContentSize().width - 30.f, 0.5f, 0.3f);

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
            asp::fs::Path path(spritePath);
            if (path.isFile()) {
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
    Button* rewardBtn = nullptr;

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
            rewardBtn = Button::createWithNode(rewardBtnSpr, [this](Button* sender) { this->onCompleteGauntlet(sender); });
            rewardBtn->setTag(gauntletIndex);
            // nested inside the gauntlet button: keep swallowing so the outer button can't steal the activation
            rewardBtn->setDragCancelDistance(5.f);
            rewardBtn->setPosition({gauntletBg->getContentSize().width / 2.f, 50.f});
            auto rewardMenu = CCMenu::create();
            rewardMenu->addChild(rewardBtn);
            if (rewardMenu) {
                rewardMenu->setPosition({0.f, 0.f});
                gauntletBg->addChild(rewardMenu, 3);
            }
        }
    } else if (!local) {
        auto rewardLabel = Label::create(numToString(node.reward), "bigFont.fnt");
        rewardLabel->setAlignment(Label::Alignment::Left);
        rewardLabel->setAnchorPoint({1.f, 0.5f});
        rewardLabel->setLimitLabelWidth(50.f, 0.5f, 0.35f);
        rewardLabel->setPosition({gauntletBg->getContentSize().width / 2.f, 50.f});
        gauntletBg->addChild(rewardLabel, 3);

        auto rewardLabelShadow = Label::create(numToString(node.reward), "bigFont.fnt");
        rewardLabelShadow->setAlignment(Label::Alignment::Left);
        rewardLabelShadow->setAnchorPoint({1.f, 0.5f});
        rewardLabelShadow->setLimitLabelWidth(50.f, 0.5f, 0.35f);
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

        auto rewardTextLabel = Label::create("Reward", "goldFont.fnt");
        rewardTextLabel->setAlignment(Label::Alignment::Center);
        rewardTextLabel->setAnchorPoint({0.5f, 0.5f});
        rewardTextLabel->setPosition({gauntletBg->getContentSize().width / 2.f, 70.f});
        rewardTextLabel->setScale(0.4f);
        gauntletBg->addChild(rewardTextLabel, 3);

        auto rewardTextLabelShadow = Label::create("Reward", "goldFont.fnt");
        rewardTextLabelShadow->setAlignment(Label::Alignment::Center);
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

            auto tagLabel = Label::create(firstTagName, "bigFont.fnt");
            if (tagLabel) {
                tagLabel->setColor({255, 255, 255});
                tagLabel->setLimitLabelWidth(90.f, 0.5f, 0.35f);
                tagCell->setContentSize({tagLabel->getScaledContentSize().width + 10.f, tagLabel->getScaledContentSize().height + 10.f});
                tagCell->addChildAtPosition(tagLabel, Anchor::Center, {0.f, 0.f}, false);
            }

            Label* extraLabel = nullptr;
            int extraCount = static_cast<int>(tagsArray.size()) - 1;
            if (extraCount > 0) {
                extraLabel = Label::create(fmt::format("+{}", extraCount), "goldFont.fnt");
                if (extraLabel) {
                    extraLabel->setScale(0.7f);
                    extraLabel->setColor({255, 255, 255});
                    extraLabel->setAnchorPoint({0.f, 0.5f});
                    tagCell->addChildAtPosition(extraLabel, Anchor::Right, {2.f, 0.f}, false);
                }
            }

            auto tagButton = Button::createWithNode(tagCell, [this](Button* sender) { this->onShowGauntletTags(sender); });
            if (tagButton) {
                tagButton->setTag(static_cast<int>(index));
                // nested inside the gauntlet button: keep swallowing so the outer button can't steal the activation
                tagButton->setDragCancelDistance(5.f);
                tagButton->setPosition({gauntletBg->getContentSize().width / 2.f, 18.f});
                auto tagMenu = CCMenu::create();
                tagMenu->addChild(tagButton);
                if (tagMenu) {
                    tagMenu->setPosition({0.f, 0.f});
                    gauntletBg->addChild(tagMenu, 4);
                }
            }
        }
    }

    auto completionLabelShadow = Label::create(fmt::format("{}/{}", completedCount, node.levelIds.size()), "bigFont.fnt");
    completionLabelShadow->setScale(0.4f);
    completionLabelShadow->setAnchorPoint({0.5f, 0.5f});
    completionLabelShadow->setPosition({imageCenter.x + 2.f, imageCenter.y - 42.f});
    completionLabelShadow->setColor({0, 0, 0});
    completionLabelShadow->setOpacity(50);
    gauntletBg->addChild(completionLabelShadow, 2);

    auto completionLabel = Label::create(fmt::format("{}/{}", completedCount, node.levelIds.size()), "bigFont.fnt");
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

        auto feedbackRow = CCNode::create();
        feedbackRow->setContentSize({100.f, 12.f});
        feedbackRow->setAnchorPoint({0.5f, 0.5f});
        feedbackRow->setPosition({imageCenter.x, imageCenter.y - 51.f});
        feedbackRow->setLayout(
            RowLayout::create()
                ->setGap(2.f)
                ->setAutoScale(false)
                ->setAxisAlignment(AxisAlignment::Center));
        gauntletBg->addChild(feedbackRow, 3);

        auto feedbackIcon = CCSprite::createWithSpriteFrameName(iconName);
        if (feedbackIcon) {
            feedbackIcon->setScale(0.4f);
            feedbackRow->addChild(feedbackIcon);
        }

        auto feedbackLabel = Label::create(numToString(displayValue), "bigFont.fnt");
        if (feedbackLabel) {
            feedbackLabel->setLimitLabelWidth(80.f, 0.35f, 0.25f);
            feedbackRow->addChild(feedbackLabel);
        }

        feedbackRow->updateLayout();
    }

    auto button = Button::createWithNode(gauntletBg, [this](Button* sender) { this->onGauntletButtonClick(sender); });
    if (!button) {
        return nullptr;
    }
    button->setTag(static_cast<int>(index));
    m_gauntletButtons.push_back(button);
    button->setScaleMultiplier(1.05f);
    // lives inside the page scroller
    button->setSwallowTouches(false);
    button->setDragCancelDistance(5.f);
    button->setVisible(true);
    return button;
}

void GDXGauntletLayer::onCompleteGauntlet(CCObject* sender) {
    auto button = static_cast<Button*>(sender);
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

    auto self = Ref<GDXGauntletLayer>(this);
    m_completeGauntletTask.spawn([self = std::move(self), button, buttonPos, rewardSpinner, url = std::move(url), body = std::move(body), accountData = std::move(accountData), gauntletIndex]() mutable -> arc::Future<> {
        auto token = co_await gdx::argonToken(accountData);
        if (token.empty()) {
            co_await async::waitForMainThread([self = std::move(self), button, rewardSpinner]() {
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
        auto response = co_await utils::web::WebRequest()
                            .url(url)
                            .header("Content-Type", "application/json")
                            .bodyJSON(body)
                            .post(url);

        if (response.error() || response.cancelled() || !response.ok()) {
            auto errMsg = gdx::getResponseMessage(response, "Failed to claim gauntlet");
            co_await async::waitForMainThread([self = std::move(self), button, rewardSpinner, errMsg = std::move(errMsg)]() {
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
            co_await async::waitForMainThread([self = std::move(self), button, rewardSpinner, errMsg = std::move(errMsg)]() {
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

        co_await async::waitForMainThread([self = std::move(self), button, buttonPos, rewardSpinner, success, newGauntletPoints, gauntletIndex, errMsg = std::move(errMsg)]() mutable {
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

    updateEmptyStateLabel();

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
            pageMenu->setLayout(RowLayout::create()->setGap(10.f)->setAxisAlignment(AxisAlignment::Center));
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

    targetScroll = GDXScrollLayer::create(pages);
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
            m_prevPageBtn = Button::createWithNode(prevSpr, [this](Button* sender) { this->onPrev(sender); });
            if (m_prevPageBtn) {
                m_prevPageBtn->setPosition({25.f, winSize.height / 2.f});
            }
        }

        if (!m_nextPageBtn) {
            auto nextSpr = CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
            m_nextPageBtn = Button::createWithNode(nextSpr, [this](Button* sender) { this->onNext(sender); });
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
            m_prevPageBtn = Button::createWithNode(prevSpr, [this](Button* sender) { this->onPrev(sender); });
            if (m_prevPageBtn) {
                m_prevPageBtn->setPosition({25.f, winSize.height / 2.f});
            }
        }

        if (!m_nextPageBtn) {
            auto nextSpr = CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
            m_nextPageBtn = Button::createWithNode(nextSpr, [this](Button* sender) { this->onNext(sender); });
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

    auto page = layer->pageNumberForPosition();
    if (page < 0 || page >= layer->getTotalPages()) {
        page = layer->getCurrentPage();
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

    auto page = layer->getCurrentPage();
    page = (page - 1 + total) % total;

    layer->moveToPage(page);
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

    auto page = layer->getCurrentPage();
    page = (page + 1) % total;

    layer->moveToPage(page);
    updatePageButtons();
}

void GDXGauntletLayer::updatePageButtons() {
    auto layer = getActiveScrollLayer();
    if (!layer) {
        return;
    }

    auto page = layer->pageNumberForPosition();
    if (page < 0 || page >= layer->getTotalPages()) {
        page = layer->getCurrentPage();
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
            m_localToggle->setDisplayNode(sprite);
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
            1.2f,
            m_searchQuery.empty() ? CircleBaseColor::Green : CircleBaseColor::Cyan,
            CircleBaseSize::Small);
        if (sprite) {
            m_searchBtn->setDisplayNode(sprite);
        }
    }

    if (m_tagFilterBtn) {
        auto tagSprite = CircleButtonSprite::createWithSpriteFrameName(
            "GDX_searchTags.png"_spr,
            1.2f,
            m_tagFilter.empty() ? CircleBaseColor::Green : CircleBaseColor::Cyan,
            CircleBaseSize::Small);
        if (tagSprite) {
            m_tagFilterBtn->setDisplayNode(tagSprite);
        }
    }
}

void GDXGauntletLayer::updateFavoriteToggleState() {
    if (m_favoriteToggle) {
        auto sprite = createFavoriteToggleSprite(m_favoriteFilter);
        if (sprite) {
            m_favoriteToggle->setDisplayNode(sprite);
        }
        // favorites only exist for online gauntlets
        m_favoriteToggle->setVisible(!m_localMode);
    }
    if (m_bottomRightMenu) {
        m_bottomRightMenu->updateLayout();
    }
}

void GDXGauntletLayer::updateRecentToggleState() {
    if (m_recentToggle) {
        auto sprite = createRecentToggleSprite(m_recentFilter);
        if (sprite) {
            m_recentToggle->setDisplayNode(sprite);
        }
        m_recentToggle->setVisible(!m_localMode);
    }
}
