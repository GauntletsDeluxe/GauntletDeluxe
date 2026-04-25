#include "GDXGauntletLayer.hpp"
#include "../popup/GDXGauntletManagePopup.hpp"
#include <algorithm>
#include <Geode/Geode.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/utils/web.hpp>
#include "../include/GDXConstant.hpp"
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

    // @geode-ignore(unknown-resource)
    auto manageLabel = CircleButtonSprite::createWithSpriteFrameName("geode.loader/persistent.png", 1.f, CircleBaseColor::Green, CircleBaseSize::Small);
    auto manageBtn = CCMenuItemSpriteExtra::create(manageLabel, this, menu_selector(GDXGauntletLayer::onManageGauntlets));
    manageBtn->setPosition({0.f, -35.f});
    bottomMenu->addChild(manageBtn);

    bottomMenu->updateLayout();

    // title styling
    auto title = CCSprite::createWithSpriteFrameName("GDX_title.png"_spr);
    title->setScale(0.8f);
    this->addChildAtPosition(title, Anchor::Top, {0, -30}, false);

    // gauntlet page container
    createGauntletPages(matjson::Value::array());
    fetchGauntlets();

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

void GDXGauntletLayer::onManageGauntlets(CCObject* sender) {
    GDXGauntletManagePopup::create()->show();
}

void GDXGauntletLayer::onGauntletButtonClick(CCObject* sender) {
    // TODO: handle gauntlet selection
}

void GDXGauntletLayer::fetchGauntlets() {
    auto url = std::string(gdx::BASE_API_URL) + "/getGauntlets";
    async::spawn([this, url = std::move(url)]() -> arc::Future<> {
        auto response = co_await geode::utils::web::WebRequest()
                            .get(url);

        if (response.error() || response.cancelled() || !response.ok()) {
            co_return;
        }

        auto jsonResult = response.json();
        if (!jsonResult) {
            co_return;
        }

        auto gauntlets = std::move(jsonResult).unwrap();
        geode::queueInMainThread([this, gauntlets = std::move(gauntlets)]() mutable {
            createGauntletPages(gauntlets);
        });

        co_return;
    });
}

GDXGauntletNode GDXGauntletNode::fromJson(const matjson::Value& gauntlet) {
    GDXGauntletNode node;
    node.id = gauntlet["id"].asInt().unwrapOr(
        gauntlet["index"].asInt().unwrapOr(0));
    node.name = gauntlet["name"].asString().unwrapOr("Unknown");
    node.description = gauntlet["description"].asString().unwrapOr("");
    node.reward = gauntlet["reward"].asInt().unwrapOr(0);
    node.r = gauntlet["r"].asInt().unwrapOr(255);
    node.g = gauntlet["g"].asInt().unwrapOr(255);
    node.b = gauntlet["b"].asInt().unwrapOr(255);

    auto levelArray = gauntlet["levelIds"];
    if (levelArray.isArray()) {
        for (auto i = 0u; i < levelArray.size(); ++i) {
            auto levelValue = levelArray[i];
            GDXGauntletLevelInfo level;
            level.levelId = levelValue["levelId"].asInt().unwrapOr(0);
            level.levelName = levelValue["levelName"].asString().unwrapOr("");
            level.creatorName = levelValue["creatorName"].asString().unwrapOr("");
            level.songName = levelValue["songName"].asString().unwrapOr("");
            level.songId = levelValue["songId"].asInt().unwrapOr(0);
            level.reward = levelValue["reward"].asInt().unwrapOr(0);
            node.levelIds.push_back(std::move(level));
        }
    }

    return node;
}

CCMenuItemSpriteExtra* GDXGauntletLayer::createGauntletButton(const matjson::Value& gauntlet) {
    auto node = GDXGauntletNode::fromJson(gauntlet);
    int gauntletIndex = node.id;
    std::string gauntletName = node.name;

    auto gauntletBg = NineSlice::create("GJ_squareB_01.png");
    gauntletBg->setContentSize({110, 240});

    std::string formattedName = gauntletName;
    std::replace(formattedName.begin(), formattedName.end(), ' ', '\n');

    auto nameLabel = CCLabelBMFont::create(formattedName.c_str(), "bigFont.fnt");
    nameLabel->setAlignment(kCCTextAlignmentCenter);
    nameLabel->setPosition({gauntletBg->getContentSize().width / 2, gauntletBg->getContentSize().height - 15});
    nameLabel->setAnchorPoint({0.5f, 1.0f});
    nameLabel->setScale(0.5f);

    auto nameLabelShadow = CCLabelBMFont::create(formattedName.c_str(), "bigFont.fnt");
    nameLabelShadow->setAlignment(kCCTextAlignmentCenter);
    nameLabelShadow->setPosition({nameLabel->getPositionX() + 2, nameLabel->getPositionY() - 2});
    nameLabelShadow->setAnchorPoint({0.5f, 1.0f});
    nameLabelShadow->setColor({0, 0, 0});
    nameLabelShadow->setOpacity(60);
    nameLabelShadow->setScale(0.5f);

    gauntletBg->addChild(nameLabel, 3);
    gauntletBg->addChild(nameLabelShadow, 2);

    gauntletBg->setColor({static_cast<GLubyte>(node.r), static_cast<GLubyte>(node.g), static_cast<GLubyte>(node.b)});

    std::string spriteName = fmt::format("GDX_gauntletUnknown.png"_spr, gauntletIndex);
    auto gauntletSprite = CCSprite::createWithSpriteFrameName(spriteName.c_str());
    auto gauntletSpriteShadow = CCSprite::createWithSpriteFrameName(spriteName.c_str());
    gauntletSpriteShadow->setColor({0, 0, 0});
    gauntletSpriteShadow->setOpacity(50);
    gauntletSpriteShadow->setScaleY(1.2f);

    gauntletBg->addChild(gauntletSprite, 3);
    gauntletBg->addChild(gauntletSpriteShadow, 2);
    gauntletSprite->setPosition(gauntletBg->getContentSize() / 2);
    gauntletSpriteShadow->setPosition({gauntletSprite->getPositionX(), gauntletSprite->getPositionY() - 6});

    auto button = CCMenuItemSpriteExtra::create(gauntletBg, this, menu_selector(GDXGauntletLayer::onGauntletButtonClick));
    button->setTag(static_cast<int>(gauntletIndex));
    m_gauntletButtons.push_back(button);
    button->m_scaleMultiplier = 1.05f;
    button->setVisible(true);
    return button;
}

void GDXGauntletLayer::createGauntletPages(const matjson::Value& gauntlets) {
    if (m_scrollLayer) {
        m_scrollLayer->removeFromParent();
        m_scrollLayer = nullptr;
    }
    for (auto button : m_gauntletButtons) {
        if (button) {
            button->removeFromParent();
        }
    }
    m_gauntletButtons.clear();

    const auto winSize = CCDirector::sharedDirector()->getWinSize();
    const float cardWidth = 110.f;

    auto pages = CCArray::create();
    pages->retain();

    CCLayer* page = nullptr;
    CCMenu* pageMenu = nullptr;
    for (auto i = 0u; i < gauntlets.size(); ++i) {
        if (i % 3 == 0) {
            page = CCLayer::create();
            page->setContentSize(winSize);
            pageMenu = CCMenu::create();
            pageMenu->setLayout(RowLayout::create()->setGap(0.f)->setAxisAlignment(AxisAlignment::Center));
            pageMenu->setPosition({page->getContentSize().width / 2, page->getContentSize().height / 2 + 35.f});
            page->addChild(pageMenu);
            pages->addObject(page);
        }

        auto gauntletButton = createGauntletButton(gauntlets[i]);
        auto slotIndex = i % 3;
        if (pageMenu) {
            pageMenu->addChild(gauntletButton);
            pageMenu->updateLayout();
        }
    }

    m_scrollLayer = BoomScrollLayer::create(pages, 0, false);
    m_scrollLayer->setPosition({0, -45});
    this->addChild(m_scrollLayer);

    auto pageCount = pages->count();
    if (pageCount > 2) {
        auto navMenu = CCMenu::create();
        navMenu->setPosition({0, 0});
        this->addChild(navMenu, 3);

        auto prevBtnSpr = CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
        prevBtnSpr->setFlipX(true);
        auto prevBtn = CCMenuItemSpriteExtra::create(
            prevBtnSpr,
            this,
            menu_selector(GDXGauntletLayer::onPrev));
        prevBtn->setPosition({winSize.width / 2.f - 100.f, 60.f});
        navMenu->addChild(prevBtn);

        auto nextBtn = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("navArrowBtn_001.png"),
            this,
            menu_selector(GDXGauntletLayer::onNext));
        nextBtn->setPosition({winSize.width / 2.f + 100.f, 60.f});
        navMenu->addChild(nextBtn);
    }

    pages->release();
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