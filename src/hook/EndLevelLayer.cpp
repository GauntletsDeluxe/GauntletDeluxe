#include <Geode/Geode.hpp>
#include <Geode/binding/GJListLayer.hpp>
#include <Geode/modify/EndLevelLayer.hpp>
#include "../include/GDXConstant.hpp"
#include "ccTypes.h"
#include <Geode/utils/web.hpp>
#include <argon/argon.hpp>
#include <arc/runtime/Runtime.hpp>

using namespace geode::prelude;

class $modify(GDXEndLevelLayer, EndLevelLayer) {
    void customSetup() override {
        EndLevelLayer::customSetup();
        log::debug("{} completed", m_playLayer->m_level->m_levelID);
        if (m_playLayer->m_level->m_normalPercent != 100) return;
        // get the summaryContainer
        auto summaryContainer = typeinfo_cast<CCNode*>(this->m_mainLayer->getChildByID("summary-container"));
        if (summaryContainer) {
            auto accountData = argon::getGameAccountData();
            auto levelId = this->m_playLayer->m_level->m_levelID;
            auto url = std::string(gdx::BASE_API_URL) + "/completeLevel";
            matjson::Value body = matjson::Value::object();
            body["accountId"] = accountData.accountId;
            body["argonToken"] = "";
            body["levelId"] = static_cast<int>(levelId);

            async::spawn([this, summaryContainer, url = std::move(url), body = std::move(body), accountData = std::move(accountData)]() mutable -> arc::Future<> {
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
                geode::queueInMainThread([this, summaryContainer, rewardValue] {
                    if (!summaryContainer || summaryContainer->getChildByID("gauntlet-reward-node")) {
                        return;
                    }

                    auto gauntletRewardNode = CCNodeRGBA::create();
                    gauntletRewardNode->setContentHeight(30.f);
                    gauntletRewardNode->setScale(1.2f);
                    gauntletRewardNode->setOpacity(0);
                    gauntletRewardNode->setID("gauntlet-reward-node");
                    auto rewardSpr = CCSprite::createWithSpriteFrameName("GDX_levelPoint.png"_spr);
                    if (rewardSpr) {
                        rewardSpr->setScale(0.4f);
                        rewardSpr->setAnchorPoint({0.f, 0.5f});
                        rewardSpr->setPosition({5.f, gauntletRewardNode->getContentSize().height / 2.f});
                        gauntletRewardNode->addChild(rewardSpr);
                    }

                    auto rewardLabel = CCLabelBMFont::create(numToString(rewardValue).c_str(), "bigFont.fnt");
                    if (rewardLabel) {
                        rewardLabel->setAnchorPoint({1.f, 0.5f});
                        rewardLabel->setScale(0.8f);
                        rewardLabel->setPosition({0.f, gauntletRewardNode->getContentSize().height / 2.f});
                        gauntletRewardNode->addChild(rewardLabel);
                    }

                    summaryContainer->addChild(gauntletRewardNode);
                    auto scaleAction = CCScaleBy::create(.6f, .6f);
                    auto bounceAction = CCEaseBounceOut::create(scaleAction);
                    auto fadeAction = CCFadeIn::create(0.6f);
                    auto spawnAction = CCSpawn::createWithTwoActions(bounceAction, fadeAction);
                    gauntletRewardNode->setScale(1.f);
                    gauntletRewardNode->runAction(spawnAction);
                    // @geode-ignore(unknown-resource)
                    FMODAudioEngine::sharedEngine()->playEffect("gold02.ogg");
                    auto circleWave = CCCircleWave::create(10.f, 110.f, 0.5f, false);
                    circleWave->setPosition(gauntletRewardNode->getPosition());
                    circleWave->m_color = ccColor3B({191, 3, 226});
                    summaryContainer->addChild(circleWave);
                    summaryContainer->updateLayout();
                });
                co_return;
            });
        }
    }
};