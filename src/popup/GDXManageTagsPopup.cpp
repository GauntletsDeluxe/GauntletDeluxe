#include "GDXManageTagsPopup.hpp"
#include "GDXAddTagsPopup.hpp"
#include "../include/GDXConstant.hpp"
#include "Geode/ui/Layout.hpp"
#include <Geode/ui/General.hpp>
#include <Geode/ui/Button.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/utils/web.hpp>
#include <argon/argon.hpp>

using namespace geode::prelude;

GDXManageTagsPopup* GDXManageTagsPopup::create() {
    auto ret = new GDXManageTagsPopup();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool GDXManageTagsPopup::init() {
    if (!Popup::init(420.f, 280.f)) {
        return false;
    }

    setTitle("Manage Tags");
    addSideArt(m_mainLayer, SideArt::TopLeft, SideArtStyle::PopupGold, false);
    addSideArt(m_mainLayer, SideArt::TopRight, SideArtStyle::PopupGold, false);

    m_searchInput = geode::TextInput::create(380.f, "Search tags", "chatFont.fnt");
    if (m_searchInput) {
        m_searchInput->setAnchorPoint({0.5f, 1.f});
        m_mainLayer->addChildAtPosition(m_searchInput, Anchor::Top, {0.f, -35.f});
    }

    auto listSize = CCSizeMake(370.f, 160.f);
    m_list = cue::ListNode::create(listSize);
    m_list->getScrollLayer()->m_contentLayer->setLayout(
        ColumnLayout::create()
            ->setGap(0.f)
            ->setAxisReverse(true)
            ->setAxisAlignment(AxisAlignment::End)
            ->setAutoGrowAxis(0.f));
    m_mainLayer->addChildAtPosition(m_list, Anchor::Center, {0.f, -15.f});
    m_scrollbar = geode::Scrollbar::create(m_list->getScrollLayer());
    m_list->addChildAtPosition(m_scrollbar, Anchor::Right, {10.f, 0.f});

    CCMenu* bottomMenu = CCMenu::create();
    bottomMenu->setLayout(RowLayout::create()->setGap(10.f)->setAxisAlignment(AxisAlignment::Center));
    bottomMenu->setContentWidth(m_mainLayer->getContentWidth() - 10.f);
    m_mainLayer->addChildAtPosition(bottomMenu, Anchor::Bottom, {0.f, 25.f}, false);

    if (gdx::isManager()) {
        auto addBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Add Tag", "goldFont.fnt", "GJ_button_01.png"),
            this,
            menu_selector(GDXManageTagsPopup::onAddTag));
        bottomMenu->addChild(addBtn);
    }

    if (gdx::isManager() || gdx::isContributor()) {
        auto webBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Tag Management", "goldFont.fnt", "GJ_button_05.png"),
            this,
            menu_selector(GDXManageTagsPopup::onManageTagsWeb));
        bottomMenu->addChild(webBtn);
    }

    bottomMenu->updateLayout();
    refreshListItems();
    fetchTags();
    scheduleUpdate();
    return true;
}

void GDXManageTagsPopup::update(float dt) {
    Popup::update(dt);
    if (!m_searchInput) {
        return;
    }

    auto query = m_searchInput->getString();
    if (query != m_searchFilter) {
        m_searchFilter = query;
        rebuildTagList();
    }
}

void GDXManageTagsPopup::onAddTag(CCObject* sender) {
    GDXAddTagsPopup::create(this)->show();
}

void GDXManageTagsPopup::onManageTagsWeb(CCObject* sender) {
    auto accountData = argon::getGameAccountData();
    m_fetchTagsTask.spawn([accountData]() -> arc::Future<> {
        auto token = co_await gdx::argonToken(accountData);
        if (token.empty()) {
            co_return;
        }

        auto url = fmt::format(
            "{}/gauntletTagsManage?accountId={}&argonToken={}",
            std::string(gdx::baseApiUrl()),
            accountData.accountId,
            token);
        co_await geode::async::waitForMainThread([url = std::move(url)]() {
            utils::web::openLinkInBrowser(url);
        });
        co_return; }, []() {});
}

void GDXManageTagsPopup::refreshListItems() {
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

void GDXManageTagsPopup::fetchTags() {
    auto url = std::string(gdx::baseApiUrl()) + "/getTags";
    m_fetchTagsTask.spawn([this, url = std::move(url)]() -> arc::Future<> {
        auto response = co_await geode::utils::web::WebRequest().get(url);
        if (response.error() || response.cancelled() || !response.ok()) {
            co_return;
        }

        auto jsonResult = response.json();
        if (!jsonResult) {
            co_return;
        }

        auto tags = std::move(jsonResult).unwrap();
        co_await geode::async::waitForMainThread([this, tags = std::move(tags)]() mutable {
            createTagList(tags);
        });
        co_return; }, []() {});
}

void GDXManageTagsPopup::createTagList(const matjson::Value& tags) {
    m_allTags = tags;
    rebuildTagList();
}

void GDXManageTagsPopup::refreshTags() {
    fetchTags();
}

static matjson::Value filterTagsByName(const matjson::Value& tags, const std::string& query) {
    if (!tags.isArray()) {
        return matjson::Value::array();
    }
    if (query.empty()) {
        return tags;
    }

    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    matjson::Value filtered = matjson::Value::array();
    for (auto const& tag : tags) {
        auto name = tag["name"].asString().unwrapOr("");
        std::string lowerName = name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        if (lowerName.find(lowerQuery) != std::string::npos) {
            filtered.push(tag);
        }
    }
    return filtered;
}

void GDXManageTagsPopup::rebuildTagList() {
    if (!m_list) {
        return;
    }

    auto filteredTags = filterTagsByName(m_allTags, m_searchFilter);
    m_tags = filteredTags;
    m_list->clear();

    if (!filteredTags.isArray() || filteredTags.size() == 0) {
        if (!m_emptyLabel) {
            m_emptyLabel = CCLabelBMFont::create("", "goldFont.fnt");
            if (m_emptyLabel) {
                m_emptyLabel->setAnchorPoint({0.5f, 0.5f});
                m_emptyLabel->setScale(0.8f);
                m_list->addChild(m_emptyLabel);
            }
        }
        if (m_emptyLabel) {
            m_emptyLabel->setString("No tags found.");
            m_emptyLabel->setPosition({m_list->getListSize().width / 2.f, m_list->getListSize().height / 2.f});
            m_emptyLabel->setVisible(true);
        }
        return;
    }

    for (auto i = 0u; i < filteredTags.size(); ++i) {
        auto const& tag = filteredTags[i];
        if (!tag.isObject()) {
            continue;
        }
        auto cell = createTagCell(tag, static_cast<int>(i));
        if (cell) {
            m_list->addCell(cell);
        }
    }
    if (m_emptyLabel) {
        m_emptyLabel->setVisible(false);
    }
    if (m_list->getScrollLayer() && m_list->getScrollLayer()->m_contentLayer) {
        m_list->getScrollLayer()->m_contentLayer->updateLayout();
    }
    m_list->scrollToTop();
}

void GDXManageTagsPopup::clearListContent() {
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

cocos2d::CCNode* GDXManageTagsPopup::createTagCell(const matjson::Value& tag, int index) {
    auto cell = CCLayer::create();
    cell->setContentSize({370.f, 90.f});

    auto r = static_cast<int>(tag["r"].asInt().unwrapOr(128));
    auto g = static_cast<int>(tag["g"].asInt().unwrapOr(128));
    auto b = static_cast<int>(tag["b"].asInt().unwrapOr(128));

    auto name = tag["name"].asString().unwrapOr("Unknown");
    auto description = tag["description"].asString().unwrapOr("");

    auto nameLabel = CCLabelBMFont::create(name.c_str(), "bigFont.fnt");
    if (nameLabel) {
        nameLabel->setScale(.6f);
    }

    // @geode-ignore(unknown-resource)
    auto namebg = NineSlice::createWithSpriteFrameName("geode.loader/tab-bg.png");
    if (namebg && nameLabel) {
        namebg->setContentSize({nameLabel->getContentSize().width + 10.f, nameLabel->getContentSize().height + 10.f});
        namebg->setAnchorPoint({0.5f, 0.5f});
        namebg->setScale(0.7f);
        namebg->setColor({static_cast<GLubyte>(std::clamp(r, 0, 255)),
            static_cast<GLubyte>(std::clamp(g, 0, 255)),
            static_cast<GLubyte>(std::clamp(b, 0, 255))});
        cell->addChildAtPosition(namebg, Anchor::Top, {0, -20}, false);
    }

    if (nameLabel) {
        cell->addChildAtPosition(nameLabel, Anchor::Top, {0, -20}, false);
    }

    auto descriptionLabel = CCLabelBMFont::create(description.c_str(), "chatFont.fnt");
    if (descriptionLabel) {
        descriptionLabel->setPosition({60.f, 45.f});
        descriptionLabel->setScale(0.4f);
        descriptionLabel->limitLabelWidth(260.f, 0.5f, 0.3f);

        auto descriptionBg = NineSlice::create("square02_small.png");
        if (descriptionBg) {
            descriptionBg->setScale(0.5f);
            descriptionBg->setOpacity(150);
            descriptionBg->setContentSize({descriptionLabel->getContentSize().width + 10.f, descriptionLabel->getContentSize().height + 10.f});
            cell->addChildAtPosition(descriptionBg, Anchor::Center, {0.f, -3.f}, false);
        }

        cell->addChildAtPosition(descriptionLabel, Anchor::Center, {0, -3}, false);
    }

    auto actionsMenu = CCMenu::create();
    actionsMenu->setLayout(RowLayout::create()->setGap(10.f)->setAxisAlignment(AxisAlignment::Center));
    actionsMenu->setContentSize({cell->getContentSize().width, 20.f});
    cell->addChildAtPosition(actionsMenu, Anchor::Bottom, {0.f, 15.f}, false);

    if (gdx::isManager()) {
        auto deleteSpr = ButtonSprite::create("Delete", 120.f, 40.f, 1.f, true, "goldFont.fnt", "GJ_button_06.png");
        deleteSpr->setScale(0.8f);
        auto deleteBtn = CCMenuItemSpriteExtra::create(
            deleteSpr,
            this,
            menu_selector(GDXManageTagsPopup::onDeleteTag));
        deleteBtn->setTag(index);
        actionsMenu->addChild(deleteBtn);
    }

    if (gdx::isManager() || gdx::isContributor()) {
        auto editSpr = ButtonSprite::create("Edit", 120.f, 40.f, 1.f, true, "goldFont.fnt", "GJ_button_05.png");
        editSpr->setScale(0.8f);
        auto editBtn = CCMenuItemSpriteExtra::create(
            editSpr,
            this,
            menu_selector(GDXManageTagsPopup::onEditTag));
        editBtn->setTag(index);
        actionsMenu->addChild(editBtn);
    }

    actionsMenu->updateLayout();
    return cell;
}

void GDXManageTagsPopup::onEditTag(CCObject* sender) {
    auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
    if (!btn) {
        return;
    }

    auto index = btn->getTag();
    if (!m_tags.isArray() || index < 0 || static_cast<size_t>(index) >= m_tags.size()) {
        return;
    }

    auto const& tag = m_tags[static_cast<size_t>(index)];
    GDXAddTagsPopup::create(this, tag, index)->show();
}

void GDXManageTagsPopup::onDeleteTag(CCObject* sender) {
    auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
    if (!btn) {
        return;
    }

    auto index = btn->getTag();
    if (!m_tags.isArray() || index < 0 || static_cast<size_t>(index) >= m_tags.size()) {
        return;
    }

    auto tag = m_tags[static_cast<size_t>(index)];
    auto name = tag["name"].asString().unwrapOr("this tag");
    createQuickPopup(
        "Delete Tag?",
        fmt::format("Are you sure you want to delete <cr>{}</c>?", name),
        "No",
        "Yes",
        [this, index](auto, bool yes) {
            if (!yes) {
                return;
            }
            deleteTagAtIndex(index);
        });
}

void GDXManageTagsPopup::deleteTagAtIndex(int index) {
    if (!m_tags.isArray() || index < 0 || static_cast<size_t>(index) >= m_tags.size()) {
        return;
    }

    auto tag = m_tags[static_cast<size_t>(index)];
    auto accountData = argon::getGameAccountData();
    auto url = std::string(gdx::baseApiUrl()) + "/deleteTag";
    matjson::Value body = matjson::Value::object();
    body["accountId"] = accountData.accountId;
    body["argonToken"] = "";
    body["index"] = tag["index"].asInt().unwrapOr(index);

    auto upopup = UploadActionPopup::create(nullptr, "Deleting tag...");
    upopup->show();
    m_deleteTagTask.spawn([this, upopup, url = std::move(url), body = std::move(body), accountData = std::move(accountData)]() mutable -> arc::Future<> {
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
            co_await geode::async::waitForMainThread([upopup, response] {
                upopup->showFailMessage(gdx::getResponseMessage(response, "Failed to delete tag."));
            });
            co_return;
        }

        auto jsonResult = response.json();
        if (!jsonResult) {
            co_await geode::async::waitForMainThread([upopup] {
                upopup->showFailMessage("Failed to delete tag.");
            });
            co_return;
        }

        auto result = std::move(jsonResult).unwrap();
        if (!result["success"].asBool().unwrapOr(false)) {
            co_await geode::async::waitForMainThread([upopup, response] {
                upopup->showFailMessage(gdx::getResponseMessage(response, "Failed to delete tag."));
            });
            co_return;
        }

        co_await geode::async::waitForMainThread([this, upopup] {
            upopup->showSuccessMessage("Tag deleted successfully.");
            fetchTags();
        });
        co_return; }, []() {});
}
