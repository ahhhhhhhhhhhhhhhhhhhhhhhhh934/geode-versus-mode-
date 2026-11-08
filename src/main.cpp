#include <Geode/Geode.hpp>
#include <Geode/modify/GJMoreGamesLayer.hpp>

using namespace geode::prelude;

namespace Versus {
    enum class MatchType { Local, Online };
    static MatchType s_matchType = MatchType::Local;

    class LobbyLayer final : public CCLayer {
    public:
        static LobbyLayer* create() {
            auto* ret = new LobbyLayer();
            if (ret && ret->init()) { ret->autorelease(); return ret; }
            CC_SAFE_DELETE(ret);
            return nullptr;
        }

        bool init() override {
            if (!CCLayer::init()) return false;
            auto size = CCDirector::sharedDirector()->getWinSize();
            this->addChild(CCLayerColor::create({8, 12, 34, 235}), -1);

            auto* title = CCLabelBMFont::create("VERSUS MODE", "goldFont.fnt");
            title->setScale(0.8f);
            title->setPosition({size.width / 2, size.height - 48.f});
            this->addChild(title);

            auto* subtitle = CCLabelBMFont::create("CHOOSE YOUR MATCH TYPE", "bigFont.fnt");
            subtitle->setScale(0.45f);
            subtitle->setPosition({size.width / 2, size.height - 82.f});
            this->addChild(subtitle);

            auto* menu = CCMenu::create();
            menu->setPosition({0.f, 0.f});
            menu->addChild(makeButton("LOCAL 1V1", menu_selector(LobbyLayer::onLocal), {size.width / 2, size.height / 2 + 35.f}));
            menu->addChild(makeButton("ONLINE 1V1", menu_selector(LobbyLayer::onOnline), {size.width / 2, size.height / 2 - 25.f}));
            menu->addChild(makeButton("BACK", menu_selector(LobbyLayer::onBack), {size.width / 2, 45.f}));
            this->addChild(menu, 5);
            return true;
        }

        void keyBackClicked() override { this->removeFromParentAndCleanup(true); }

    private:
        CCMenuItemSpriteExtra* makeButton(char const* label, SEL_MenuHandler callback, CCPoint pos) {
            auto* sprite = ButtonSprite::create(label, 40, true, "goldFont.fnt", "GJ_button_01.png", 28.f, 0.65f);
            auto* item = CCMenuItemSpriteExtra::create(sprite, this, callback);
            item->setPosition(pos);
            return item;
        }
        void onLocal(CCObject*) {
            s_matchType = MatchType::Local;
            FLAlertLayer::create("LOCAL VERSUS", "Local 1v1 selected. Two-player gameplay is still in development.", "OK")->show();
        }
        void onOnline(CCObject*) {
            s_matchType = MatchType::Online;
            FLAlertLayer::create("ONLINE VERSUS", "Online matchmaking and player synchronization are not implemented yet.", "OK")->show();
        }
        void onBack(CCObject*) { this->removeFromParentAndCleanup(true); }
    };
}

// The Versus entry belongs on the Online menu grid (Create / Saved / Scores / Quests / Versus).
class $modify(VersusMoreGamesLayer, GJMoreGamesLayer) {
    bool init() {
        if (!GJMoreGamesLayer::init()) return false;

        auto size = CCDirector::sharedDirector()->getWinSize();
        auto* menu = CCMenu::create();
        menu->setID("versus-mode-grid-entry");
        menu->setPosition({0.f, 0.f});

        // The locked Versus tile is the fifth tile in the top row.
        // Place our live tile directly over that slot without changing other grid entries.
        auto* tile = ButtonSprite::create("VERSUS", 86, true, "goldFont.fnt", "GJ_button_01.png", 58.f, 0.8f);
        tile->setScale(1.0f);
        auto* button = CCMenuItemSpriteExtra::create(tile, this, menu_selector(VersusMoreGamesLayer::onVersus));
        button->setID("versus-mode-button");
        button->setPosition({size.width * 0.788f, size.height * 0.808f});
        menu->addChild(button);
        this->addChild(menu, 100);
        return true;
    }

    void onVersus(CCObject*) {
        auto* lobby = Versus::LobbyLayer::create();
        if (lobby) {
            lobby->setID("versus-mode-lobby");
            this->addChild(lobby, 1000);
        }
    }
};
