#include "GDXAddTagsPopup.hpp"
#include "GDXManageTagsPopup.hpp"
#include "../include/GDXConstant.hpp"
#include "Geode/ui/Layout.hpp"
#include <Geode/ui/General.hpp>
#include <Geode/ui/Button.hpp>
#include <Geode/ui/ColorPickPopup.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/UploadActionPopup.hpp>
#include <Geode/utils/web.hpp>
#include <argon/argon.hpp>

using namespace geode::prelude;

GDXAddTagsPopup* GDXAddTagsPopup::create(GDXManageTagsPopup* parent) {
    auto ret = new GDXAddTagsPopup();
    ret->m_parent = parent;
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

GDXAddTagsPopup* GDXAddTagsPopup::create(GDXManageTagsPopup* parent, const matjson::Value& tag, int index) {
    auto ret = new GDXAddTagsPopup();
    ret->m_parent = parent;
    ret->m_editTag = tag;
    ret->m_editIndex = index;
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool GDXAddTagsPopup::init() {
    if (!Popup::init(250.f, 160.f)) {
        return false;
    }

    setTitle(m_editIndex >= 0 ? "Edit Tag" : "Add Tag");
    addSideArt(m_mainLayer, SideArt::BottomLeft, SideArtStyle::PopupGold, false);
    addSideArt(m_mainLayer, SideArt::BottomRight, SideArtStyle::PopupGold, false);
    addSideArt(m_mainLayer, SideArt::TopRight, SideArtStyle::PopupGold, false);

    m_nameInput = TextInput::create(160.f, "Tag Name", "chatFont.fnt");
    m_nameInput->setLabel("Tag Name");
    m_nameInput->setCommonFilter(CommonFilter::Any);
    m_nameInput->setAnchorPoint({0.5f, 1.f});
    m_mainLayer->addChildAtPosition(m_nameInput, Anchor::Top, {-30.f, -40.f});

    m_descriptionInput = TextInput::create(220.f, "Tag Description", "chatFont.fnt");
    m_descriptionInput->setLabel("Tag Description");
    m_descriptionInput->setCommonFilter(CommonFilter::Any);
    m_descriptionInput->setAnchorPoint({0.5f, 1.f});
    m_mainLayer->addChildAtPosition(m_descriptionInput, Anchor::Top, {0.f, -90.f});

    m_r = m_editTag["r"].asInt().unwrapOr(m_r);
    m_g = m_editTag["g"].asInt().unwrapOr(m_g);
    m_b = m_editTag["b"].asInt().unwrapOr(m_b);

    // @geode-ignore(unknown-resource)
    m_colorSpr = CCSprite::createWithSpriteFrameName("geode.loader/tab-bg.png");
    if (m_colorSpr) {
        m_colorSpr->setScale(0.5f);
        m_colorSpr->setColor({static_cast<GLubyte>(std::clamp(m_r, 0, 255)),
            static_cast<GLubyte>(std::clamp(m_g, 0, 255)),
            static_cast<GLubyte>(std::clamp(m_b, 0, 255))});
    }
    auto colorBtn = CCMenuItemSpriteExtra::create(
        m_colorSpr,
        this,
        menu_selector(GDXAddTagsPopup::onSelectColor));
    colorBtn->m_scaleMultiplier = 1.05f;
    auto colorLabel = CCLabelBMFont::create("Color", "goldFont.fnt");
    colorLabel->setScale(0.4f);
    colorLabel->setAnchorPoint({0, 0});
    colorBtn->addChildAtPosition(colorLabel, Anchor::TopLeft, {-2.f, 2.f}, false);
    m_buttonMenu->addChildAtPosition(colorBtn, Anchor::Top, {85.f, -55.f}, false);

    auto submitBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create(m_editIndex >= 0 ? "Update" : "Add", "goldFont.fnt", "GJ_button_01.png"),
        this,
        menu_selector(GDXAddTagsPopup::onSubmit));
    m_buttonMenu->addChildAtPosition(submitBtn, Anchor::Bottom, {0.f, 20.f}, false);

    if (m_editIndex >= 0 && m_editTag.isObject()) {
        m_nameInput->setString(m_editTag["name"].asString().unwrapOr(""));
        m_descriptionInput->setString(m_editTag["description"].asString().unwrapOr(""));
        m_r = m_editTag["r"].asInt().unwrapOr(m_r);
        m_g = m_editTag["g"].asInt().unwrapOr(m_g);
        m_b = m_editTag["b"].asInt().unwrapOr(m_b);
    }

    return true;
}

void GDXAddTagsPopup::onSelectColor(CCObject* sender) {
    auto color = cocos2d::ccColor3B{
        static_cast<GLubyte>(std::clamp(m_r, 0, 255)),
        static_cast<GLubyte>(std::clamp(m_g, 0, 255)),
        static_cast<GLubyte>(std::clamp(m_b, 0, 255))};

    m_colorPopup = geode::ColorPickPopup::create(color);
    if (m_colorPopup) {
        m_colorPopup->setCallback([this](const cocos2d::ccColor4B& selectedColor) {
            m_r = selectedColor.r;
            m_g = selectedColor.g;
            m_b = selectedColor.b;
            if (m_colorSpr) {
                m_colorSpr->setColor({selectedColor.r, selectedColor.g, selectedColor.b});
            }
        });
        m_colorPopup->show();
    }
}

void GDXAddTagsPopup::onSubmit(CCObject* sender) {
    auto name = m_nameInput ? m_nameInput->getString() : gd::string();
    auto description = m_descriptionInput ? m_descriptionInput->getString() : gd::string();
    if (name.empty()) {
        Notification::create("Tag name cannot be empty.", NotificationIcon::Warning)->show();
        return;
    }

    auto accountData = argon::getGameAccountData();
    auto url = std::string(gdx::baseApiUrl()) + (m_editIndex >= 0 ? "/editTag" : "/addTag");
    matjson::Value body = matjson::Value::object();
    body["accountId"] = accountData.accountId;
    body["argonToken"] = "";
    body["name"] = std::string(name);
    body["description"] = std::string(description);
    body["r"] = m_r;
    body["g"] = m_g;
    body["b"] = m_b;
    if (m_editIndex >= 0 && m_editTag.isObject()) {
        body["index"] = m_editTag["index"].asInt().unwrapOr(m_editIndex);
    }

    auto upopup = UploadActionPopup::create(nullptr, m_editIndex >= 0 ? "Updating tag..." : "Adding tag...");
    upopup->show();

    auto self = geode::Ref<GDXAddTagsPopup>(this);
    auto upopupRef = geode::Ref<UploadActionPopup>(upopup);
    m_requestTask.spawn([self = std::move(self), upopupRef = std::move(upopupRef), url = std::move(url), body = std::move(body), accountData = std::move(accountData)]() mutable -> arc::Future<> {
        auto token = co_await gdx::argonToken(accountData);
        if (token.empty()) {
            co_await geode::async::waitForMainThread([upopupRef = std::move(upopupRef), self = std::move(self)] {
                if (upopupRef) upopupRef->showFailMessage("Authentication failed.");
            });
            co_return;
        }

        body["argonToken"] = std::move(token);
        auto response = co_await geode::utils::web::WebRequest()
                            .url(url)
                            .header("Content-Type", "application/json")
                            .bodyJSON(body)
                            .post(url);
        if (response.error() || response.cancelled() || !response.ok()) {
            auto errMsg = gdx::getResponseMessage(response, "Request failed.");
            co_await geode::async::waitForMainThread([upopupRef = std::move(upopupRef), self = std::move(self), errMsg = std::move(errMsg)] {
                if (upopupRef) upopupRef->showFailMessage(errMsg);
            });
            co_return;
        }

        auto jsonResult = response.json();
        if (!jsonResult) {
            co_await geode::async::waitForMainThread([upopupRef = std::move(upopupRef), self = std::move(self)] {
                if (upopupRef) upopupRef->showFailMessage("Failed to parse response.");
            });
            co_return;
        }

        auto result = std::move(jsonResult).unwrap();
        if (!result["success"].asBool().unwrapOr(false)) {
            auto errMsg = gdx::getResponseMessage(response, "Request was not successful.");
            co_await geode::async::waitForMainThread([upopupRef = std::move(upopupRef), self = std::move(self), errMsg = std::move(errMsg)] {
                if (upopupRef) upopupRef->showFailMessage(errMsg);
            });
            co_return;
        }

        co_await geode::async::waitForMainThread([self = std::move(self), upopupRef = std::move(upopupRef)] {
            if (self) {
                if (upopupRef) upopupRef->showSuccessMessage(self->m_editIndex >= 0 ? "Tag updated." : "Tag added.");
                if (self->m_parent) {
                    self->m_parent->refreshTags();
                }
                self->m_unsaved = false;
                self->onClose(nullptr);
            }
        });
        co_return; }, []() {});
}

void GDXAddTagsPopup::onClose(CCObject* sender) {
    if (m_unsaved) {
        createQuickPopup(
            "Cancel Adding Tag?",
            fmt::format("Are you sure you want to <cr>cancel adding this tag</c>?\n<cy>All progress will be lost</c>."),
            "No",
            "Yes",
            [this](auto, bool yes) {
                if (!yes) return;

                this->removeFromParent();
            });
    } else {
        Popup::onClose(sender);
    }
}