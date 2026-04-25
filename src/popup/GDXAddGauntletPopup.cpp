#include "GDXAddGauntletPopup.hpp"
#include "GDXGauntletManagePopup.hpp"
#include <algorithm>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/LevelCell.hpp>
#include <Geode/binding/GameLevelManager.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/GJSearchObject.hpp>
#include <Geode/ui/ColorPickPopup.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/ui/Layout.hpp>
#include <Geode/utils/general.hpp>
#include <Geode/utils/web.hpp>
#include "../include/GDXConstant.hpp"
#include <arc/runtime/Runtime.hpp>
#include <argon/argon.hpp>
#include <cue/ListNode.hpp>
#include <string>
#include <string_view>

using namespace geode;
using namespace geode::prelude;

void GDXAddGauntletPopup::refreshLevelList() {
    if (!m_levelList) {
        return;
    }

    if (m_placeholderLabel) {
        m_placeholderLabel->removeFromParent();
        m_placeholderLabel = nullptr;
    }

    m_levelList->clear();
    m_levelCells.clear();
    if (m_levels.empty()) {
        m_placeholderLabel = CCLabelBMFont::create("No levels added yet.", "goldFont.fnt");
        m_levelList->addChildAtPosition(m_placeholderLabel, Anchor::Center, {0, 0}, false);
        return;
    }

    for (auto const& entry : m_levels) {
        auto lineText = std::string("- ") + numToString(entry.levelId) + " (reward " + numToString(entry.reward) + ")";
        auto line = CCLabelBMFont::create(lineText.c_str(), "chatFont.fnt");
        line->setAnchorPoint({0.f, 0.5f});
        m_levelList->addCell(line);
    }
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

bool GDXAddGauntletPopup::init() {
    if (!Popup::init(530.f, 280.f)) {
        return false;
    }

    this->setTitle("Add New Gauntlet");

    m_nameInput = TextInput::create(80.f, "Gauntlet Name", "chatFont.fnt");
    m_nameInput->setLabel("Gauntlet Name");
    m_mainLayer->addChildAtPosition(m_nameInput, Anchor::Left, {65, 80});

    m_gauntletReward = TextInput::create(80.f, "Gauntlet Reward", "chatFont.fnt");
    m_gauntletReward->setCommonFilter(CommonFilter::Int);
    m_gauntletReward->setLabel("Gauntlet Reward");
    m_mainLayer->addChildAtPosition(m_gauntletReward, Anchor::Left, {155, 80});

    m_descriptionInput = TextInput::create(105.f, "Gauntlet Description", "chatFont.fnt");
    m_descriptionInput->setLabel("Gauntlet Description");
    m_mainLayer->addChildAtPosition(m_descriptionInput, Anchor::Left, {75, 30});

    m_levelInput = TextInput::create(80.f, "Level ID", "chatFont.fnt");
    m_levelInput->setCommonFilter(CommonFilter::Int);
    m_levelInput->setLabel("Level ID");
    m_mainLayer->addChildAtPosition(m_levelInput, Anchor::Left, {65, -20});

    m_levelRewardInput = TextInput::create(80.f, "Level Reward", "chatFont.fnt");
    m_levelRewardInput->setCommonFilter(CommonFilter::Int);
    m_levelRewardInput->setLabel("Reward");
    m_mainLayer->addChildAtPosition(m_levelRewardInput, Anchor::Left, {155, -20});

    auto addLevelBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Add Level", "goldFont.fnt", "GJ_button_05.png"),
        this,
        menu_selector(GDXAddGauntletPopup::onAddLevel));
    m_buttonMenu->addChildAtPosition(addLevelBtn, Anchor::Left, {110, -60}, false);

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
    m_buttonMenu->addChild(colorBtn);

    auto listSize = CCSizeMake(356.f, 280.f);
    m_levelList = cue::ListNode::create(listSize);
    m_levelList->setScale(0.8f);
    m_mainLayer->addChildAtPosition(m_levelList, Anchor::Right, {-180.f, -10.f});
    refreshLevelList();

    auto saveBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Save", "goldFont.fnt", "GJ_button_01.png"),
        this,
        menu_selector(GDXAddGauntletPopup::onSave));
    m_buttonMenu->addChildAtPosition(saveBtn, Anchor::BottomLeft, {110, 25}, false);

    return true;
}

void GDXAddGauntletPopup::onAddLevel(CCObject* sender) {
    if (!m_levelInput || !m_levelRewardInput || !m_levelList || !m_gauntletReward || m_searchingLevel) {
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

void GDXAddGauntletPopup::loadLevelsFinished(cocos2d::CCArray* levels, char const* key, int type) {
    if (!m_searchingLevel) {
        return;
    }
    if (key && m_pendingSearchKey != key) {
        return;
    }

    m_searchingLevel = false;

    if (!levels || levels->count() == 0) {
        geode::queueInMainThread([this] {
            Notification::create("Online level was found for that ID.", NotificationIcon::Error)->show();
        });
        return;
    }

    auto level = static_cast<GJGameLevel*>(levels->objectAtIndex(0));
    if (!level) {
        geode::queueInMainThread([this] {
            Notification::create("Search returned an invalid result.", NotificationIcon::Error)->show();
        });
        return;
    }

    m_levels.push_back({m_pendingLevelId, m_pendingLevelReward});
    m_levelInput->setString("");
    m_levelRewardInput->setString("");

    if (m_placeholderLabel) {
        m_placeholderLabel->removeFromParent();
        m_placeholderLabel = nullptr;
    }

    auto cell = LevelCell::create(356.f, 90.f);
    if (cell) {
        cell->loadFromLevel(level);
        cell->setContentHeight(90.f);
        m_levelCells.push_back(cell);
        m_levelList->addCell(cell);
        m_levelList->updateLayout();

        if (cell->m_mainMenu) {
            // disable the view button and use its spot for the remove button
            auto viewBtn = cell->m_mainLayer->getChildByIDRecursive("view-button");
            CCPoint removePos = CCPointZero;
            if (viewBtn) {
                removePos = viewBtn->getPosition();
                viewBtn->removeFromParent();
            }

            auto deleteSpr = CCSprite::createWithSpriteFrameName("GJ_deleteBtn_001.png");
            auto deleteBtn = CCMenuItemSpriteExtra::create(deleteSpr, this, menu_selector(GDXAddGauntletPopup::onDeleteLevel));

            deleteBtn->setPosition(removePos);
            cell->m_mainMenu->addChild(deleteBtn);
            deleteBtn->setUserObject(cell);
        }
    }
}

void GDXAddGauntletPopup::loadLevelsFailed(char const* key, int type) {
    if (!m_searchingLevel) {
        return;
    }
    if (key && m_pendingSearchKey != key) {
        return;
    }

    m_searchingLevel = false;
    geode::queueInMainThread([this, key] {
        Notification::create("Failed to fetch the online level", NotificationIcon::Error)->show();
    });
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
    m_levelList->updateLayout();
}

void GDXAddGauntletPopup::onSave(CCObject* sender) {
    if (!m_owner || !m_nameInput || !m_descriptionInput) {
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
    auto levels = m_levels;
    auto color = m_selectedColor;
    auto url = std::string(gdx::BASE_API_URL) + "/addGauntlet";
    matjson::Value body = matjson::Value::object();
    body["accountId"] = accountData.accountId;
    body["argonToken"] = std::string(accountData.gjp2);
    body["name"] = std::string(name);
    body["description"] = std::string(description);
    body["reward"] = reward;
    body["r"] = color.r;
    body["g"] = color.g;
    body["b"] = color.b;
    body["levels"] = matjson::Value::array();
    for (auto const& entry : levels) {
        matjson::Value levelObj = matjson::Value::object();
        levelObj["levelId"] = entry.levelId;
        levelObj["reward"] = entry.reward;
        body["levels"].push(std::move(levelObj));
    }

    async::spawn([this, upoup, url = std::move(url), body = std::move(body), accountData = std::move(accountData)]() mutable -> arc::Future<> {
        auto authResult = co_await argon::startAuth(accountData);
        if (!authResult) {
            geode::queueInMainThread([upoup] {
                upoup->showFailMessage("Authentication failed.");
            });
            co_return;
        }

        auto token = std::move(authResult).unwrap();
        body["argonToken"] = std::move(token);

        auto response = co_await geode::utils::web::WebRequest()
                            .url(url)
                            .header("Content-Type", "application/json")
                            .bodyJSON(body)
                            .post(url);

        if (response.error() || response.cancelled() || !response.ok()) {
            geode::queueInMainThread([upoup, response] {
                upoup->showFailMessage(gdx::getResponseFailMessage(response, "Failed to add gauntlet."));
            });
            co_return;
        }

        if (response.ok()) {
            geode::queueInMainThread([this, upoup] {
                if (m_owner) {
                    m_owner->refreshList();
                }
                upoup->showSuccessMessage("Gauntlet added successfully!");
                this->onClose(nullptr);
            });
        }

        co_return;
    });
}
