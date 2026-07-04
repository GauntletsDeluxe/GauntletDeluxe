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
    CCMenuItemToggler* m_promoteBtn = nullptr;
    CCMenuItemToggler* m_promoteModBtn = nullptr;
    LoadingSpinner* m_loadingSpinner = nullptr;
    bool m_isExcluded = false;
    bool m_isContributor = false;
    bool m_isLeaderboardMod = false;
    geode::async::TaskHolder<> m_findAccountTask;
    geode::async::TaskHolder<> m_excludeTask;
    geode::async::TaskHolder<> m_promoteTask;
    geode::async::TaskHolder<> m_promoteModTask;
    void onExclude(CCObject* sender);
    void onPromote(CCObject* sender);
    void onPromoteMod(CCObject* sender);
    void onFindAccountID(CCObject* sender);
    void updateExcludeButton();
    void updatePromoteButton();
    void updatePromoteModButton();
};