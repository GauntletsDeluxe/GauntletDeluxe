#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/BasedButtonSprite.hpp>
#include <Geode/ui/Label.hpp>
#include <Geode/ui/LazySprite.hpp>
#include <Geode/ui/NineSlice.hpp>
#include <asp/fs.hpp>
#include <asp/time/SystemTime.hpp>
#include <dankmeme.argon/argon.hpp>
#include <arc/runtime/Runtime.hpp>
#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

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

    inline int g_playingGauntletLevelId = 0;
    inline int g_localPlayingGauntletLevelId = 0;

    inline void setPlayingGauntletLevel(bool playing, int levelId = 0) {
        log::debug("Setting playing gauntlet level to {} (level {})", playing, levelId);
        g_isPlayingGauntletLevel = playing;
        g_playingGauntletLevelId = playing ? levelId : 0;
    }

    // pass the level id to make sure the level being checked is the gauntlet level that was opened
    inline bool isPlayingGauntletLevel(int levelId = 0) {
        if (!g_isPlayingGauntletLevel) return false;
        return levelId == 0 || levelId == g_playingGauntletLevelId;
    }

    // the color scheme of the gauntlet whose level is being opened, used to tint the level page
    inline cocos2d::ccColor3B g_gauntletColor = {0, 102, 255};

    inline void setGauntletColor(cocos2d::ccColor3B const& color) {
        g_gauntletColor = color;
    }

    inline cocos2d::ccColor3B gauntletColor() {
        return g_gauntletColor;
    }

    inline void setLocalPlayingGauntletLevel(bool playing, int levelId = 0) {
        log::debug("Setting local playing gauntlet level to {} (level {})", playing, levelId);
        g_isLocalPlayingGauntletLevel = playing;
        g_localPlayingGauntletLevelId = playing ? levelId : 0;
    }

    inline bool isLocalPlayingGauntletLevel(int levelId = 0) {
        if (!g_isLocalPlayingGauntletLevel) return false;
        return levelId == 0 || levelId == g_localPlayingGauntletLevelId;
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

    // Save file helpers

    inline asp::fs::Path gauntletsSaveDir() {
        auto dir = dirs::getModsSaveDir() / Mod::get()->getID();
        if (auto res = asp::fs::createDirAll(dir); !res) {
            log::warn("Failed to create gauntlets save directory: {}", res.unwrapErr().message());
        }
        return dir;
    }

    inline asp::fs::Path getCompletedGauntletLevelsPath(bool local = false) {
        return gauntletsSaveDir() / (local ? "completed_gauntlet_levels_local.json" : "completed_gauntlet_levels.json");
    }

    inline asp::fs::Path getCompletedGauntletPath() {
        return gauntletsSaveDir() / "completed_gauntlets.json";
    }

    inline asp::fs::Path getLocalGauntletPath() {
        return gauntletsSaveDir() / "local_gauntlets.json";
    }

    inline asp::fs::Path completedFilePath(char const* name) {
        return gauntletsSaveDir() / name;
    }

    // Completed gauntlet levels

    inline std::unordered_set<int> loadCompletedGauntletLevels(bool local = false) {
        std::unordered_set<int> out;
        auto path = getCompletedGauntletLevelsPath(local);
        if (!path.isFile()) {
            return out;
        }

        auto content = asp::fs::readToString(path);
        if (!content) {
            log::warn("Failed to read completed gauntlet levels file: {}", content.unwrapErr());
            return out;
        }

        auto jsonResult = matjson::parse(content.unwrap());
        if (!jsonResult) {
            log::warn("Failed to parse completed gauntlet levels JSON: {}", jsonResult.unwrapErr());
            return out;
        }

        auto data = std::move(jsonResult).unwrap();
        if (data.isObject() && data["completedLevels"].isArray()) {
            data = data["completedLevels"];
        }
        if (!data.isArray()) {
            return out;
        }

        for (auto const& value : data) {
            if (!value.isNumber()) {
                continue;
            }
            auto maybeId = value.asInt();
            if (maybeId) {
                out.insert(static_cast<int>(maybeId.unwrap()));
            }
        }
        return out;
    }

    inline bool saveCompletedGauntletLevels(std::unordered_set<int> const& levels, bool local = false) {
        auto path = getCompletedGauntletLevelsPath(local);
        matjson::Value array = matjson::Value::array();
        for (auto levelId : levels) {
            array.push(levelId);
        }
        auto data = array.dump();
        auto res = asp::fs::write(path, data.c_str(), data.size());
        if (!res) {
            log::warn("Failed to save completed gauntlet levels: {}", res.unwrapErr());
        }
        return static_cast<bool>(res);
    }

    inline bool addCompletedGauntletLevel(int levelId, bool local = false) {
        auto levels = loadCompletedGauntletLevels(local);
        levels.insert(levelId);
        return saveCompletedGauntletLevels(levels, local);
    }

    inline bool hasCompletedGauntletLevel(int levelId) {
        return loadCompletedGauntletLevels(false).contains(levelId);
    }

    // Completed gauntlets

    inline std::unordered_set<int> loadCompletedGauntlets() {
        std::unordered_set<int> out;
        auto path = getCompletedGauntletPath();
        if (!path.isFile()) {
            return out;
        }

        auto content = asp::fs::readToString(path);
        if (!content) {
            log::warn("Failed to read completed gauntlets file: {}", content.unwrapErr());
            return out;
        }

        auto jsonResult = matjson::parse(content.unwrap());
        if (!jsonResult) {
            log::warn("Failed to parse completed gauntlets JSON: {}", jsonResult.unwrapErr());
            return out;
        }

        auto data = std::move(jsonResult).unwrap();
        if (data.isObject() && data["completedGauntlets"].isArray()) {
            data = data["completedGauntlets"];
        }
        if (!data.isArray()) {
            return out;
        }

        for (auto const& value : data) {
            if (!value.isNumber()) {
                continue;
            }
            auto maybeId = value.asInt();
            if (maybeId) {
                out.insert(static_cast<int>(maybeId.unwrap()));
            }
        }
        return out;
    }

    inline bool saveCompletedGauntlets(std::unordered_set<int> const& gauntlets) {
        auto path = getCompletedGauntletPath();
        matjson::Value array = matjson::Value::array();
        for (auto gauntletId : gauntlets) {
            array.push(gauntletId);
        }
        auto data = array.dump();
        auto res = asp::fs::write(path, data.c_str(), data.size());
        if (!res) {
            log::warn("Failed to save completed gauntlets: {}", res.unwrapErr());
        }
        return static_cast<bool>(res);
    }

    inline bool addCompletedGauntlet(int gauntletId) {
        auto gauntlets = loadCompletedGauntlets();
        gauntlets.insert(gauntletId);
        return saveCompletedGauntlets(gauntlets);
    }

    // Local gauntlets

    inline matjson::Value loadLocalGauntlets() {
        auto path = getLocalGauntletPath();
        if (!path.isFile()) {
            return matjson::Value::array();
        }

        auto content = asp::fs::readToString(path);
        if (!content) {
            log::warn("Failed to read local gauntlet file: {}", content.unwrapErr());
            return matjson::Value::array();
        }

        auto jsonResult = matjson::parse(content.unwrap());
        if (!jsonResult) {
            log::warn("Failed to parse local gauntlet JSON: {}", jsonResult.unwrapErr());
            return matjson::Value::array();
        }

        auto data = std::move(jsonResult).unwrap();
        if (data.isObject() && data["gauntlets"].isArray()) {
            return data["gauntlets"];
        }
        if (data.isArray()) {
            return data;
        }
        return matjson::Value::array();
    }

    inline bool saveLocalGauntlets(matjson::Value const& gauntlets) {
        auto path = getLocalGauntletPath();
        matjson::Value data = matjson::Value::array();
        if (gauntlets.isArray()) {
            for (auto const& item : gauntlets) {
                data.push(item);
            }
        }
        auto dumped = data.dump();
        auto result = asp::fs::write(path, dumped.c_str(), dumped.size());
        if (!result) {
            log::warn("Failed to save local gauntlets: {}", result.unwrapErr());
        }
        return static_cast<bool>(result);
    }

    // case-insensitive filter of a json array of objects by their "name" field
    inline matjson::Value filterJsonByName(matjson::Value const& items, std::string const& query) {
        if (!items.isArray()) {
            return matjson::Value::array();
        }
        if (query.empty()) {
            return items;
        }

        std::string lowerQuery = query;
        std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

        matjson::Value filtered = matjson::Value::array();
        for (auto const& item : items) {
            auto name = item["name"].asString().unwrapOr("");
            std::string lowerName = name;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            if (lowerName.find(lowerQuery) != std::string::npos) {
                filtered.push(item);
            }
        }
        return filtered;
    }

    inline matjson::Value filterLocalGauntlets(matjson::Value const& gauntlets, std::string const& query) {
        if (!gauntlets.isArray()) {
            return gauntlets;
        }
        return filterJsonByName(gauntlets, query);
    }

    inline matjson::Value filterGauntletsByName(matjson::Value const& gauntlets, std::string const& query) {
        return filterJsonByName(gauntlets, query);
    }

    inline matjson::Value filterTagsByName(matjson::Value const& tags, std::string const& query) {
        return filterJsonByName(tags, query);
    }

    // a gauntlet is identified by its id, falling back to its index like the rest of the layer
    inline int gauntletIdOf(matjson::Value const& gauntlet) {
        return gauntlet["id"].asInt().unwrapOr(gauntlet["index"].asInt().unwrapOr(-1));
    }

    inline matjson::Value filterFavoriteGauntlets(matjson::Value const& gauntlets, std::unordered_set<int> const& favorites) {
        matjson::Value filtered = matjson::Value::array();
        if (!gauntlets.isArray()) {
            return filtered;
        }
        for (auto const& gauntlet : gauntlets) {
            if (favorites.contains(gauntletIdOf(gauntlet))) {
                filtered.push(gauntlet);
            }
        }
        return filtered;
    }

    // Leaderboard sync helpers

    inline void insertIds(std::unordered_set<int>& out, matjson::Value const& data) {
        if (!data.isArray()) return;
        for (auto const& entry : data) {
            if (entry.isNumber()) {
                if (auto id = entry.asInt()) out.insert(static_cast<int>(id.unwrap()));
            } else if (entry.isString()) {
                if (auto id = numFromString<int>(entry.asString().unwrapOr(""))) out.insert(id.unwrap());
            }
        }
    }

    inline std::unordered_set<int> loadIds(char const* fileName, char const* objectKey) {
        std::unordered_set<int> out;
        auto path = completedFilePath(fileName);
        if (!path.isFile()) return out;

        auto content = asp::fs::readToString(path);
        if (!content) {
            log::warn("Failed to read {}: {}", fileName, content.unwrapErr());
            return out;
        }
        auto parsed = matjson::parse(content.unwrap());
        if (!parsed) {
            log::warn("Failed to parse {}: {}", fileName, parsed.unwrapErr());
            return out;
        }
        auto data = std::move(parsed).unwrap();
        if (data.isObject() && data[objectKey].isArray()) data = data[objectKey];
        insertIds(out, data);
        return out;
    }

    inline bool saveIds(char const* fileName, std::unordered_set<int> const& ids) {
        matjson::Value array = matjson::Value::array();
        for (auto id : ids) array.push(id);
        auto data = array.dump();
        auto res = asp::fs::write(completedFilePath(fileName), data.c_str(), data.size());
        if (!res) log::warn("Failed to save {}: {}", fileName, res.unwrapErr());
        return static_cast<bool>(res);
    }

    // Sprite helpers

    inline LazySprite* createLazySpriteWithFallback(CCNode* parent, std::string const& url, CCPoint const& position, CCSize const& size, int zOrder = 2) {
        auto sprite = LazySprite::create(size, false);
        if (!sprite) {
            return nullptr;
        }

        sprite->setID("gauntlet-image");
        sprite->setAutoResize(true);
        sprite->setPosition(position);

        auto fallbackShadow = CCSprite::createWithSpriteFrameName("GDX_gauntletUnknown.png"_spr);
        if (fallbackShadow) {
            fallbackShadow->setColor({0, 0, 0});
            fallbackShadow->setOpacity(50);
            if (fallbackShadow->getContentSize().width > 0 && fallbackShadow->getContentSize().height > 0) {
                const float fitScale = std::min(
                    size.width / fallbackShadow->getContentSize().width,
                    size.height / fallbackShadow->getContentSize().height);
                fallbackShadow->setScale(fitScale * 1.05f);
                fallbackShadow->setScaleY(fitScale + 0.1f);
            }
            fallbackShadow->setPosition(position);
            fallbackShadow->setPositionY(position.y - 10.f);
            if (parent) {
                parent->addChild(fallbackShadow, zOrder - 2);
            }
        }

        auto fallbackSprite = CCSprite::createWithSpriteFrameName("GDX_gauntletUnknown.png"_spr);
        if (fallbackSprite) {
            if (fallbackSprite->getContentSize().width > 0 && fallbackSprite->getContentSize().height > 0) {
                const float fitScale = std::min(
                    size.width / fallbackSprite->getContentSize().width,
                    size.height / fallbackSprite->getContentSize().height);
                fallbackSprite->setScale(fitScale);
            }
            fallbackSprite->setPosition(position);
            if (parent) {
                parent->addChild(fallbackSprite, zOrder - 1);
            }
        }

        sprite->setLoadCallback([fallbackSprite = Ref<CCSprite>(fallbackSprite), fallbackShadow = Ref<CCSprite>(fallbackShadow), url, sprite = Ref<LazySprite>(sprite), position, size, parent = Ref<CCNode>(parent), zOrder](Result<> const& result) {
            if (!result) {
                return;
            }
            if (fallbackSprite) {
                fallbackSprite->removeFromParent();
            }
            if (fallbackShadow) {
                fallbackShadow->removeFromParent();
            }
            if (auto texture = sprite->getTexture()) {
                cacheGauntletTexture(url, texture);
                if (sprite->getContentSize().width > 0 && sprite->getContentSize().height > 0) {
                    const float fitScale = std::min(
                        size.width / sprite->getContentSize().width,
                        size.height / sprite->getContentSize().height);
                    sprite->setScale(fitScale);
                }
                auto shadow = CCSprite::createWithTexture(texture);
                if (shadow) {
                    if (shadow->getContentSize().width > 0 && shadow->getContentSize().height > 0) {
                        const float fitScale = std::min(
                            size.width / shadow->getContentSize().width,
                            size.height / shadow->getContentSize().height);
                        shadow->setScale(fitScale);
                        shadow->setScaleY(fitScale + 0.1f);
                    }
                    shadow->setPosition(position);
                    shadow->setPositionY(position.y - 10.f);
                    shadow->setColor({0, 0, 0});
                    shadow->setOpacity(50);
                    if (parent) {
                        parent->addChild(shadow, zOrder - 2);
                    }
                }
            }
        });
        sprite->loadFromUrl(url, CCImage::kFmtPng, true);

        if (parent) {
            parent->addChild(sprite, zOrder);
        }
        return sprite;
    }

    inline LazySprite* createLocalSprite(CCNode* parent, asp::fs::Path const& path, CCPoint const& position, CCSize const& size, int zOrder = 2) {
        auto sprite = LazySprite::create(size, false);
        if (!sprite) {
            return nullptr;
        }

        sprite->setID("gauntlet-image");
        sprite->setAutoResize(true);
        sprite->setPosition(position);

        auto fallbackShadow = CCSprite::createWithSpriteFrameName("GDX_gauntletUnknown.png"_spr);
        if (fallbackShadow) {
            fallbackShadow->setColor({0, 0, 0});
            fallbackShadow->setOpacity(50);
            if (fallbackShadow->getContentSize().width > 0 && fallbackShadow->getContentSize().height > 0) {
                const float fitScale = std::min(
                    size.width / fallbackShadow->getContentSize().width,
                    size.height / fallbackShadow->getContentSize().height);
                fallbackShadow->setScale(fitScale * 1.05f);
                fallbackShadow->setScaleY(fitScale + 0.1f);
            }
            fallbackShadow->setPosition(position);
            fallbackShadow->setPositionY(position.y - 10.f);
            if (parent) {
                parent->addChild(fallbackShadow, zOrder - 2);
            }
        }

        auto fallbackSprite = CCSprite::createWithSpriteFrameName("GDX_gauntletUnknown.png"_spr);
        if (fallbackSprite) {
            if (fallbackSprite->getContentSize().width > 0 && fallbackSprite->getContentSize().height > 0) {
                const float fitScale = std::min(
                    size.width / fallbackSprite->getContentSize().width,
                    size.height / fallbackSprite->getContentSize().height);
                fallbackSprite->setScale(fitScale);
            }
            fallbackSprite->setPosition(position);
            if (parent) {
                parent->addChild(fallbackSprite, zOrder - 1);
            }
        }

        sprite->setLoadCallback([fallbackSprite = Ref<CCSprite>(fallbackSprite), fallbackShadow = Ref<CCSprite>(fallbackShadow), path, position, size, parent = Ref<CCNode>(parent), zOrder, sprite = Ref<LazySprite>(sprite)](Result<> const& result) {
            if (!result) {
                return;
            }
            if (fallbackSprite) {
                fallbackSprite->removeFromParent();
            }
            if (fallbackShadow) {
                fallbackShadow->removeFromParent();
            }
            if (auto texture = sprite->getTexture()) {
                if (texture->getName() != 0) {
                    auto shadow = CCSprite::createWithTexture(texture);
                    if (shadow) {
                        if (shadow->getContentSize().width > 0 && shadow->getContentSize().height > 0) {
                            const float fitScale = std::min(
                                size.width / shadow->getContentSize().width,
                                size.height / shadow->getContentSize().height);
                            shadow->setScale(fitScale);
                            shadow->setScaleY(fitScale + 0.1f);
                        }
                        shadow->setPosition(position);
                        shadow->setPositionY(position.y - 10.f);
                        shadow->setColor({0, 0, 0});
                        shadow->setOpacity(50);
                        if (parent) {
                            parent->addChild(shadow, zOrder - 2);
                        }
                    }
                }
            }
        });
        sprite->loadFromFile(path, CCImage::kFmtPng, true);

        if (parent) {
            parent->addChild(sprite, zOrder);
        }
        return sprite;
    }

    inline CCSprite* createRemoteSprite(CCNode* parent, std::string const& url, CCPoint const& position, CCSize const& size, int zOrder = 2) {
        if (auto texture = findGauntletTexture(url)) {
            if (texture->getName() != 0) {
                auto sprite = CCSprite::createWithTexture(texture);
                if (sprite) {
                    if (sprite->getContentSize().width > 0 && sprite->getContentSize().height > 0) {
                        const float scale = std::min(
                            size.width / sprite->getContentSize().width,
                            size.height / sprite->getContentSize().height);
                        sprite->setScale(scale);
                    }
                    sprite->setPosition(position);
                    if (parent) {
                        parent->addChild(sprite, zOrder);
                    }

                    auto shadow = CCSprite::createWithTexture(texture);
                    if (shadow) {
                        if (shadow->getContentSize().width > 0 && shadow->getContentSize().height > 0) {
                            const float scale = std::min(
                                size.width / shadow->getContentSize().width,
                                size.height / shadow->getContentSize().height);
                            shadow->setScale(scale);
                            shadow->setScaleY(scale + 0.1f);
                        }
                        shadow->setPosition(position);
                        shadow->setPositionY(position.y - 10.f);
                        shadow->setColor({0, 0, 0});
                        shadow->setOpacity(50);
                        if (parent) {
                            parent->addChild(shadow, zOrder - 2);
                        }
                    }

                    return sprite;
                }
            }
        }
        return createLazySpriteWithFallback(parent, url, position, size, zOrder);
    }

    inline CCSprite* createCachedGauntletSprite(CCNode* parent, std::string const& url, CCPoint const& position, CCSize const& size, int zOrder = 2) {
        if (!parent) {
            return nullptr;
        }

        if (auto texture = findGauntletTexture(url)) {
            auto sprite = CCSprite::createWithTexture(texture);
            if (!sprite) {
                return nullptr;
            }
            sprite->setID("gauntlet-image");
            if (sprite->getContentSize().width > 0 && sprite->getContentSize().height > 0) {
                const float fitScale = std::min(size.width / sprite->getContentSize().width, size.height / sprite->getContentSize().height);
                sprite->setScale(fitScale);
            }
            sprite->setPosition(position);
            parent->addChild(sprite, zOrder);
            return sprite;
        }

        auto sprite = LazySprite::create(size, false);
        if (!sprite) {
            return nullptr;
        }
        sprite->setID("gauntlet-image");
        sprite->setAutoResize(true);
        sprite->setPosition(position);
        sprite->setLoadCallback([sprite, url](Result<> const& result) {
            if (!result) {
                return;
            }
            if (auto texture = sprite->getTexture()) {
                cacheGauntletTexture(url, texture);
            }
        });
        sprite->loadFromUrl(url, CCImage::kFmtPng, true);
        parent->addChild(sprite, zOrder);
        return sprite;
    }

    inline CCSprite* createRecentToggleSprite(bool on) {
        return CircleButtonSprite::createWithSpriteFrameName(
            "GDX_recentGauntlet.png"_spr,
            1.2f,
            on ? CircleBaseColor::Cyan : CircleBaseColor::Gray,
            CircleBaseSize::Small);
    }

    inline CCSprite* createFavoriteToggleSprite(bool on) {
        return CircleButtonSprite::createWithSpriteFrameName(
            on ? "gj_heartOn_001.png" : "gj_heartOff_001.png",
            1.f,
            on ? CircleBaseColor::Cyan : CircleBaseColor::Gray,
            CircleBaseSize::Small);
    }

    // same sprite the favorite filter toggle on the gauntlet list uses, so the two read as one feature
    inline CCSprite* createFavoriteSprite(bool on) {
        return CircleButtonSprite::createWithSpriteFrameName(
            on ? "gj_heartOn_001.png" : "gj_heartOff_001.png",
            0.85f,
            on ? CircleBaseColor::Cyan : CircleBaseColor::Gray,
            CircleBaseSize::Small);
    }

    inline CCSprite* createLocalToggleSprite(bool on) {
        return CircleButtonSprite::createWithSpriteFrameName(
            "GDX_localGauntlet.png"_spr,
            1.2f,
            on ? CircleBaseColor::Cyan : CircleBaseColor::Gray,
            CircleBaseSize::Small);
    }

    inline cocos2d::CCNode* createGauntletCreditsTagCell(matjson::Value const& tag) {
        std::string tagName = tag["name"].asString().unwrapOr("Tag");
        if (tagName.empty()) {
            tagName = "Tag";
        }

        // @geode-ignore(unknown-resource)
        auto tagCell = NineSlice::createWithSpriteFrameName("geode.loader/tab-bg.png");
        if (!tagCell) {
            return nullptr;
        }

        auto r = static_cast<GLubyte>(tag["r"].asInt().unwrapOr(255));
        auto g = static_cast<GLubyte>(tag["g"].asInt().unwrapOr(255));
        auto b = static_cast<GLubyte>(tag["b"].asInt().unwrapOr(255));
        tagCell->setColor({r, g, b});
        tagCell->setOpacity(255);

        auto tagLabel = Label::create(tagName, "bigFont.fnt");
        if (tagLabel) {
            tagLabel->setScale(0.4f);
            tagLabel->setAnchorPoint({0.5f, 0.5f});
            tagLabel->setColor({255, 255, 255});
            tagCell->setContentSize({tagLabel->getScaledContentSize().width + 10.f, tagLabel->getScaledContentSize().height + 10.f});
            tagCell->addChildAtPosition(tagLabel, Anchor::Center, {0.f, 0.f}, false);
        }

        return tagCell;
    }

    // UI / misc helpers

    inline CCNode* findChildByIDRecursive(CCNode* node, std::string const& id) {
        if (!node) {
            return nullptr;
        }
        if (node->getID() == id) {
            return node;
        }
        auto children = node->getChildren();
        if (!children) {
            return nullptr;
        }
        for (auto i = 0u; i < children->count(); ++i) {
            auto child = static_cast<CCNode*>(children->objectAtIndex(i));
            if (!child) {
                continue;
            }
            if (auto found = findChildByIDRecursive(child, id)) {
                return found;
            }
        }
        return nullptr;
    }

    // works with any container whose elements expose a `levelId` field
    template <typename LevelContainer>
    inline bool isLevelUnlocked(std::size_t index, LevelContainer const& levels, std::unordered_set<int> const& completedLevels) {
        if (index >= levels.size()) {
            return false;
        }
        if (completedLevels.contains(levels[index].levelId)) {
            return true;
        }
        if (index == 0) {
            return true;
        }
        for (std::size_t j = 0; j < index; ++j) {
            if (!completedLevels.contains(levels[j].levelId)) {
                return false;
            }
        }
        return true;
    }

    inline std::string getStringFromDict(CCDictionary* dict, char const* key) {
        if (!dict) {
            return "";
        }
        auto value = static_cast<CCString*>(dict->objectForKey(key));
        return value ? value->getCString() : std::string();
    }

    inline int getIntFromDict(CCDictionary* dict, char const* key) {
        if (!dict) {
            return 0;
        }
        auto value = static_cast<CCInteger*>(dict->objectForKey(key));
        return value ? value->getValue() : 0;
    }

    inline std::string formatBackgroundName(int bgIndex) {
        int index = std::clamp(bgIndex, 1, 59);
        return fmt::format("game_bg_{:02d}_001.png", index);
    }

    inline std::string getBgIconSpriteName(int index) {
        int bgIndex = std::clamp(index, 1, 59);
        return fmt::format("bgIcon_{:02d}_001.png", bgIndex);
    }

    inline std::string urlEncode(std::string const& value) {
        static const char* hex = "0123456789ABCDEF";
        std::string encoded;
        encoded.reserve(value.size() * 3);
        for (unsigned char c : value) {
            if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                encoded.push_back(c);
            } else {
                encoded.push_back('%');
                encoded.push_back(hex[c >> 4]);
                encoded.push_back(hex[c & 0xF]);
            }
        }
        return encoded;
    }

    inline double nowSeconds() {
        return static_cast<double>(asp::time::SystemTime::now().timeSinceEpoch().micros()) / 1'000'000.0;
    }

    // Comic RSS / HTML helpers (this is just ass but honestly probs gonna refactor this when i get around to adding another param for the backend and such :v)

    inline std::string replaceAll(std::string str, std::string_view from, std::string_view to) {
        size_t pos = 0;
        while ((pos = str.find(from, pos)) != std::string::npos) {
            str.replace(pos, from.size(), to);
            pos += to.size();
        }
        return str;
    }

    inline std::string decodeEntities(std::string str) {
        str = replaceAll(std::move(str), "&lt;", "<");
        str = replaceAll(std::move(str), "&gt;", ">");
        str = replaceAll(std::move(str), "&quot;", "\"");
        str = replaceAll(std::move(str), "&apos;", "'");
        return replaceAll(std::move(str), "&amp;", "&");
    }

    // text between <tag> and </tag> in the first match, CDATA unwrapped
    inline std::string tagContent(std::string_view src, std::string_view tag) {
        auto open = fmt::format("<{}>", tag);
        auto close = fmt::format("</{}>", tag);
        auto start = src.find(open);
        if (start == std::string_view::npos) return "";
        start += open.size();
        auto end = src.find(close, start);
        if (end == std::string_view::npos) return "";

        auto content = src.substr(start, end - start);
        constexpr std::string_view cdataOpen = "<![CDATA[";
        constexpr std::string_view cdataClose = "]]>";
        if (content.starts_with(cdataOpen) && content.ends_with(cdataClose)) {
            return std::string(content.substr(cdataOpen.size(), content.size() - cdataOpen.size() - cdataClose.size()));
        }
        return decodeEntities(std::string(content));
    }

    // the headings of a description, which carry the comic's own text
    inline std::string descriptionText(std::string_view html) {
        std::string out;
        size_t pos = 0;
        while (pos < html.size()) {
            auto end = html.find("</", pos);
            // a block ends at its closing tag, and the closing tag itself is skipped
            auto block = html.substr(pos, end == std::string_view::npos ? std::string_view::npos : end - pos);
            pos = end == std::string_view::npos ? html.size() : html.find('>', end);
            if (pos == std::string_view::npos)
                pos = html.size();
            else
                ++pos;

            // only the headings (<h1>..<h6>) are wanted, not the paragraphs
            auto heading = block.find("<h");
            if (heading == std::string_view::npos || heading + 2 >= block.size() || block[heading + 2] < '1' || block[heading + 2] > '6') continue;

            std::string text;
            bool inTag = false;
            for (char c : block) {
                if (c == '<')
                    inTag = true;
                else if (c == '>')
                    inTag = false;
                else if (!inTag)
                    text += c;
            }
            text = decodeEntities(std::move(text));

            // discord mentions like <@&123> would read as rich text tags
            for (size_t at; (at = text.find("<@")) != std::string::npos;) {
                auto close = text.find('>', at);
                text.erase(at, close == std::string::npos ? std::string::npos : close - at + 1);
            }
            text = replaceAll(replaceAll(std::move(text), "<", ""), ">", "");

            auto first = text.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) continue;
            auto last = text.find_last_not_of(" \t\r\n");
            if (!out.empty()) out += "\n\n";
            out += text.substr(first, last - first + 1);
        }
        return out;
    }

    // value of the first `attr="..."` after the first occurrence of `marker`
    inline std::string attrAfter(std::string_view src, std::string_view marker, std::string_view attr) {
        auto at = src.find(marker);
        if (at == std::string_view::npos) return "";
        auto key = fmt::format("{}=\"", attr);
        auto start = src.find(key, at);
        if (start == std::string_view::npos) return "";
        start += key.size();
        auto end = src.find('"', start);
        if (end == std::string_view::npos) return "";
        return std::string(src.substr(start, end - start));
    }

    // Favorite gauntlets are stored as a plain json array of gauntlet ids in favorite_gauntlet.json.
    inline asp::fs::Path favoriteGauntletsPath() {
        auto dir = dirs::getModsSaveDir() / Mod::get()->getID();
        if (auto res = asp::fs::createDirAll(dir); !res) {
            log::warn("Failed to create favorites save directory: {}", res.unwrapErr().message());
        }
        return dir / "favorite_gauntlet.json";
    }

    inline std::unordered_set<int> loadFavoriteGauntlets() {
        std::unordered_set<int> out;
        auto path = favoriteGauntletsPath();
        if (!path.isFile()) {
            return out;
        }

        auto content = asp::fs::readToString(path);
        if (!content) {
            log::warn("Failed to read favorite gauntlets file: {}", content.unwrapErr());
            return out;
        }

        auto parsed = matjson::parse(content.unwrap());
        if (!parsed) {
            log::warn("Failed to parse favorite gauntlets JSON: {}", parsed.unwrapErr());
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
            log::warn("Failed to save favorite gauntlets: {}", res.unwrapErr());
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