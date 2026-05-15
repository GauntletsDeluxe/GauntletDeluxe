#pragma once

#include <Geode/Geode.hpp>
#include <Geode/binding/LevelManagerDelegate.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/binding/SelectArtDelegate.hpp>
#include <cue/ListNode.hpp>
#include <vector>

class SelectArtLayer;

struct LevelRewardEntry {
    int levelId = 0;
    std::string levelName;
    std::string creatorName;
    int reward = 0;
};

class GDXGauntletManagePopup;

class GDXAddGauntletPopup : public geode::Popup, public LevelManagerDelegate, public SelectArtDelegate {
public:
    static GDXAddGauntletPopup* create(GDXGauntletManagePopup* owner, bool localMode = false);
    static GDXAddGauntletPopup* create(GDXGauntletManagePopup* owner, const matjson::Value& gauntlet, int index, bool localMode = false);

private:
    struct PendingLevelFetch {
        gd::string key;
        gd::string query;
        int levelId = 0;
        int reward = 0;
        size_t index = 0;
    };

    bool init() override;
    void onClose(CCObject* sender) override;
    void onSave(CCObject* sender);
    void onCancel(CCObject* sender);
    void onAddLevel(CCObject* sender);
    void onPickColor(CCObject* sender);
    void onPickBackground(CCObject* sender);
    void onDeleteLevel(CCObject* sender);
    void onMoveLevelUp(CCObject* sender);
    void onMoveLevelDown(CCObject* sender);
    void onToggleFeatured(CCObject* sender);
    void onAddSprite(CCObject* sender);
    void updateLocalSpriteNameLabel();
    void refreshLevelList();
    void applyEditMode();
    void updateBgIcon();
    void updateFeatureToggleState();
    void loadNextPendingLevel();
    LevelCell* createLevelCellFromLevel(GJGameLevel* level, int reward);
    void selectArtClosed(SelectArtLayer* layer) override;
    void configureLevelCell(LevelCell* cell, int reward);

    void loadLevelsFinished(cocos2d::CCArray* levels, char const* key, int type) override;
    void loadLevelsFailed(char const* key, int type) override;

    GDXGauntletManagePopup* m_owner = nullptr;
    int m_pendingLevelId = 0;
    int m_pendingLevelReward = 0;
    bool m_searchingLevel = false;
    bool m_unsaved = true;
    gd::string m_pendingSearchKey;
    geode::TextInput* m_nameInput = nullptr;
    geode::TextInput* m_gauntletReward = nullptr;
    geode::TextInput* m_descriptionInput = nullptr;
    geode::TextInput* m_suggestedByInput = nullptr;
    geode::TextInput* m_spriteByInput = nullptr;
    int m_bgIndex = 14;
    CCMenuItemSpriteExtra* m_bgIndexButton = nullptr;
    cocos2d::CCSprite* m_bgIconSpr = nullptr;
    geode::TextInput* m_levelInput = nullptr;
    geode::TextInput* m_levelRewardInput = nullptr;
    cue::ListNode* m_levelList = nullptr;
    cocos2d::ccColor3B m_selectedColor = {255, 255, 255};
    cocos2d::CCSprite* m_colorSpr = nullptr;
    geode::ColorPickPopup* m_colorPopup = nullptr;
    CCMenuItemSpriteExtra* m_featureToggle = nullptr;
    bool m_isFeatured = false;
    std::vector<LevelRewardEntry> m_levels;
    std::vector<LevelCell*> m_levelCells;
    std::vector<PendingLevelFetch> m_pendingLevelFetches;
    bool m_editMode = false;
    int m_editIndex = -1;
    bool m_localMode = false;
    matjson::Value m_editGauntlet;
    std::string m_spritePath;
    cocos2d::CCLabelBMFont* m_spriteNameLabel = nullptr;
    cocos2d::CCMenu* m_settingsMenu = nullptr;
    geode::async::TaskHolder<> m_addGauntletTask;
    geode::async::TaskHolder<> m_addSpriteTask;
};
