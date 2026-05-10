
#include <Geode/Geode.hpp>
#include "../include/GDXConstant.hpp"
#include <argon/argon.hpp>
#include <Geode/modify/ProfilePage.hpp>
#include <Geode/modify/CommentCell.hpp>

using namespace geode::prelude;

class $modify(ProfilePage) {
    void loadPageFromUserInfo(GJUserScore* a2) {
        ProfilePage::loadPageFromUserInfo(a2);
        auto layer = m_mainLayer;
        CCMenu* usernameMenu = static_cast<CCMenu*>(layer->getChildByIDRecursive("username-menu"));
        if (!usernameMenu) return;

        auto menuRef = Ref<CCMenu>(usernameMenu);
        int targetAccountId = a2 ? a2->m_accountID : 0;

        auto accountData = argon::getGameAccountData();
        auto url = std::string(gdx::BASE_API_URL) + "/getUser";
        matjson::Value body = matjson::Value::object();
        body["accountId"] = accountData.accountId;
        body["targetAccountId"] = targetAccountId;

        geode::async::spawn([menuRef, url = std::move(url), body = std::move(body), accountData]() mutable -> arc::Future<> {
            auto token = co_await gdx::argonToken(accountData);
            if (token.empty()) co_return;
            body["argonToken"] = std::move(token);

            auto response = co_await geode::utils::web::WebRequest()
                                .url(url)
                                .header("Content-Type", "application/json")
                                .bodyJSON(body)
                                .post(url);

            if (response.error() || response.cancelled() || !response.ok()) co_return;

            auto jsonResult = response.json();
            if (!jsonResult) co_return;

            auto userData = std::move(jsonResult).unwrap();
            if (!userData.isObject()) co_return;

            bool isManager = userData["isManager"].asBool().unwrapOr(false);
            bool isMod = userData["isMod"].asBool().unwrapOr(false);

            co_await geode::async::waitForMainThread([menuRef, isManager, isMod]() {
                if (isManager) {
                    auto badge = CCSprite::createWithSpriteFrameName("GDX_manager_badge.png"_spr);
                    badge->setID("GDX-manager-badge:101"_spr);
                    menuRef->addChild(badge);
                }
                if (isMod) {
                    auto badge = CCSprite::createWithSpriteFrameName("GDX_contributor_badge.png"_spr);
                    badge->setID("GDX-contributor-badge:101"_spr);
                    menuRef->addChild(badge);
                }
                menuRef->updateLayout();
            });

            co_return; });
    }
};

class $modify(CommentCell) {
    void loadFromComment(GJComment* p0) {
        CommentCell::loadFromComment(p0);
        auto layer = m_mainLayer;

        CCMenu* usernameMenu = static_cast<CCMenu*>(layer->getChildByIDRecursive("username-menu"));
        if (!usernameMenu) return;

        auto menuRef = Ref<CCMenu>(usernameMenu);
        int targetAccountId = p0 ? p0->m_accountID : 0;

        auto accountData = argon::getGameAccountData();
        auto url = std::string(gdx::BASE_API_URL) + "/getUser";
        matjson::Value body = matjson::Value::object();
        body["accountId"] = accountData.accountId;
        body["targetAccountId"] = targetAccountId;

        geode::async::spawn([menuRef, url = std::move(url), body = std::move(body), accountData]() mutable -> arc::Future<> {
            auto token = co_await gdx::argonToken(accountData);
            if (token.empty()) co_return;
            body["argonToken"] = std::move(token);

            auto response = co_await geode::utils::web::WebRequest()
                                .url(url)
                                .header("Content-Type", "application/json")
                                .bodyJSON(body)
                                .post(url);

            if (response.error() || response.cancelled() || !response.ok()) co_return;

            auto jsonResult = response.json();
            if (!jsonResult) co_return;

            auto userData = std::move(jsonResult).unwrap();
            if (!userData.isObject()) co_return;

            bool isManager = userData["isManager"].asBool().unwrapOr(false);
            bool isMod = userData["isMod"].asBool().unwrapOr(false);

            co_await geode::async::waitForMainThread([menuRef, isManager, isMod]() {
                if (isManager) {
                    auto badge = CCSprite::createWithSpriteFrameName("GDX_manager_badge.png"_spr);
                    badge->setScale(0.7f);
                    badge->setID("GDX-manager-badge:101"_spr);
                    menuRef->addChild(badge);
                }
                if (isMod) {
                    auto badge = CCSprite::createWithSpriteFrameName("GDX_contributor_badge.png"_spr);
                    badge->setScale(0.7f);
                    badge->setID("GDX-contributor-badge:101"_spr);
                    menuRef->addChild(badge);
                }
                menuRef->updateLayout();
            });

            co_return; });
    }
};