#include "GDXShowTagsPopup.hpp"
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/ui/General.hpp>

using namespace geode::prelude;

GDXShowTagsPopup* GDXShowTagsPopup::create(std::string const& gauntletName, const matjson::Value& tags) {
    auto ret = new GDXShowTagsPopup();
    if (ret && ret->init(gauntletName, tags)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool GDXShowTagsPopup::init(std::string const& gauntletName, const matjson::Value& tags) {
    if (!Popup::init(360.f, 260.f)) {
        return false;
    }

    m_gauntletName = gauntletName;
    setTitle((m_gauntletName.empty() ? "Gauntlet Tags" : m_gauntletName).c_str());
    addSideArt(m_mainLayer, SideArt::All, SideArtStyle::PopupGold, false);

    m_tags = tags;

    m_tagMenu = CCMenu::create();
    if (m_tagMenu) {
        m_tagMenu->setLayout(RowLayout::create()
                ->setGap(5.f)
                ->setAxisAlignment(AxisAlignment::Center)
                ->setGrowCrossAxis(true)
                ->setAxisReverse(true)
                ->setCrossAxisOverflow(false));
        m_tagMenu->setContentSize({m_mainLayer->getContentWidth() - 30.f, m_mainLayer->getContentHeight() - 80.f});
        m_tagMenu->setPosition({0.f, 0.f});
        m_tagMenu->setZOrder(2);
        m_mainLayer->addChildAtPosition(m_tagMenu, Anchor::Center, {0.f, -10.f}, false);
    }

    auto tagListBg = NineSlice::create("square02_001.png");
    if (tagListBg) {
        tagListBg->setContentSize({m_tagMenu->getContentSize().width + 10.f, m_tagMenu->getContentSize().height + 10.f});
        tagListBg->setOpacity(100);
        m_mainLayer->addChildAtPosition(tagListBg, Anchor::Center, {0.f, -10.f}, false);
    }

    if (!m_tags.isArray() || m_tags.size() == 0) {
        auto emptyLabel = CCLabelBMFont::create("No tags applied.", "goldFont.fnt");
        if (emptyLabel) {
            emptyLabel->setScale(0.8f);
            emptyLabel->setAnchorPoint({0.5f, 0.5f});
            m_mainLayer->addChildAtPosition(emptyLabel, Anchor::Center, {0.f, 5.f}, false);
        }
    } else {
        for (auto i = 0u; i < m_tags.size(); ++i) {
            auto tagCell = createTagCell(m_tags[i], static_cast<int>(i));
            if (tagCell) {
                m_tagMenu->addChild(tagCell);
            }
        }
        if (m_tagMenu) {
            m_tagMenu->updateLayout();
        }
    }

    return true;
}

void GDXShowTagsPopup::update(float dt) {
    Popup::update(dt);
}

cocos2d::CCNode* GDXShowTagsPopup::createTagCell(const matjson::Value& tag, int index) {
    // @geode-ignore(unknown-resource)
    auto cell = NineSlice::createWithSpriteFrameName("geode.loader/tab-bg.png");
    if (!cell) {
        return nullptr;
    }

    std::string tagName = tag["name"].asString().unwrapOr("Unnamed Tag");
    if (tagName.empty()) {
        tagName = "Unnamed Tag";
    }

    auto r = static_cast<GLubyte>(tag["r"].asInt().unwrapOr(255));
    auto g = static_cast<GLubyte>(tag["g"].asInt().unwrapOr(255));
    auto b = static_cast<GLubyte>(tag["b"].asInt().unwrapOr(255));
    cell->setColor({r, g, b});
    cell->setOpacity(255);

    auto tagLabel = CCLabelBMFont::create(tagName.c_str(), "bigFont.fnt");
    if (tagLabel) {
        tagLabel->setScale(0.45f);
        tagLabel->setAnchorPoint({0.5f, 0.5f});
        tagLabel->setColor({255, 255, 255});
        cell->setContentSize({tagLabel->getScaledContentSize().width + 10.f, tagLabel->getScaledContentSize().height + 10.f});
        cell->addChildAtPosition(tagLabel, Anchor::Center, {0.f, 0.f}, false);
    }

    return cell;
}
