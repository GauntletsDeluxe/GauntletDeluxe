#include "GDXLeaderboardLayer.hpp"
#include "../include/GDXConstant.hpp"
#include "Geode/ui/BasedButton.hpp"
#include "Geode/ui/General.hpp"
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/ui/Layout.hpp>
#include <Geode/ui/Button.hpp>
#include <Geode/ui/LoadingSpinner.hpp>
#include <Geode/utils/web.hpp>
#include <argon/argon.hpp>
#include <arc/runtime/Runtime.hpp>
#include <cue/ListNode.hpp>
#include <cue/PlayerIcon.hpp>

using namespace geode::prelude;

GDXLeaderboardLayer* GDXLeaderboardLayer::create() {
    auto ret = new GDXLeaderboardLayer();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool GDXLeaderboardLayer::init() {
    if (!CCLayer::init()) {
        return false;
    }

    auto director = CCDirector::sharedDirector();
    auto winSize = director->getWinSize();
    addBackButton(this, BackButtonStyle::Green);
    addSideArt(this, SideArt::All, SideArtStyle::LayerGray, false);

    auto bg = createLayerBG();
    bg->setColor({50, 50, 50});
    this->addChild(bg, -5);

    auto listSize = CCSizeMake(356.f, 220.f);
    m_list = cue::ListNode::create(listSize, {191, 114, 62, 255}, cue::ListBorderStyle::SlimLevels);
    if (m_list) {
        m_list->setPosition({winSize.width / 2.f, winSize.height / 2.f - 5.f});
        m_list->setCellHeight(50.f);
        auto contentLayer = m_list->getScrollLayer()->m_contentLayer;
        if (contentLayer) {
            auto layout = ColumnLayout::create();
            layout->setGap(0.f);
            layout->setAutoGrowAxis(0.f);
            layout->setAxisReverse(true);
            contentLayer->setLayout(layout);
        }
        this->addChild(m_list);

        m_loadingSpinner = LoadingSpinner::create(60.f);
        if (m_loadingSpinner) {
            m_loadingSpinner->setPosition(m_list->getPosition());
            m_loadingSpinner->setVisible(false);
            this->addChild(m_loadingSpinner, 4);
        }
    }

    m_scrollbar = geode::Scrollbar::create(m_list->getScrollLayer());
    m_list->addChildAtPosition(m_scrollbar, Anchor::Right, {25.f, 0.f});
    createTabs();

    this->setKeypadEnabled(true);
    return true;
}

void GDXLeaderboardLayer::createTabs() {
    auto director = CCDirector::sharedDirector();
    auto winSize = director->getWinSize();
    auto tabMenu = CCMenu::create();
    tabMenu->setLayout(RowLayout::create()->setGap(10.f)->setAxisAlignment(AxisAlignment::Center));

    m_levelPointsTabSprite = TabButton::create(TabBaseColor::Unselected, TabBaseColor::UnselectedDark, "Level Points", this, menu_selector(GDXLeaderboardLayer::onTabSelected));
    if (m_levelPointsTabSprite) {
        m_levelPointsTabSprite->setTag(1);
        tabMenu->addChild(m_levelPointsTabSprite);
    }

    m_gauntletPointsTabSprite = TabButton::create(TabBaseColor::Unselected, TabBaseColor::UnselectedDark, "Gauntlet Points", this, menu_selector(GDXLeaderboardLayer::onTabSelected));
    if (m_gauntletPointsTabSprite) {
        m_gauntletPointsTabSprite->setTag(2);
        tabMenu->addChild(m_gauntletPointsTabSprite);
    }

    updateTabSelection();
    tabMenu->setZOrder(-1);
    tabMenu->setContentWidth(m_list->getContentWidth());
    tabMenu->updateLayout();
    m_list->addChildAtPosition(tabMenu, Anchor::Top, {0.f, 10.f}, {0.5, 0.f});
}

void GDXLeaderboardLayer::updateTabSelection() {
    if (m_levelPointsTabSprite) {
        m_levelPointsTabSprite->toggle(m_type == 1 ? true : false);
    }
    if (m_gauntletPointsTabSprite) {
        m_gauntletPointsTabSprite->toggle(m_type == 2 ? true : false);
    }
}

void GDXLeaderboardLayer::onTabSelected(CCObject* sender) {
    auto item = static_cast<CCMenuItemSpriteExtra*>(sender);
    if (!item) {
        return;
    }

    auto selectedType = item->getTag();
    if (selectedType == m_type) {
        return;
    }

    m_type = selectedType;
    updateTabSelection();
    fetchLeaderboard();
}

void GDXLeaderboardLayer::onEnter() {
    CCLayer::onEnter();
    fetchLeaderboard();
}

void GDXLeaderboardLayer::keyBackClicked() {
    CCDirector::sharedDirector()->popSceneWithTransition(0.5f, PopTransition::kPopTransitionFade);
}

void GDXLeaderboardLayer::fetchLeaderboard() {
    if (!m_list) {
        return;
    }

    if (m_loadingSpinner) {
        m_loadingSpinner->setVisible(true);
    }
    m_list->clear();
    auto accountData = argon::getGameAccountData();
    auto url = std::string(gdx::baseApiUrl()) + "/getLeaderboard";
    matjson::Value body = matjson::Value::object();
    body["accountId"] = accountData.accountId;
    body["argonToken"] = "";
    body["type"] = m_type;

    auto self = geode::Ref<GDXLeaderboardLayer>(this);
    m_leaderboardTask.spawn([self = std::move(self), url = std::move(url), body = std::move(body), accountData = std::move(accountData)]() mutable -> arc::Future<> {
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

        auto leaderboard = std::move(jsonResult).unwrap();
        co_await geode::async::waitForMainThread([self = std::move(self), leaderboard = std::move(leaderboard)]() mutable {
            if (!self || !self->m_list) {
                return;
            }

            if (!leaderboard.isArray()) {
                return;
            }

            for (auto i = 0u; i < leaderboard.size(); ++i) {
                auto entry = leaderboard[i];
                if (!entry.isObject()) {
                    continue;
                }

                auto userName = entry["username"].asString().unwrapOr("Unknown");
                int accountId = entry["accountId"].asInt().unwrapOr(0);
                int icon = entry["icon"].asInt().unwrapOr(0);
                int color1 = entry["iconColor1"].asInt().unwrapOr(0);
                int color2 = entry["iconColor2"].asInt().unwrapOr(0);
                int color3 = entry["iconColor3"].asInt().unwrapOr(-1);
                auto levelPoints = entry["levelPoints"].asInt().unwrapOr(0);
                auto gauntletPoints = entry["gauntletPoints"].asInt().unwrapOr(0);

                auto cell = CCLayer::create();
                cell->setContentSize({356.f, 50.f});

                auto rankLabel = CCLabelBMFont::create(numToString(i + 1).c_str(), "goldFont.fnt");
                rankLabel->setAnchorPoint({0.5f, 0.5f});
                rankLabel->setScale(0.6f);
                rankLabel->setPosition({20.f, cell->getContentSize().height / 2.f});
                cell->addChild(rankLabel);

                auto playerIcon = SimplePlayer::create(icon);
                auto gm = GameManager::sharedState();
                playerIcon->updatePlayerFrame(icon, IconType::Cube);
                playerIcon->setColors(gm->colorForIdx(color1), gm->colorForIdx(color2));
                if (color3 >= 0) {
                    playerIcon->setGlowOutline(gm->colorForIdx(color3));
                }
                playerIcon->setPosition({55.f, cell->getContentSize().height / 2.f});
                cell->addChild(playerIcon);

                auto usernameLabel = geode::Button::createWithLabel(userName.c_str(), "goldFont.fnt", [accountId](geode::Button* sender) {
                    ProfilePage::create(accountId, false)->show();
                });
                usernameLabel->setPosition({85.f, cell->getContentSize().height / 2.f});
                usernameLabel->setAnchorPoint({0.f, 0.5f});
                usernameLabel->setScale(0.8f);
                cell->addChild(usernameLabel);

                // level points
                auto levelPointSpr = CCSprite::createWithSpriteFrameName("GDX_levelPoint.png"_spr);
                if (levelPointSpr) {
                    levelPointSpr->setScale(0.2f);
                    levelPointSpr->setPosition({cell->getContentSize().width - 20.f, cell->getContentSize().height / 2.f + 10.f});
                    cell->addChild(levelPointSpr);
                }
                auto levelPointLabel = CCLabelBMFont::create(GameToolbox::pointsToString(levelPoints).c_str(), "bigFont.fnt");
                levelPointLabel->setAnchorPoint({1.f, 0.5f});
                levelPointLabel->setPosition({cell->getContentSize().width - 35.f, cell->getContentSize().height / 2.f + 10.f});
                levelPointLabel->limitLabelWidth(100.f, 0.5f, 0.35f);
                cell->addChild(levelPointLabel);

                // gauntlet points
                auto gauntletPointSpr = CCSprite::createWithSpriteFrameName("GDX_gauntletPoint.png"_spr);
                if (gauntletPointSpr) {
                    gauntletPointSpr->setScale(0.3f);
                    gauntletPointSpr->setPosition({cell->getContentSize().width - 20.f, cell->getContentSize().height / 2.f - 10.f});
                    cell->addChild(gauntletPointSpr);
                }
                auto gauntletPointLabel = CCLabelBMFont::create(GameToolbox::pointsToString(gauntletPoints).c_str(), "bigFont.fnt");
                gauntletPointLabel->setAnchorPoint({1.f, 0.5f});
                gauntletPointLabel->setPosition({cell->getContentSize().width - 35.f, cell->getContentSize().height / 2.f - 10.f});
                gauntletPointLabel->limitLabelWidth(100.f, 0.5f, 0.35f);
                cell->addChild(gauntletPointLabel);

                // top 3 gradient
                if (i == 0) {
                    auto goldGradient = CCLayerGradient::create({255, 215, 0, 255}, {255, 215, 0, 0}, {1.f, 0.f});
                    goldGradient->setContentSize(cell->getContentSize());
                    goldGradient->setPosition({0, 0});
                    cell->addChild(goldGradient, -1);
                } else if (i == 1) {
                    auto silverGradient = CCLayerGradient::create({192, 192, 192, 255}, {192, 192, 192, 0}, {1.f, 0.f});
                    silverGradient->setContentSize(cell->getContentSize());
                    silverGradient->setPosition({0, 0});
                    cell->addChild(silverGradient, -1);
                } else if (i == 2) {
                    auto bronzeGradient = CCLayerGradient::create({205, 127, 50, 255}, {205, 127, 50, 0}, {1.f, 0.f});
                    bronzeGradient->setContentSize(cell->getContentSize());
                    bronzeGradient->setPosition({0, 0});
                    cell->addChild(bronzeGradient, -1);
                }

                // current user
                if (accountId == argon::getGameAccountData().accountId) {
                    auto glow = CCLayerGradient::create({0, 255, 0, 100}, {0, 255, 0, 0}, {-1.f, 0.f});
                    glow->setContentSize(cell->getContentSize());
                    glow->setPosition({0, 0});
                    cell->addChild(glow, -1);
                }

                self->m_list->addCell(cell);
            }

            self->m_list->updateLayout();
            self->m_list->scrollToTop();

            if (self->m_loadingSpinner) {
                self->m_loadingSpinner->setVisible(false);
            }
        });
        co_return;
    }, []() {});
}

