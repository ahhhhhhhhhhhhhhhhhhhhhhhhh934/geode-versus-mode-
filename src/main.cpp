#include <Geode/Geode.hpp>
#include <Geode/modify/CreatorLayer.hpp>
#include "ui/LobbyLayer.hpp"
#include "play/MatchHooks.hpp"

using namespace geode::prelude;

// Register Globed room-roster listeners at mod startup, not only when the
// Versus menu is opened, so the initial room snapshot is not missed.
$execute {
    VersusLobby::initialize();
}

// The screenshot's Create/Saved/Scores/Quests/Versus grid belongs to CreatorLayer,
// not GJMoreGamesLayer. Add the clickable tile to that actual screen.
class $modify(VersusCreatorLayer, CreatorLayer) {
    bool init() {
        if (!CreatorLayer::init()) return false;

        auto size = CCDirector::sharedDirector()->getWinSize();
        auto* menu = CCMenu::create();
        menu->setID("versus-mode-grid-entry");
        menu->setPosition({0.f, 0.f});

        // Use Geometry Dash's own Versus icon frame instead of a generic
        // ButtonSprite. Keep a stock GD tile as the button's backing plate.
        auto* tile = CCSprite::createWithSpriteFrameName("GJ_versusBtn_001.png");
        if (!tile) tile = CCSprite::createWithSpriteFrameName("GJ_createBtn_001.png");
        // A missing atlas frame must never create a null, untappable menu item.
        if (!tile) {
            tile = ButtonSprite::create("VERSUS", 50, true, "goldFont.fnt",
                "GJ_button_05.png", 40.f, 0.62f);
        }
        if (!tile) return true;
        auto* item = CCMenuItemSpriteExtra::create(
            tile, this, menu_selector(VersusCreatorLayer::onVersus)
        );
        if (!item) return true;
        item->setID("versus-mode-button");
        // CreatorLayer's stock grid tiles are rendered at 80% of atlas size.
        // Apply the same scale to the native Versus tile, not a text ButtonSprite.
        item->setScale(0.80f);
        // Match the Versus slot at the far right of the first row in CreatorLayer.
        item->setPosition({size.width * 0.788f, size.height * 0.808f});
        menu->addChild(item);
        this->addChild(menu, 1000);
        return true;
    }

    void onVersus(CCObject*) {
        auto* scene = Versus::LobbyLayer::scene();
        if (scene) {
            scene->setID("versus-mode-scene");
            CCDirector::sharedDirector()->pushScene(
                CCTransitionFade::create(0.25f, scene)
            );
        }
    }
};
