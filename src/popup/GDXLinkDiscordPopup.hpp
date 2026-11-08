#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/ui/Button.hpp>
#include <Geode/ui/Label.hpp>
#include "Geode/ui/NineSlice.hpp"
#include <cue/LoadingCircle.hpp>

using namespace geode::prelude;

class GDXLinkDiscordPopup : public Popup {
public:
    static GDXLinkDiscordPopup* create();

private:
    bool init();

    // UI elements
    TextInput* m_discordIdInput = nullptr;
    TextInput* m_usernameInput = nullptr;
    Button* m_linkBtn = nullptr;
    Button* m_unlinkBtn = nullptr;
    Button* m_codeBtn = nullptr;
    Label* m_codeLabel = nullptr;
    SimpleTextArea* m_statusLabel = nullptr;
    Label* m_codeHintLabel = nullptr;
    NineSlice* m_codeBg = nullptr;
    cue::LoadingCircle* m_loadingCircle = nullptr;
    Label* m_expiresLabel = nullptr;
    Label* m_linkedUsernameLabel = nullptr;
    Label* m_linkedIdLabel = nullptr;
    Label* m_linkedStatusLabel = nullptr;
    int m_expiresIn = 0;

    std::string m_linkCode;
    bool m_isLinked = false;

    // Async tasks
    async::TaskHolder<> m_checkStatusTask;
    async::TaskHolder<> m_linkTask;
    async::TaskHolder<> m_unlinkTask;

    void fetchDiscordStatus(bool silent = false);
    void checkLinkScheduler(float dt);
    void onLink(CCObject* sender);
    void onUnlink(CCObject* sender);
    void onCopyCode(CCObject* sender);
    void updateTimer(float dt);
    void updateUI(bool isLinked, const std::string& username, const std::string& discordId, bool isVerified, const std::string& linkCode, int expiresIn = 0);
};
