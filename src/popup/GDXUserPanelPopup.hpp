#include <Geode/Geode.hpp>
#include "Geode/ui/LoadingSpinner.hpp"
#include "Geode/ui/NineSlice.hpp"
#include <cue/ListNode.hpp>

using namespace geode::prelude;

class GDXUserPanelPopup : public Popup {
public:
    static GDXUserPanelPopup* create();

private:
    static constexpr int kUsersPerPage = 20;

    bool init();
    TextInput* m_accountIdInput = nullptr;
    CCMenu* m_manageMenu = nullptr;
    NineSlice* m_manageMenuBg = nullptr;
    NineSlice* m_usernameBackground = nullptr;
    CCLabelBMFont* m_usernameLabel = nullptr;
    CCMenuItemToggler* m_excludeBtn = nullptr;
    CCMenuItemToggler* m_promoteBtn = nullptr;
    CCMenuItemToggler* m_promoteModBtn = nullptr;
    LoadingSpinner* m_loadingSpinner = nullptr;
    bool m_isExcluded = false;
    bool m_isContributor = false;
    bool m_isLeaderboardMod = false;

    // user list (right side)
    cue::ListNode* m_userList = nullptr;
    TextInput* m_usernameSearchInput = nullptr;
    std::string m_searchQuery;
    LoadingSpinner* m_listSpinner = nullptr;
    CCLabelBMFont* m_pageInfoLabel = nullptr;
    Button* m_prevPageBtn = nullptr;
    Button* m_nextPageBtn = nullptr;
    int m_page = 1;
    int m_totalPages = 1;
    int m_totalUsers = 0;

    async::TaskHolder<> m_findAccountTask;
    async::TaskHolder<> m_excludeTask;
    async::TaskHolder<> m_promoteTask;
    async::TaskHolder<> m_promoteModTask;
    async::TaskHolder<> m_fetchUsersTask;
    void onExclude(CCObject* sender);
    void onPromote(CCObject* sender);
    void onPromoteMod(CCObject* sender);
    void onFindAccountID(CCObject* sender);
    void updateExcludeButton();
    void updatePromoteButton();
    void updatePromoteModButton();
    void fetchUsers(int page);
    void updatePageInfo();
    CCNode* createUserCell(matjson::Value const& entry, float width);
};
