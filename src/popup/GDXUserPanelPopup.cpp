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
    if (!Popup::init(360.f, 200.f, "GJ_square02.png")) {
        return false;
    }

    setTitle("Gauntlet Deluxe User Management");
    addSideArt(m_mainLayer, SideArt::All, SideArtStyle::PopupGold, false);

    m_accountIdInput = TextInput::create(220.f, "Account ID", "chatFont.fnt");
    m_accountIdInput->setLabel("Target Account ID");
    m_accountIdInput->setCommonFilter(CommonFilter::Int);
    m_mainLayer->addChildAtPosition(m_accountIdInput, Anchor::Top, {-20, -60});

    m_usernameLabel = CCLabelBMFont::create("-", "bigFont.fnt");
    m_usernameLabel->setScale(0.3f);
    m_mainLayer->addChildAtPosition(m_usernameLabel, Anchor::Bottom, {0.f, 15.f});

    m_usernameBackground = NineSlice::create("square02_small.png");
    m_usernameBackground->setContentSize(m_usernameLabel->getContentSize() + CCSizeMake(10.f, 10.f));
    m_usernameBackground->setOpacity(100);
    m_usernameBackground->setScale(m_usernameLabel->getScale());
    m_mainLayer->addChildAtPosition(m_usernameBackground, Anchor::Bottom, {0.f, 15.f});

    m_loadingSpinner = LoadingSpinner::create(50.f);
    m_mainLayer->addChildAtPosition(m_loadingSpinner, Anchor::Center, {0.f, -30.f});

    // management button menu
    m_manageMenu = CCMenu::create();
    m_manageMenu->setLayout(RowLayout::create()
            ->setGap(10.f)
            ->setAxisAlignment(AxisAlignment::Center)
            ->setGrowCrossAxis(true)
            ->setCrossAxisOverflow(false));
    m_manageMenu->setContentSize({m_mainLayer->getContentWidth() - 40.f, 80.f});
    m_mainLayer->addChildAtPosition(m_manageMenu, Anchor::Center, {0.f, -30.f}, false);

    // check button on the provided accountId
    auto findBtn = geode::Button::createWithSpriteFrameName("gj_findBtn_001.png");
    findBtn->setActivateCallback([this](CCObject* sender) {
        GDXUserPanelPopup::onFindAccountID(sender);
    });
    m_accountIdInput->addChildAtPosition(findBtn, Anchor::Right, {25.f, 0.f}, false);

    m_manageMenu->updateLayout();

    return true;
};

void GDXUserPanelPopup::updateExcludeButton() {
    if (m_excludeBtn) {
        auto offSprite = ButtonSprite::create("Exclude User", "goldFont.fnt", "GJ_button_06.png");
        auto onSprite = ButtonSprite::create("Include User", "goldFont.fnt", "GJ_button_02.png");
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
    auto offNode = ButtonSprite::create("Exclude User", "goldFont.fnt", "GJ_button_06.png");
    auto onNode = ButtonSprite::create("Include User", "goldFont.fnt", "GJ_button_02.png");
    m_excludeBtn = CCMenuItemToggler::create(offNode, onNode, this, menu_selector(GDXUserPanelPopup::onExclude));
    m_excludeBtn->setVisible(false);
    m_excludeBtn->toggle(m_isExcluded);
    m_manageMenu->addChild(m_excludeBtn);
}

void GDXUserPanelPopup::updatePromoteButton() {
    if (m_promoteBtn) {
        auto offSprite = ButtonSprite::create("Promote to Contributor", "goldFont.fnt", "GJ_button_05.png");
        auto onSprite = ButtonSprite::create("Demote from Contributor", "goldFont.fnt", "GJ_button_02.png");
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

    auto offNode = ButtonSprite::create("Promote to Contributor", "goldFont.fnt", "GJ_button_05.png");
    auto onNode = ButtonSprite::create("Demote from Contributor", "goldFont.fnt", "GJ_button_02.png");
    m_promoteBtn = CCMenuItemToggler::create(offNode, onNode, this, menu_selector(GDXUserPanelPopup::onPromote));
    m_promoteBtn->setVisible(false);
    m_promoteBtn->toggle(m_isContributor);
    m_manageMenu->addChild(m_promoteBtn);
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
    auto url = std::string(gdx::BASE_API_URL) + "/setUser";
    matjson::Value body = matjson::Value::object();
    body["accountId"] = accountData.accountId;
    body["targetAccountId"] = numFromString<int>(accountIdStr).unwrapOr(0);
    body["isExcluded"] = !m_isExcluded;  // Toggle the state
    body["argonToken"] = "";             // Will be set later after fetching the token

    auto self = geode::Ref<GDXUserPanelPopup>(this);
    m_excludeTask.spawn([upopup, self = std::move(self), accountIdStr = std::move(accountIdStr), url = std::move(url), body = std::move(body), accountData = std::move(accountData)]() mutable -> arc::Future<> {
        auto token = co_await gdx::argonToken(accountData);
        if (token.empty()) {
            co_await geode::async::waitForMainThread([upopup, self]() {
                upopup->showFailMessage("Failed to get access.");
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
            co_await geode::async::waitForMainThread([upopup, self, response]() {
                upopup->showFailMessage(gdx::getResponseMessage(response, "Failed to update exclusion status"));
            });
            co_return;
        }

        auto jsonResult = response.json();
        if (!jsonResult) {
            co_await geode::async::waitForMainThread([upopup, self, response]() {
                upopup->showFailMessage(gdx::getResponseMessage(response, "Invalid response"));
            });
            co_return;
        }

        auto data = std::move(jsonResult).unwrap();
        if (!data.isObject() || !data["success"].asBool().unwrapOr(false)) {
            co_await geode::async::waitForMainThread([upopup, self, response]() {
                upopup->showFailMessage(gdx::getResponseMessage(response, "Failed to update user"));
            });
            co_return;
        }

        co_await geode::async::waitForMainThread([upopup, self]() {
            self->m_isExcluded = !self->m_isExcluded;
            self->updateExcludeButton();
            std::string message = self->m_isExcluded ? "User excluded" : "User included";
            upopup->showSuccessMessage(message);
            self->m_loadingSpinner->setVisible(false);
            
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
    auto url = std::string(gdx::BASE_API_URL) + "/getUser";
    matjson::Value body = matjson::Value::object();
    body["accountId"] = accountData.accountId;
    body["targetAccountId"] = numFromString<int>(accountIdStr).unwrapOr(0);
    body["argonToken"] = "";  // Will be set later after fetching the token

    auto self = geode::Ref<GDXUserPanelPopup>(this);
    m_findAccountTask.spawn([self = std::move(self), accountIdStr = std::move(accountIdStr), url = std::move(url), body = std::move(body), accountData = std::move(accountData)]() mutable -> arc::Future<> {
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
            co_await geode::async::waitForMainThread([self, response]() {
                Notification::create(gdx::getResponseMessage(response, "Request failed"), NotificationIcon::Error)->show();
            });
            co_return;
        }

        auto jsonResult = response.json();
        if (!jsonResult) {
            co_await geode::async::waitForMainThread([self, response]() {
                Notification::create(gdx::getResponseMessage(response, "Invalid response"), NotificationIcon::Error)->show();
            });
            co_return;
        }

        auto userData = std::move(jsonResult).unwrap();
        if (!userData.isObject() || !userData["success"].asBool().unwrapOr(false)) {
            co_await geode::async::waitForMainThread([self, response]() {
                Notification::create(gdx::getResponseMessage(response, "User not found"), NotificationIcon::Warning)->show();
            });
            co_return;
        }

        bool isExcluded = userData["isExcluded"].asBool().unwrapOr(false);
        std::string username = userData["username"].asString().unwrapOr("Unknown");

        co_await geode::async::waitForMainThread([self, isExcluded, username = std::move(username), isContributor = userData["isContributor"].asBool().unwrapOr(false)]() {
            self->m_isExcluded = isExcluded;
            self->updateExcludeButton();
            self->m_excludeBtn->setVisible(true);
            self->m_isContributor = isContributor;
            self->updatePromoteButton();
            if (self->m_promoteBtn) {
                self->m_promoteBtn->setVisible(gdx::isManager());
            }
            self->m_manageMenu->updateLayout();
            self->m_usernameLabel->setString(username.c_str());
            self->m_usernameBackground->setContentSize(self->m_usernameLabel->getContentSize() + CCSizeMake(10.f, 10.f));
            self->m_manageMenu->setVisible(true);
            self->m_loadingSpinner->setVisible(false);
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
    auto url = std::string(gdx::BASE_API_URL) + "/setUser";
    matjson::Value body = matjson::Value::object();
    body["accountId"] = accountData.accountId;
    body["targetAccountId"] = numFromString<int>(accountIdStr).unwrapOr(0);
    body["isContributor"] = !m_isContributor;  // toggle
    body["argonToken"] = "";   // Will be set later after fetching the token

    auto self = geode::Ref<GDXUserPanelPopup>(this);
    m_promoteTask.spawn([upopup, self = std::move(self), accountIdStr = std::move(accountIdStr), url = std::move(url), body = std::move(body), accountData = std::move(accountData)]() mutable -> arc::Future<> {
        auto token = co_await gdx::argonToken(accountData);
        if (token.empty()) {
            co_await geode::async::waitForMainThread([upopup, self]() {
                upopup->showFailMessage("Failed to get access.");
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
            co_await geode::async::waitForMainThread([upopup, self, response]() {
                upopup->showFailMessage(gdx::getResponseMessage(response, "Failed to update contributor status"));
            });
            co_return;
        }

        auto jsonResult = response.json();
        if (!jsonResult) {
            co_await geode::async::waitForMainThread([upopup, self]() {
                upopup->showFailMessage("Invalid response");
            });
            co_return;
        }

        auto data = std::move(jsonResult).unwrap();
        if (!data.isObject() || !data["success"].asBool().unwrapOr(false)) {
            co_await geode::async::waitForMainThread([upopup, self, response]() {
                upopup->showFailMessage(gdx::getResponseMessage(response, "Failed to update user"));
            });
            co_return;
        }

        co_await geode::async::waitForMainThread([upopup, self]() {
            self->m_isContributor = !self->m_isContributor;
            self->updatePromoteButton();
            std::string message = self->m_isContributor ? "User promoted to contributor" : "User demoted from contributor";
            upopup->showSuccessMessage(message);
            self->m_loadingSpinner->setVisible(false);
        });

        co_return; }, []() {});
}