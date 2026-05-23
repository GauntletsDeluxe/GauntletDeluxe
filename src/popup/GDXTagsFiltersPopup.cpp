#include "GDXTagsFiltersPopup.hpp"
#include "../include/GDXConstant.hpp"
#include <Geode/ui/Layout.hpp>
#include <Geode/ui/General.hpp>
#include <Geode/ui/Button.hpp>
#include <Geode/ui/LoadingSpinner.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/utils/web.hpp>

using namespace geode::prelude;

GDXTagsFiltersPopup* GDXTagsFiltersPopup::create(std::function<void(std::string const&)> const& onApply, std::string const& initialTag) {
    auto ret = new GDXTagsFiltersPopup();
    if (ret && ret->init()) {
        ret->autorelease();
        ret->m_onApply = onApply;
        ret->m_selectedTag = initialTag;
        ret->m_selectedIndex = -1;
        return ret;
    }
    delete ret;
    return nullptr;
}

bool GDXTagsFiltersPopup::init() {
    if (!Popup::init(350.f, 230.f)) {
        return false;
    }

    setTitle("Search Tags");
    addSideArt(m_mainLayer, SideArt::All, SideArtStyle::PopupGold, false);

    m_tagMenu = CCMenu::create();
    if (m_tagMenu) {
        m_tagMenu->setLayout(RowLayout::create()->setGap(5.f)->setAxisAlignment(AxisAlignment::Center)->setGrowCrossAxis(true)->setAxisReverse(true)->setCrossAxisOverflow(false));
        m_tagMenu->setContentSize({m_mainLayer->getContentWidth() - 30.f, m_mainLayer->getContentHeight() - 100.f});
        m_tagMenu->setPosition({0.f, 0.f});
        m_tagMenu->setZOrder(2);
        m_mainLayer->addChildAtPosition(m_tagMenu, Anchor::Center, {0.f, 5.f}, false);
    }

    auto tagMenuBg = NineSlice::create("square02_001.png");
    tagMenuBg->setContentSize({m_tagMenu->getContentWidth() + 10.f, m_tagMenu->getContentHeight() + 10.f});
    tagMenuBg->setOpacity(100);
    m_mainLayer->addChildAtPosition(tagMenuBg, Anchor::Center, {0.f, 5.f}, false);

    m_loadingSpinner = LoadingSpinner::create(40.f);
    if (m_loadingSpinner) {
        m_loadingSpinner->setVisible(false);
        m_tagMenu->addChildAtPosition(m_loadingSpinner, Anchor::Center, {0.f, 0.f}, false);
    }

    CCMenu* bottomMenu = CCMenu::create();
    bottomMenu->setLayout(RowLayout::create()->setGap(10.f)->setAxisAlignment(AxisAlignment::Center));
    bottomMenu->setContentWidth(m_mainLayer->getContentWidth() - 20.f);
    m_mainLayer->addChildAtPosition(bottomMenu, Anchor::Bottom, {0.f, 25.f}, false);

    auto applyBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Apply", "goldFont.fnt", "GJ_button_01.png"),
        this,
        menu_selector(GDXTagsFiltersPopup::onApply));
    bottomMenu->addChild(applyBtn);

    auto clearBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Clear", "goldFont.fnt", "GJ_button_06.png"),
        this,
        menu_selector(GDXTagsFiltersPopup::onClear));
    bottomMenu->addChild(clearBtn);

    bottomMenu->updateLayout();
    fetchTags();
    scheduleUpdate();
    return true;
}

void GDXTagsFiltersPopup::update(float dt) {
    Popup::update(dt);
}

void GDXTagsFiltersPopup::onApply(CCObject* sender) {
    if (m_onApply && !m_selectedTag.empty()) {
        m_onApply(m_selectedTag);
    }
    this->onClose(nullptr);
}

void GDXTagsFiltersPopup::onClear(CCObject* sender) {
    m_selectedIndex = -1;
    m_selectedTag.clear();
    if (m_onApply) {
        m_onApply(m_selectedTag);
    }
    this->onClose(nullptr);
}

void GDXTagsFiltersPopup::onTagCell(CCObject* sender) {
    auto item = static_cast<CCMenuItemSpriteExtra*>(sender);
    if (!item) {
        return;
    }

    auto index = item->getTag();
    if (index < 0 || !m_tags.isArray() || static_cast<std::size_t>(index) >= m_tags.size()) {
        return;
    }

    auto tag = m_tags[index];
    if (tag.isString()) {
        m_selectedTag = tag.asString().unwrapOr("");
    } else {
        m_selectedTag = tag["name"].asString().unwrapOr("");
    }
    m_selectedIndex = index;
    rebuildTagList();
}

void GDXTagsFiltersPopup::fetchTags() {
    if (m_loadingSpinner) {
        m_loadingSpinner->setVisible(true);
    }

    auto url = std::string(gdx::baseApiUrl()) + "/getTags";
    auto self = geode::Ref<GDXTagsFiltersPopup>(this);
    m_fetchTagsTask.spawn([self = std::move(self), url = std::move(url)]() mutable -> arc::Future<> {
        auto response = co_await geode::utils::web::WebRequest().get(url);
        if (response.error() || response.cancelled() || !response.ok()) {
            co_await geode::async::waitForMainThread([self = std::move(self)] {
                if (self && self->m_loadingSpinner) {
                    self->m_loadingSpinner->setVisible(false);
                }
            });
            co_return;
        }

        auto jsonResult = response.json();
        if (!jsonResult) {
            co_await geode::async::waitForMainThread([self = std::move(self)] {
                if (self && self->m_loadingSpinner) {
                    self->m_loadingSpinner->setVisible(false);
                }
            });
            co_return;
        }

        auto tags = std::move(jsonResult).unwrap();
        co_await geode::async::waitForMainThread([self = std::move(self), tags = std::move(tags)]() mutable {
            if (self) {
                if (self->m_loadingSpinner) {
                    self->m_loadingSpinner->setVisible(false);
                }
                self->createTagList(tags);
            }
        });
        co_return; }, []() {});
}

void GDXTagsFiltersPopup::createTagList(const matjson::Value& tags) {
    m_tags = tags;
    if (!m_selectedTag.empty() && m_tags.isArray()) {
        m_selectedIndex = -1;
        for (auto i = 0u; i < m_tags.size(); ++i) {
            auto const& tag = m_tags[i];
            std::string tagName;
            if (tag.isString()) {
                tagName = tag.asString().unwrapOr("");
            } else {
                tagName = tag["name"].asString().unwrapOr("");
            }
            if (tagName == m_selectedTag) {
                m_selectedIndex = static_cast<int>(i);
                break;
            }
        }
    }
    rebuildTagList();
}

void GDXTagsFiltersPopup::rebuildTagList() {
    if (!m_tagMenu) {
        return;
    }

    clearListContent();

    if (!m_tags.isArray() || m_tags.size() == 0) {
        if (!m_emptyLabel) {
            m_emptyLabel = CCLabelBMFont::create("No tags available.", "goldFont.fnt");
            if (m_emptyLabel) {
                m_emptyLabel->setAnchorPoint({0.5f, 0.5f});
                m_emptyLabel->setScale(0.8f);
                m_tagMenu->addChild(m_emptyLabel);
            }
        }
        if (m_emptyLabel) {
            m_emptyLabel->setPosition({0.f, 0.f});
            m_emptyLabel->setVisible(true);
        }
        return;
    }

    for (auto i = 0u; i < m_tags.size(); ++i) {
        auto const& tag = m_tags[i];
        auto cell = createTagCell(tag, static_cast<int>(i));
        if (cell) {
            m_tagMenu->addChild(cell);
        }
    }

    if (m_emptyLabel) {
        m_emptyLabel->setVisible(false);
    }

    if (m_tagMenu) {
        m_tagMenu->updateLayout();
    }
}

void GDXTagsFiltersPopup::clearListContent() {
    if (!m_tagMenu) {
        return;
    }
    m_tagMenu->removeAllChildrenWithCleanup(true);
    m_emptyLabel = nullptr;
}

CCNode* GDXTagsFiltersPopup::createTagCell(const matjson::Value& tag, int index) {
    // @geode-ignore(unknown-resource)
    auto cell = NineSlice::createWithSpriteFrameName("geode.loader/tab-bg.png");

    std::string tagName;
    if (tag.isString()) {
        tagName = tag.asString().unwrapOr("");
    } else {
        tagName = tag["name"].asString().unwrapOr("");
    }
    if (tagName.empty()) {
        tagName = "Unnamed Tag";
    }

    auto r = static_cast<GLubyte>(tag["r"].asInt().unwrapOr(255));
    auto g = static_cast<GLubyte>(tag["g"].asInt().unwrapOr(255));
    auto b = static_cast<GLubyte>(tag["b"].asInt().unwrapOr(255));
    if (cell) {
        cell->setColor({r, g, b});
    }

    auto tagLabel = CCLabelBMFont::create(tagName.c_str(), "bigFont.fnt");
    if (!tagLabel) {
        return nullptr;
    }
    if (index == m_selectedIndex) {
        cell->setOpacity(255);
        tagLabel->setOpacity(255);
    } else {
        cell->setOpacity(150);
        tagLabel->setOpacity(150);
    }
    tagLabel->setScale(0.45f);
    cell->setContentSize({tagLabel->getScaledContentSize().width + 10.f, tagLabel->getScaledContentSize().height + 10.f});
    cell->addChildAtPosition(tagLabel, Anchor::Center, {0.f, 0.f}, false);

    auto button = CCMenuItemSpriteExtra::create(cell, this, menu_selector(GDXTagsFiltersPopup::onTagCell));
    if (!button) {
        return nullptr;
    }
    button->setTag(index);

    return button;
}
