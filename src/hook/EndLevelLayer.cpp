#include <Geode/Geode.hpp>
#include <Geode/binding/GJListLayer.hpp>
#include <Geode/modify/EndLevelLayer.hpp>
#include <Geode/ui/Label.hpp>
#include "../include/GDXConstant.hpp"
#include "ccTypes.h"
#include <Geode/utils/file.hpp>
#include <Geode/utils/web.hpp>
#include <asp/fs.hpp>
#include <dankmeme.argon/argon.hpp>
#include <arc/runtime/Runtime.hpp>

using namespace geode::prelude;
using namespace gdx;

class $modify(GDXEndLevelLayer, EndLevelLayer) {
    struct Fields {
        async::TaskHolder<> m_requestTask;
    };

    void customSetup() override {
        EndLevelLayer::customSetup();
        if (m_playLayer->m_isPracticeMode) return;
        int playedLevelId = static_cast<int>(m_playLayer->m_level->m_levelID);
        if (gdx::isLocalPlayingGauntletLevel(playedLevelId)) {
            if (m_playLayer->m_level->m_normalPercent >= 100) {
                log::debug("local gauntlet level {} completed", m_playLayer->m_level->m_levelID);
                if (this->m_listLayer) {
                    int levelId = static_cast<int>(this->m_playLayer->m_level->m_levelID);
                    auto completedLevels = loadCompletedGauntletLevels(true);
                    if (!completedLevels.contains(levelId)) {
                        completedLevels.insert(levelId);
                        saveCompletedGauntletLevels(completedLevels, true);

                        auto completedIcon = CCSprite::createWithSpriteFrameName("GJ_completesIcon_001.png");
                        if (completedIcon) {
                            completedIcon->setPosition({55.f, this->m_listLayer->getContentSize().height / 2.f});
                            completedIcon->setScale(1.4f);
                            completedIcon->setOpacity(0);
                            this->m_listLayer->addChild(completedIcon, 3);

                            auto scaleAction = CCScaleBy::create(.6f, .8f);
                            auto bounceAction = CCEaseBounceOut::create(scaleAction);
                            auto fadeAction = CCFadeIn::create(0.6f);
                            auto spawnAction = CCSpawn::createWithTwoActions(bounceAction, fadeAction);
                            completedIcon->runAction(spawnAction);
                        }

                        Notification::create("Local gauntlet level completed!", NotificationIcon::Success)->show();
                        // @geode-ignore(unknown-resource)
                        FMODAudioEngine::sharedEngine()->playEffect("gold02.ogg");
                        auto circleWave = CCCircleWave::create(10.f, 110.f, 0.5f, false);
                        circleWave->setPosition({55.f, this->m_listLayer->getContentSize().height / 2.f});
                        circleWave->m_color = ccColor3B({110, 210, 40});
                        this->m_listLayer->addChild(circleWave, 3);
                    }
                }
            }
            gdx::setLocalPlayingGauntletLevel(false);
            return;
        }

        if (!gdx::isPlayingGauntletLevel(playedLevelId)) {
            log::debug("Not a gauntlet level, skipping completion check");
            return;
        }

        if (m_playLayer->m_level->m_normalPercent >= 100) {
            log::debug("gauntlet level {} completed", m_playLayer->m_level->m_levelID);
            // get the summaryContainer
            if (this->m_listLayer) {
                auto accountData = argon::getGameAccountData();
                int levelId = static_cast<int>(this->m_playLayer->m_level->m_levelID);
                auto url = std::string(gdx::baseApiUrl()) + "/completeLevel";
                matjson::Value body = matjson::Value::object();
                body["accountId"] = accountData.accountId;
                body["argonToken"] = "";
                body["levelId"] = static_cast<int>(levelId);
                body["attempts"] = static_cast<int>(this->m_playLayer->m_level->m_attempts);
                body["time"] = static_cast<int>(this->m_playLayer->m_level->m_attemptTime);
                body["jumps"] = static_cast<int>(this->m_playLayer->m_level->m_jumps);

                auto self = Ref<EndLevelLayer>(this);
                m_fields->m_requestTask.spawn([self = std::move(self), url = std::move(url), body = std::move(body), accountData = std::move(accountData), levelId]() mutable -> arc::Future<> {
                auto token = co_await gdx::argonToken(accountData);
                if (token.empty()) {
                    log::warn("Failed to get argon token for completing level");
                    co_return;
                }

                body["argonToken"] = std::move(token);
                auto response = co_await utils::web::WebRequest()
                                    .url(url)
                                    .header("Content-Type", "application/json")
                                    .bodyJSON(body)
                                    .post(url);
                if (response.error() || response.cancelled() || !response.ok()) {
                    log::warn("Failed to complete level: {}", gdx::getResponseMessage(response, "Unknown error"));
                    co_return;
                }

                auto jsonResult = response.json();
                if (!jsonResult) {
                    log::warn("Failed to parse complete level response JSON");
                    co_return;
                }

                auto result = std::move(jsonResult).unwrap();
                if (!result["success"].asBool().unwrapOr(false)) {
                    log::warn("Complete level request was not successful");
                    co_return;
                }

                auto rewardValue = result["reward"].asInt().unwrapOr(0);
                auto levelPointsValue = result["levelPoints"].asInt().unwrapOr(0);
                co_await async::waitForMainThread([self = std::move(self), rewardValue, levelPointsValue, levelId] {
                    if (!self) return;
                    auto gdxSelf = static_cast<GDXEndLevelLayer*>(self.data());
                    auto completedLevels = loadCompletedGauntletLevels(false);
                    if (!completedLevels.contains(levelId)) {
                        completedLevels.insert(levelId);
                        saveCompletedGauntletLevels(completedLevels, false);
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
                    auto rewardLabel = Label::create(rewardText, "bigFont.fnt");
                    if (rewardLabel) {
                        rewardLabel->setAnchorPoint({1.f, 0.5f});
                        rewardLabel->setScale(0.8f);
                        rewardLabel->setPosition({0.f, gauntletRewardNode->getContentSize().height * 0.65f});
                        gauntletRewardNode->addChild(rewardLabel);
                    }

                    auto levelPointsText = std::string("(" + numToString(levelPointsValue) + ")");
                    auto levelPointsLabel = Label::create(levelPointsText, "bigFont.fnt");
                    if (levelPointsLabel) {
                        levelPointsLabel->setAnchorPoint({1.f, 0.5f});
                        levelPointsLabel->setScale(0.4f);
                        levelPointsLabel->setColor({255, 115, 255});
                        levelPointsLabel->setPosition({0.f, gauntletRewardNode->getContentSize().height * 0.25f});
                        gauntletRewardNode->addChild(levelPointsLabel);
                    }

                    gauntletRewardNode->setPosition({55.f, self->m_listLayer->getContentSize().height / 2 - 15});
                    self->m_listLayer->addChild(gauntletRewardNode);

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
                    self->m_listLayer->addChild(circleWave);
                    // gdx::setPlayingGauntletLevel(false);
                });
                co_return; }, []() {});
            }
        }
    }
};
