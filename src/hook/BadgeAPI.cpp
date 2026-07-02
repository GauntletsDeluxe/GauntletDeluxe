
#include <Geode/Geode.hpp>
#include "../include/GDXConstant.hpp"
#include "Geode/cocos/sprite_nodes/CCSprite.h"
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <argon/argon.hpp>
#include <Geode/modify/ProfilePage.hpp>
#include <Geode/modify/CommentCell.hpp>
#include <Geode/ui/Button.hpp>

using namespace geode::prelude;

class $modify(GDXHookProfilePage, ProfilePage) {
    void loadPageFromUserInfo(GJUserScore* a2) {
        ProfilePage::loadPageFromUserInfo(a2);
        auto layer = m_mainLayer;
        CCMenu* usernameMenu = static_cast<CCMenu*>(layer->getChildByIDRecursive("username-menu"));
        if (!usernameMenu) return;

        auto menuRef = Ref<CCMenu>(usernameMenu);
        int targetAccountId = a2 ? a2->m_accountID : 0;

        auto accountData = argon::getGameAccountData();
        auto url = std::string(gdx::baseApiUrl()) + "/getUser";
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
            bool isContributor = userData["isContributor"].asBool().unwrapOr(false);
            bool isLeaderboardMod = userData["isLeaderboardMod"].asBool().unwrapOr(false);

            co_await geode::async::waitForMainThread([menuRef, isManager, isContributor, isLeaderboardMod]() {
                if (isManager && !menuRef->getChildByIDRecursive("GDX-manager-badge:101"_spr)) {
                    auto badgeSpr = CCSprite::createWithSpriteFrameName("GDX_manager_badge.png"_spr);
                    auto badge = CCMenuItemSpriteExtra::create(badgeSpr, menuRef, menu_selector(GDXHookProfilePage::onManagerBadge));
                    badge->setID("GDX-manager-badge:101"_spr);
                    menuRef->addChild(badge);
                }
                if (isContributor && !menuRef->getChildByIDRecursive("GDX-contributor-badge:101"_spr)) {
                    auto badgeSpr = CCSprite::createWithSpriteFrameName("GDX_contributor_badge.png"_spr);
                    auto badgeBtn = CCMenuItemSpriteExtra::create(badgeSpr, menuRef, menu_selector(GDXHookProfilePage::onContributorBadge));
                    badgeBtn->setID("GDX-contributor-badge:101"_spr);
                    menuRef->addChild(badgeBtn);
                }
                if (isLeaderboardMod && !menuRef->getChildByIDRecursive("GDX-leaderboard-badge:101"_spr)) {
                    auto badgeSpr = CCSprite::createWithSpriteFrameName("GDX_leaderboard_badge.png"_spr);
                    auto badgeBtn = CCMenuItemSpriteExtra::create(badgeSpr, menuRef, menu_selector(GDXHookProfilePage::onLeaderboardModBadge));
                    badgeBtn->setID("GDX-leaderboardmod-badge:101"_spr);
                    menuRef->addChild(badgeBtn);
                }
                menuRef->updateLayout();
            });

            co_return; });
    }

    void onContributorBadge(CCObject* sender) {
        gdx::onContributorBadge();
    }
    void onManagerBadge(CCObject* sender) {
        gdx::onManagerBadge();
    }
    void onLeaderboardModBadge(CCObject* sender) {
        gdx::onLeaderboardModBadge();
    }
};

class $modify(GDXHookCommentCell, CommentCell) {
    void loadFromComment(GJComment* p0) {
        CommentCell::loadFromComment(p0);
        auto layer = m_mainLayer;

        CCMenu* usernameMenu = static_cast<CCMenu*>(layer->getChildByIDRecursive("username-menu"));
        if (!usernameMenu) return;

        auto menuRef = Ref<CCMenu>(usernameMenu);
        int targetAccountId = p0 ? p0->m_accountID : 0;

        auto accountData = argon::getGameAccountData();
        auto url = std::string(gdx::baseApiUrl()) + "/getUser";
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
            bool isContributor = userData["isContributor"].asBool().unwrapOr(false);

            co_await geode::async::waitForMainThread([menuRef, isManager, isContributor]() {
                if (isManager && !menuRef->getChildByIDRecursive("GDX-manager-badge:101"_spr)) {
                    auto badgeSpr = CCSprite::createWithSpriteFrameName("GDX_manager_badge.png"_spr);
                    badgeSpr->setScale(0.7f);
                    auto badge = CCMenuItemSpriteExtra::create(badgeSpr, menuRef, menu_selector(GDXHookProfilePage::onManagerBadge));
                    badge->setID("GDX-manager-badge:101"_spr);
                    menuRef->addChild(badge);
                }
                if (isContributor && !menuRef->getChildByIDRecursive("GDX-contributor-badge:101"_spr)) {
                    auto badgeSpr = CCSprite::createWithSpriteFrameName("GDX_contributor_badge.png"_spr);
                    badgeSpr->setScale(0.7f);
                    auto badgeBtn = CCMenuItemSpriteExtra::create(badgeSpr, menuRef, menu_selector(GDXHookProfilePage::onContributorBadge));
                    badgeBtn->setID("GDX-contributor-badge:101"_spr);
                    menuRef->addChild(badgeBtn);
                }
                menuRef->updateLayout();
            });

            co_return; });
    }

    void onContributorBadge(CCObject* sender) {
        gdx::onContributorBadge();
    }
    void onManagerBadge(CCObject* sender) {
        gdx::onManagerBadge();
    }
};
