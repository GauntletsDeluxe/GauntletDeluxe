#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include <Geode/ui/Scrollbar.hpp>

// dont ask me how i know the boomscrolllayer exactly
// i just have the yum code for it and attempted to mimic it, its sorta a mess
class GDXScrollLayer : public cocos2d::CCLayer {
public:
    static GDXScrollLayer* create(cocos2d::CCArray* pages);

    int getCurrentPage() const { return m_page; }
    int getTotalPages() const;
    int pageNumberForPosition() const;

    void moveToPage(int page, bool animated = true);
    void updatePages();
    void setScrollbarVisible(bool visible);
    void setScrollbarPosition(cocos2d::CCPoint const& pos);

protected:
    bool init(cocos2d::CCArray* pages);
    void registerWithTouchDispatcher() override;
    bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void ccTouchCancelled(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void scrollWheel(float y, float x) override;
    void update(float dt) override;

private:
    enum class Drag { None, Pending, Content };

    float pageWidth() const;
    float contentXForPage(float page) const;
    float scrollProgress() const;
    bool isOnScrollbar(cocos2d::CCPoint const& worldPos) const;
    void syncScrollbar();
    void stealTouch(cocos2d::CCTouch* touch, cocos2d::CCEvent* event);

    cocos2d::CCArray* m_pages = nullptr;
    cocos2d::CCLayer* m_contentLayer = nullptr;
    geode::ScrollLayer* m_scrollProxy = nullptr;
    geode::Scrollbar* m_scrollbar = nullptr;
    bool m_scrollbarWasTouching = false;

    cocos2d::CCTouch* m_touch = nullptr;
    Drag m_drag = Drag::None;
    float m_touchStartX = 0.f;
    float m_contentStartX = 0.f;
    float m_lastTouchX = 0.f;
    double m_lastTouchTime = 0.0;
    float m_velocity = 0.f;
    bool m_ignoreTouchCancel = false;

    int m_page = 0;
    float m_slideThreshold = 20.f;
    float m_scrollbarLength = 240.f;
};
