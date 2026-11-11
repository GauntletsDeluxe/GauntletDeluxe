#include "GDXGeeheeComic.hpp"
#include "GDXScrollLayer.hpp"
#include "../include/GDXConstant.hpp"
#include <Geode/ui/LazySprite.hpp>
#include <Geode/ui/LoadingSpinner.hpp>
#include <Geode/ui/TextArea.hpp>
#include <Geode/ui/Button.hpp>
#include <Geode/utils/base64.hpp>
#include <Geode/utils/web.hpp>
#include <algorithm>

using namespace geode::prelude;
using namespace gdx;

GDXGeeheeComic* GDXGeeheeComic::create() {
    auto ret = new GDXGeeheeComic();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool GDXGeeheeComic::init() {
    if (!CCBlockLayer::init()) return false;

    auto winSize = CCDirector::sharedDirector()->getWinSize();
    this->setColor({0, 0, 0});
    this->setOpacity(150);
    this->setContentSize(winSize);

    auto backBtn = Button::createWithSpriteFrameName(
        "GJ_arrow_02_001.png",
        [this](Button* sender) {
            this->onClose(sender);
        });
    backBtn->setID("back-button");
    backBtn->setPosition({25.0f, CCDirector::get()->getWinSize().height - 25.0f});
    backBtn->setZOrder(5);
    this->addChild(backBtn, 1);

    m_panelSize = CCSize{winSize.width - 100.f, winSize.height - 80.f};
    m_panelCenter = CCPoint{winSize.width / 2.f, winSize.height / 2.f + 10.f};

    m_buttonMenu = CCMenu::create();
    m_buttonMenu->setPosition({0.f, 0.f});
    m_buttonMenu->setContentSize(winSize);
    this->addChild(m_buttonMenu, 10);

    m_loadingSpinner = LoadingSpinner::create(60.f);
    m_loadingSpinner->setPosition(m_panelCenter);
    this->addChild(m_loadingSpinner, 5);

    m_statusLabel = Label::create("", "bigFont.fnt");
    m_statusLabel->setScale(0.5f);
    m_statusLabel->setPosition(m_panelCenter);
    this->addChild(m_statusLabel, 1);

    m_workInProgressLabel = Label::create("Work in progress", "chatFont.fnt");
    m_workInProgressLabel->setScale(0.5f);
    m_workInProgressLabel->setPosition({m_panelCenter.x, 15.f});
    this->addChild(m_workInProgressLabel, 1);

    this->setTouchEnabled(true);
    this->setKeypadEnabled(true);
    this->fetchComics();
    return true;
}

void GDXGeeheeComic::show() {
    auto scene = CCDirector::sharedDirector()->getRunningScene();
    if (!scene) return;
    scene->addChild(this, scene->getHighestChildZ() + 1);
}

void GDXGeeheeComic::keyBackClicked() {
    this->removeFromParent();
}

void GDXGeeheeComic::onClose(CCObject*) {
    this->keyBackClicked();
}

void GDXGeeheeComic::onPrev(CCObject*) {
    if (m_scroll) m_scroll->moveToPage(m_scroll->getCurrentPage() - 1);
}

void GDXGeeheeComic::onNext(CCObject*) {
    if (m_scroll) m_scroll->moveToPage(m_scroll->getCurrentPage() + 1);
}

void GDXGeeheeComic::onOpenLink(CCObject*) {
    if (!m_scroll) return;
    auto page = static_cast<size_t>(m_scroll->getCurrentPage());
    if (page >= m_links.size() || m_links[page].empty()) return;
    utils::web::openLinkInBrowser(m_links[page]);
}

void GDXGeeheeComic::onTap(CCPoint const& worldPos) {
    if (!m_scroll) return;
    auto page = static_cast<size_t>(m_scroll->getCurrentPage());
    if (page >= m_views.size()) return;

    auto& view = m_views[page];
    if (!view.sprite || !view.text || !view.sprite->isLoaded()) return;

    auto local = view.sprite->convertToNodeSpace(worldPos);
    auto size = view.sprite->getContentSize();
    if (local.x < 0.f || local.y < 0.f || local.x > size.width || local.y > size.height) return;

    view.textShown = !view.textShown;
    view.sprite->setColor(view.textShown ? ccColor3B{60, 60, 60} : ccColor3B{255, 255, 255});
    view.text->setVisible(view.textShown);
}

void GDXGeeheeComic::showError(std::string const& message) {
    if (m_loadingSpinner) m_loadingSpinner->setVisible(false);
    if (m_statusLabel) {
        m_statusLabel->setText(message);
        m_statusLabel->setLimitLabelWidth(m_panelSize.width - 40.f, 0.5f, 0.2f);
    }
}

void GDXGeeheeComic::fetchComics() {
    // weak so the layer is never released (and destroyed) on the worker thread if it was closed mid-request
    auto self = WeakRef<GDXGeeheeComic>(this);
    m_fetchTask.spawn([self = std::move(self)]() mutable -> arc::Future<> {
        auto response = co_await utils::web::WebRequest().get("https://gdxbot.arcticwoof.xyz/api/rss/geehee.xml?comicOnly=true");
        if (response.error() || response.cancelled() || !response.ok()) {
            co_await async::waitForMainThread([self] {
                if (auto layer = self.lock()) layer->showError("Failed to load the comics.");
            });
            co_return;
        }

        auto body = response.string().unwrapOr("");
        std::vector<Entry> entries;
        std::string_view view = body;
        size_t pos = 0;
        while ((pos = view.find("<item>", pos)) != std::string_view::npos) {
            auto end = view.find("</item>", pos);
            if (end == std::string_view::npos) break;
            auto item = view.substr(pos, end - pos);
            pos = end;

            Entry entry;
            entry.title = tagContent(item, "title");
            entry.link = tagContent(item, "link");
            auto description = tagContent(item, "description");
            entry.text = descriptionText(description);

            // prefer the Newgrounds page the post links to over the discord message
            auto ngLink = attrAfter(description, "href=\"https://www.newgrounds.com", "href");
            if (!ngLink.empty()) entry.link = decodeEntities(ngLink);

            auto src = decodeEntities(attrAfter(description, "<img", "src"));
            if (src.empty()) src = decodeEntities(attrAfter(item, "<enclosure", "url"));

            constexpr std::string_view marker = "base64,";
            if (src.starts_with("data:") && src.find(marker) != std::string::npos) {
                auto decoded = utils::base64::decode(
                    std::string_view(src).substr(src.find(marker) + marker.size()),
                    utils::base64::Base64Variant::Normal);
                if (decoded) entry.imageData = std::move(decoded).unwrap();
            } else {
                entry.imageUrl = std::move(src);
            }

            if (entry.imageData.empty() && entry.imageUrl.empty()) continue;
            entries.push_back(std::move(entry));
        }

        // the feed is newest first, but the comic reads oldest first
        std::reverse(entries.begin(), entries.end());

        co_await async::waitForMainThread([self, entries = std::move(entries)]() mutable {
            auto layer = self.lock();
            if (!layer) return;
            if (entries.empty()) {
                layer->showError("No comics found.");
                return;
            }
            layer->buildPages(std::move(entries));
        });
        co_return; }, []() {});
}

void GDXGeeheeComic::buildPages(std::vector<Entry> entries) {
    if (m_loadingSpinner) m_loadingSpinner->setVisible(false);

    auto winSize = CCDirector::sharedDirector()->getWinSize();
    auto imageSize = CCSize{m_panelSize.width - 30.f, m_panelSize.height - 60.f};
    auto pages = CCArray::create();

    for (auto& entry : entries) {
        auto page = CCLayer::create();
        page->setContentSize(winSize);

        auto titleLabel = Label::create(entry.title, "goldFont.fnt");
        titleLabel->setLimitLabelWidth(m_panelSize.width - 120.f, 0.8f, 0.3f);
        titleLabel->setPosition({m_panelCenter.x, m_panelCenter.y + m_panelSize.height / 2.f});
        page->addChild(titleLabel);

        auto imagePos = CCPoint{m_panelCenter.x, m_panelCenter.y - 10.f};
        auto sprite = LazySprite::create(imageSize, true);
        sprite->setPosition(imagePos);
        sprite->setLoadCallback([sprite = Ref<LazySprite>(sprite), imageSize](Result<> const& result) {
            if (!result) return;
            auto size = sprite->getContentSize();
            if (size.width <= 0.f || size.height <= 0.f) return;
            sprite->setScale(std::min(imageSize.width / size.width, imageSize.height / size.height));
        });
        if (!entry.imageData.empty())
            sprite->loadFromData(std::move(entry.imageData));
        else
            sprite->loadFromUrl(entry.imageUrl, CCImage::kFmtUnKnown, true);
        page->addChild(sprite);

        auto view = PageView{.sprite = sprite};
        if (!entry.text.empty()) {
            auto text = SimpleTextArea::create(entry.text, "chatFont.fnt", 0.8f, imageSize.width);
            text->setAnchorPoint({0.5f, 0.5f});
            text->setAlignment(kCCTextAlignmentCenter);
            text->setWrappingMode(WrappingMode::WORD_WRAP);
            text->setPosition(imagePos);
            text->setVisible(false);
            page->addChild(text, 1);
            view.text = text;
        }
        m_views.push_back(view);

        m_links.push_back(std::move(entry.link));
        pages->addObject(page);
    }

    m_scroll = GDXScrollLayer::create(pages);
    if (!m_scroll) return;
    m_scroll->setScrollTouchPriority(-505);
    m_scroll->setTapCallback([this](CCPoint const& worldPos) {
        this->onTap(worldPos);
    });
    m_scroll->setScrollbarPosition({winSize.width / 2.f, 30.f});
    this->addChild(m_scroll, 1);

    if (pages->count() > 1) {
        auto prevSpr = CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
        prevSpr->setFlipX(true);
        auto prevBtn = Button::createWithNode(prevSpr, [this](Button* sender) { this->onPrev(sender); });
        prevBtn->setTouchPriority(-504);
        prevBtn->setPosition({m_panelCenter.x - m_panelSize.width / 2.f - 25.f, m_panelCenter.y});
        m_buttonMenu->addChild(prevBtn);

        auto nextSpr = CCSprite::createWithSpriteFrameName("navArrowBtn_001.png");
        auto nextBtn = Button::createWithNode(nextSpr, [this](Button* sender) { this->onNext(sender); });
        nextBtn->setTouchPriority(-504);
        nextBtn->setPosition({m_panelCenter.x + m_panelSize.width / 2.f + 25.f, m_panelCenter.y});
        m_buttonMenu->addChild(nextBtn);
    }

    auto linkSpr = ButtonSprite::create("View Online", "goldFont.fnt", "GJ_button_01.png", 0.6f);
    linkSpr->setScale(0.7f);
    auto linkBtn = Button::createWithNode(linkSpr, [this](Button* sender) { this->onOpenLink(sender); });
    linkBtn->setTouchPriority(-504);
    linkBtn->setPosition({m_panelCenter.x, 50.f});
    m_buttonMenu->addChild(linkBtn);
}
