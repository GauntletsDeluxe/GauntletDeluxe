#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include "Geode/ui/NineSlice.hpp"
#include <cue/LoadingCircle.hpp>

using namespace geode::prelude;

class GDXLinkDiscordPopup : public geode::Popup {
public:
    static GDXLinkDiscordPopup* create();

private:
    bool init();

    // UI elements
    TextInput* m_discordIdInput = nullptr;
    TextInput* m_usernameInput = nullptr;
    CCMenuItemSpriteExtra* m_linkBtn = nullptr;
    CCMenuItemSpriteExtra* m_unlinkBtn = nullptr;
    CCMenuItemSpriteExtra* m_codeBtn = nullptr;
    CCLabelBMFont* m_codeLabel = nullptr;
    SimpleTextArea* m_statusLabel = nullptr;
    CCLabelBMFont* m_codeHintLabel = nullptr;
    NineSlice* m_codeBg = nullptr;
    cue::LoadingCircle* m_loadingCircle = nullptr;
    CCLabelBMFont* m_expiresLabel = nullptr;
    CCLabelBMFont* m_linkedUsernameLabel = nullptr;
    CCLabelBMFont* m_linkedIdLabel = nullptr;
    CCLabelBMFont* m_linkedStatusLabel = nullptr;
    int m_expiresIn = 0;

    std::string m_linkCode;
    bool m_isLinked = false;

    // Async tasks
    geode::async::TaskHolder<> m_checkStatusTask;
    geode::async::TaskHolder<> m_linkTask;
    geode::async::TaskHolder<> m_unlinkTask;

    void fetchDiscordStatus(bool silent = false);
    void checkLinkScheduler(float dt);
    void onLink(CCObject* sender);
    void onUnlink(CCObject* sender);
    void onCopyCode(CCObject* sender);
    void updateTimer(float dt);
    void updateUI(bool isLinked, const std::string& username, const std::string& discordId, bool isVerified, const std::string& linkCode, int expiresIn = 0);
};
