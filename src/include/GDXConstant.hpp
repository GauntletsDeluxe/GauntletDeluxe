#pragma once

using namespace geode::prelude;

namespace gdx {
    // Base URL for all Rated Layouts API endpoints.
    constexpr std::string_view BASE_API_URL = "https://gdgauntletdeluxe.arcticwoof.xyz";

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
}