#include "GDXLikeItemPopup.hpp"
#include "../include/GDXConstant.hpp"
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/UploadActionPopup.hpp>
#include <Geode/ui/Layout.hpp>
#include <Geode/utils/web.hpp>
#include <argon/argon.hpp>
#include <Geode/Geode.hpp>

using namespace geode::prelude;

GDXLikeItemPopup* GDXLikeItemPopup::create(const matjson::Value& gauntlet) {
    auto ret = new GDXLikeItemPopup();
    if (ret && ret->init(gauntlet)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool GDXLikeItemPopup::init(const matjson::Value& gauntlet) {
    if (!Popup::init(200.f, 135.f, "GJ_square01.png")) {
        return false;
    }

    m_gauntlet = gauntlet;
    m_gauntletIndex = gauntlet["id"].asInt().unwrapOr(gauntlet["index"].asInt().unwrapOr(0));

    auto gauntletName = gauntlet["name"].asString().unwrapOr("Gauntlet");
    this->setTitle(gauntletName.c_str());
    m_title->limitLabelWidth(m_mainLayer->getContentWidth() - 20.f, 0.5f, 0.1f);
    m_title->setPositionY(m_title->getPositionY() - 2.f);
    m_title->setFntFile("bigFont.fnt");

    auto buttonMenu = CCMenu::create();
    if (!buttonMenu) {
        return true;
    }
    buttonMenu->setPosition({0.f, 0.f});
    buttonMenu->setContentWidth(m_mainLayer->getContentSize().width - 10);
    buttonMenu->setLayout(RowLayout::create()->setGap(20.f)->setAxisAlignment(AxisAlignment::Center));
    m_mainLayer->addChildAtPosition(buttonMenu, Anchor::Center, {0.f, 0.f}, false);

    auto likeSprite = CCSprite::createWithSpriteFrameName("GJ_likeBtn_001.png");
    if (likeSprite) {
        likeSprite->setScale(1.2f);
        m_likeButton = CCMenuItemSpriteExtra::create(likeSprite, this, menu_selector(GDXLikeItemPopup::onLike));
        if (m_likeButton) {
            buttonMenu->addChild(m_likeButton);
        }
    }

    auto dislikeSprite = CCSprite::createWithSpriteFrameName("GJ_dislikeBtn_001.png");
    if (dislikeSprite) {
        dislikeSprite->setScale(1.2f);
        m_dislikeButton = CCMenuItemSpriteExtra::create(dislikeSprite, this, menu_selector(GDXLikeItemPopup::onDislike));
        if (m_dislikeButton) {
            buttonMenu->addChild(m_dislikeButton);
        }
    }

    buttonMenu->updateLayout();

    // total likes and dislikes of the gauntlet, shown under the buttons
    auto feedback = gauntlet["feedback"];
    int likes = feedback["likes"].asInt().unwrapOr(0);
    int dislikes = feedback["dislikes"].asInt().unwrapOr(0);

    auto statsMenu = CCMenu::create();
    statsMenu->setContentWidth(m_mainLayer->getContentSize().width - 10);
    statsMenu->setContentHeight(25.f);
    statsMenu->setLayout(RowLayout::create()
            ->setGap(10.f)
            ->setAxisAlignment(AxisAlignment::Center)
            ->setCrossAxisOverflow(false));

    auto addStat = [statsMenu](char const* frameName, int count) {
        auto icon = CCSprite::createWithSpriteFrameName(frameName);
        if (icon) {
            icon->setScale(0.6f);
            statsMenu->addChild(icon);
        }
        auto label = CCLabelBMFont::create(GameToolbox::pointsToString(count).c_str(), "bigFont.fnt");
        if (label) {
            label->limitLabelWidth(100.f, 0.4f, 0.35f);
            statsMenu->addChild(label);
        }
    };
    addStat("GJ_likesIcon_001.png", likes);
    addStat("GJ_dislikesIcon_001.png", dislikes);

    m_mainLayer->addChildAtPosition(statsMenu, Anchor::Bottom, {0.f, 20.f}, false);
    statsMenu->updateLayout();
    return true;
}

void GDXLikeItemPopup::onLike(CCObject* sender) {
    sendFeedback("like");
}

void GDXLikeItemPopup::onDislike(CCObject* sender) {
    sendFeedback("dislike");
}

void GDXLikeItemPopup::sendFeedback(const std::string& type) {
    if (m_isSending || !m_gauntlet.isObject()) {
        return;
    }

    m_isSending = true;

    auto upopup = UploadActionPopup::create(nullptr, "Sending feedback...");
    if (upopup) {
        upopup->show();
    }

    auto accountData = argon::getGameAccountData();
    auto url = std::string(gdx::baseApiUrl()) + "/sendGauntletFeedback";
    matjson::Value body = matjson::Value::object();
    body["accountId"] = accountData.accountId;
    body["argonToken"] = "";
    body["index"] = m_gauntletIndex;
    body["type"] = type;

    m_selfHold = this;
    auto self = Ref<GDXLikeItemPopup>(this);
    auto upopupRef = Ref<UploadActionPopup>(upopup);
    m_requestTask.spawn([self = std::move(self), upopupRef = std::move(upopupRef), url = std::move(url), body = std::move(body), accountData = std::move(accountData)]() mutable -> arc::Future<> {
        auto token = co_await gdx::argonToken(accountData);
        if (token.empty()) {
            co_await async::waitForMainThread([self = std::move(self), upopupRef = std::move(upopupRef)]() {
                if (self) self->m_isSending = false;
                if (!upopupRef) return;
                if (upopupRef) {
                    upopupRef->showFailMessage("Authentication failed.");
                }
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
            auto errMsg = gdx::getResponseMessage(response, "Failed to send feedback.");
            co_await async::waitForMainThread([self = std::move(self), upopupRef = std::move(upopupRef), errMsg = std::move(errMsg)]() {
                if (self) self->m_isSending = false;
                if (!upopupRef) return;
                if (upopupRef) {
                    upopupRef->showFailMessage(errMsg);
                }
            });
            co_return;
        }

        auto jsonResult = response.json();
        if (!jsonResult) {
            co_await async::waitForMainThread([self = std::move(self), upopupRef = std::move(upopupRef)]() {
                if (self) self->m_isSending = false;
                if (!upopupRef) return;
                if (upopupRef) {
                    upopupRef->showFailMessage("Failed to parse response.");
                }
            });
            co_return;
        }

        auto result = std::move(jsonResult).unwrap();
        if (!result["success"].asBool().unwrapOr(false)) {
            auto errMsg = gdx::getResponseMessage(response, "Request was not successful.");
            co_await async::waitForMainThread([self = std::move(self), upopupRef = std::move(upopupRef), errMsg = std::move(errMsg)]() {
                if (self) self->m_isSending = false;
                if (!upopupRef) return;
                if (upopupRef) {
                    upopupRef->showFailMessage(errMsg);
                }
            });
            co_return;
        }

        co_await async::waitForMainThread([self = std::move(self), upopupRef = std::move(upopupRef)]() {
            if (self) self->m_isSending = false;
            if (!upopupRef) return;
            if (upopupRef) {
                upopupRef->showSuccessMessage("Feedback sent.");
            }
            if (self) self->onClose(nullptr);
        });

        co_return; }, []() {});
}