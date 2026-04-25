#include "GDXGauntletManagePopup.hpp"
#include "GDXAddGauntletPopup.hpp"
#include "../include/GDXConstant.hpp"
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/utils/web.hpp>

using namespace geode;
using namespace geode::prelude;

GDXGauntletManagePopup* GDXGauntletManagePopup::create() {
    auto ret = new GDXGauntletManagePopup();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool GDXGauntletManagePopup::init() {
    if (!Popup::init(480.f, 290.f)) {
        return false;
    }

    setTitle("Manage Gauntlets");

    auto listSize = CCSizeMake(356.f, 200.f);
    m_list = cue::ListNode::create(listSize);
    m_mainLayer->addChildAtPosition(m_list, Anchor::Center, {0.f, 0.f});

    auto addBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Add Gauntlet", "goldFont.fnt", "GJ_button_01.png"),
        this,
        menu_selector(GDXGauntletManagePopup::onAdd));
    m_buttonMenu->addChildAtPosition(addBtn, Anchor::Bottom, {0, 25}, false);

    refreshListItems();
    fetchGauntlets();
    return true;
}

void GDXGauntletManagePopup::onAdd(CCObject* sender) {
    GDXAddGauntletPopup::create(this)->show();
}

void GDXGauntletManagePopup::refreshList() {
    refreshListItems();
}

void GDXGauntletManagePopup::refreshListItems() {
    if (!m_list) {
        return;
    }

    m_list->clear();
    auto emptyLabel = CCLabelBMFont::create("Loading gauntlets...", "chatFont.fnt");
    emptyLabel->setAnchorPoint({0.5f, 0.5f});
    emptyLabel->setPosition({m_list->getListSize().width / 2.f, m_list->getListSize().height / 2.f});
    m_list->addCell(emptyLabel);
}

void GDXGauntletManagePopup::fetchGauntlets() {
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
            createGauntletList(gauntlets);
        });
        co_return;
    });
}

void GDXGauntletManagePopup::createGauntletList(const matjson::Value& gauntlets) {
    if (!m_list) {
        return;
    }

    m_list->clear();
    if (!gauntlets.isArray() || gauntlets.size() == 0) {
        auto emptyLabel = CCLabelBMFont::create("No gauntlets available.", "chatFont.fnt");
        emptyLabel->setAnchorPoint({0.5f, 0.5f});
        emptyLabel->setPosition({m_list->getListSize().width / 2.f, m_list->getListSize().height / 2.f});
        m_list->addCell(emptyLabel);
        return;
    }

    for (auto i = 0u; i < gauntlets.size(); ++i) {
        auto const& gauntlet = gauntlets[i];
        if (!gauntlet.isObject()) {
            continue;
        }
        auto cell = createGauntletCell(gauntlet);
        if (cell) {
            m_list->addCell(cell);
            m_list->updateLayout();
        }
    }
    m_list->updateLayout();
}

CCNode* GDXGauntletManagePopup::createGauntletCell(const matjson::Value& gauntlet) {
    auto cell = CCLayer::create();
    cell->setContentSize({356.f, 90.f});

    auto name = gauntlet["name"].asString().unwrapOr("Unknown");
    auto description = gauntlet["description"].asString().unwrapOr("");
    auto reward = gauntlet["reward"].asInt().unwrapOr(0);

    // temp gauntlet sprite (REPLACE WITH ACTUAL GAUNTLET)
    auto gauntletSprite = CCSprite::createWithSpriteFrameName("GDX_gauntletUnknown.png"_spr);

    if (gauntletSprite) {
        gauntletSprite->setScale(0.65f);
        gauntletSprite->setPosition({40.f, cell->getContentSize().height / 2.f});
        cell->addChild(gauntletSprite);
    }

    auto nameLabel = CCLabelBMFont::create(name.c_str(), "goldFont.fnt");
    nameLabel->setAnchorPoint({0.f, 1.f});
    nameLabel->setPosition({80.f, cell->getContentSize().height - 2.f});
    nameLabel->setScale(0.8f);
    cell->addChild(nameLabel);

    if (!description.empty()) {
        auto descriptionLabel = CCLabelBMFont::create(description.c_str(), "chatFont.fnt");
        descriptionLabel->setAnchorPoint({0.f, 1.f});
        descriptionLabel->setPosition({80.f, cell->getContentSize().height - 30.f});
        descriptionLabel->setScale(0.55f);
        cell->addChild(descriptionLabel);
    }

    auto rewardLabel = CCLabelBMFont::create(("Reward: " + numToString(reward)).c_str(), "bigFont.fnt");
    rewardLabel->setAnchorPoint({0.f, 0.5f});
    rewardLabel->setPosition({80.f, 16.f});
    rewardLabel->setScale(0.35f);
    cell->addChild(rewardLabel);

    return cell;
}
