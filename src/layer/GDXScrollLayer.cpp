#include "GDXScrollLayer.hpp"
#include <algorithm>
#include <cmath>
#include <asp/time/SystemTime.hpp>

using namespace geode::prelude;

constexpr int kScrollActionTag = 2;
constexpr float kRubberBand = 0.35f;
constexpr float kFlingSpeed = 450.f;
constexpr int kTouchPriority = -138;

double nowSeconds() {
    return static_cast<double>(asp::time::SystemTime::now().timeSinceEpoch().micros()) / 1'000'000.0;
}

GDXScrollLayer* GDXScrollLayer::create(CCArray* pages) {
    auto ret = new GDXScrollLayer();
    if (ret->init(pages)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool GDXScrollLayer::init(CCArray* pages) {
    if (!CCLayer::init() || !pages) return false;

    auto winSize = CCDirector::sharedDirector()->getWinSize();
    this->setContentSize(winSize);

    m_pages = pages;
    m_pages->retain();

    m_contentLayer = CCLayer::create();
    m_contentLayer->setContentSize(winSize);
    this->addChild(m_contentLayer);

    m_scrollProxy = ScrollLayer::create(CCRect{0.f, 0.f, 12.f, m_scrollbarLength}, false, true);
    m_scrollProxy->setVisible(false);
    this->addChild(m_scrollProxy);

    m_scrollbar = Scrollbar::create(m_scrollProxy);
    m_scrollbar->setRotation(-90.f);  // vertical top -> left, so the first page is at the left end
    m_scrollbar->setPosition({winSize.width / 2.f, 60.f});
    this->addChild(m_scrollbar, 5);

    this->updatePages();
    this->setTouchEnabled(true);
    this->scheduleUpdate();
    return true;
}

void GDXScrollLayer::registerWithTouchDispatcher() {
    CCDirector::sharedDirector()->getTouchDispatcher()->addPrioTargetedDelegate(this, m_touchPrio, false); // ew touch prio
}

void GDXScrollLayer::setScrollTouchPriority(int priority) {
    m_touchPrio = priority;
    if (this->isTouchEnabled()) {
        this->setTouchEnabled(false);
        this->setTouchEnabled(true);
    }
}

int GDXScrollLayer::getTotalPages() const {
    return m_pages ? static_cast<int>(m_pages->count()) : 0;
}

float GDXScrollLayer::pageWidth() const {
    return this->getContentSize().width;
}

float GDXScrollLayer::contentXForPage(float page) const {
    return -page * this->pageWidth();
}

float GDXScrollLayer::scrollProgress() const {
    auto total = this->getTotalPages();
    if (total <= 1) return 0.f;
    return std::clamp(-m_contentLayer->getPositionX() / (static_cast<float>(total - 1) * this->pageWidth()), 0.f, 1.f);
}

int GDXScrollLayer::pageNumberForPosition() const {
    auto total = this->getTotalPages();
    if (total <= 0) return 0;
    auto page = static_cast<int>(std::lround(-m_contentLayer->getPositionX() / this->pageWidth()));
    return std::clamp(page, 0, total - 1);
}

void GDXScrollLayer::updatePages() {
    for (auto i = 0u; i < m_pages->count(); ++i) {
        auto page = static_cast<CCLayer*>(m_pages->objectAtIndex(i));
        page->setAnchorPoint({0.f, 0.f});
        page->setContentSize(CCDirector::sharedDirector()->getWinSize());
        page->setPosition({this->pageWidth() * static_cast<float>(i), 0.f});
        if (!page->getParent()) m_contentLayer->addChild(page);
    }
    m_page = std::clamp(m_page, 0, std::max(this->getTotalPages() - 1, 0));
    m_scrollProxy->m_contentLayer->setContentSize(
        {12.f, m_scrollbarLength * static_cast<float>(std::max(this->getTotalPages(), 1))});
    m_scrollbar->setVisible(this->getTotalPages() > 1);
    this->syncScrollbar();
}

void GDXScrollLayer::moveToPage(int page, bool animated) {
    auto total = this->getTotalPages();
    if (total <= 0) return;
    page = std::clamp(page, 0, total - 1);
    m_page = page;

    m_contentLayer->stopActionByTag(kScrollActionTag);
    auto targetX = this->contentXForPage(static_cast<float>(page));
    if (!animated) {
        m_contentLayer->setPositionX(targetX);
        return;
    }
    auto move = CCEaseExponentialOut::create(CCMoveTo::create(0.4f, {targetX, 0.f}));
    move->setTag(kScrollActionTag);
    m_contentLayer->runAction(move);
}

void GDXScrollLayer::setScrollbarVisible(bool visible) {
    m_scrollbar->setVisible(visible && this->getTotalPages() > 1);
}

void GDXScrollLayer::setScrollbarPosition(CCPoint const& pos) {
    m_scrollbar->setPosition(pos);
}

void GDXScrollLayer::syncScrollbar() {
    auto total = this->getTotalPages();
    if (total <= 1) return;

    auto proxyContent = m_scrollProxy->m_contentLayer;
    auto range = m_scrollbarLength * static_cast<float>(total - 1);

    if (m_scrollbar->isTouching()) {
        m_scrollbarWasTouching = true;
        auto progress = std::clamp(1.f + proxyContent->getPositionY() / range, 0.f, 1.f);
        m_contentLayer->stopActionByTag(kScrollActionTag);
        m_contentLayer->setPositionX(-progress * static_cast<float>(total - 1) * this->pageWidth());
        return;
    }

    if (m_scrollbarWasTouching) {
        m_scrollbarWasTouching = false;
        this->moveToPage(this->pageNumberForPosition());
    }
    proxyContent->setPositionY(-range * (1.f - this->scrollProgress()));
}

void GDXScrollLayer::update(float dt) {
    this->syncScrollbar();
}

bool GDXScrollLayer::isOnScrollbar(CCPoint const& worldPos) const {
    if (!m_scrollbar->isVisible()) return false;
    auto local = m_scrollbar->convertToNodeSpace(worldPos);
    auto size = m_scrollbar->getContentSize();
    return local.x >= -12.f && local.x <= size.width + 12.f && local.y >= -12.f && local.y <= size.height + 12.f;
}

void GDXScrollLayer::stealTouch(CCTouch* touch, CCEvent* event) {
    auto touches = new CCSet();
    touches->addObject(touch);
    touches->autorelease();
    m_ignoreTouchCancel = true;
    auto dispatcher = CCDirector::sharedDirector()->getTouchDispatcher();
    dispatcher->touchesCancelled(touches, event);
    m_ignoreTouchCancel = false;
    if (auto handler = static_cast<CCTargetedTouchHandler*>(dispatcher->findHandler(this))) {
        if (!handler->getClaimedTouches()->containsObject(touch)) {
            handler->getClaimedTouches()->addObject(touch);
        }
    }
}

bool GDXScrollLayer::ccTouchBegan(CCTouch* touch, CCEvent* event) {
    if (m_touch || !nodeIsVisible(this) || this->getTotalPages() <= 0) return false;

    auto location = touch->getLocation();
    if (this->isOnScrollbar(location)) return false;
    m_touch = touch;
    m_velocity = 0.f;
    m_touchStartX = location.x;
    m_lastTouchX = location.x;
    m_lastTouchTime = nowSeconds();

    m_drag = Drag::Pending;
    return true;
}

void GDXScrollLayer::ccTouchMoved(CCTouch* touch, CCEvent* event) {
    if (touch != m_touch) return;
    auto location = touch->getLocation();

    if (m_drag == Drag::Pending) {
        if (std::abs(location.x - m_touchStartX) < m_slideThreshold) return;
        m_contentLayer->stopActionByTag(kScrollActionTag);
        m_drag = Drag::Content;
        m_contentStartX = m_contentLayer->getPositionX();
        m_touchStartX = location.x;
        m_lastTouchX = location.x;
        m_lastTouchTime = nowSeconds();
        this->stealTouch(touch, event);
        return;
    }

    if (m_drag != Drag::Content) return;

    auto now = nowSeconds();
    auto elapsed = static_cast<float>(now - m_lastTouchTime);
    if (elapsed > 0.f) {
        // smoothed so one jittery frame doesn't decide the fling
        m_velocity = m_velocity * 0.6f + ((location.x - m_lastTouchX) / elapsed) * 0.4f;
    }
    m_lastTouchX = location.x;
    m_lastTouchTime = now;

    auto x = m_contentStartX + (location.x - m_touchStartX);
    auto minX = this->contentXForPage(static_cast<float>(std::max(this->getTotalPages() - 1, 0)));
    // resist dragging past the first / last page
    if (x > 0.f)
        x *= kRubberBand;
    else if (x < minX)
        x = minX + (x - minX) * kRubberBand;
    m_contentLayer->setPositionX(x);
}

void GDXScrollLayer::ccTouchEnded(CCTouch* touch, CCEvent* event) {
    if (touch != m_touch) return;
    m_touch = nullptr;

    auto drag = m_drag;
    m_drag = Drag::None;

    if (drag == Drag::Content) {
        auto page = this->pageNumberForPosition();
        // a fast flick moves one page even if the drag was short
        if (page == m_page) {
            if (m_velocity <= -kFlingSpeed)
                page++;
            else if (m_velocity >= kFlingSpeed)
                page--;
        }
        this->moveToPage(page);
    } else if (drag == Drag::Pending && m_onTap) {
        m_onTap(touch->getLocation());
    }
}

void GDXScrollLayer::ccTouchCancelled(CCTouch* touch, CCEvent* event) {
    if (m_ignoreTouchCancel || touch != m_touch) return;
    m_touch = nullptr;
    m_drag = Drag::None;
    this->moveToPage(m_page);
}

void GDXScrollLayer::scrollWheel(float y, float x) {
    if (!nodeIsVisible(this)) return;
    auto delta = std::abs(y) >= std::abs(x) ? y : x;
    if (delta == 0.f) return;

    static double lastWheel = 0.0;
    auto now = nowSeconds();
    if (now - lastWheel < 0.2) return;
    lastWheel = now;

    this->moveToPage(m_page + (delta > 0.f ? 1 : -1));
}
