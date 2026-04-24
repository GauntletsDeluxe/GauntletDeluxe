#include "GDXGauntletLayer.hpp"
#include <algorithm>
#include <Geode/Geode.hpp>
#include <Geode/binding/BoomScrollLayer.hpp>
#include "Geode/ui/Layout.hpp"

using namespace geode::prelude;

GDXGauntletLayer* GDXGauntletLayer::create() {
    auto ret = new GDXGauntletLayer();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool GDXGauntletLayer::init() {
    if (!CCLayer::init()) return false;

    // ui stuff
    auto director = CCDirector::sharedDirector();
    auto winSize = director->getWinSize();
    addBackButton(this, BackButtonStyle::Green);
    addSideArt(this, SideArt::All, SideArtStyle::LayerGray, false);
    auto bg = createLayerBG();
    bg->setColor({50, 50, 50});
    this->addChild(bg, -5);

    // info button
    auto bottomMenu = CCMenu::create();
    bottomMenu->setPosition({30, 70});
    bottomMenu->setContentHeight(100);
    bottomMenu->setLayout(ColumnLayout::create()->setGap(5.f)->setAxisAlignment(AxisAlignment::Start));
    this->addChild(bottomMenu, 2);

    auto infoIconSpr = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
    auto infoIconBtn = CCMenuItemSpriteExtra::create(infoIconSpr, this, menu_selector(GDXGauntletLayer::onInfo));
    bottomMenu->addChild(infoIconBtn);

    bottomMenu->updateLayout();

    // title styling
    auto titleOutline = CCLabelBMFont::create("Gauntlet Deluxe", "goldFont.fnt");
    titleOutline->setColor(ccc3(0, 0, 0));
    titleOutline->setPosition({winSize.width / 2.f, winSize.height - 30.f});
    this->addChild(titleOutline, 37);

    auto titleHighlight = CCLabelBMFont::create("Gauntlet Deluxe", "goldFont.fnt");
    titleHighlight->setColor(ccc3(255, 255, 255));
    titleHighlight->setPosition(titleOutline->getPosition());
    this->addChild(titleHighlight, 38);

    // boomscrolllayer
    {
        auto pages = CCArray::create();
        pages->retain();

        auto createPage = [&](const char* text) {
            auto page = CCLayer::create();
            auto label = CCLabelBMFont::create(text, "chatFont.fnt");
            label->setPosition({winSize.width / 2.f, winSize.height / 2.f});
            page->setContentSize(winSize);
            page->addChild(label);
            return page;
        };

        pages->addObject(createPage("Gauntlet Page 1"));
        pages->addObject(createPage("Gauntlet Page 2"));
        pages->addObject(createPage("Gauntlet Page 3"));

        m_scrollLayer = BoomScrollLayer::create(pages, 0, true);
        m_scrollLayer->setPosition({0, 0});
        this->addChild(m_scrollLayer);

        if (m_scrollLayer->m_dots) {
            for (auto i = 0u; i < m_scrollLayer->m_dots->count(); ++i) {
                auto dot = static_cast<CCSprite*>(m_scrollLayer->m_dots->objectAtIndex(i));
                if (dot) {
                    dot->setVisible(false);
                }
            }
        }
        m_scrollLayer->m_dotsVisible = false;

        auto arrowMenu = CCMenu::create();
        arrowMenu->setPosition({0, 0});

        auto leftArrowSpr = CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
        leftArrowSpr->setFlipX(true);
        leftArrowSpr->setScale(0.8f);
        auto leftArrowBtn = CCMenuItemSpriteExtra::create(leftArrowSpr, this, (SEL_MenuHandler)&GDXGauntletLayer::onPrev);
        leftArrowBtn->setPosition({25.f, winSize.height / 2});
        arrowMenu->addChild(leftArrowBtn);

        auto rightArrowSpr = CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
        rightArrowSpr->setScale(0.8f);
        auto rightArrowBtn = CCMenuItemSpriteExtra::create(rightArrowSpr, this, (SEL_MenuHandler)&GDXGauntletLayer::onNext);
        rightArrowBtn->setPosition({winSize.width - 25.f, winSize.height / 2});
        arrowMenu->addChild(rightArrowBtn);

        this->addChild(arrowMenu, 2);

        m_dotsMenu = CCMenu::create();
        m_dotsMenu->setLayout(AxisLayout::create());
        m_dotsMenu->setPosition({winSize.width / 2.f, director->getScreenBottom() + 15.f});
        this->addChild(m_dotsMenu);

        for (auto i = 0u; i < pages->count(); ++i) {
            auto spr = CCSprite::createWithSpriteFrameName("gj_navDotBtn_off_001.png");
            spr->setScale(0.8f);
            auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(GDXGauntletLayer::onDot));
            m_dotsMenu->addChild(btn);
            m_dots.push_back(btn);
        }

        m_dotsMenu->updateLayout();
        updateDots();

        pages->release();
    }

    this->scheduleUpdate();
    this->setKeypadEnabled(true);

    return true;
}

void GDXGauntletLayer::keyBackClicked() {
    CCDirector::sharedDirector()->popSceneWithTransition(
        0.5f, PopTransition::kPopTransitionFade);
}

void GDXGauntletLayer::onInfo(CCObject* sender) {
    FLAlertLayer::create(
        "Gauntlet Deluxe",
        "This is a work in progress mod that adds a new gauntlet mode to Geometry Dash. "
        "The mod is currently in early development, so expect bugs and missing features. "
        "If you want to help out, join the Discord server linked on the mod page.",
        "OK")
        ->show();
}

void GDXGauntletLayer::update(float dt) {
    if (!m_scrollLayer) {
        return;
    }

    auto page = m_scrollLayer->m_page;
    if (page != m_currentPage) {
        m_currentPage = page;
        updateDots();
    }
}

void GDXGauntletLayer::onDot(CCObject* sender) {
    auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
    auto it = std::find(m_dots.begin(), m_dots.end(), btn);
    if (it == m_dots.end() || !m_scrollLayer) {
        return;
    }

    auto idx = static_cast<int>(it - m_dots.begin());
    m_scrollLayer->moveToPage(idx);
    m_scrollLayer->updatePages();
    m_scrollLayer->repositionPagesLooped();
    updateDots();
}

void GDXGauntletLayer::onPrev(CCObject* sender) {
    if (!m_scrollLayer) {
        return;
    }

    auto total = m_scrollLayer->getTotalPages();
    if (total <= 0) {
        return;
    }

    auto page = m_scrollLayer->m_page;
    page = (page - 1 + total) % total;

    m_scrollLayer->moveToPage(page);
    m_scrollLayer->updatePages();
    m_scrollLayer->repositionPagesLooped();
    updateDots();
}

void GDXGauntletLayer::onNext(CCObject* sender) {
    if (!m_scrollLayer) {
        return;
    }

    auto total = m_scrollLayer->getTotalPages();
    if (total <= 0) {
        return;
    }

    auto page = m_scrollLayer->m_page;
    page = (page + 1) % total;

    m_scrollLayer->moveToPage(page);
    m_scrollLayer->updatePages();
    m_scrollLayer->repositionPagesLooped();
    updateDots();
}

void GDXGauntletLayer::updateDots() {
    if (m_dots.empty() || !m_scrollLayer) {
        return;
    }

    auto page = m_scrollLayer->m_page;
    if (page < 0) {
        page = static_cast<int>(m_dots.size()) + page;
    }
    if (page >= static_cast<int>(m_dots.size())) {
        page %= static_cast<int>(m_dots.size());
    }

    auto sfc = CCSpriteFrameCache::sharedSpriteFrameCache();
    for (auto i = 0u; i < m_dots.size(); ++i) {
        auto btn = m_dots[i];
        auto spr = static_cast<CCSprite*>(btn->getNormalImage());
        if (!spr) {
            continue;
        }

        auto frame = (static_cast<int>(i) == page)
                         ? sfc->spriteFrameByName("gj_navDotBtn_on_001.png")
                         : sfc->spriteFrameByName("gj_navDotBtn_off_001.png");
        spr->setDisplayFrame(frame);
    }
}