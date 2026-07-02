#pragma once

#include <argon/argon.hpp>
#include <arc/runtime/Runtime.hpp>
#include <unordered_map>

using namespace geode::prelude;

namespace gdx {
    constexpr std::string_view BASE_API_URL = "https://gdgauntletdeluxe.arcticwoof.xyz";

    inline std::string baseApiUrl() {
        if (Mod::get()->getSettingValue<bool>("enableLocalServer")) {
            return "http://127.0.0.1:1027";
        }
        return std::string(BASE_API_URL);
    }

    // since im tired to getting the argon token in multiple places, just make a helper function for it
    inline auto argonToken = [](const argon::AccountData& accountData) -> arc::Future<std::string> {
        auto authResult = co_await argon::startAuth(accountData);
        if (!authResult) {
            co_return std::string();
        }
        co_return std::move(authResult).unwrap();
    };

    inline std::string getResponseMessage(web::WebResponse const& response,
        std::string const& fallback) {
        auto message = response.string().unwrapOrDefault();
        if (!message.empty())
            return message;
        return fallback;
    }

    inline bool isManager() {
        return Mod::get()->getSavedValue<bool>("isManager");
    }

    inline bool isContributor() {
        return Mod::get()->getSavedValue<bool>("isContributor");
    }

    inline bool isLeaderboardMod() {
        return Mod::get()->getSavedValue<bool>("isLeaderboardMod");
    }

    inline extern const std::string featuredParticle = "30,2065,2,435,3,135,155,1,156,2,145,30a-1a3a0.3a1a90a90a0a0a70a100a0a0a4a0a0a0a24a1a0a0a1a0a0.823529a0a0.227451a0a1a0a0a1a0a190a1a0a0.996078a0a0.980392a0a1a0a0.4a0a0.3a0a0a0a0a0a0a0a0a2a1a0a0a0a27a0a0a0a0a0a0a0a0a0a0a0a0a0a0;";

    inline bool g_isPlayingGauntletLevel = false;
    inline bool g_isLocalPlayingGauntletLevel = false;

    inline void setPlayingGauntletLevel(bool playing) {
        log::debug("Setting playing gauntlet level to {}", playing);
        g_isPlayingGauntletLevel = playing;
    }

    inline bool isPlayingGauntletLevel() {
        return g_isPlayingGauntletLevel;
    }

    inline void setLocalPlayingGauntletLevel(bool playing) {
        log::debug("Setting local playing gauntlet level to {}", playing);
        g_isLocalPlayingGauntletLevel = playing;
    }

    inline bool isLocalPlayingGauntletLevel() {
        return g_isLocalPlayingGauntletLevel;
    }

    inline std::unordered_map<std::string, CCTexture2D*>& gauntletTextureCache() {
        static std::unordered_map<std::string, CCTexture2D*> cache;
        return cache;
    }

    inline CCTexture2D* findGauntletTexture(std::string const& url) {
        auto& cache = gauntletTextureCache();
        auto it = cache.find(url);
        return it == cache.end() ? nullptr : it->second;
    }

    inline void cacheGauntletTexture(std::string const& url, CCTexture2D* texture) {
        if (!texture) {
            return;
        }

        auto& cache = gauntletTextureCache();
        auto it = cache.find(url);
        if (it != cache.end()) {
            if (it->second == texture) {
                return;
            }
            if (it->second) {
                it->second->release();
            }
        }

        texture->retain();
        cache[url] = texture;
    }

    inline bool isHelperBotUserId(const std::string& id) {
        return id == "1521546288041299999";
    }

    inline void onManagerBadge() {
        FLAlertLayer::create("Gauntlet Manager", "This user has the same ability as <cl>contributor</c> but can also <cg>create online gauntlets</c> and <cc>create gauntlet tags</c> in <cr>Gauntlets Deluxe</c>.", "OK")->show();
    }

    inline void onContributorBadge() {
        FLAlertLayer::create("Gauntlet Contributor", "This user has the ability to <cl>edit created gauntlets</c>, <co>manage the leaderboard</c> is the one <cg>in charge of looking over gauntlet ideas</c> from <cy>other users</c> in <cr>Gauntlets Deluxe</c>.", "OK")->show();
    }

    inline void onLeaderboardModBadge() {
        FLAlertLayer::create("Gauntlet Leaderboard Mod", "This user has the ability to <cg>manage the leaderboards</c>, and <co>Ban/Unban users</c> in <cr>Gauntlets Deluxe</c>.", "OK")->show();
    }
}