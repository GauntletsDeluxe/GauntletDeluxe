#include "GDXAddGauntletPopup.hpp"
#include "GDXGauntletManagePopup.hpp"
#include <algorithm>
#include <unordered_map>
#include "../include/GDXConstant.hpp"
#include <arc/runtime/Runtime.hpp>
#include <argon/argon.hpp>
#include <cue/ListNode.hpp>
#include <Geode/binding/SelectArtLayer.hpp>
#include <Geode/ui/LoadingSpinner.hpp>
#include <string>
#include <string_view>

using namespace geode;
using namespace geode::prelude;

static std::string getBgIconSpriteName(int index) {
    int bgIndex = std::clamp(index, 1, 59);
    return fmt::format("bgIcon_{:02d}_001.png", bgIndex);
}

LevelCell* GDXAddGauntletPopup::createLevelCellFromLevel(GJGameLevel* level, int reward) {
    if (!level) {
        return nullptr;
    }

    auto cell = LevelCell::create(356.f, 50.f);
    if (!cell) {
        return nullptr;
    }

    cell->m_compactView = true;
    cell->loadFromLevel(level);
    cell->setContentHeight(50.f);
    configureLevelCell(cell, reward);
    return cell;
}

void GDXAddGauntletPopup::loadNextPendingLevel() {
    if (m_searchingLevel || m_pendingLevelFetches.empty()) {
        return;
    }

    auto pending = m_pendingLevelFetches.front();
    auto glm = GameLevelManager::get();
    if (!glm) {
        return;
    }

    m_searchingLevel = true;
    m_pendingSearchKey = pending.key;
    if (glm->m_levelManagerDelegate == this) {
        glm->m_levelManagerDelegate = nullptr;
    }
    glm->m_levelManagerDelegate = this;

    auto searchObj = GJSearchObject::create(SearchType::Type19, pending.query);
    if (!searchObj) {
        m_searchingLevel = false;
        m_pendingLevelFetches.erase(m_pendingLevelFetches.begin());
        loadNextPendingLevel();
        return;
    }

    glm->getOnlineLevels(searchObj);
}

void GDXAddGauntletPopup::configureLevelCell(LevelCell* cell, int reward) {
    if (!cell || !cell->m_mainMenu) {
        return;
    }

    auto viewBtn = cell->m_mainLayer->getChildByIDRecursive("view-button");
    CCPoint removePos = CCPointZero;
    if (viewBtn) {
        removePos = viewBtn->getPosition();
        viewBtn->removeFromParent();
    }

    if (cell->m_backgroundLayer) {
        auto rewardInput = TextInput::create(60.f, numToString(reward).c_str(), "bigFont.fnt");
        if (rewardInput) {
            rewardInput->setCommonFilter(CommonFilter::Int);
            rewardInput->setString(numToString(reward).c_str());
            rewardInput->setAnchorPoint({1.f, 0.5f});
            rewardInput->setScale(0.55f);
            rewardInput->setPosition({cell->m_backgroundLayer->getContentSize().width - 30.f, 10.f});
            rewardInput->setID("level-reward-input");
            cell->m_mainLayer->addChild(rewardInput, 3);
        }

        auto rewardIcon = CCSprite::createWithSpriteFrameName("GDX_levelPoint.png"_spr);
        if (rewardIcon) {
            rewardIcon->setScale(0.2f);
            rewardIcon->setAnchorPoint({0.f, 0.5f});
            rewardIcon->setPosition({cell->m_backgroundLayer->getContentSize().width - 25.f, 10.f});
            cell->m_mainLayer->addChild(rewardIcon);
        }
    }

    auto deleteSpr = CCSprite::createWithSpriteFrameName("GJ_deleteBtn_001.png");
    deleteSpr->setScale(0.5f);
    auto deleteBtn = CCMenuItemSpriteExtra::create(deleteSpr, this, menu_selector(GDXAddGauntletPopup::onDeleteLevel));
    deleteBtn->setPosition(removePos + ccp(20.f, 10.f));
    cell->m_mainMenu->addChild(deleteBtn);
    deleteBtn->setUserObject(cell);

    auto upSpr = CCSprite::createWithSpriteFrameName("PBtn_Arrow_001.png");
    if (upSpr) {
        upSpr->setRotation(180.f);
        auto upBtn = CCMenuItemSpriteExtra::create(upSpr, this, menu_selector(GDXAddGauntletPopup::onMoveLevelUp));
        upBtn->setPosition({-20, removePos.y + 10.f});
        upBtn->setUserObject(cell);
        cell->m_mainMenu->addChild(upBtn);
    }

    auto downSpr = CCSprite::createWithSpriteFrameName("PBtn_Arrow_001.png");
    if (downSpr) {
        auto downBtn = CCMenuItemSpriteExtra::create(downSpr, this, menu_selector(GDXAddGauntletPopup::onMoveLevelDown));
        downBtn->setPosition({-20, removePos.y - 10.f});
        downBtn->setUserObject(cell);
        cell->m_mainMenu->addChild(downBtn);
    }
}

void GDXAddGauntletPopup::refreshLevelList() {
    if (!m_levelList) {
        return;
    }

    m_levelList->clear();

    if (m_levelCells.size() < m_levels.size()) {
        m_levelCells.resize(m_levels.size(), nullptr);
    }

    if (m_levels.empty()) {
        m_levelList->updateLayout(true);
        return;
    }

    for (size_t i = 0; i < m_levels.size(); ++i) {
        auto* cell = m_levelCells[i];
        if (cell) {
            m_levelList->addCell(cell);
            continue;
        }

        auto spinnerCell = CCLayer::create();
        spinnerCell->setContentSize({356.f, 50.f});

        auto spinner = LoadingSpinner::create(30.f);
        if (spinner) {
            spinner->setAnchorPoint({0.5f, 0.5f});
            spinner->setPosition({spinnerCell->getContentSize().width / 2.f, spinnerCell->getContentSize().height / 2.f});
            spinnerCell->addChild(spinner);
        }

        m_levelList->addCell(spinnerCell);
    }

    m_levelList->updateLayout(true);
}

GDXAddGauntletPopup* GDXAddGauntletPopup::create(GDXGauntletManagePopup* owner) {
    auto ret = new GDXAddGauntletPopup();
    ret->m_owner = owner;
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

GDXAddGauntletPopup* GDXAddGauntletPopup::create(GDXGauntletManagePopup* owner, const matjson::Value& gauntlet, int index) {
    auto ret = new GDXAddGauntletPopup();
    ret->m_owner = owner;
    ret->m_editMode = true;
    ret->m_editIndex = index;
    ret->m_editGauntlet = gauntlet;
    if (ret && ret->init()) {
        ret->applyEditMode();
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool GDXAddGauntletPopup::init() {
    if (!Popup::init(530.f, 280.f)) {
        return false;
    }

    this->setTitle("Add New Gauntlet");
    addSideArt(m_mainLayer, SideArt::TopLeft, SideArtStyle::PopupGold, false);
    addSideArt(m_mainLayer, SideArt::BottomLeft, SideArtStyle::PopupGold, false);

    m_nameInput = TextInput::create(80.f, "Gauntlet Name", "chatFont.fnt");
    m_nameInput->setLabel("Gauntlet Name");
    m_nameInput->setCommonFilter(CommonFilter::Any);
    m_mainLayer->addChildAtPosition(m_nameInput, Anchor::Left, {65, 80});

    m_gauntletReward = TextInput::create(80.f, "Gauntlet Reward", "chatFont.fnt");
    m_gauntletReward->setCommonFilter(CommonFilter::Int);
    m_gauntletReward->setLabel("Gauntlet Reward");
    m_mainLayer->addChildAtPosition(m_gauntletReward, Anchor::Left, {155, 80});

    m_descriptionInput = TextInput::create(105.f, "Gauntlet Description", "chatFont.fnt");
    m_descriptionInput->setLabel("Gauntlet Description");
    m_descriptionInput->setCommonFilter(CommonFilter::Any);
    m_mainLayer->addChildAtPosition(m_descriptionInput, Anchor::Left, {75, 30});

    m_suggestedByInput = TextInput::create(80.f, "Suggested By", "chatFont.fnt");
    m_suggestedByInput->setLabel("Suggested By");
    m_suggestedByInput->setCommonFilter(CommonFilter::Int);
    m_mainLayer->addChildAtPosition(m_suggestedByInput, Anchor::Left, {65, -20});

    m_spriteByInput = TextInput::create(80.f, "Sprite By", "chatFont.fnt");
    m_spriteByInput->setLabel("Sprite By");
    m_spriteByInput->setCommonFilter(CommonFilter::Int);
    m_mainLayer->addChildAtPosition(m_spriteByInput, Anchor::Left, {155, -20});

    m_bgIndex = 14;
    auto bgIcon = CCSprite::createWithSpriteFrameName(getBgIconSpriteName(m_bgIndex).c_str());
    bgIcon->setAnchorPoint({0.5f, 0.5f});
    bgIcon->setScale(0.7f);
    auto bgBtn = CCMenuItemSpriteExtra::create(
        bgIcon,
        this,
        menu_selector(GDXAddGauntletPopup::onPickBackground));
    if (bgBtn) {
        bgBtn->setAnchorPoint({0.5f, 0.5f});
        m_bgIndexButton = bgBtn;
        m_bgIconSpr = bgIcon;
        m_buttonMenu->addChildAtPosition(bgBtn, Anchor::TopRight, {-100.f, -15.f}, false);
        if (auto bgLabel = CCLabelBMFont::create("BG", "bigFont.fnt")) {
            bgLabel->setScale(0.35f);
            bgLabel->setAnchorPoint({1.f, 0.5f});
            bgLabel->setPosition({-5.f, 15.f});
            bgBtn->addChild(bgLabel);
        }
    }

    m_levelInput = TextInput::create(120.f, "Level ID", "chatFont.fnt");
    m_levelInput->setCommonFilter(CommonFilter::Int);
    m_levelInput->setZOrder(2);
    m_mainLayer->addChildAtPosition(m_levelInput, Anchor::Bottom, {10, 22});

    m_levelRewardInput = TextInput::create(120.f, "Level Reward", "chatFont.fnt");
    m_levelRewardInput->setCommonFilter(CommonFilter::Int);
    m_levelRewardInput->setZOrder(2);
    m_mainLayer->addChildAtPosition(m_levelRewardInput, Anchor::Bottom, {160, 22});

    auto colorSpr = CCSprite::create("GJ_squareB_01.png");
    colorSpr->setScale(0.4f);
    if (colorSpr) {
        colorSpr->setColor(m_selectedColor);
    }
    m_colorSpr = colorSpr;
    auto colorBtn = CCMenuItemSpriteExtra::create(
        colorSpr,
        this,
        menu_selector(GDXAddGauntletPopup::onPickColor));
    colorBtn->setPosition({m_descriptionInput->getPositionX() + 90, m_descriptionInput->getPositionY()});
    colorBtn->m_scaleMultiplier = 1.05f;
    m_buttonMenu->addChild(colorBtn);

    auto colorLabel = CCLabelBMFont::create("Color", "goldFont.fnt");
    colorLabel->setScale(0.4);
    colorBtn->addChildAtPosition(colorLabel, Anchor::Top, ccp(0, 0), ccp(0.5, 0));

    // featured toggle
    auto offFeaturedNode = CCSprite::createWithSpriteFrameName("GDX_emptyBox.png"_spr);
    auto onFeaturedNode = CCSprite::createWithSpriteFrameName("GDX_checkBox.png"_spr);
    offFeaturedNode->setScale(0.6f);
    onFeaturedNode->setScale(0.6f);
    m_featureToggle = CCMenuItemToggler::create(offFeaturedNode, onFeaturedNode, this, menu_selector(GDXAddGauntletPopup::onToggleFeatured));
    m_featureToggle->toggle(m_isFeatured);
    m_buttonMenu->addChildAtPosition(m_featureToggle, Anchor::TopRight, {-20.f, -20.f}, false);
    auto featuredLabel = CCLabelBMFont::create("Set\nFeatured", "bigFont.fnt");
    featuredLabel->setAlignment(CCTextAlignment::kCCTextAlignmentRight);
    featuredLabel->setAnchorPoint({1.f, 0.5f});
    featuredLabel->setScale(0.3f);
    m_featureToggle->addChildAtPosition(featuredLabel, Anchor::Left, {-5.f, 0.f}, false);

    auto listSize = CCSizeMake(356.f, 250.f);
    m_levelList = cue::ListNode::create(listSize);
    m_levelList->setScale(0.8f);
    m_levelList->setCellHeight(50.f);
    m_levelList->getScrollLayer()->m_contentLayer->setLayout(
        ColumnLayout::create()
            ->setGap(0.f)
            ->setAxisReverse(true)
            ->setAxisAlignment(AxisAlignment::End)
            ->setAutoGrowAxis(listSize.height));
    m_mainLayer->addChildAtPosition(m_levelList, Anchor::Right, {-180.f, 0.f});
    refreshLevelList();

    auto addLevelBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Add Level", "goldFont.fnt", "GJ_button_05.png"),
        this,
        menu_selector(GDXAddGauntletPopup::onAddLevel));

    auto saveBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Save", "goldFont.fnt", "GJ_button_01.png"),
        this,
        menu_selector(GDXAddGauntletPopup::onSave));

    m_settingsMenu = CCMenu::create(addLevelBtn, saveBtn, nullptr);
    m_settingsMenu->setLayout(ColumnLayout::create()
            ->setGap(10.f)
            ->setAxisAlignment(AxisAlignment::Center)
            ->setAxisReverse(true));
    m_settingsMenu->setContentHeight(100.f);
    m_settingsMenu->updateLayout();
    m_mainLayer->addChildAtPosition(m_settingsMenu, Anchor::BottomLeft, {110, 55}, false);

    return true;
}

void GDXAddGauntletPopup::applyEditMode() {
    if (!m_editMode || !m_editGauntlet.isObject()) {
        return;
    }

    this->setTitle("Edit Gauntlet");
    m_nameInput->setString(m_editGauntlet["name"].asString().unwrapOr(""));
    m_descriptionInput->setString(m_editGauntlet["description"].asString().unwrapOr(""));
    if (m_suggestedByInput) {
        m_suggestedByInput->setString(numToString(m_editGauntlet["suggestedBy"]["accountId"].asInt().unwrapOr(0)).c_str());
    }
    if (m_spriteByInput) {
        m_spriteByInput->setString(numToString(m_editGauntlet["spriteBy"]["accountId"].asInt().unwrapOr(0)).c_str());
    }
    m_bgIndex = m_editGauntlet["bgIndex"].asInt().unwrapOr(14);
    updateBgIcon();
    m_gauntletReward->setString(numToString(m_editGauntlet["reward"].asInt().unwrapOr(0)).c_str());
    m_selectedColor.r = static_cast<GLubyte>(m_editGauntlet["r"].asInt().unwrapOr(255));
    m_selectedColor.g = static_cast<GLubyte>(m_editGauntlet["g"].asInt().unwrapOr(255));
    m_selectedColor.b = static_cast<GLubyte>(m_editGauntlet["b"].asInt().unwrapOr(255));
    if (m_colorSpr) {
        m_colorSpr->setColor(m_selectedColor);
    }

    m_levels.clear();
    m_levelCells.clear();
    m_pendingLevelFetches.clear();
    m_isFeatured = m_editGauntlet["isFeatured"].asBool().unwrapOr(false);
    if (m_featureToggle) {
        m_featureToggle->toggle(m_isFeatured);
    }

    std::string levelIds;
    for (auto i = 0u; i < m_editGauntlet["levelIds"].size(); ++i) {
        auto entry = m_editGauntlet["levelIds"][i];
        if (!entry.isObject()) {
            continue;
        }

        auto levelId = static_cast<int>(entry["levelId"].asInt().unwrapOr(0));
        auto reward = static_cast<int>(entry["reward"].asInt().unwrapOr(0));
        if (levelId <= 0) {
            continue;
        }

        if (!levelIds.empty()) {
            levelIds += ",";
        }
        levelIds += numToString(levelId);

        m_levels.push_back({levelId, reward});
        m_levelCells.emplace_back(nullptr);
    }

    refreshLevelList();

    if (!levelIds.empty()) {
        auto glm = GameLevelManager::get();
        if (glm) {
            auto searchObj = GJSearchObject::create(SearchType::Type19, levelIds);
            if (searchObj) {
                m_pendingLevelFetches.push_back({searchObj->getKey(), levelIds, 0, 0, SIZE_MAX});
                loadNextPendingLevel();
            }
        }
    }
}

void GDXAddGauntletPopup::onAddLevel(CCObject* sender) {
    if (!m_levelInput || !m_levelRewardInput || !m_levelList || !m_gauntletReward || m_searchingLevel) {
        return;
    }

    if (m_levels.size() >= 5) {
        Notification::create("You may only add up to 5 levels.", NotificationIcon::Error)->show();
        return;
    }

    auto levelValue = m_levelInput->getString();
    auto rewardValue = m_levelRewardInput->getString();
    if (levelValue.empty() || rewardValue.empty()) {
        return;
    }

    int levelId = numFromString<int>(levelValue).unwrapOr(0);
    int levelReward = numFromString<int>(rewardValue).unwrapOr(0);
    if (levelId <= 0 || levelReward < 0) {
        return;
    }

    auto glm = GameLevelManager::get();
    if (!glm) {
        return;
    }

    m_pendingLevelId = levelId;
    m_pendingLevelReward = levelReward;
    m_searchingLevel = true;

    if (glm->m_levelManagerDelegate == this) {
        glm->m_levelManagerDelegate = nullptr;
    }
    glm->m_levelManagerDelegate = this;

    auto searchObj = GJSearchObject::create(SearchType::Type19, gd::string(levelValue));
    if (!searchObj) {
        m_searchingLevel = false;
        return;
    }

    m_pendingSearchKey = searchObj->getKey();
    glm->getOnlineLevels(searchObj);
}

void GDXAddGauntletPopup::onPickColor(CCObject* sender) {
    m_colorPopup = ColorPickPopup::create(m_selectedColor);
    if (m_colorPopup) {
        m_colorPopup->setCallback([this](const cocos2d::ccColor4B& color) {
            m_selectedColor.r = color.r;
            m_selectedColor.g = color.g;
            m_selectedColor.b = color.b;
            if (m_colorSpr) {
                m_colorSpr->setColor({color.r, color.g, color.b});
            }
        });
        m_colorPopup->show();
    }
}

void GDXAddGauntletPopup::onPickBackground(CCObject* sender) {
    auto layer = SelectArtLayer::create(SelectArtType::Background, m_bgIndex);
    if (!layer) {
        return;
    }
    layer->m_delegate = this;
    layer->show();
}

void GDXAddGauntletPopup::selectArtClosed(SelectArtLayer* layer) {
    if (!layer) {
        return;
    }

    m_bgIndex = layer->m_art;
    updateBgIcon();
}

void GDXAddGauntletPopup::updateBgIcon() {
    if (!m_bgIndexButton) {
        return;
    }

    auto icon = CCSprite::createWithSpriteFrameName(getBgIconSpriteName(m_bgIndex).c_str());
    icon->setScale(0.7f);
    if (!icon) {
        return;
    }
    m_bgIconSpr = icon;
    m_bgIndexButton->setNormalImage(icon);
}

void GDXAddGauntletPopup::onToggleFeatured(CCObject* sender) {
    m_isFeatured = !m_isFeatured;
}

void GDXAddGauntletPopup::loadLevelsFinished(cocos2d::CCArray* levels, char const* key, int type) {
    if (!m_searchingLevel) {
        return;
    }
    if (!key || m_pendingSearchKey != key) {
        return;
    }

    m_searchingLevel = false;
    auto glm = GameLevelManager::get();
    if (glm && glm->m_levelManagerDelegate == this) {
        glm->m_levelManagerDelegate = nullptr;
    }

    size_t pendingIndex = SIZE_MAX;
    for (size_t i = 0; i < m_pendingLevelFetches.size(); ++i) {
        if (m_pendingLevelFetches[i].key == key) {
            pendingIndex = i;
            break;
        }
    }

    if (pendingIndex == SIZE_MAX) {
        if (!levels || levels->count() == 0) {
            Notification::create("No online level was found for that ID.", NotificationIcon::Error)->show();
            return;
        }

        auto level = static_cast<GJGameLevel*>(levels->objectAtIndex(0));
        if (!level) {
            Notification::create("Search returned an invalid result.", NotificationIcon::Error)->show();
            return;
        }

        m_levels.push_back({m_pendingLevelId, m_pendingLevelReward});
        m_levelInput->setString("");
        m_levelRewardInput->setString("");

        auto cell = createLevelCellFromLevel(level, m_pendingLevelReward);
        if (cell) {
            m_levelCells.push_back(cell);
            m_levelList->addCell(cell);
            m_levelList->getScrollLayer()->m_contentLayer->updateLayout(true);
        }
        return;
    }

    auto pending = m_pendingLevelFetches[pendingIndex];
    m_pendingLevelFetches.erase(m_pendingLevelFetches.begin() + pendingIndex);
    if (pending.index == SIZE_MAX) {
        if (!levels || levels->count() == 0) {
            Notification::create("No online levels were found.", NotificationIcon::Error)->show();
            return;
        }

        std::unordered_map<int, GJGameLevel*> foundLevels;
        for (auto i = 0u; i < levels->count(); ++i) {
            if (auto level = static_cast<GJGameLevel*>(levels->objectAtIndex(i))) {
                foundLevels[level->m_levelID] = level;
            }
        }

        bool inserted = false;
        for (size_t i = 0; i < m_levels.size(); ++i) {
            if (m_levelCells[i]) {
                continue;
            }
            auto const& entry = m_levels[i];
            auto it = foundLevels.find(entry.levelId);
            if (it == foundLevels.end()) {
                continue;
            }
            auto cell = createLevelCellFromLevel(it->second, entry.reward);
            m_levelCells[i] = cell;
            inserted = true;
        }

        if (inserted) {
            refreshLevelList();
        }
        return;
    }

    if (!levels || levels->count() == 0) {
        Notification::create(fmt::format("Could not fetch level {}.", pending.levelId), NotificationIcon::Error)->show();
        loadNextPendingLevel();
        return;
    }

    auto level = static_cast<GJGameLevel*>(levels->objectAtIndex(0));
    if (!level) {
        Notification::create("Search returned an invalid result.", NotificationIcon::Error)->show();
        loadNextPendingLevel();
        return;
    }

    auto cell = createLevelCellFromLevel(level, pending.reward);
    if (pending.index < m_levelCells.size()) {
        m_levelCells[pending.index] = cell;
    }

    if (cell) {
        m_levelList->getScrollLayer()->m_contentLayer->updateLayout(true);
    }

    refreshLevelList();
    loadNextPendingLevel();
}

void GDXAddGauntletPopup::onClose(CCObject* sender) {
    if (m_unsaved) {
        createQuickPopup(
            "Cancel Adding Gauntlet?",
            fmt::format("Are you sure you want to <cr>cancel adding this gauntlet</c>?\n<cy>All progress will be lost</c>."),
            "No",
            "Yes",
            [this](auto, bool yes) {
                if (!yes) return;

                auto glm = GameLevelManager::get();
                if (glm && glm->m_levelManagerDelegate == this) {
                    glm->m_levelManagerDelegate = nullptr;
                }

                this->removeFromParent();
            });
    } else {
        auto glm = GameLevelManager::get();
        if (glm && glm->m_levelManagerDelegate == this) {
            glm->m_levelManagerDelegate = nullptr;
        }
        Popup::onClose(sender);
    }
}

void GDXAddGauntletPopup::loadLevelsFailed(char const* key, int type) {
    if (!m_searchingLevel) {
        return;
    }
    if (!key || m_pendingSearchKey != key) {
        return;
    }

    m_searchingLevel = false;
    auto glm = GameLevelManager::get();
    if (glm && glm->m_levelManagerDelegate == this) {
        glm->m_levelManagerDelegate = nullptr;
    }

    size_t pendingIndex = SIZE_MAX;
    for (size_t i = 0; i < m_pendingLevelFetches.size(); ++i) {
        if (m_pendingLevelFetches[i].key == key) {
            pendingIndex = i;
            break;
        }
    }

    if (pendingIndex == SIZE_MAX) {
        Notification::create("Failed to fetch the online level", NotificationIcon::Error)->show();
        return;
    }

    auto pending = m_pendingLevelFetches[pendingIndex];
    m_pendingLevelFetches.erase(m_pendingLevelFetches.begin() + pendingIndex);
    if (pending.index == SIZE_MAX) {
        Notification::create("Failed to fetch online levels for this gauntlet.", NotificationIcon::Error)->show();
    } else {
        Notification::create(fmt::format("Failed to fetch level {}.", pending.levelId), NotificationIcon::Error)->show();
    }

    loadNextPendingLevel();
}

void GDXAddGauntletPopup::onDeleteLevel(CCObject* sender) {
    auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
    if (!btn || !m_levelList) {
        return;
    }

    auto cell = static_cast<LevelCell*>(btn->getUserObject());
    if (!cell) {
        return;
    }

    auto it = std::find(m_levelCells.begin(), m_levelCells.end(), cell);
    if (it == m_levelCells.end()) {
        return;
    }

    auto idx = static_cast<size_t>(it - m_levelCells.begin());
    m_levelCells.erase(it);
    if (idx < m_levels.size()) {
        m_levels.erase(m_levels.begin() + idx);
    }

    m_levelList->removeCell(idx);
    m_levelList->updateLayout(true);
}

void GDXAddGauntletPopup::onMoveLevelUp(CCObject* sender) {
    auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
    if (!btn) {
        return;
    }

    auto cell = static_cast<LevelCell*>(btn->getUserObject());
    if (!cell) {
        return;
    }

    auto it = std::find(m_levelCells.begin(), m_levelCells.end(), cell);
    if (it == m_levelCells.end()) {
        return;
    }

    auto idx = static_cast<size_t>(it - m_levelCells.begin());
    if (idx == 0) {
        return;
    }

    std::swap(m_levelCells[idx], m_levelCells[idx - 1]);
    if (idx < m_levels.size()) {
        std::swap(m_levels[idx], m_levels[idx - 1]);
    }
    m_unsaved = true;
    refreshLevelList();
}

void GDXAddGauntletPopup::onMoveLevelDown(CCObject* sender) {
    auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
    if (!btn) {
        return;
    }

    auto cell = static_cast<LevelCell*>(btn->getUserObject());
    if (!cell) {
        return;
    }

    auto it = std::find(m_levelCells.begin(), m_levelCells.end(), cell);
    if (it == m_levelCells.end()) {
        return;
    }

    auto idx = static_cast<size_t>(it - m_levelCells.begin());
    if (idx + 1 >= m_levelCells.size()) {
        return;
    }

    std::swap(m_levelCells[idx], m_levelCells[idx + 1]);
    if (idx < m_levels.size() - 1) {
        std::swap(m_levels[idx], m_levels[idx + 1]);
    }
    m_unsaved = true;
    refreshLevelList();
}

void GDXAddGauntletPopup::onSave(CCObject* sender) {
    if (!m_owner || !m_nameInput || !m_descriptionInput || !m_suggestedByInput || !m_spriteByInput) {
        return;
    }

    auto upoup = UploadActionPopup::create(nullptr, "Adding Gauntlet...");
    upoup->show();

    auto name = m_nameInput->getString();
    auto description = m_descriptionInput->getString();
    auto reward = numFromString<int>(m_gauntletReward->getString()).unwrapOr(0);
    if (name.empty() || description.empty()) {
        upoup->showFailMessage("Name and description cannot be empty.");
        return;
    }

    auto accountData = argon::getGameAccountData();
    for (auto i = 0u; i < m_levels.size() && i < m_levelCells.size(); ++i) {
        if (!m_levelCells[i]) {
            continue;
        }
        auto rewardInput = typeinfo_cast<TextInput*>(m_levelCells[i]->getChildByIDRecursive("level-reward-input"));
        if (!rewardInput) {
            continue;
        }
        auto rewardString = rewardInput->getString();
        m_levels[i].reward = numFromString<int>(rewardString).unwrapOr(m_levels[i].reward);
    }

    auto levels = m_levels;
    auto color = m_selectedColor;
    auto url = std::string(gdx::BASE_API_URL) + (m_editMode ? "/editGauntlet" : "/addGauntlet");
    matjson::Value body = matjson::Value::object();
    body["accountId"] = accountData.accountId;
    body["argonToken"] = "";
    body["name"] = std::string(name);
    body["description"] = std::string(description);
    body["reward"] = reward;
    body["r"] = color.r;
    body["g"] = color.g;
    body["b"] = color.b;
    body["isFeatured"] = m_isFeatured;
    body["suggestedBy"] = numFromString<int>(m_suggestedByInput->getString()).unwrapOr(0);
    body["spriteBy"] = numFromString<int>(m_spriteByInput->getString()).unwrapOr(0);
    body["bgIndex"] = m_bgIndex;
    if (m_editMode) {
        body["index"] = m_editIndex;
    }
    body["levels"] = matjson::Value::array();
    for (auto const& entry : levels) {
        matjson::Value levelObj = matjson::Value::object();
        levelObj["levelId"] = entry.levelId;
        levelObj["reward"] = entry.reward;
        body["levels"].push(std::move(levelObj));
    }

    m_addGauntletTask.spawn([this, upoup, url = std::move(url), body = std::move(body), accountData = std::move(accountData)]() mutable -> arc::Future<> {
        auto token = co_await gdx::argonToken(accountData);
        if (token.empty()) {
            co_await geode::async::waitForMainThread([upoup] {
                upoup->showFailMessage("Authentication failed.");
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
            co_await geode::async::waitForMainThread([upoup, response] {
                upoup->showFailMessage(gdx::getResponseMessage(response, "Failed to add gauntlet."));
            });
            co_return;
        }

        if (response.ok()) {
            co_await geode::async::waitForMainThread([this, upoup] {
                if (m_owner) {
                    m_owner->refreshList();
                }
                upoup->showSuccessMessage(m_editMode ? "Gauntlet updated successfully!" : "Gauntlet added successfully!");
                m_unsaved = false;
                this->onClose(nullptr);
            });
        }

        co_return; }, []() {});
}
