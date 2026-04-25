#pragma once

#include <Geode/Geode.hpp>
#include <Geode/binding/LevelManagerDelegate.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include <cue/ListNode.hpp>
#include <vector>

struct LevelRewardEntry {
    int levelId = 0;
    int reward = 0;
};

class GDXGauntletManagePopup;

class GDXAddGauntletPopup : public geode::Popup, public LevelManagerDelegate {
public:
    static GDXAddGauntletPopup* create(GDXGauntletManagePopup* owner);

private:
    bool init() override;
    void onSave(CCObject* sender);
    void onCancel(CCObject* sender);
    void onAddLevel(CCObject* sender);
    void onPickColor(CCObject* sender);
    void onDeleteLevel(CCObject* sender);
    void refreshLevelList();

    void loadLevelsFinished(cocos2d::CCArray* levels, char const* key, int type) override;
    void loadLevelsFailed(char const* key, int type) override;

    GDXGauntletManagePopup* m_owner = nullptr;
    cocos2d::CCLabelBMFont* m_placeholderLabel = nullptr;
    int m_pendingLevelId = 0;
    int m_pendingLevelReward = 0;
    bool m_searchingLevel = false;
    gd::string m_pendingSearchKey;
    geode::TextInput* m_nameInput = nullptr;
    geode::TextInput* m_gauntletReward = nullptr;
    geode::TextInput* m_descriptionInput = nullptr;
    geode::TextInput* m_levelInput = nullptr;
    geode::TextInput* m_levelRewardInput = nullptr;
    cue::ListNode* m_levelList = nullptr;
    cocos2d::ccColor3B m_selectedColor = {255, 255, 255};
    cocos2d::CCSprite* m_colorSpr = nullptr;
    geode::ColorPickPopup* m_colorPopup = nullptr;
    std::vector<LevelRewardEntry> m_levels;
    std::vector<LevelCell*> m_levelCells;
};
