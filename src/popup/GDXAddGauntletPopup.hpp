#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>

class GDXGauntletManagePopup;

class GDXAddGauntletPopup : public geode::Popup {
public:
    static GDXAddGauntletPopup* create(GDXGauntletManagePopup* owner);

private:
    bool init() override;
    void onSave(CCObject* sender);
    void onCancel(CCObject* sender);

    GDXGauntletManagePopup* m_owner = nullptr;
    geode::TextInput* m_nameInput = nullptr;
    geode::TextInput* m_descriptionInput = nullptr;
};
