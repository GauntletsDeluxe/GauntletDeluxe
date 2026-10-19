#pragma once

#include <Geode/Geode.hpp>
#include <asp/fs.hpp>
#include <unordered_set>

// Favorite gauntlets are stored as a plain json array of gauntlet ids in favorite_gauntlet.json.
namespace gdx {
    inline asp::fs::path favoriteGauntletsPath() {
        auto dir = geode::dirs::getModsSaveDir() / geode::Mod::get()->getID();
        if (auto res = asp::fs::createDirAll(dir); !res) {
            geode::log::warn("Failed to create favorites save directory: {}", res.unwrapErr().message());
        }
        return dir / "favorite_gauntlet.json";
    }

    inline std::unordered_set<int> loadFavoriteGauntlets() {
        std::unordered_set<int> out;
        auto path = favoriteGauntletsPath();
        if (!asp::fs::isFile(path).unwrapOr(false)) {
            return out;
        }

        auto content = asp::fs::readToString(path);
        if (!content) {
            geode::log::warn("Failed to read favorite gauntlets file: {}", content.unwrapErr());
            return out;
        }

        auto parsed = matjson::parse(content.unwrap());
        if (!parsed) {
            geode::log::warn("Failed to parse favorite gauntlets JSON: {}", parsed.unwrapErr());
            return out;
        }

        auto data = std::move(parsed).unwrap();
        if (!data.isArray()) {
            return out;
        }
        for (auto const& value : data) {
            if (!value.isNumber()) continue;
            if (auto id = value.asInt()) out.insert(static_cast<int>(id.unwrap()));
        }
        return out;
    }

    inline bool saveFavoriteGauntlets(std::unordered_set<int> const& favorites) {
        matjson::Value array = matjson::Value::array();
        for (auto id : favorites) {
            array.push(id);
        }
        auto data = array.dump();
        auto res = asp::fs::write(favoriteGauntletsPath(), data.c_str(), data.size());
        if (!res) {
            geode::log::warn("Failed to save favorite gauntlets: {}", res.unwrapErr());
        }
        return static_cast<bool>(res);
    }

    inline bool isFavoriteGauntlet(int gauntletId) {
        return loadFavoriteGauntlets().contains(gauntletId);
    }

    // flips the favorite state of a gauntlet and returns the new state
    inline bool toggleFavoriteGauntlet(int gauntletId) {
        auto favorites = loadFavoriteGauntlets();
        bool nowFavorite = !favorites.contains(gauntletId);
        if (nowFavorite) favorites.insert(gauntletId);
        else favorites.erase(gauntletId);
        saveFavoriteGauntlets(favorites);
        return nowFavorite;
    }
}
