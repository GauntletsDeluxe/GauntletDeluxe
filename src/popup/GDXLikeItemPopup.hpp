#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>

using namespace geode::prelude;

class GDXLikeItemPopup : public Popup {
public:
    static GDXLikeItemPopup* create(const matjson::Value& gauntlet);

private:
    bool init(const matjson::Value& gauntlet);
    void onLike(CCObject* sender);
    void onDislike(CCObject* sender);
    void sendFeedback(const std::string& type);

    matjson::Value m_gauntlet;
    int m_gauntletIndex = 0;
    bool m_isSending = false;
    CCMenuItemSpriteExtra* m_likeButton = nullptr;
    CCMenuItemSpriteExtra* m_dislikeButton = nullptr;
    Ref<cocos2d::CCObject> m_selfHold;
    async::TaskHolder<> m_requestTask;
};