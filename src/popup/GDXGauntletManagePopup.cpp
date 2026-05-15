#include "GDXGauntletManagePopup.hpp"
#include "GDXAddGauntletPopup.hpp"
#include "../include/GDXConstant.hpp"
#include <asp/fs.hpp>
#include "GDXUserPanelPopup.hpp"
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

namespace {
    static asp::fs::path getLocalGauntletPath() {
        auto dir = geode::dirs::getModsSaveDir() / geode::Mod::get()->getID();
        if (auto res = asp::fs::createDirAll(dir); !res) {
            log::warn("Failed to create local gauntlet save directory: {}", res.unwrapErr().message());
        }
        return dir / "local_gauntlets.json";
    }

    static matjson::Value loadLocalGauntlets() {
        auto path = getLocalGauntletPath();
        if (!asp::fs::isFile(path).unwrapOr(false)) {
            return matjson::Value::array();
        }

        auto content = asp::fs::readToString(path);
        if (!content) {
            log::warn("Failed to read local gauntlet file: {}", content.unwrapErr());
            return matjson::Value::array();
        }

        auto jsonResult = matjson::parse(content.unwrap());
        if (!jsonResult) {
            log::warn("Failed to parse local gauntlet JSON: {}", jsonResult.unwrapErr());
            return matjson::Value::array();
        }

        auto data = std::move(jsonResult).unwrap();
        if (data.isObject() && data["gauntlets"].isArray()) {
            return data["gauntlets"];
        }
        if (data.isArray()) {
            return data;
        }
        return matjson::Value::array();
    }

    static bool saveLocalGauntlets(matjson::Value const& gauntlets) {
        auto path = getLocalGauntletPath();
        matjson::Value data = matjson::Value::array();
        if (gauntlets.isArray()) {
            for (auto const& item : gauntlets) {
                data.push(item);
            }
        }
        auto result = asp::fs::write(path, data.dump().c_str(), data.dump().size());
        if (!result) {
            log::warn("Failed to save local gauntlets: {}", result.unwrapErr());
        }
        return static_cast<bool>(result);
    }

    static matjson::Value filterGauntletsByName(const matjson::Value& gauntlets, const std::string& query) {
        if (!gauntlets.isArray()) {
            return matjson::Value::array();
        }
        if (query.empty()) {
            return gauntlets;
        }

        std::string lowerQuery = query;
        std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

        matjson::Value filtered = matjson::Value::array();
        for (auto const& gauntlet : gauntlets) {
            auto name = gauntlet["name"].asString().unwrapOr("");
            std::string lowerName = name;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            if (lowerName.find(lowerQuery) != std::string::npos) {
                filtered.push(gauntlet);
            }
        }
        return filtered;
    }
}

GDXGauntletManagePopup* GDXGauntletManagePopup::create() {
    return create(false);
}

GDXGauntletManagePopup* GDXGauntletManagePopup::create(bool localMode) {
    auto ret = new GDXGauntletManagePopup();
    ret->m_localMode = localMode;
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

    setTitle(m_localMode ? "Manage Local Gauntlets" : "Manage Gauntlets Deluxe");
    addSideArt(m_mainLayer, SideArt::TopLeft, SideArtStyle::PopupGold, false);
    addSideArt(m_mainLayer, SideArt::TopRight, SideArtStyle::PopupGold, false);

    auto listSize = CCSizeMake(356.f, 150.f);
    m_list = cue::ListNode::create(listSize);
    m_list->getScrollLayer()->m_contentLayer->setLayout(
        ColumnLayout::create()
            ->setGap(0.f)
            ->setAxisReverse(true)
            ->setAxisAlignment(AxisAlignment::End)
            ->setAutoGrowAxis(0.f));
    m_mainLayer->addChildAtPosition(m_list, Anchor::Center, {0.f, -40.f});
    m_scrollbar = geode::Scrollbar::create(m_list->getScrollLayer());
    m_list->addChildAtPosition(m_scrollbar, Anchor::Right, {10.f, 0.f});

    m_searchInput = geode::TextInput::create(320.f, "Search gauntlets", "chatFont.fnt");
    if (m_searchInput) {
        m_searchInput->setAnchorPoint({0.5f, 1.f});
        m_mainLayer->addChildAtPosition(m_searchInput, Anchor::Top, {0.f, -30.f});
    }

    this->scheduleUpdate();

    CCMenu* bottomMenu = CCMenu::create();
    bottomMenu->setLayout(RowLayout::create()->setGap(10.f)->setAxisAlignment(AxisAlignment::Center));
    bottomMenu->setContentWidth(m_mainLayer->getContentWidth() - 10.f);
    m_mainLayer->addChildAtPosition(bottomMenu, Anchor::Bottom, {0.f, 25.f}, false);

    if (m_localMode) {
        auto addBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Add Local Gauntlet", "goldFont.fnt", "GJ_button_01.png"),
            this,
            menu_selector(GDXGauntletManagePopup::onAdd));
        bottomMenu->addChild(addBtn);
    } else {
        // manager only
        if (gdx::isManager()) {
            auto addBtn = CCMenuItemSpriteExtra::create(
                ButtonSprite::create("Add Gauntlet", "goldFont.fnt", "GJ_button_01.png"),
                this,
                menu_selector(GDXGauntletManagePopup::onAdd));
            bottomMenu->addChild(addBtn);
        }

        // manager and contributor
        if (gdx::isManager() || gdx::isContributor()) {
            auto openManageBtn = CCMenuItemSpriteExtra::create(
                ButtonSprite::create("Asset Manager", "goldFont.fnt", "GJ_button_05.png"),
                this,
                menu_selector(GDXGauntletManagePopup::onManageAssets));
            bottomMenu->addChild(openManageBtn);

            auto userBtn = CCMenuItemSpriteExtra::create(
                ButtonSprite::create("User Panel", "goldFont.fnt", "GJ_button_05.png"),
                this,
                menu_selector(GDXGauntletManagePopup::onUserPanel));
            bottomMenu->addChild(userBtn);
        }
    }

    bottomMenu->updateLayout();

    refreshListItems();
    fetchGauntlets();
    return true;
}

void GDXGauntletManagePopup::onUserPanel(CCObject* sender) {
    GDXUserPanelPopup::create()->show();
}

void GDXGauntletManagePopup::onManageAssets(CCObject* sender) {
    auto accountData = argon::getGameAccountData();
    m_manageAssetsTask.spawn([accountData]() -> arc::Future<> {
        auto token = co_await gdx::argonToken(accountData);
        if (token.empty()) {
            co_return;
        }

        auto url = fmt::format(
            "{}/gauntletImageManage?accountId={}&argonToken={}",
            std::string(gdx::baseApiUrl()),
            accountData.accountId,
            token);
        co_await geode::async::waitForMainThread([url = std::move(url)]() {
            utils::web::openLinkInBrowser(url);
        });
        co_return; }, []() {});
}

void GDXGauntletManagePopup::onAdd(CCObject* sender) {
    GDXAddGauntletPopup::create(this, m_localMode)->show();
}

void GDXGauntletManagePopup::refreshList() {
    refreshListItems();
    fetchGauntlets();
}

void GDXGauntletManagePopup::clearListContent() {
    if (!m_list) {
        return;
    }

    m_list->clear();
    if (m_list->getScrollLayer() && m_list->getScrollLayer()->m_contentLayer) {
        m_list->getScrollLayer()->m_contentLayer->removeAllChildrenWithCleanup(true);
    }
    if (m_emptyLabel) {
        m_emptyLabel->setVisible(false);
    }
}

void GDXGauntletManagePopup::refreshListItems() {
    if (!m_list) {
        return;
    }

    clearListContent();

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
    if (m_localMode) {
        auto gauntlets = loadLocalGauntlets();
        m_allGauntlets = gauntlets;
        rebuildGauntletList();
        return;
    }

    auto url = std::string(gdx::baseApiUrl()) + "/getGauntlets";
    m_fetchGauntletsTask.spawn([this, url = std::move(url)]() -> arc::Future<> {
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
        co_await geode::async::waitForMainThread([this, gauntlets = std::move(gauntlets)]() mutable {
            createGauntletList(gauntlets);
        });
        co_return; }, []() {});
}

void GDXGauntletManagePopup::createGauntletList(const matjson::Value& gauntlets) {
    m_allGauntlets = gauntlets;
    rebuildGauntletList();
}

void GDXGauntletManagePopup::rebuildGauntletList() {
    if (!m_list) {
        return;
    }

    auto filteredGauntlets = filterGauntletsByName(m_allGauntlets, m_searchFilter);
    m_gauntlets = filteredGauntlets;
    m_list->clear();

    if (!filteredGauntlets.isArray() || filteredGauntlets.size() == 0) {
        if (!m_emptyLabel) {
            m_emptyLabel = CCLabelBMFont::create("", "goldFont.fnt");
            if (m_emptyLabel) {
                m_emptyLabel->setAnchorPoint({0.5f, 0.5f});
                m_emptyLabel->setScale(0.8f);
                m_list->addChild(m_emptyLabel);
            }
        }
        if (m_emptyLabel) {
            m_emptyLabel->setString(m_localMode ? "No local gauntlets saved." : "No gauntlets available.");
            m_emptyLabel->setPosition({m_list->getListSize().width / 2.f, m_list->getListSize().height / 2.f});
            m_emptyLabel->setVisible(true);
        }
        return;
    }

    for (auto i = 0u; i < filteredGauntlets.size(); ++i) {
        auto const& gauntlet = filteredGauntlets[i];
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
    if (!m_searchInput) {
        return;
    }

    auto query = m_searchInput->getString();
    if (query != m_searchFilter) {
        m_searchFilter = query;
        rebuildGauntletList();
    }
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

    auto tagValue = m_localMode ? index : gauntletIndex;

    if (m_localMode || gdx::isManager()) {
        auto deleteSpr = CCSprite::createWithSpriteFrameName("GJ_deleteBtn_001.png");
        deleteSpr->setScale(0.7f);

        auto deleteBtn = CCMenuItemSpriteExtra::create(deleteSpr, this, menu_selector(GDXGauntletManagePopup::onDelete));
        deleteBtn->setTag(tagValue);
        cellMenu->addChild(deleteBtn);
    }

    if (m_localMode || gdx::isManager() || gdx::isContributor()) {
        auto editSpr = CCSprite::createWithSpriteFrameName("GJ_editBtn_001.png");
        editSpr->setScale(0.4f);
        auto editBtn = CCMenuItemSpriteExtra::create(editSpr, this, menu_selector(GDXGauntletManagePopup::onEdit));
        editBtn->setTag(tagValue);
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

    auto gauntletImage = LazySprite::create({50.f, 120.f}, false);
    if (gauntletImage) {
        gauntletImage->setAutoResize(true);
        gauntletImage->setPosition(gauntletSpritePosition);
        gauntletImage->setLoadCallback([fallbackSprite](geode::Result<> const& result) {
            if (result && fallbackSprite) {
                fallbackSprite->removeFromParent();
            }
        });

        if (m_localMode && gauntlet["spritePath"].isString()) {
            auto spritePath = gauntlet["spritePath"].asString().unwrapOr("");
            if (!spritePath.empty()) {
                gauntletImage->loadFromFile(spritePath, CCImage::kFmtPng, true);
            } else {
                auto imageUrl = std::string(gdx::baseApiUrl()) + "/gauntlet/gauntlet_" + numToString(gauntletIndex) + ".png?v2=true";
                gauntletImage->loadFromUrl(imageUrl, CCImage::kFmtPng, true);
            }
        } else {
            auto imageUrl = std::string(gdx::baseApiUrl()) + "/gauntlet/gauntlet_" + numToString(gauntletIndex) + ".png?v2=true";
            gauntletImage->loadFromUrl(imageUrl, CCImage::kFmtPng, true);
        }
        cell->addChild(gauntletImage, 3);
    }

    auto nameLabel = CCLabelBMFont::create(name.c_str(), "goldFont.fnt");
    nameLabel->setAnchorPoint({0.f, .5f});
    nameLabel->setPosition({80.f, cell->getContentSize().height - 10.f});
    nameLabel->limitLabelWidth(200.f, 0.8f, 0.35f);
    cell->addChild(nameLabel);

    if (gauntlet["isFeatured"].asBool().unwrapOr(false)) {
        auto featuredLabel = CCLabelBMFont::create("Featured", "goldFont.fnt");
        featuredLabel->setAnchorPoint({0.f, .5f});
        featuredLabel->setScale(0.35f);
        featuredLabel->setColor({255, 215, 0});
        featuredLabel->setPosition({5.f, 5.f});
        cell->addChild(featuredLabel, 2);
    }

    if (!description.empty()) {
        auto descriptionLabel = SimpleTextArea::create(
            description,
            "chatFont.fnt",
            .5f,
            200.f);
        if (descriptionLabel) {
            descriptionLabel->setAnchorPoint({0.f, 1.f});
            descriptionLabel->setPosition({85.f, cell->getContentSize().height - 30.f});
            descriptionLabel->setMaxLines(4);
            cell->addChild(descriptionLabel, 1);
        }
        auto descriptionBg = NineSlice::create("square02_small.png");
        descriptionBg->setContentSize({220, 40});
        descriptionBg->setInsets({5, 5, 5, 5});
        descriptionBg->setOpacity(100);

        cell->addChildAtPosition(descriptionBg, Anchor::Center, {12.f, 0.f}, false);
    }

    if (!m_localMode) {
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
    }

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

    if (m_localMode) {
        if (gauntletIndex < 0 || static_cast<size_t>(gauntletIndex) >= m_gauntlets.size()) {
            return;
        }
        auto const& gauntlet = m_gauntlets[static_cast<size_t>(gauntletIndex)];
        auto popup = GDXAddGauntletPopup::create(this, gauntlet, gauntletIndex, true);
        if (popup) {
            popup->show();
        }
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
    if (m_localMode) {
        auto gauntlets = loadLocalGauntlets();
        if (!gauntlets.isArray() || index < 0 || static_cast<size_t>(index) >= gauntlets.size()) {
            return;
        }
        matjson::Value updated = matjson::Value::array();
        for (auto i = 0u; i < gauntlets.size(); ++i) {
            if (static_cast<int>(i) == index) {
                continue;
            }
            updated.push(gauntlets[i]);
        }
        if (saveLocalGauntlets(updated)) {
            refreshList();
        }
        return;
    }

    auto accountData = argon::getGameAccountData();
    auto url = std::string(gdx::baseApiUrl()) + "/deleteGauntlet";
    matjson::Value body = matjson::Value::object();
    body["accountId"] = accountData.accountId;
    body["argonToken"] = "";
    body["gauntletIndex"] = index;

    auto upopup = UploadActionPopup::create(nullptr, "Deleting Gauntlet...");
    upopup->show();
    m_deleteGauntletTask.spawn([this, upopup, url = std::move(url), body = std::move(body), accountData = std::move(accountData)]() mutable -> arc::Future<> {
        auto token = co_await gdx::argonToken(accountData);
        if (token.empty()) {
            co_await geode::async::waitForMainThread([upopup] {
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
            co_await geode::async::waitForMainThread([response, upopup] {
                upopup->showFailMessage(gdx::getResponseMessage(response, "Failed to delete gauntlet."));
            });
            co_return;
        }

        co_await geode::async::waitForMainThread([this, upopup] {
            this->refreshList();
            upopup->showSuccessMessage("Gauntlet deleted successfully.");
        });
        co_return; }, []() {});
}
