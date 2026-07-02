#include "GDXLinkDiscordPopup.hpp"
#include "../include/GDXConstant.hpp"
#include "Geode/cocos/cocoa/CCGeometry.h"
#include "Geode/ui/MDPopup.hpp"
#include "Geode/ui/NineSlice.hpp"
#include "Geode/ui/TextArea.hpp"
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/FLAlertLayer.hpp>
#include <Geode/ui/Layout.hpp>
#include <Geode/ui/General.hpp>
#include <Geode/ui/Button.hpp>
#include <Geode/ui/Notification.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/utils/general.hpp>
#include <argon/argon.hpp>

using namespace geode::prelude;

GDXLinkDiscordPopup* GDXLinkDiscordPopup::create() {
    auto ret = new GDXLinkDiscordPopup();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool GDXLinkDiscordPopup::init() {
    if (!Popup::init(340.f, 220.f, "GJ_square01.png")) {
        return false;
    }

    setTitle("Link Discord Account");
    addSideArt(m_mainLayer, SideArt::All, SideArtStyle::PopupGold, false);

    m_discordIdInput = TextInput::create(240.f, "Discord User ID", "bigFont.fnt");
    m_discordIdInput->setLabel("Discord User ID");
    m_discordIdInput->setCommonFilter(CommonFilter::Int);
    m_mainLayer->addChildAtPosition(m_discordIdInput, Anchor::Center, {0.f, 30.f});
    m_discordIdInput->setVisible(false);

    m_usernameInput = TextInput::create(240.f, "Discord Username", "bigFont.fnt");
    m_usernameInput->setLabel("Discord Username");
    m_usernameInput->setCommonFilter(CommonFilter::Any);
    m_mainLayer->addChildAtPosition(m_usernameInput, Anchor::Center, {0.f, -20.f});
    m_usernameInput->setVisible(false);

    m_statusLabel = SimpleTextArea::create("", "bigFont.fnt", 0.5f, m_mainLayer->getContentWidth() - 10.f);
    m_statusLabel->setAlignment(CCTextAlignment::kCCTextAlignmentCenter);
    m_mainLayer->addChildAtPosition(m_statusLabel, Anchor::Center, {0.f, 25.f});
    m_statusLabel->setVisible(false);

    m_codeHintLabel = CCLabelBMFont::create("Click code below to copy to clipboard", "goldFont.fnt");
    m_codeHintLabel->limitLabelWidth(m_mainLayer->getContentWidth() - 10.f, 0.6f, 0.2f);
    m_codeHintLabel->setAlignment(CCTextAlignment::kCCTextAlignmentCenter);
    m_mainLayer->addChildAtPosition(m_codeHintLabel, Anchor::Center, {0.f, 55.f});
    m_codeHintLabel->setVisible(false);

    auto linkSprite = ButtonSprite::create("Link", "goldFont.fnt", "GJ_button_01.png");
    m_linkBtn = CCMenuItemSpriteExtra::create(linkSprite, this, menu_selector(GDXLinkDiscordPopup::onLink));
    m_buttonMenu->addChildAtPosition(m_linkBtn, Anchor::Bottom, {0.f, 35.f});
    m_linkBtn->setVisible(false);

    auto unlinkSprite = ButtonSprite::create("Unlink", "goldFont.fnt", "GJ_button_06.png");
    m_unlinkBtn = CCMenuItemSpriteExtra::create(unlinkSprite, this, menu_selector(GDXLinkDiscordPopup::onUnlink));
    m_buttonMenu->addChildAtPosition(m_unlinkBtn, Anchor::Bottom, {0.f, 35.f});
    m_unlinkBtn->setVisible(false);

    m_codeLabel = CCLabelBMFont::create("000000", "goldFont.fnt");
    m_codeBtn = CCMenuItemSpriteExtra::create(m_codeLabel, this, menu_selector(GDXLinkDiscordPopup::onCopyCode));
    m_buttonMenu->addChildAtPosition(m_codeBtn, Anchor::Center, {0.f, -20.f});
    m_codeBtn->setVisible(false);

    m_codeBg = NineSlice::create("square02_small.png");
    m_codeBg->setAnchorPoint({0.5f, 0.5f});
    m_codeBg->setContentSize(m_codeBtn->getScaledContentSize() + CCSize(10, 10));
    m_codeBg->setOpacity(100);
    m_mainLayer->addChildAtPosition(m_codeBg, Anchor::Center, {0.f, -20.f});
    m_codeBg->setVisible(false);

    m_loadingCircle = cue::LoadingCircle::create(false);
    m_mainLayer->addChildAtPosition(m_loadingCircle, Anchor::Center, {0.f, 0.f}, 999);

    m_expiresLabel = CCLabelBMFont::create("", "goldFont.fnt");
    m_expiresLabel->setScale(0.55f);
    m_mainLayer->addChildAtPosition(m_expiresLabel, Anchor::Center, {0.f, -50.f});
    m_expiresLabel->setVisible(false);

    m_linkedUsernameLabel = CCLabelBMFont::create("", "goldFont.fnt");
    m_linkedUsernameLabel->setScale(0.85f);
    m_mainLayer->addChildAtPosition(m_linkedUsernameLabel, Anchor::Center, {0.f, 40.f});
    m_linkedUsernameLabel->setVisible(false);

    m_linkedIdLabel = CCLabelBMFont::create("", "chatFont.fnt");
    m_linkedIdLabel->setScale(0.7f);
    m_linkedIdLabel->setColor({200, 200, 200});
    m_mainLayer->addChildAtPosition(m_linkedIdLabel, Anchor::Center, {0.f, 10.f});
    m_linkedIdLabel->setVisible(false);

    m_linkedStatusLabel = CCLabelBMFont::create("", "bigFont.fnt");
    m_linkedStatusLabel->setScale(0.5f);
    m_mainLayer->addChildAtPosition(m_linkedStatusLabel, Anchor::Center, {0.f, -25.f});
    m_linkedStatusLabel->setVisible(false);

    // info
    auto infoBtn = geode::Button::createWithSpriteFrameName("GJ_infoIcon_001.png", [this](geode::Button* sender) {
        FLAlertLayer::create("Discord Linking",
            "Linking your <cl>Discord account</c> allows you to <cg>submit gauntlets</c>, <cc>view your submission history</c>, and <co>track contributions</c> in <cr>Gauntlets Deluxe</c>.\n\n"
            "<cy>Access all these features via the</c> <co>GauntletsHelper</c> <cy>Discord bot on our community server!</c>",
            "OK")
            ->show();
    });
    m_buttonMenu->addChildAtPosition(infoBtn, Anchor::BottomRight, {-25.f, 25.f});

    fetchDiscordStatus();

    return true;
}

void GDXLinkDiscordPopup::updateUI(bool isLinked, const std::string& username, const std::string& discordId, bool isVerified, const std::string& linkCode, int expiresIn) {
    m_isLinked = isLinked;

    this->unschedule(schedule_selector(GDXLinkDiscordPopup::updateTimer));
    this->unschedule(schedule_selector(GDXLinkDiscordPopup::checkLinkScheduler));
    if (m_expiresLabel) {
        m_expiresLabel->setVisible(false);
    }
    if (m_linkedUsernameLabel) m_linkedUsernameLabel->setVisible(false);
    if (m_linkedIdLabel) m_linkedIdLabel->setVisible(false);
    if (m_linkedStatusLabel) m_linkedStatusLabel->setVisible(false);

    if (!linkCode.empty()) {
        m_linkCode = linkCode;
        m_discordIdInput->setVisible(false);
        m_usernameInput->setVisible(false);
        m_linkBtn->setVisible(false);
        m_unlinkBtn->setVisible(false);

        m_codeLabel->setString(linkCode.c_str());
        m_codeBtn->setVisible(true);
        m_codeBg->setVisible(true);
        m_codeHintLabel->setVisible(true);

        m_statusLabel->setText(("Verification code generated for:\n@" + username).c_str());
        m_statusLabel->setVisible(true);

        if (m_expiresLabel && expiresIn > 0) {
            m_expiresIn = expiresIn;
            m_expiresLabel->setVisible(true);
            this->updateTimer(0.f);
            this->schedule(schedule_selector(GDXLinkDiscordPopup::updateTimer), 1.0f);
        }
        this->schedule(schedule_selector(GDXLinkDiscordPopup::checkLinkScheduler), 5.0f);
        return;
    }

    if (isLinked) {
        m_discordIdInput->setVisible(false);
        m_usernameInput->setVisible(false);
        m_linkBtn->setVisible(false);
        m_codeBtn->setVisible(false);
        m_codeBg->setVisible(false);
        m_codeHintLabel->setVisible(false);
        m_statusLabel->setVisible(false);

        if (m_linkedUsernameLabel) {
            m_linkedUsernameLabel->setString(("@" + username).c_str());
            m_linkedUsernameLabel->setVisible(true);
        }
        if (m_linkedIdLabel && !discordId.empty()) {
            m_linkedIdLabel->setString(("Discord ID: " + discordId).c_str());
            m_linkedIdLabel->setVisible(true);
        }
        if (m_linkedStatusLabel) {
            if (isVerified) {
                m_linkedStatusLabel->setString("Status: Linked & Verified");
                m_linkedStatusLabel->setColor({50, 255, 50});
            } else {
                m_linkedStatusLabel->setString("Status: Pending Verification");
                m_linkedStatusLabel->setColor({255, 200, 50});
            }
            m_linkedStatusLabel->setVisible(true);
        }
        m_unlinkBtn->setVisible(true);
    } else {
        m_discordIdInput->setVisible(true);
        m_usernameInput->setVisible(true);
        m_linkBtn->setVisible(true);

        m_unlinkBtn->setVisible(false);
        m_codeBtn->setVisible(false);
        m_codeHintLabel->setVisible(false);
        m_statusLabel->setVisible(false);
    }
}

void GDXLinkDiscordPopup::updateTimer(float dt) {
    if (dt > 0.f && m_expiresIn > 0) {
        m_expiresIn--;
    }
    if (m_expiresLabel) {
        if (m_expiresIn <= 0) {
            m_expiresLabel->setString("Code expired!");
            this->unschedule(schedule_selector(GDXLinkDiscordPopup::updateTimer));
            this->unschedule(schedule_selector(GDXLinkDiscordPopup::checkLinkScheduler));
        } else {
            int mins = m_expiresIn / 60;
            int secs = m_expiresIn % 60;
            m_expiresLabel->setString(fmt::format("Expires in {:02d}:{:02d}", mins, secs).c_str());
        }
    }
}

void GDXLinkDiscordPopup::checkLinkScheduler(float dt) {
    this->fetchDiscordStatus(true);
}

void GDXLinkDiscordPopup::onCopyCode(CCObject* sender) {
    if (!m_linkCode.empty()) {
        geode::utils::clipboard::write(m_linkCode);
        Notification::create("Copied verification code to clipboard!", NotificationIcon::Success)->show();
    }
}

void GDXLinkDiscordPopup::fetchDiscordStatus(bool silent) {
    if (!silent && m_loadingCircle) {
        m_loadingCircle->fadeIn();
    }

    auto accountData = argon::getGameAccountData();
    auto url = std::string(gdx::baseApiUrl()) + "/getDiscordUser";
    matjson::Value body = matjson::Value::object();
    body["accountId"] = accountData.accountId;
    body["argonToken"] = "";

    auto self = geode::Ref<GDXLinkDiscordPopup>(this);

    m_checkStatusTask.spawn([self = std::move(self), url = std::move(url), body = std::move(body), accountData = std::move(accountData), silent]() mutable -> arc::Future<> {
        auto token = co_await gdx::argonToken(accountData);
        if (token.empty()) {
            if (!silent) {
                co_await geode::async::waitForMainThread([self = std::move(self)]() {
                    if (self) {
                        if (self->m_loadingCircle) self->m_loadingCircle->fadeOut();
                        self->updateUI(false, "", "", false, "");
                        if (self->m_statusLabel) {
                            self->m_statusLabel->setText("Authentication failed.");
                            self->m_statusLabel->setVisible(true);
                        }
                    }
                });
            }
            co_return;
        }

        body["argonToken"] = std::move(token);
        auto response = co_await geode::utils::web::WebRequest()
                            .url(url)
                            .header("Content-Type", "application/json")
                            .bodyJSON(body)
                            .post(url);

        if (response.error() || response.cancelled() || !response.ok()) {
            if (!silent) {
                auto errMsg = gdx::getResponseMessage(response, "Failed to fetch Discord status.");
                co_await geode::async::waitForMainThread([self = std::move(self), errMsg = std::move(errMsg)]() {
                    if (self) {
                        if (self->m_loadingCircle) self->m_loadingCircle->fadeOut();
                        self->updateUI(false, "", "", false, "");
                        if (self->m_statusLabel) {
                            self->m_statusLabel->setText(errMsg.c_str());
                            self->m_statusLabel->setVisible(true);
                        }
                    }
                });
            }
            co_return;
        }

        auto jsonResult = response.json();
        if (!jsonResult) {
            if (!silent) {
                co_await geode::async::waitForMainThread([self = std::move(self)]() {
                    if (self) {
                        if (self->m_loadingCircle) self->m_loadingCircle->fadeOut();
                        self->updateUI(false, "", "", false, "");
                        if (self->m_statusLabel) {
                            self->m_statusLabel->setText("Failed to parse response.");
                            self->m_statusLabel->setVisible(true);
                        }
                    }
                });
            }
            co_return;
        }

        auto result = std::move(jsonResult).unwrap();
        if (!result["success"].asBool().unwrapOr(false)) {
            if (!silent) {
                auto errMsg = result["message"].asString().unwrapOr(gdx::getResponseMessage(response, "Failed to check status."));
                co_await geode::async::waitForMainThread([self = std::move(self), errMsg = std::move(errMsg)]() {
                    if (self) {
                        if (self->m_loadingCircle) self->m_loadingCircle->fadeOut();
                        self->updateUI(false, "", "", false, "");
                        if (self->m_statusLabel) {
                            self->m_statusLabel->setText(errMsg.c_str());
                            self->m_statusLabel->setVisible(true);
                        }
                    }
                });
            }
            co_return;
        }

        bool isLinked = result["isLinked"].asBool().unwrapOr(false);
        bool isVerified = result["isVerified"].asBool().unwrapOr(false);
        std::string discordId = result["discordId"].asString().unwrapOr("");
        std::string username = result["discordUsername"].asString().unwrapOr("");

        co_await geode::async::waitForMainThread([self = std::move(self), isLinked, username = std::move(username), discordId = std::move(discordId), isVerified, silent]() {
            if (self) {
                if (!silent && self->m_loadingCircle) self->m_loadingCircle->fadeOut();
                if (!silent || isLinked) {
                    self->updateUI(isLinked, username, discordId, isVerified, "");
                }
            }
        });

        co_return; }, []() {});
}

void GDXLinkDiscordPopup::onLink(CCObject* sender) {
    std::string discordId = m_discordIdInput ? m_discordIdInput->getString() : "";
    std::string username = m_usernameInput ? m_usernameInput->getString() : "";

    if (discordId.empty() || username.empty()) {
        Notification::create("Please fill in both Discord ID and Username.", NotificationIcon::Error)->show();
        return;
    }

    if (discordId.length() < 17 || discordId.length() > 20) {
        Notification::create("Invalid Discord User ID.", NotificationIcon::Error)->show();
        return;
    }

    if (gdx::isHelperBotUserId(discordId)) {
        Notification::create("Invalid Discord User ID.", NotificationIcon::Error)->show();
        return;
    }

    if (m_loadingCircle) {
        m_loadingCircle->fadeIn();
    }
    if (m_linkBtn) {
        m_linkBtn->setEnabled(false);
    }
    if (m_statusLabel) {
        m_statusLabel->setVisible(false);
    }

    auto accountData = argon::getGameAccountData();
    auto url = std::string(gdx::baseApiUrl()) + "/linkDiscord";
    matjson::Value body = matjson::Value::object();
    body["accountId"] = accountData.accountId;
    body["argonToken"] = "";
    body["discordId"] = discordId;
    body["discordUsername"] = username;

    auto self = geode::Ref<GDXLinkDiscordPopup>(this);

    m_linkTask.spawn([self = std::move(self), url = std::move(url), body = std::move(body), accountData = std::move(accountData), username = std::move(username), discordId = std::move(discordId)]() mutable -> arc::Future<> {
        auto token = co_await gdx::argonToken(accountData);
        if (token.empty()) {
            co_await geode::async::waitForMainThread([self = std::move(self)]() {
                if (self) {
                    if (self->m_loadingCircle) self->m_loadingCircle->fadeOut();
                    if (self->m_linkBtn) self->m_linkBtn->setEnabled(true);
                    FLAlertLayer::create("Error", "Authentication failed.", "OK")->show();
                }
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
            auto errMsg = gdx::getResponseMessage(response, "Failed to initiate link.");
            co_await geode::async::waitForMainThread([self = std::move(self), errMsg = std::move(errMsg)]() {
                if (self) {
                    if (self->m_loadingCircle) self->m_loadingCircle->fadeOut();
                    if (self->m_linkBtn) self->m_linkBtn->setEnabled(true);
                    FLAlertLayer::create("Error", errMsg, "OK")->show();
                }
            });
            co_return;
        }

        auto jsonResult = response.json();
        if (!jsonResult) {
            co_await geode::async::waitForMainThread([self = std::move(self)]() {
                if (self) {
                    if (self->m_loadingCircle) self->m_loadingCircle->fadeOut();
                    if (self->m_linkBtn) self->m_linkBtn->setEnabled(true);
                    FLAlertLayer::create("Error", "Failed to parse response.", "OK")->show();
                }
            });
            co_return;
        }

        auto result = std::move(jsonResult).unwrap();
        if (!result["success"].asBool().unwrapOr(false)) {
            auto errMsg = result["message"].asString().unwrapOr(gdx::getResponseMessage(response, "Failed to link Discord account."));
            co_await geode::async::waitForMainThread([self = std::move(self), errMsg = std::move(errMsg)]() {
                if (self) {
                    if (self->m_loadingCircle) self->m_loadingCircle->fadeOut();
                    if (self->m_linkBtn) self->m_linkBtn->setEnabled(true);
                    FLAlertLayer::create("Error", errMsg, "OK")->show();
                }
            });
            co_return;
        }

        std::string linkCode = result["linkCode"].asString().unwrapOr("");
        std::string message = result["message"].asString().unwrapOr("Link code generated successfully!");
        int expiresIn = result["expiresIn"].asInt().unwrapOr(0);

        co_await geode::async::waitForMainThread([self = std::move(self), message = std::move(message), linkCode = std::move(linkCode), username = std::move(username), discordId = std::move(discordId), expiresIn]() {
            if (self) {
                if (self->m_loadingCircle) self->m_loadingCircle->fadeOut();
                if (self->m_linkBtn) self->m_linkBtn->setEnabled(true);
                self->updateUI(false, username, discordId, false, linkCode, expiresIn);
                if (self->m_statusLabel) {
                    self->m_statusLabel->setText(message.c_str());
                    self->m_statusLabel->setVisible(true);
                }
            }
        });

        co_return; }, []() {});
}

void GDXLinkDiscordPopup::onUnlink(CCObject* sender) {
    if (m_loadingCircle) {
        m_loadingCircle->fadeIn();
    }
    if (m_unlinkBtn) {
        m_unlinkBtn->setEnabled(false);
    }

    auto accountData = argon::getGameAccountData();
    auto url = std::string(gdx::baseApiUrl()) + "/unlinkDiscord";
    matjson::Value body = matjson::Value::object();
    body["accountId"] = accountData.accountId;
    body["argonToken"] = "";

    auto self = geode::Ref<GDXLinkDiscordPopup>(this);

    m_unlinkTask.spawn([self = std::move(self), url = std::move(url), body = std::move(body), accountData = std::move(accountData)]() mutable -> arc::Future<> {
        auto token = co_await gdx::argonToken(accountData);
        if (token.empty()) {
            co_await geode::async::waitForMainThread([self = std::move(self)]() {
                if (self) {
                    if (self->m_loadingCircle) self->m_loadingCircle->fadeOut();
                    if (self->m_unlinkBtn) self->m_unlinkBtn->setEnabled(true);
                    FLAlertLayer::create("Error", "Authentication failed.", "OK")->show();
                }
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
            auto errMsg = gdx::getResponseMessage(response, "Failed to unlink Discord account.");
            co_await geode::async::waitForMainThread([self = std::move(self), errMsg = std::move(errMsg)]() {
                if (self) {
                    if (self->m_loadingCircle) self->m_loadingCircle->fadeOut();
                    if (self->m_unlinkBtn) self->m_unlinkBtn->setEnabled(true);
                    FLAlertLayer::create("Error", errMsg, "OK")->show();
                }
            });
            co_return;
        }

        auto jsonResult = response.json();
        if (!jsonResult) {
            co_await geode::async::waitForMainThread([self = std::move(self)]() {
                if (self) {
                    if (self->m_loadingCircle) self->m_loadingCircle->fadeOut();
                    if (self->m_unlinkBtn) self->m_unlinkBtn->setEnabled(true);
                    FLAlertLayer::create("Error", "Failed to parse response.", "OK")->show();
                }
            });
            co_return;
        }

        auto result = std::move(jsonResult).unwrap();
        if (!result["success"].asBool().unwrapOr(false)) {
            auto errMsg = result["message"].asString().unwrapOr(gdx::getResponseMessage(response, "Failed to unlink Discord account."));
            co_await geode::async::waitForMainThread([self = std::move(self), errMsg = std::move(errMsg)]() {
                if (self) {
                    if (self->m_loadingCircle) self->m_loadingCircle->fadeOut();
                    if (self->m_unlinkBtn) self->m_unlinkBtn->setEnabled(true);
                    FLAlertLayer::create("Error", errMsg, "OK")->show();
                }
            });
            co_return;
        }

        std::string message = result["message"].asString().unwrapOr("Discord account unlinked successfully!");

        co_await geode::async::waitForMainThread([self = std::move(self), message = std::move(message)]() {
            if (self) {
                if (self->m_loadingCircle) self->m_loadingCircle->fadeOut();
                if (self->m_unlinkBtn) self->m_unlinkBtn->setEnabled(true);
                FLAlertLayer::create("Success", message, "OK")->show();
                self->updateUI(false, "", "", false, "");
            }
        });

        co_return; }, []() {});
}
