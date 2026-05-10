#include <Geode/Geode.hpp>
#include "Geode/ui/LoadingSpinner.hpp"
#include "Geode/ui/NineSlice.hpp"

using namespace geode::prelude;

class GDXUserPanelPopup : public Popup {
public:
    static GDXUserPanelPopup* create();

private:
    bool init();
    TextInput* m_accountIdInput = nullptr;
    CCMenu* m_manageMenu = nullptr;
    geode::NineSlice* m_usernameBackground = nullptr;
    CCLabelBMFont* m_usernameLabel = nullptr;
    CCMenuItemToggler* m_excludeBtn = nullptr;
    LoadingSpinner* m_loadingSpinner = nullptr;
    bool m_isExcluded = false;
    geode::async::TaskHolder<> m_findAccountTask;
    geode::async::TaskHolder<> m_excludeTask;
    void onExclude(CCObject* sender);
    void onFindAccountID(CCObject* sender);
    void updateExcludeButton();
};