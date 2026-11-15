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

// Electrodynamix is main-level ID 15. GameStatsManager only marks it complete
// after a legitimate completion, so practice-mode progress does not unlock Versus.
static bool hasUnlockedVersus() {
    auto* stats = GameStatsManager::sharedState();
    return stats && stats->hasCompletedMainLevel(15);
}

// Use Geometry Dash's native dialogue UI on every supported platform.
// The two DialogObjects reproduce the Keymaster's two-page blue dialogue.
static void showVersusLockedDialog() {
    auto* first = DialogObject::create(
        "The Keymaster", "One doesn't simply play versus mode ",
        2, 1.f, false, ccColor3B{85, 175, 255}
    );
    auto* second = DialogObject::create(
        "The Keymaster", "First you must complete <s260><cx>Electrodynamix</c></s>",
        2, 1.f, false, ccColor3B{85, 175, 255}
    );
    auto* messages = CCArray::create();
    if (first && second && messages) {
        messages->addObject(first);
        messages->addObject(second);
        if (auto* dialog = DialogLayer::createWithObjects(messages, 0)) {
            dialog->addToMainScene();
            return;
        }
    }

    FLAlertLayer::create(
        "The Keymaster",
        "One doesn't simply play versus mode.\nFirst you must complete Electrodynamix.",
        "OK"
    )->show();
}

// Developer access is tied to the logged-in Geometry Dash account, not the
// editable local player-name setting. Only the account username iio103 is allowed.
static bool isAuthorizedDeveloper() {
    auto* account = GJAccountManager::sharedState();
    return account && account->m_accountID > 0 && account->m_username == "iio103";
}

static void showDeveloperPanel() {
    if (!isAuthorizedDeveloper()) return;

    auto* stats = GameStatsManager::sharedState();
    bool electrodynamixComplete = stats && stats->hasCompletedMainLevel(15);

    // Keep the alert text as a compile-time string: gd::string does not
    // provide std::string-style operator+= in the Android Geode bindings.
    auto const* details = electrodynamixComplete
        ? "Developer access: AUTHORIZED\n"
          "Geometry Dash account: iio103\n"
          "Versus unlock requirement: Electrodynamix\n"
          "Electrodynamix status: COMPLETED\n"
          "Mod: Versus Mode v1.0.12"
        : "Developer access: AUTHORIZED\n"
          "Geometry Dash account: iio103\n"
          "Versus unlock requirement: Electrodynamix\n"
          "Electrodynamix status: NOT COMPLETED\n"
          "Mod: Versus Mode v1.0.12";

    FLAlertLayer::create(
        "VERSUS DEVELOPER PANEL",
        details,
        "CLOSE"
    )->show();
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

        // The developer entry is created only for the authenticated GD account.
        if (isAuthorizedDeveloper()) {
            auto* devSprite = ButtonSprite::create(
                "DEV", 35, true, "goldFont.fnt", "GJ_button_04.png", 30.f, 0.7f
            );
            if (devSprite) {
                auto* devItem = CCMenuItemSpriteExtra::create(
                    devSprite, this, menu_selector(VersusCreatorLayer::onDeveloperPanel)
                );
                if (devItem) {
                    devItem->setID("versus-developer-panel-button");
                    devItem->setPosition({size.width * 0.16f, size.height * 0.91f});
                    menu->addChild(devItem);
                }
            }
        }

        this->addChild(menu, 1000);
        return true;
    }

    void onDeveloperPanel(CCObject*) {
        if (!isAuthorizedDeveloper()) return;
        showDeveloperPanel();
    }

    void onVersus(CCObject*) {
        if (!hasUnlockedVersus()) {
            showVersusLockedDialog();
            return;
        }

        auto* scene = Versus::LobbyLayer::scene();
        if (scene) {
            scene->setID("versus-mode-scene");
            CCDirector::sharedDirector()->pushScene(
                CCTransitionFade::create(0.25f, scene)
            );
        }
    }
};
