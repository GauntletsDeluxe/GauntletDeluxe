#include <Geode/Geode.hpp>
#include "../include/GDXConstant.hpp"
#include <Geode/cocos/sprite_nodes/CCSprite.h>
#include <argon/argon.hpp>
#include <alphalaneous.badgify/include/Badgify.hpp>

using namespace geode::prelude;

namespace {
    struct UserRoles {
        bool isManager = false;
        bool isContributor = false;
        bool isLeaderboardMod = false;
        bool loaded = false;
    };

    void fetchUserRoles(int targetAccountId, geode::Function<void(const UserRoles&)> callback) {
        static std::unordered_map<int, UserRoles> s_roleCache;
        static std::unordered_map<int, std::vector<geode::Function<void(const UserRoles&)>>> s_pending;

        if (auto it = s_roleCache.find(targetAccountId); it != s_roleCache.end() && it->second.loaded) {
            callback(it->second);
            return;
        }

        s_pending[targetAccountId].push_back(std::move(callback));
        if (s_pending[targetAccountId].size() > 1) {
            return;
        }

        auto accountData = argon::getGameAccountData();
        auto url = std::string(gdx::baseApiUrl()) + "/getUser";
        matjson::Value body = matjson::Value::object();
        body["accountId"] = accountData.accountId;
        body["targetAccountId"] = targetAccountId;

        geode::async::spawn([targetAccountId, url = std::move(url), body = std::move(body), accountData]() mutable -> arc::Future<> {
            auto token = co_await gdx::argonToken(accountData);
            if (token.empty()) {
                co_await geode::async::waitForMainThread([targetAccountId]() {
                    auto pending = std::move(s_pending[targetAccountId]);
                    s_pending.erase(targetAccountId);
                    for (auto& cb : pending) cb(UserRoles{});
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
                co_await geode::async::waitForMainThread([targetAccountId]() {
                    auto pending = std::move(s_pending[targetAccountId]);
                    s_pending.erase(targetAccountId);
                    for (auto& cb : pending) cb(UserRoles{});
                });
                co_return;
            }

            auto jsonResult = response.json();
            if (!jsonResult) {
                co_await geode::async::waitForMainThread([targetAccountId]() {
                    auto pending = std::move(s_pending[targetAccountId]);
                    s_pending.erase(targetAccountId);
                    for (auto& cb : pending) cb(UserRoles{});
                });
                co_return;
            }

            auto userData = std::move(jsonResult).unwrap();
            if (!userData.isObject()) {
                co_await geode::async::waitForMainThread([targetAccountId]() {
                    auto pending = std::move(s_pending[targetAccountId]);
                    s_pending.erase(targetAccountId);
                    for (auto& cb : pending) cb(UserRoles{});
                });
                co_return;
            }

            UserRoles roles;
            roles.isManager = userData["isManager"].asBool().unwrapOr(false);
            roles.isContributor = userData["isContributor"].asBool().unwrapOr(false);
            roles.isLeaderboardMod = userData["isLeaderboardMod"].asBool().unwrapOr(false);
            roles.loaded = true;

            co_await geode::async::waitForMainThread([targetAccountId, roles]() {
                s_roleCache[targetAccountId] = roles;
                auto pending = std::move(s_pending[targetAccountId]);
                s_pending.erase(targetAccountId);
                for (auto& cb : pending) {
                    cb(roles);
                }
            });

            co_return;
        });
    }
}

$execute {
    alpha::badgify::registerBadge(
        "GDX-manager-badge"_spr,
        "Gauntlet Manager",
        "This user has the same ability as <cl>contributor</c> but can also <cg>create online gauntlets</c> and <cc>create gauntlet tags</c> in <cr>Gauntlets Deluxe</c>.",
        [](const alpha::badgify::Badge& badge) {
            int targetAccountId = badge.user ? badge.user->m_accountID : 0;
            if (targetAccountId == 0) return;

            fetchUserRoles(targetAccountId, [badge](const UserRoles& roles) {
                if (roles.isManager) {
                    alpha::badgify::showBadge(badge, CCSprite::createWithSpriteFrameName("GDX_manager_badge.png"_spr));
                }
            });
        });

    alpha::badgify::registerBadge(
        "GDX-contributor-badge"_spr,
        "Gauntlet Contributor",
        "This user has the ability to <cl>edit created gauntlets</c>, <co>manage the leaderboard</c> is the one <cg>in charge of looking over gauntlet ideas</c> from <cy>other users</c> in <cr>Gauntlets Deluxe</c>.",
        [](const alpha::badgify::Badge& badge) {
            int targetAccountId = badge.user ? badge.user->m_accountID : 0;
            if (targetAccountId == 0) return;

            fetchUserRoles(targetAccountId, [badge](const UserRoles& roles) {
                if (roles.isContributor) {
                    alpha::badgify::showBadge(badge, CCSprite::createWithSpriteFrameName("GDX_contributor_badge.png"_spr));
                }
            });
        });

    alpha::badgify::registerBadge(
        "GDX-leaderboardmod-badge"_spr,
        "Gauntlet Leaderboard Mod",
        "This user has the ability to <cg>manage the leaderboards</c>, and <co>Ban/Unban users</c> in <cr>Gauntlets Deluxe</c>.",
        [](const alpha::badgify::Badge& badge) {
            int targetAccountId = badge.user ? badge.user->m_accountID : 0;
            if (targetAccountId == 0) return;

            fetchUserRoles(targetAccountId, [badge](const UserRoles& roles) {
                if (roles.isLeaderboardMod) {
                    alpha::badgify::showBadge(badge, CCSprite::createWithSpriteFrameName("GDX_leaderboard_badge.png"_spr));
                }
            });
        });
}
