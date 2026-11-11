#pragma once

#include <Geode/Geode.hpp>
#include <Geode/binding/CCBlockLayer.hpp>
#include <Geode/ui/LazySprite.hpp>
#include <Geode/ui/TextArea.hpp>
#include <Geode/ui/Label.hpp>

using namespace geode::prelude;

class GDXScrollLayer;

class GDXGeeheeComic : public CCBlockLayer {
public:
    static GDXGeeheeComic* create();

    void show();

private:
    struct Entry {
        std::string title;
        std::string link;
        std::string imageUrl;
        std::string text;
    };

    // the tap-to-reveal text sits over its comic image on the same page
    struct PageView {
        LazySprite* sprite = nullptr;
        SimpleTextArea* text = nullptr;
        bool textShown = false;
    };

    bool init() override;
    void keyBackClicked() override;

    void fetchComics();
    void onRSSLink(CCObject* sender);
    void showError(std::string const& message);
    void buildPages(std::vector<Entry> entries);
    void onClose(cocos2d::CCObject* sender);
    void onPrev(cocos2d::CCObject* sender);
    void onNext(cocos2d::CCObject* sender);
    void onOpenLink(cocos2d::CCObject* sender);
    void onTap(cocos2d::CCPoint const& worldPos);

    cocos2d::CCSize m_panelSize;
    cocos2d::CCPoint m_panelCenter;
    cocos2d::CCMenu* m_buttonMenu = nullptr;
    cocos2d::CCMenu* m_bottomLeftMenu = nullptr;
    LoadingSpinner* m_loadingSpinner = nullptr;
    Label* m_statusLabel = nullptr;
    Label* m_workInProgressLabel = nullptr;
    GDXScrollLayer* m_scroll = nullptr;
    std::vector<std::string> m_links;
    std::vector<PageView> m_views;
    async::TaskHolder<> m_fetchTask;
};
