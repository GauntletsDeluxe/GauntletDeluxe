#include "GDXGauntletManagePopup.hpp"
#include "GDXAddGauntletPopup.hpp"
#include "../include/GDXConstant.hpp"
#include "Geode/ui/Layout.hpp"
#include "Geode/utils/general.hpp"
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/MultilineBitmapFont.hpp>
#include <Geode/ui/LoadingSpinner.hpp>
#include <Geode/ui/LazySprite.hpp>
#include <Geode/utils/web.hpp>
#include <argon/argon.hpp>
#include <Geode/ui/Scrollbar.hpp>

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
    m_list->getScrollLayer()->m_contentLayer->setLayout(
        ColumnLayout::create()
            ->setGap(0.f)
            ->setAxisReverse(true)
            ->setAxisAlignment(AxisAlignment::End)
            ->setAutoGrowAxis(0.f));
    m_mainLayer->addChildAtPosition(m_list, Anchor::Center, {0.f, 0.f});
    m_scrollbar = geode::Scrollbar::create(m_list->getScrollLayer());
    m_list->addChildAtPosition(m_scrollbar, Anchor::Right, {10.f, 0.f});
    this->scheduleUpdate();

    if (gdx::isManager()) {
        auto addBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Add Gauntlet", "goldFont.fnt", "GJ_button_01.png"),
            this,
            menu_selector(GDXGauntletManagePopup::onAdd));
        m_buttonMenu->addChildAtPosition(addBtn, Anchor::BottomRight, {-110, 25}, false);

        auto openManageBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Asset Manager", "goldFont.fnt", "GJ_button_05.png"),
            this,
            menu_selector(GDXGauntletManagePopup::onManageAssets));
        m_buttonMenu->addChildAtPosition(openManageBtn, Anchor::BottomLeft, {115, 25}, false);
    }

    refreshListItems();
    fetchGauntlets();
    return true;
}

void GDXGauntletManagePopup::onManageAssets(CCObject* sender) {
    auto accountData = argon::getGameAccountData();
    async::spawn([accountData]() -> arc::Future<> {
        auto token = co_await gdx::argonToken(accountData);
        if (token.empty()) {
            co_return;
        }

        auto url = fmt::format(
            "{}/gauntletImageManage?accountId={}&argonToken={}",
            std::string(gdx::BASE_API_URL),
            accountData.accountId,
            token);
        geode::queueInMainThread([url = std::move(url)]() {
            utils::web::openLinkInBrowser(url);
        });
        co_return;
    });
}

void GDXGauntletManagePopup::onAdd(CCObject* sender) {
    GDXAddGauntletPopup::create(this)->show();
}

void GDXGauntletManagePopup::refreshList() {
    refreshListItems();
    fetchGauntlets();
}

void GDXGauntletManagePopup::refreshListItems() {
    if (!m_list) {
        return;
    }

    m_list->clear();
    if (m_list->getScrollLayer() && m_list->getScrollLayer()->m_contentLayer) {
        m_list->getScrollLayer()->m_contentLayer->removeAllChildrenWithCleanup(true);
    }

    auto cell = CCLayer::create();
    cell->setContentSize(m_list->getListSize());

    auto spinner = LoadingSpinner::create(60.f);
    if (spinner) {
        spinner->setAnchorPoint({0.5f, 0.5f});
        spinner->setPosition({cell->getContentSize().width / 2.f, cell->getContentSize().height / 2.f});
        cell->addChild(spinner);
    }

    m_list->addCell(cell);
    if (m_list->getScrollLayer() && m_list->getScrollLayer()->m_contentLayer) {
        m_list->getScrollLayer()->m_contentLayer->updateLayout();
    }
    m_list->scrollToTop();
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

    m_gauntlets = gauntlets;
    m_list->clear();
    if (!gauntlets.isArray() || gauntlets.size() == 0) {
        auto emptyLabel = CCLabelBMFont::create("No gauntlets available.", "goldFont.fnt");
        m_list->setCellHeight(m_list->getListSize().height);
        m_list->addCell(emptyLabel);
        emptyLabel->setAnchorPoint({0.5f, 0.5f});
        emptyLabel->setScale(0.8f);
        emptyLabel->setPosition({m_list->getListSize().width / 2.f, m_list->getListSize().height / 2.f});
        m_list->getScrollLayer()->m_contentLayer->updateLayout();
        m_list->scrollToTop();
        return;
    }

    for (auto i = 0u; i < gauntlets.size(); ++i) {
        auto const& gauntlet = gauntlets[i];
        if (!gauntlet.isObject()) {
            continue;
        }
        auto cell = createGauntletCell(gauntlet, static_cast<int>(i));
        if (cell) {
            m_list->addCell(cell);
        }
    }
    m_list->getScrollLayer()->m_contentLayer->updateLayout();
    m_list->scrollToTop();
}

void GDXGauntletManagePopup::update(float dt) {
    geode::Popup::update(dt);
}

CCNode* GDXGauntletManagePopup::createGauntletCell(const matjson::Value& gauntlet, int index) {
    auto cell = CCLayer::create();
    cell->setContentSize({356.f, 90.f});

    auto r = static_cast<int>(gauntlet["r"].asInt().unwrapOr(200));
    auto g = static_cast<int>(gauntlet["g"].asInt().unwrapOr(120));
    auto b = static_cast<int>(gauntlet["b"].asInt().unwrapOr(40));
    auto startColor = cocos2d::ccColor4B{
        static_cast<GLubyte>(std::clamp(r, 0, 255)),
        static_cast<GLubyte>(std::clamp(g, 0, 255)),
        static_cast<GLubyte>(std::clamp(b, 0, 255)),
        255};
    auto endColor = cocos2d::ccColor4B{
        static_cast<GLubyte>(std::clamp(r / 2, 0, 255)),
        static_cast<GLubyte>(std::clamp(g / 2, 0, 255)),
        static_cast<GLubyte>(std::clamp(b / 2, 0, 255)),
        255};

    if (auto gradient = CCLayerGradient::create(startColor, endColor, {1.f, 0.f})) {
        gradient->setContentSize(cell->getContentSize());
        gradient->setAnchorPoint({0.f, 0.f});
        gradient->setPosition({0.f, 0.f});
        cell->addChild(gradient);
    }

    auto name = gauntlet["name"].asString().unwrapOr("Unknown");
    auto description = gauntlet["description"].asString().unwrapOr("");
    auto reward = gauntlet["reward"].asInt().unwrapOr(0);
    auto gauntletIndex = gauntlet["index"].asInt().unwrapOr(index);

    auto cellMenu = CCMenu::create();
    if (cellMenu) {
        cellMenu->setPosition({cell->getContentSize().width - 30, cell->getContentSize().height / 2.f});
        cellMenu->setContentSize({50, cell->getContentSize().height});
        cellMenu->setLayout(ColumnLayout::create()
                ->setGap(10.f)
                ->setAxisAlignment(AxisAlignment::Center));
        cell->addChild(cellMenu);
    }

    // delete gauntlet
    if (gdx::isManager()) {
        // delete gauntlet
        auto deleteSpr = CCSprite::createWithSpriteFrameName("GJ_deleteBtn_001.png");
        deleteSpr->setScale(0.7f);

        auto deleteBtn = CCMenuItemSpriteExtra::create(deleteSpr, this, menu_selector(GDXGauntletManagePopup::onDelete));
        deleteBtn->setTag(gauntletIndex);
        cellMenu->addChild(deleteBtn);
    }

    // edit gauntlet
    if (gdx::isManager() || gdx::isMod()) {
        auto editSpr = CCSprite::createWithSpriteFrameName("GJ_editBtn_001.png");
        editSpr->setScale(0.4f);
        auto editBtn = CCMenuItemSpriteExtra::create(editSpr, this, menu_selector(GDXGauntletManagePopup::onEdit));
        editBtn->setTag(gauntletIndex);
        cellMenu->addChild(editBtn);
    }

    cellMenu->updateLayout();

    auto const gauntletSpritePosition = ccp(40.f, cell->getContentSize().height / 2.f);
    auto fallbackSprite = CCSprite::createWithSpriteFrameName("GDX_gauntletUnknown.png"_spr);

    if (fallbackSprite) {
        fallbackSprite->setScale(0.65f);
        fallbackSprite->setPosition(gauntletSpritePosition);
        cell->addChild(fallbackSprite, 2);
    }

    auto imageUrl = std::string(gdx::BASE_API_URL) + "/gauntlet/gauntlet_" + numToString(gauntletIndex) + ".png";
    auto gauntletImage = LazySprite::create({72.f, 72.f}, false);
    if (gauntletImage) {
        gauntletImage->setAutoResize(true);
        gauntletImage->setPosition(gauntletSpritePosition);
        gauntletImage->loadFromUrl(imageUrl, CCImage::kFmtPng, true);
        cell->addChild(gauntletImage, 3);
    }

    auto nameLabel = CCLabelBMFont::create(name.c_str(), "goldFont.fnt");
    nameLabel->setAnchorPoint({0.f, .5f});
    nameLabel->setPosition({80.f, cell->getContentSize().height - 10.f});
    nameLabel->limitLabelWidth(200.f, 0.8f, 0.35f);
    cell->addChild(nameLabel);

    if (!description.empty()) {
        auto descriptionLabel = MultilineBitmapFont::createWithFont(
            "chatFont.fnt",
            description,
            0.55f,
            200.f,
            {0.f, 1.f},
            10,
            false);
        if (descriptionLabel) {
            descriptionLabel->setAnchorPoint({0.f, 1.f});
            descriptionLabel->setPosition({85.f, cell->getContentSize().height - 30.f});
            cell->addChild(descriptionLabel, 1);
        }
        auto descriptionBg = NineSlice::create("square02_small.png");
        descriptionBg->setContentSize({220, 40});
        descriptionBg->setInsets({5, 5, 5, 5});
        descriptionBg->setOpacity(100);

        cell->addChildAtPosition(descriptionBg, Anchor::Center, {12.f, 0.f}, false);
    }

    auto rewardSpr = CCSprite::createWithSpriteFrameName("GDX_gauntletPoint.png"_spr);
    if (rewardSpr) {
        rewardSpr->setScale(0.25f);
        rewardSpr->setAnchorPoint({1.f, 0.5f});
        rewardSpr->setPosition({95.f, 13.f});
        cell->addChild(rewardSpr);
    }

    auto rewardLabel = CCLabelBMFont::create((numToString(reward)).c_str(), "bigFont.fnt");
    rewardLabel->setAnchorPoint({0.f, 0.5f});
    rewardLabel->setPosition({100.f, 13.f});
    rewardLabel->limitLabelWidth(180.f, 0.35f, 0.2f);
    cell->addChild(rewardLabel);

    return cell;
}

void GDXGauntletManagePopup::onDelete(CCObject* sender) {
    auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
    if (!btn) {
        return;
    }

    auto gauntletIndex = btn->getTag();
    createQuickPopup(
        "Delete Gauntlet?",
        fmt::format("Are you sure you want to <cr>delete this gauntlet</c>?"),
        "No",
        "Yes",
        [this, gauntletIndex](auto, bool yes) {
            if (!yes) {
                return;
            }
            this->deleteGauntletAtIndex(gauntletIndex);
        });
}

void GDXGauntletManagePopup::onEdit(CCObject* sender) {
    auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
    if (!btn) {
        return;
    }

    auto gauntletIndex = btn->getTag();
    if (!m_gauntlets.isArray()) {
        return;
    }

    for (auto i = 0u; i < m_gauntlets.size(); ++i) {
        auto const& gauntlet = m_gauntlets[i];
        if (!gauntlet.isObject()) {
            continue;
        }
        auto index = gauntlet["index"].asInt().unwrapOr(static_cast<int>(i));
        if (index == gauntletIndex) {
            auto popup = GDXAddGauntletPopup::create(this, gauntlet, index);
            if (popup) {
                popup->show();
            }
            return;
        }
    }
}

void GDXGauntletManagePopup::deleteGauntletAtIndex(int index) {
    auto accountData = argon::getGameAccountData();
    auto url = std::string(gdx::BASE_API_URL) + "/deleteGauntlet";
    matjson::Value body = matjson::Value::object();
    body["accountId"] = accountData.accountId;
    body["argonToken"] = "";
    body["gauntletIndex"] = index;

    auto upopup = UploadActionPopup::create(nullptr, "Deleting Gauntlet...");
    upopup->show();
    async::spawn([this, upopup, url = std::move(url), body = std::move(body), accountData = std::move(accountData)]() mutable -> arc::Future<> {
        auto token = co_await gdx::argonToken(accountData);
        if (token.empty()) {
            geode::queueInMainThread([upopup] {
                upopup->showFailMessage("Authentication failed.");
            });
            co_return;
        }

        body["argonToken"] = std::move(token);

        auto response = co_await geode::utils::web::WebRequest()
                            .url(url)
                            .header("Content-Type", "application/json")
                            .bodyJSON(body)
                            .post(url);

        if (response.error() || response.cancelled() || !response.ok()) {
            geode::queueInMainThread([response, upopup] {
                upopup->showFailMessage(gdx::getResponseMessage(response, "Failed to delete gauntlet."));
            });
            co_return;
        }

        geode::queueInMainThread([this, upopup] {
            this->refreshList();
            upopup->showSuccessMessage("Gauntlet deleted successfully.");
        });
        co_return;
    });
}
