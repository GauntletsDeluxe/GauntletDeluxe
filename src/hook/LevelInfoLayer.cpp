#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/ui/Label.hpp>
#include <asp/fs.hpp>
#include "../include/GDXConstant.hpp"

using namespace geode::prelude;
using namespace gdx;

class $modify(GDXLevelInfoLayer, LevelInfoLayer) {
    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) return false;

        if (gdx::isPlayingGauntletLevel(static_cast<int>(level->m_levelID))) {
            auto self = Ref<GDXLevelInfoLayer>(this);
            int levelId = static_cast<int>(level->m_levelID);
            auto accountData = argon::getGameAccountData();
            auto url = fmt::format("{}/getLevel", gdx::baseApiUrl());
            matjson::Value body;
            body["accountId"] = accountData.accountId;
            body["argonToken"] = "";
            body["levelId"] = static_cast<int>(levelId);

            auto difficultySprite = self->getChildByID("difficulty-sprite");
            if (!difficultySprite) {
                return true;
            }
            CCPoint difficultySpritePos = difficultySprite->getPosition();
            auto loadingSpinner = LoadingSpinner::create(25.f);
            if (loadingSpinner) {
                loadingSpinner->setID("gauntlet-points-spinner");
                loadingSpinner->setPosition({difficultySpritePos.x, 160.f});
                this->addChild(loadingSpinner);
            }

            auto gdxBg = typeinfo_cast<CCSprite*>(this->getChildByID("background"));
            if (gdxBg) {
                gdxBg->setColor(gdx::gauntletColor()); // color based of the gauntlet color scheme
            }

            async::spawn([self, difficultySpritePos, loadingSpinner, url = std::move(url), body = std::move(body), accountData = std::move(accountData), levelId]() mutable -> arc::Future<> {
                auto token = co_await gdx::argonToken(accountData);
                if (token.empty()) {
                    co_return;
                }

                body["argonToken"] = std::move(token);

                auto response = co_await utils::web::WebRequest()
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
                auto levelPoints = result["levelPoints"].asInt().unwrapOr(-1);
                if (levelPoints < 0) {
                    co_return;
                }

                co_await async::waitForMainThread([self, levelPoints, loadingSpinner, difficultySpritePos, levelId] {
                    if (!self) {
                        return;
                    }

                    auto levelPointsNode = CCNodeRGBA::create();
                    levelPointsNode->setScale(.5f);
                    levelPointsNode->setPosition({difficultySpritePos.x, 160.f});
                    levelPointsNode->setContentSize({80.f, 30.f});
                    levelPointsNode->setID("gauntlet-points-node");
                    self->addChild(levelPointsNode);

                    auto levelPointNodeBg = NineSlice::create("square02_small.png");
                    levelPointNodeBg->setOpacity(150);
                    levelPointNodeBg->setContentSize(levelPointsNode->getContentSize() + CCSize(10.f, 10.f));
                    levelPointsNode->addChild(levelPointNodeBg);

                    auto levelPointsSpr = CCSprite::createWithSpriteFrameName("GDX_levelPoint.png"_spr);
                    if (levelPointsSpr) {
                        levelPointsSpr->setScale(0.35f);
                        levelPointsSpr->setAnchorPoint({0.f, 0.5f});
                        levelPointsSpr->setPosition({0.f, 0.f});
                        levelPointsNode->addChild(levelPointsSpr);
                    }

                    auto levelPointsLabel = Label::create(fmt::format("{}", levelPoints), "bigFont.fnt");
                    if (levelPointsLabel) {
                        levelPointsLabel->setAnchorPoint({1.f, 0.5f});
                        levelPointsLabel->setPosition({-5.f, 0.f});
                        levelPointsLabel->setLimitLabelWidth(80.f, 0.8f, 0.4f);
                        if (hasCompletedGauntletLevel(static_cast<int>(levelId))) levelPointsLabel->setColor(ccColor3B{225, 140, 255});
                        levelPointsNode->addChild(levelPointsLabel);
                    }

                    if (loadingSpinner) {
                        loadingSpinner->removeFromParent();
                    }

                    if (hasCompletedGauntletLevel(static_cast<int>(levelId))) {
                        auto completedIcon = CCSprite::createWithSpriteFrameName("GJ_completesIcon_001.png");
                        if (completedIcon) {
                            completedIcon->setPosition(levelPointsSpr->getContentSize() / 2.f);
                            completedIcon->setAnchorPoint({0.f, 1.f});
                            completedIcon->setScale(2.f);
                            levelPointsSpr->addChild(completedIcon, 2);
                        }
                    }
                });
            });
        }
        return true;
    }
};