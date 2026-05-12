#pragma once

#include <argon/argon.hpp>
#include <arc/runtime/Runtime.hpp>
#include <unordered_map>

using namespace geode::prelude;

namespace gdx {
    constexpr std::string_view BASE_API_URL = "https://gdgauntletdeluxe.arcticwoof.xyz";

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

    inline bool isMod() {
        return Mod::get()->getSavedValue<bool>("isMod");
    }

    inline bool g_isPlayingGauntletLevel = false;

    inline void setPlayingGauntletLevel(bool playing) {
        log::debug("Setting playing gauntlet level to {}", playing);
        g_isPlayingGauntletLevel = playing;
    }

    inline bool isPlayingGauntletLevel() {
        return g_isPlayingGauntletLevel;
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

    inline void onManagerBadge() {
        FLAlertLayer::create("Gauntlet Manager", "This user has the ability to <cg>create gauntlets</c> and <co>manage the leaderboard</c> in <cr>Gauntlets Deluxe</c>.", "OK")->show();
    }

    inline void onContributorBadge() {
        FLAlertLayer::create("Gauntlet Contributor", "This user has the ability to <cl>edit created gauntlets</c> and is the one <cg>in charge of looking over gauntlet ideas</c> from <cy>other users</c> in <cr>Gauntlets Deluxe</c>.", "OK")->show();
    }
}