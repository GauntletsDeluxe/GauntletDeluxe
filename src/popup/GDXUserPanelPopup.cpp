#include <Geode/Geode.hpp>
#include "GDXUserPanelPopup.hpp"
#include "../include/GDXConstant.hpp"
#include "Geode/ui/General.hpp"
#include "Geode/ui/TextInput.hpp"
#include "Geode/ui/Button.hpp"
#include "Geode/utils/general.hpp"
#include <Geode/binding/UploadActionPopup.hpp>
#include <argon/argon.hpp>

using namespace geode::prelude;

constexpr float kLeftColumnWidth = 160.f;
constexpr float kRightColumnX = 350.f;
constexpr float kListWidth = 310.f;
constexpr float kListHeight = 160.f;
constexpr float kCellHeight = 28.f;

GDXUserPanelPopup* GDXUserPanelPopup::create() {
    auto ret = new GDXUserPanelPopup();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool GDXUserPanelPopup::init() {
    if (!Popup::init(530.f, 280.f, "GJ_square02.png")) {
        return false;
    }

    setTitle("Gauntlet Deluxe User Management");
    addSideArt(m_mainLayer, SideArt::All, SideArtStyle::PopupGold, false);

    // left side: the looked up user and the actions available for them
    m_usernameLabel = CCLabelBMFont::create("-", "bigFont.fnt");
    m_usernameLabel->setScale(0.3f);
    m_mainLayer->addChildAtPosition(m_usernameLabel, Anchor::BottomLeft, {kLeftColumnWidth / 2.f + 15.f, 20.f});

    m_usernameBackground = NineSlice::create("square02_small.png");
    m_usernameBackground->setContentSize(m_usernameLabel->getContentSize() + CCSizeMake(10.f, 10.f));
    m_usernameBackground->setOpacity(100);
    m_usernameBackground->setScale(m_usernameLabel->getScale());
    m_mainLayer->addChildAtPosition(m_usernameBackground, Anchor::BottomLeft, {kLeftColumnWidth / 2.f + 15.f, 20.f});

    m_loadingSpinner = LoadingSpinner::create(40.f);
    m_loadingSpinner->setVisible(false);
    m_mainLayer->addChildAtPosition(m_loadingSpinner, Anchor::Left, {kLeftColumnWidth / 2.f + 15.f, -10.f});

    // management button menu
    m_manageMenu = CCMenu::create();
    m_manageMenu->setLayout(ColumnLayout::create()
            ->setGap(6.f)
            ->setAxisReverse(true)
            ->setAxisAlignment(AxisAlignment::Center)
            ->setCrossAxisOverflow(false));
    m_manageMenu->setContentSize({kLeftColumnWidth, 190.f});
    m_manageMenu->setZOrder(2);
    m_mainLayer->addChildAtPosition(m_manageMenu, Anchor::Left, {kLeftColumnWidth / 2.f + 15.f, -10.f}, false);

    m_manageMenuBg = NineSlice::create("square02_small.png");
    m_manageMenuBg->setContentSize(m_manageMenu->getContentSize() + CCSizeMake(10.f, 10.f));
    m_manageMenuBg->setOpacity(100);
    m_manageMenuBg->setZOrder(1);
    m_mainLayer->addChildAtPosition(m_manageMenuBg, Anchor::Left, {kLeftColumnWidth / 2.f + 15.f, -10.f}, false);

    // right side: account id lookup and the list of every user
    m_accountIdInput = TextInput::create(230.f, "Account ID", "chatFont.fnt");
    m_accountIdInput->setLabel("Target Account ID");
    m_accountIdInput->setCommonFilter(CommonFilter::Int);
    m_mainLayer->addChildAtPosition(m_accountIdInput, Anchor::BottomLeft, {kRightColumnX - 25.f, 220.f});

    // check button on the provided accountId
    auto findBtn = geode::Button::createWithSpriteFrameName("gj_findBtn_001.png");
    findBtn->setActivateCallback([this](CCObject* sender) {
        GDXUserPanelPopup::onFindAccountID(sender);
    });
    m_accountIdInput->addChildAtPosition(findBtn, Anchor::Right, {25.f, 0.f}, false);

    m_userList = cue::ListNode::create({kListWidth, kListHeight}, {191, 114, 62, 255}, cue::ListBorderStyle::CommentsBlue);
    m_userList->setCellHeight(kCellHeight);
    if (auto contentLayer = m_userList->getScrollLayer()->m_contentLayer) {
        auto layout = ColumnLayout::create();
        layout->setGap(0.f);
        layout->setAutoGrowAxis(0.f);
        layout->setAxisReverse(true);
        contentLayer->setLayout(layout);
    }
    m_mainLayer->addChildAtPosition(m_userList, Anchor::BottomLeft, {kRightColumnX, 110.f});

    m_userScrollbar = geode::Scrollbar::create(m_userList->getScrollLayer());
    m_userList->addChildAtPosition(m_userScrollbar, Anchor::Right, {10.f, 0.f});

    m_listSpinner = LoadingSpinner::create(40.f);
    m_mainLayer->addChildAtPosition(m_listSpinner, Anchor::BottomLeft, {kRightColumnX, 125.f});

    // below the list: previous page, the totals, next page
    m_pageInfoLabel = CCLabelBMFont::create("Loading users...", "goldFont.fnt");
    m_pageInfoLabel->setScale(0.35f);
    m_mainLayer->addChildAtPosition(m_pageInfoLabel, Anchor::BottomLeft, {kRightColumnX, 15.f});

    auto prevSpr = CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
    prevSpr->setFlipX(true);
    prevSpr->setScale(0.4f);
    m_prevPageBtn = geode::Button::createWithNode(prevSpr, [this](geode::Button*) {
        if (m_page > 1) fetchUsers(m_page - 1);
    });
    m_mainLayer->addChildAtPosition(m_prevPageBtn, Anchor::BottomLeft, {kRightColumnX - 135.f, 18.f});

    auto nextSpr = CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
    nextSpr->setScale(0.4f);
    m_nextPageBtn = geode::Button::createWithNode(nextSpr, [this](geode::Button*) {
        if (m_page < m_totalPages) fetchUsers(m_page + 1);
    });
    m_mainLayer->addChildAtPosition(m_nextPageBtn, Anchor::BottomLeft, {kRightColumnX + 135.f, 18.f});

    m_manageMenu->updateLayout();
    updatePageInfo();
    fetchUsers(1);

    return true;
};

void GDXUserPanelPopup::updateExcludeButton() {
    if (m_excludeBtn) {
        auto offSprite = ButtonSprite::create("Exclude User", 130.f, 80.f, 1.f, true, "goldFont.fnt", "GJ_button_06.png");
        auto onSprite = ButtonSprite::create("Include User", 130.f, 80.f, 1.f, true, "goldFont.fnt", "GJ_button_02.png");
        if (m_excludeBtn->m_offButton) {
            m_excludeBtn->m_offButton->setSprite(offSprite);
            m_excludeBtn->m_offButton->updateSprite();
        }
        if (m_excludeBtn->m_onButton) {
            m_excludeBtn->m_onButton->setSprite(onSprite);
            m_excludeBtn->m_onButton->updateSprite();
        }
        m_excludeBtn->toggle(m_isExcluded);
        return;
    }

    // create toggle with off/on nodes
    auto offNode = ButtonSprite::create("Exclude User", 130.f, 80.f, 1.f, true, "goldFont.fnt", "GJ_button_06.png");
    auto onNode = ButtonSprite::create("Include User", 130.f, 80.f, 1.f, true, "goldFont.fnt", "GJ_button_02.png");
    m_excludeBtn = CCMenuItemToggler::create(offNode, onNode, this, menu_selector(GDXUserPanelPopup::onExclude));
    m_excludeBtn->setVisible(false);
    m_excludeBtn->toggle(m_isExcluded);
    m_manageMenu->addChild(m_excludeBtn);
}

void GDXUserPanelPopup::updatePromoteButton() {
    if (m_promoteBtn) {
        auto offSprite = ButtonSprite::create("Promote to Contributor", 130.f, 80.f, 1.f, true, "goldFont.fnt", "GJ_button_05.png");
        auto onSprite = ButtonSprite::create("Demote from Contributor", 130.f, 80.f, 1.f, true, "goldFont.fnt", "GJ_button_06.png");
        if (m_promoteBtn->m_offButton) {
            m_promoteBtn->m_offButton->setSprite(offSprite);
            m_promoteBtn->m_offButton->updateSprite();
        }
        if (m_promoteBtn->m_onButton) {
            m_promoteBtn->m_onButton->setSprite(onSprite);
            m_promoteBtn->m_onButton->updateSprite();
        }
        m_promoteBtn->toggle(m_isContributor);
        return;
    }

    auto offNode = ButtonSprite::create("Promote to Contributor", 130.f, 80.f, 1.f, true, "goldFont.fnt", "GJ_button_05.png");
    auto onNode = ButtonSprite::create("Demote from Contributor", 130.f, 80.f, 1.f, true, "goldFont.fnt", "GJ_button_06.png");
    m_promoteBtn = CCMenuItemToggler::create(offNode, onNode, this, menu_selector(GDXUserPanelPopup::onPromote));
    m_promoteBtn->setVisible(false);
    m_promoteBtn->toggle(m_isContributor);
    m_manageMenu->addChild(m_promoteBtn);
}

void GDXUserPanelPopup::updatePromoteModButton() {
    if (m_promoteModBtn) {
        auto offSprite = ButtonSprite::create("Promote to LB Mod", 130.f, 80.f, 1.f, true, "goldFont.fnt", "GJ_button_05.png");
        auto onSprite = ButtonSprite::create("Demote from LB Mod", 130.f, 80.f, 1.f, true, "goldFont.fnt", "GJ_button_06.png");
        if (m_promoteModBtn->m_offButton) {
            m_promoteModBtn->m_offButton->setSprite(offSprite);
            m_promoteModBtn->m_offButton->updateSprite();
        }
        if (m_promoteModBtn->m_onButton) {
            m_promoteModBtn->m_onButton->setSprite(onSprite);
            m_promoteModBtn->m_onButton->updateSprite();
        }
        m_promoteModBtn->toggle(m_isLeaderboardMod);
        return;
    }

    auto offNode = ButtonSprite::create("Promote to LB Mod", 130.f, 80.f, 1.f, true, "goldFont.fnt", "GJ_button_05.png");
    auto onNode = ButtonSprite::create("Demote from LB Mod", 130.f, 80.f, 1.f, true, "goldFont.fnt", "GJ_button_06.png");
    m_promoteModBtn = CCMenuItemToggler::create(offNode, onNode, this, menu_selector(GDXUserPanelPopup::onPromoteMod));
    m_promoteModBtn->setVisible(false);
    m_promoteModBtn->toggle(m_isLeaderboardMod);
    m_manageMenu->addChild(m_promoteModBtn);
}

void GDXUserPanelPopup::onExclude(CCObject* sender) {
    auto accountIdStr = m_accountIdInput->getString();
    if (accountIdStr.empty()) {
        Notification::create("Please enter an account ID")->show();
        return;
    }

    if (m_loadingSpinner) {
        m_loadingSpinner->setVisible(true);
    }

    auto upopup = UploadActionPopup::create(nullptr, "Updating user...");
    upopup->show();

    auto accountData = argon::getGameAccountData();
    auto url = std::string(gdx::baseApiUrl()) + "/setUser";
    matjson::Value body = matjson::Value::object();
    body["accountId"] = accountData.accountId;
    body["targetAccountId"] = numFromString<int>(accountIdStr).unwrapOr(0);
    body["isExcluded"] = !m_isExcluded;  // Toggle the state
    body["argonToken"] = "";             // Will be set later after fetching the token

    auto self = geode::Ref<GDXUserPanelPopup>(this);
    auto upopupRef = geode::Ref<UploadActionPopup>(upopup);
    m_excludeTask.spawn([upopupRef = std::move(upopupRef), self = std::move(self), accountIdStr = std::move(accountIdStr), url = std::move(url), body = std::move(body), accountData = std::move(accountData)]() mutable -> arc::Future<> {
        auto token = co_await gdx::argonToken(accountData);
        if (token.empty()) {
            co_await geode::async::waitForMainThread([upopupRef = std::move(upopupRef), self = std::move(self)]() {
                if (!upopupRef) return;
                if (upopupRef) upopupRef->showFailMessage("Failed to get access.");
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
            auto errMsg = gdx::getResponseMessage(response, "Failed to update exclusion status");
            co_await geode::async::waitForMainThread([upopupRef = std::move(upopupRef), self = std::move(self), errMsg = std::move(errMsg)]() {
                if (!upopupRef) return;
                if (upopupRef) upopupRef->showFailMessage(errMsg);
            });
            co_return;
        }

        auto jsonResult = response.json();
        if (!jsonResult) {
            auto errMsg = gdx::getResponseMessage(response, "Invalid response");
            co_await geode::async::waitForMainThread([upopupRef = std::move(upopupRef), self = std::move(self), errMsg = std::move(errMsg)]() {
                if (!upopupRef) return;
                if (upopupRef) upopupRef->showFailMessage(errMsg);
            });
            co_return;
        }

        auto data = std::move(jsonResult).unwrap();
        if (!data.isObject() || !data["success"].asBool().unwrapOr(false)) {
            auto errMsg = gdx::getResponseMessage(response, "Failed to update user");
            co_await geode::async::waitForMainThread([upopupRef = std::move(upopupRef), self = std::move(self), errMsg = std::move(errMsg)]() {
                if (!upopupRef) return;
                if (upopupRef) upopupRef->showFailMessage(errMsg);
            });
            co_return;
        }

        co_await geode::async::waitForMainThread([upopupRef = std::move(upopupRef), self = std::move(self)]() {
            if (self) {
                self->m_isExcluded = !self->m_isExcluded;
                self->updateExcludeButton();
                if (self->m_loadingSpinner) {
                    self->m_loadingSpinner->setVisible(false);
                }
                self->fetchUsers(self->m_page);
            }
            if (!upopupRef) return;
            if (upopupRef) {
                std::string message = (self && self->m_isExcluded) ? "User excluded" : "User included";
                upopupRef->showSuccessMessage(message);
            }
        });

        co_return; }, []() {});
}

void GDXUserPanelPopup::onFindAccountID(CCObject* sender) {
    auto accountIdStr = m_accountIdInput->getString();

    if (accountIdStr.empty()) {
        Notification::create("Please enter an account ID", NotificationIcon::Warning)->show();
        return;
    }

    if (m_manageMenu) m_manageMenu->setVisible(false);

    if (m_loadingSpinner != nullptr) {
        m_loadingSpinner->setVisible(true);
    }

    auto accountData = argon::getGameAccountData();
    auto url = std::string(gdx::baseApiUrl()) + "/getUser";
    matjson::Value body = matjson::Value::object();
    body["accountId"] = accountData.accountId;
    body["targetAccountId"] = numFromString<int>(accountIdStr).unwrapOr(0);
    body["argonToken"] = "";  // Will be set later after fetching the token

    auto self = geode::Ref<GDXUserPanelPopup>(this);
    m_findAccountTask.spawn([self = std::move(self), accountIdStr = std::move(accountIdStr), url = std::move(url), body = std::move(body), accountData = std::move(accountData)]() mutable -> arc::Future<> {
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
            auto errMsg = gdx::getResponseMessage(response, "Request failed");
            co_await geode::async::waitForMainThread([self = std::move(self), errMsg = std::move(errMsg)]() {
                Notification::create(errMsg, NotificationIcon::Error)->show();
            });
            co_return;
        }

        auto jsonResult = response.json();
        if (!jsonResult) {
            auto errMsg = gdx::getResponseMessage(response, "Invalid response");
            co_await geode::async::waitForMainThread([self = std::move(self), errMsg = std::move(errMsg)]() {
                Notification::create(errMsg, NotificationIcon::Error)->show();
            });
            co_return;
        }

        auto userData = std::move(jsonResult).unwrap();
        if (!userData.isObject() || !userData["success"].asBool().unwrapOr(false)) {
            auto errMsg = gdx::getResponseMessage(response, "User not found");
            co_await geode::async::waitForMainThread([self = std::move(self), errMsg = std::move(errMsg)]() {
                Notification::create(errMsg, NotificationIcon::Warning)->show();
            });
            co_return;
        }

        bool isExcluded = userData["isExcluded"].asBool().unwrapOr(false);
        std::string username = userData["username"].asString().unwrapOr("Unknown");
        bool isContributor = userData["isContributor"].asBool().unwrapOr(false);
        bool isLeaderboardMod = userData["isLeaderboardMod"].asBool().unwrapOr(false);

        co_await geode::async::waitForMainThread([self = std::move(self), isExcluded, username = std::move(username), isContributor, isLeaderboardMod]() {
            if (self) {
                self->m_isExcluded = isExcluded;
                self->updateExcludeButton();
                if (self->m_excludeBtn) {
                    self->m_excludeBtn->setVisible(true);
                }
                self->m_isContributor = isContributor;
                self->updatePromoteButton();
                if (self->m_promoteBtn) {
                    self->m_promoteBtn->setVisible(gdx::isManager());
                }
                self->m_isLeaderboardMod = isLeaderboardMod;
                self->updatePromoteModButton();
                if (self->m_promoteModBtn) {
                    self->m_promoteModBtn->setVisible(gdx::isManager());
                }
                if (self->m_manageMenu) {
                    self->m_manageMenu->updateLayout();
                    self->m_manageMenu->setVisible(true);
                }
                if (self->m_usernameLabel) {
                    self->m_usernameLabel->setString(username.c_str());
                    if (self->m_usernameBackground) {
                        self->m_usernameBackground->setContentSize(self->m_usernameLabel->getContentSize() + CCSizeMake(10.f, 10.f));
                    }
                }
                if (self->m_loadingSpinner) {
                    self->m_loadingSpinner->setVisible(false);
                }
            }
        });

        co_return; }, []() {});
}

void GDXUserPanelPopup::onPromote(CCObject* sender) {
    auto accountIdStr = m_accountIdInput->getString();
    if (accountIdStr.empty()) {
        Notification::create("Please enter an account ID")->show();
        return;
    }

    if (m_loadingSpinner) {
        m_loadingSpinner->setVisible(true);
    }

    auto upopup = UploadActionPopup::create(nullptr, "Updating user...");
    upopup->show();

    auto accountData = argon::getGameAccountData();
    auto url = std::string(gdx::baseApiUrl()) + "/setUser";
    matjson::Value body = matjson::Value::object();
    body["accountId"] = accountData.accountId;
    body["targetAccountId"] = numFromString<int>(accountIdStr).unwrapOr(0);
    body["isContributor"] = !m_isContributor;  // toggle
    body["argonToken"] = "";                   // Will be set later after fetching the token

    auto self = geode::Ref<GDXUserPanelPopup>(this);
    auto upopupRef = geode::Ref<UploadActionPopup>(upopup);
    m_promoteTask.spawn([upopupRef = std::move(upopupRef), self = std::move(self), accountIdStr = std::move(accountIdStr), url = std::move(url), body = std::move(body), accountData = std::move(accountData)]() mutable -> arc::Future<> {
        auto token = co_await gdx::argonToken(accountData);
        if (token.empty()) {
            co_await geode::async::waitForMainThread([upopupRef = std::move(upopupRef), self = std::move(self)]() {
                if (!upopupRef) return;
                if (upopupRef) upopupRef->showFailMessage("Failed to get access.");
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
            auto errMsg = gdx::getResponseMessage(response, "Failed to update contributor status");
            co_await geode::async::waitForMainThread([upopupRef = std::move(upopupRef), self = std::move(self), errMsg = std::move(errMsg)]() {
                if (!upopupRef) return;
                if (upopupRef) upopupRef->showFailMessage(errMsg);
            });
            co_return;
        }

        auto jsonResult = response.json();
        if (!jsonResult) {
            co_await geode::async::waitForMainThread([upopupRef = std::move(upopupRef), self = std::move(self)]() {
                if (!upopupRef) return;
                if (upopupRef) upopupRef->showFailMessage("Invalid response");
            });
            co_return;
        }

        auto data = std::move(jsonResult).unwrap();
        if (!data.isObject() || !data["success"].asBool().unwrapOr(false)) {
            auto errMsg = gdx::getResponseMessage(response, "Failed to update user");
            co_await geode::async::waitForMainThread([upopupRef = std::move(upopupRef), self = std::move(self), errMsg = std::move(errMsg)]() {
                if (!upopupRef) return;
                if (upopupRef) upopupRef->showFailMessage(errMsg);
            });
            co_return;
        }

        co_await geode::async::waitForMainThread([upopupRef = std::move(upopupRef), self = std::move(self)]() {
            if (self) {
                self->m_isContributor = !self->m_isContributor;
                self->updatePromoteButton();
                if (self->m_loadingSpinner) {
                    self->m_loadingSpinner->setVisible(false);
                }
                self->fetchUsers(self->m_page);
            }
            if (!upopupRef) return;
            if (upopupRef) {
                std::string message = (self && self->m_isContributor) ? "User promoted to contributor" : "User demoted from contributor";
                upopupRef->showSuccessMessage(message);
            }
        });

        co_return; }, []() {});
}

void GDXUserPanelPopup::onPromoteMod(CCObject* sender) {
    auto accountIdStr = m_accountIdInput->getString();
    if (accountIdStr.empty()) {
        Notification::create("Please enter an account ID")->show();
        return;
    }

    if (m_loadingSpinner) {
        m_loadingSpinner->setVisible(true);
    }

    auto upopup = UploadActionPopup::create(nullptr, "Updating user...");
    upopup->show();

    auto accountData = argon::getGameAccountData();
    auto url = std::string(gdx::baseApiUrl()) + "/setUser";
    matjson::Value body = matjson::Value::object();
    body["accountId"] = accountData.accountId;
    body["targetAccountId"] = numFromString<int>(accountIdStr).unwrapOr(0);
    body["isLeaderboardMod"] = !m_isLeaderboardMod;  // toggle
    body["argonToken"] = "";                         // Will be set later after fetching the token

    auto self = geode::Ref<GDXUserPanelPopup>(this);
    auto upopupRef = geode::Ref<UploadActionPopup>(upopup);
    m_promoteModTask.spawn([upopupRef = std::move(upopupRef), self = std::move(self), accountIdStr = std::move(accountIdStr), url = std::move(url), body = std::move(body), accountData = std::move(accountData)]() mutable -> arc::Future<> {
        auto token = co_await gdx::argonToken(accountData);
        if (token.empty()) {
            co_await geode::async::waitForMainThread([upopupRef = std::move(upopupRef), self = std::move(self)]() {
                if (!upopupRef) return;
                if (upopupRef) upopupRef->showFailMessage("Failed to get access.");
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
            auto errMsg = gdx::getResponseMessage(response, "Failed to update leaderboard mod status");
            co_await geode::async::waitForMainThread([upopupRef = std::move(upopupRef), self = std::move(self), errMsg = std::move(errMsg)]() {
                if (!upopupRef) return;
                if (upopupRef) upopupRef->showFailMessage(errMsg);
            });
            co_return;
        }

        auto jsonResult = response.json();
        if (!jsonResult) {
            co_await geode::async::waitForMainThread([upopupRef = std::move(upopupRef), self = std::move(self)]() {
                if (!upopupRef) return;
                if (upopupRef) upopupRef->showFailMessage("Invalid response");
            });
            co_return;
        }

        auto data = std::move(jsonResult).unwrap();
        if (!data.isObject() || !data["success"].asBool().unwrapOr(false)) {
            auto errMsg = gdx::getResponseMessage(response, "Failed to update user");
            co_await geode::async::waitForMainThread([upopupRef = std::move(upopupRef), self = std::move(self), errMsg = std::move(errMsg)]() {
                if (!upopupRef) return;
                if (upopupRef) upopupRef->showFailMessage(errMsg);
            });
            co_return;
        }

        co_await geode::async::waitForMainThread([upopupRef = std::move(upopupRef), self = std::move(self)]() {
            if (self) {
                self->m_isLeaderboardMod = !self->m_isLeaderboardMod;
                self->updatePromoteModButton();
                if (self->m_loadingSpinner) {
                    self->m_loadingSpinner->setVisible(false);
                }
                self->fetchUsers(self->m_page);
            }
            if (!upopupRef) return;
            if (upopupRef) {
                std::string message = (self && self->m_isLeaderboardMod) ? "User promoted to Leaderboard Mod" : "User demoted from Leaderboard Mod";
                upopupRef->showSuccessMessage(message);
            }
        });

        co_return; }, []() {});
}

CCNode* GDXUserPanelPopup::createUserCell(matjson::Value const& entry, float width) {
    auto username = entry["username"].asString().unwrapOr("Unknown");
    int accountId = entry["accountId"].asInt().unwrapOr(0);
    bool isManager = entry["isManager"].asBool().unwrapOr(false);
    bool isContributor = entry["isContributor"].asBool().unwrapOr(false);
    bool isLeaderboardMod = entry["isLeaderboardMod"].asBool().unwrapOr(false);
    bool isExcluded = entry["isExcluded"].asBool().unwrapOr(false);

    auto cell = CCLayer::create();
    cell->setContentSize({width, kCellHeight});

    // selecting a user fills in the account id and looks them up
    auto nameBtn = geode::Button::createWithLabel(username.c_str(), "goldFont.fnt", [this, accountId](geode::Button*) {
        m_accountIdInput->setString(numToString(accountId));
        onFindAccountID(nullptr);
    });
    nameBtn->setAnchorPoint({0.f, 0.5f});
    nameBtn->setScale(0.5f);
    nameBtn->setPosition({10.f, kCellHeight / 2.f + 5.f});
    cell->addChild(nameBtn);

    auto idLabel = CCLabelBMFont::create(fmt::format("ID: {}", accountId).c_str(), "chatFont.fnt");
    idLabel->setAnchorPoint({0.f, 0.5f});
    idLabel->setScale(0.45f);
    idLabel->setPosition({10.f, 6.f});
    cell->addChild(idLabel);

    std::vector<std::string> roles;
    if (isManager) roles.push_back("Manager");
    if (isContributor) roles.push_back("Contributor");
    if (isLeaderboardMod) roles.push_back("LB Mod");
    if (isExcluded) roles.push_back("Excluded");
    if (!roles.empty()) {
        std::string text;
        for (auto const& role : roles) {
            if (!text.empty()) text += ", ";
            text += role;
        }
        auto roleLabel = CCLabelBMFont::create(text.c_str(), "bigFont.fnt");
        roleLabel->setAnchorPoint({1.f, 0.5f});
        roleLabel->limitLabelWidth(width * 0.5f, 0.35f, 0.15f);
        roleLabel->setPosition({width - 22.f, kCellHeight / 2.f});
        if (isExcluded) {
            roleLabel->setColor({255, 90, 90});
        }
        else if (isManager) {
            roleLabel->setColor({255, 200, 0});
        }
        else if (isContributor) {
            roleLabel->setColor({125, 255, 125});
        }
        else {
            roleLabel->setColor({125, 200, 255});
        }
        cell->addChild(roleLabel);
    }

    return cell;
}

void GDXUserPanelPopup::updatePageInfo() {
    if (m_pageInfoLabel) {
        if (m_totalUsers <= 0) {
            m_pageInfoLabel->setString("No users found");
        } else {
            int first = (m_page - 1) * kUsersPerPage + 1;
            int last = std::min(m_page * kUsersPerPage, m_totalUsers);
            m_pageInfoLabel->setString(fmt::format("Page {}/{}  |  {}-{} of {} users", m_page, m_totalPages, first, last, m_totalUsers).c_str());
        }
        m_pageInfoLabel->limitLabelWidth(200.f, 0.35f, 0.15f);
    }
    if (m_prevPageBtn) m_prevPageBtn->setVisible(m_page > 1);
    if (m_nextPageBtn) m_nextPageBtn->setVisible(m_page < m_totalPages);
}

void GDXUserPanelPopup::fetchUsers(int page) {
    if (!m_userList) return;

    if (m_listSpinner) m_listSpinner->setVisible(true);
    m_userList->clear();

    auto accountData = argon::getGameAccountData();
    auto url = std::string(gdx::baseApiUrl()) + "/getUsers";
    matjson::Value body = matjson::Value::object();
    body["accountId"] = accountData.accountId;
    body["argonToken"] = "";
    body["page"] = page;
    body["perPage"] = kUsersPerPage;

    auto self = geode::Ref<GDXUserPanelPopup>(this);
    m_fetchUsersTask.spawn([self = std::move(self), url = std::move(url), body = std::move(body), accountData = std::move(accountData)]() mutable -> arc::Future<> {
        auto fail = [](geode::Ref<GDXUserPanelPopup> self, std::string message) -> arc::Future<> {
            co_await geode::async::waitForMainThread([self = std::move(self), message = std::move(message)]() {
                if (!self) return;
                if (self->m_listSpinner) self->m_listSpinner->setVisible(false);
                if (self->m_pageInfoLabel) {
                    self->m_pageInfoLabel->setString("Failed to load users");
                    self->m_pageInfoLabel->limitLabelWidth(200.f, 0.35f, 0.15f);
                }
                Notification::create(message, NotificationIcon::Error)->show();
            });
        };

        auto token = co_await gdx::argonToken(accountData);
        if (token.empty()) {
            co_await fail(self, "Authentication failed.");
            co_return;
        }

        body["argonToken"] = std::move(token);
        auto response = co_await geode::utils::web::WebRequest()
                            .url(url)
                            .header("Content-Type", "application/json")
                            .bodyJSON(body)
                            .post(url);

        if (response.error() || response.cancelled() || !response.ok()) {
            co_await fail(self, gdx::getResponseMessage(response, "Failed to load users"));
            co_return;
        }

        auto jsonResult = response.json();
        if (!jsonResult) {
            co_await fail(self, "Invalid response");
            co_return;
        }

        auto data = std::move(jsonResult).unwrap();
        if (!data.isObject() || !data["success"].asBool().unwrapOr(false) || !data["users"].isArray()) {
            co_await fail(self, "Failed to load users");
            co_return;
        }

        co_await geode::async::waitForMainThread([self = std::move(self), data = std::move(data)]() mutable {
            if (!self || !self->m_userList) return;

            self->m_page = data["page"].asInt().unwrapOr(1);
            self->m_totalPages = std::max(static_cast<int>(data["totalPages"].asInt().unwrapOr(1)), 1);
            self->m_totalUsers = static_cast<int>(data["total"].asInt().unwrapOr(0));

            for (auto const& entry : data["users"]) {
                if (entry.isObject()) {
                    self->m_userList->addCell(self->createUserCell(entry, kListWidth));
                }
            }
            self->m_userList->updateLayout();
            self->m_userList->scrollToTop();

            if (self->m_listSpinner) self->m_listSpinner->setVisible(false);
            self->updatePageInfo();
        });

        co_return; }, []() {});
}
