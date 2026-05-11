#include <Geode/Geode.hpp>
#include <Geode/binding/GJListLayer.hpp>
#include <Geode/modify/EndLevelLayer.hpp>
#include "../include/GDXConstant.hpp"
#include "ccTypes.h"
#include <Geode/utils/file.hpp>
#include <Geode/utils/web.hpp>
#include <asp/fs.hpp>
#include <argon/argon.hpp>
#include <arc/runtime/Runtime.hpp>

using namespace geode::prelude;

namespace {
    static asp::fs::path getCompletedGauntletLevelsPath() {
        auto dir = geode::dirs::getModsSaveDir() / geode::Mod::get()->getID();
        if (auto res = asp::fs::createDirAll(dir); !res) {
            log::warn("Failed to create completed levels save directory: {}", res.unwrapErr().message());
        }
        return dir / "completed_gauntlet_levels.json";
    }

    static std::vector<int> loadCompletedGauntletLevels() {
        std::vector<int> out;
        auto path = getCompletedGauntletLevelsPath();
        if (!asp::fs::isFile(path).unwrapOr(false)) {
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
                out.push_back(static_cast<int>(maybeId.unwrap()));
            }
        }
        return out;
    }

    static bool saveCompletedGauntletLevels(std::vector<int> const& levels) {
        auto path = getCompletedGauntletLevelsPath();
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

    static void addCompletedGauntletLevel(int levelId) {
        auto levels = loadCompletedGauntletLevels();
        bool hasLevel = false;
        for (auto existingLevelId : levels) {
            if (existingLevelId == levelId) {
                hasLevel = true;
                break;
            }
        }
        if (!hasLevel) {
            levels.push_back(levelId);
            saveCompletedGauntletLevels(levels);
        }
    }
}

class $modify(GDXEndLevelLayer, EndLevelLayer) {
    struct Fields {
        geode::async::TaskHolder<> m_requestTask;
    };

    void customSetup() override {
        EndLevelLayer::customSetup();
        log::debug("{} completed", m_playLayer->m_level->m_levelID);
        if (m_playLayer->m_level->m_normalPercent != 100) return;
        // get the summaryContainer
        if (this->m_listLayer) {
            auto accountData = argon::getGameAccountData();
            int levelId = static_cast<int>(this->m_playLayer->m_level->m_levelID);
            auto url = std::string(gdx::BASE_API_URL) + "/completeLevel";
            matjson::Value body = matjson::Value::object();
            body["accountId"] = accountData.accountId;
            body["argonToken"] = "";
            body["levelId"] = static_cast<int>(levelId);
            body["attempts"] = static_cast<int>(this->m_playLayer->m_level->m_attempts);
            body["time"] = static_cast<int>(this->m_playLayer->m_level->m_attemptTime);
            body["jumps"] = static_cast<int>(this->m_playLayer->m_level->m_jumps);

            m_fields->m_requestTask.spawn([this, url = std::move(url), body = std::move(body), accountData = std::move(accountData), levelId]() mutable -> arc::Future<> {
                auto token = co_await gdx::argonToken(accountData);
                if (token.empty()) {
                    co_return;
                }

                body["argonToken"] = std::move(token);
                auto response = co_await geode::utils::web::WebRequest()
                                    .url(url)
                                    .header("Content-Type", "application/json")
                                    .bodyJSON(body)
                                    .post(url);
                if (response.error() || response.cancelled() || !response.ok()) {
                    co_return;
                }

                auto jsonResult = response.json();
                if (!jsonResult) {
                    co_return;
                }

                auto result = std::move(jsonResult).unwrap();
                if (!result["success"].asBool().unwrapOr(false)) {
                    co_return;
                }

                auto rewardValue = result["reward"].asInt().unwrapOr(0);
                auto levelPointsValue = result["levelPoints"].asInt().unwrapOr(0);
                co_await geode::async::waitForMainThread([this, rewardValue, levelPointsValue, levelId] {
                    auto completedLevels = loadCompletedGauntletLevels();
                    bool hasLevel = false;
                    for (auto existingLevelId : completedLevels) {
                        if (existingLevelId == levelId) {
                            hasLevel = true;
                            break;
                        }
                    }
                    if (!hasLevel) {
                        completedLevels.push_back(levelId);
                        saveCompletedGauntletLevels(completedLevels);
                    }

                    auto gauntletRewardNode = CCNodeRGBA::create();
                    gauntletRewardNode->setContentHeight(45.f);
                    gauntletRewardNode->setScale(1.8f);
                    gauntletRewardNode->setOpacity(0);
                    gauntletRewardNode->setID("gauntlet-reward-node");
                    auto rewardSpr = CCSprite::createWithSpriteFrameName("GDX_levelPoint.png"_spr);
                    if (rewardSpr) {
                        rewardSpr->setScale(0.4f);
                        rewardSpr->setAnchorPoint({0.f, 0.5f});
                        rewardSpr->setPosition({5.f, gauntletRewardNode->getContentSize().height * 0.65f});
                        gauntletRewardNode->addChild(rewardSpr);
                    }

                    auto rewardText = std::string("+" + numToString(rewardValue));
                    auto rewardLabel = CCLabelBMFont::create(rewardText.c_str(), "bigFont.fnt");
                    if (rewardLabel) {
                        rewardLabel->setAnchorPoint({1.f, 0.5f});
                        rewardLabel->setScale(0.8f);
                        rewardLabel->setPosition({0.f, gauntletRewardNode->getContentSize().height * 0.65f});
                        gauntletRewardNode->addChild(rewardLabel);
                    }

                    auto levelPointsText = std::string("(" + numToString(levelPointsValue) + ")");
                    auto levelPointsLabel = CCLabelBMFont::create(levelPointsText.c_str(), "bigFont.fnt");
                    if (levelPointsLabel) {
                        levelPointsLabel->setAnchorPoint({1.f, 0.5f});
                        levelPointsLabel->setScale(0.4f);
                        levelPointsLabel->setColor({255, 115, 255});
                        levelPointsLabel->setPosition({0.f, gauntletRewardNode->getContentSize().height * 0.25f});
                        gauntletRewardNode->addChild(levelPointsLabel);
                    }

                    gauntletRewardNode->setPosition({55.f, m_listLayer->getContentSize().height / 2 - 15});
                    m_listLayer->addChild(gauntletRewardNode);

                    auto scaleAction = CCScaleBy::create(.6f, .8f);
                    auto bounceAction = CCEaseBounceOut::create(scaleAction);
                    auto fadeAction = CCFadeIn::create(0.6f);
                    auto spawnAction = CCSpawn::createWithTwoActions(bounceAction, fadeAction);
                    gauntletRewardNode->setScale(1.f);
                    gauntletRewardNode->runAction(spawnAction);
                    // @geode-ignore(unknown-resource)
                    FMODAudioEngine::sharedEngine()->playEffect("gold02.ogg");
                    auto circleWave = CCCircleWave::create(10.f, 110.f, 0.5f, false);
                    circleWave->setPosition(gauntletRewardNode->getPosition() + ccp(0.f, 10.f));
                    circleWave->m_color = ccColor3B({191, 3, 226});
                    m_listLayer->addChild(circleWave);
                });
                co_return; }, []() {});
        }
    }
};