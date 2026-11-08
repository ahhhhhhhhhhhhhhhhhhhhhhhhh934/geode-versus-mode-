#include <Geode/Geode.hpp>
#include <Geode/modify/LevelBrowserLayer.hpp>

using namespace geode::prelude;

namespace Versus {
    enum class MatchType { Local, Online };

    struct LobbyState {
        MatchType type = MatchType::Local;
    };
    static LobbyState s_lobby;

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
            auto winSize = CCDirector::sharedDirector()->getWinSize();
            this->addChild(CCLayerColor::create({8, 12, 34, 235}), -1);

            auto* title = CCLabelBMFont::create("VERSUS MODE", "goldFont.fnt");
            title->setScale(0.8f);
            title->setPosition({winSize.width / 2, winSize.height - 48.f});
            this->addChild(title);

            auto* subtitle = CCLabelBMFont::create("CHOOSE YOUR MATCH TYPE", "bigFont.fnt");
            subtitle->setScale(0.45f);
            subtitle->setPosition({winSize.width / 2, winSize.height - 82.f});
            this->addChild(subtitle);

            auto* menu = CCMenu::create();
            menu->setPosition({0.f, 0.f});
            menu->addChild(makeButton("LOCAL 1V1", menu_selector(LobbyLayer::onLocal), {winSize.width / 2, winSize.height / 2 + 35.f}));
            menu->addChild(makeButton("ONLINE 1V1", menu_selector(LobbyLayer::onOnline), {winSize.width / 2, winSize.height / 2 - 25.f}));
            menu->addChild(makeButton("BACK", menu_selector(LobbyLayer::onBack), {winSize.width / 2, 45.f}));
            this->addChild(menu, 5);

            auto* note = CCLabelBMFont::create("ONLINE MATCHMAKING REQUIRES A SERVER", "chatFont.fnt");
            note->setScale(0.35f);
            note->setColor({190, 200, 225});
            note->setPosition({winSize.width / 2, 18.f});
            this->addChild(note);
            return true;
        }

        void keyBackClicked() override { this->removeFromParentAndCleanup(true); }

    private:
        CCMenuItemSpriteExtra* makeButton(char const* text, SEL_MenuHandler callback, CCPoint position) {
            auto* sprite = ButtonSprite::create(text, 40, true, "goldFont.fnt", "GJ_button_01.png", 28.f, 0.65f);
            auto* item = CCMenuItemSpriteExtra::create(sprite, this, callback);
            item->setPosition(position);
            return item;
        }
        void onLocal(CCObject*) {
            s_lobby.type = MatchType::Local;
            FLAlertLayer::create("LOCAL VERSUS", "Local lobby selected.<cr><cr>Two-player controls and race gameplay are still being developed.", "OK")->show();
        }
        void onOnline(CCObject*) {
            s_lobby.type = MatchType::Online;
            FLAlertLayer::create("ONLINE VERSUS", "Online matchmaking needs a server and player synchronization. Those features are not implemented yet.", "OK")->show();
        }
        void onBack(CCObject*) { this->removeFromParentAndCleanup(true); }
    };
}

class $modify(VersusLevelBrowserLayer, LevelBrowserLayer) {
    bool init(GJSearchObject* searchObject) {
        if (!LevelBrowserLayer::init(searchObject)) return false;

        // Place Versus inside the Online Levels browser, not the main menu.
        auto* menu = CCMenu::create();
        menu->setID("versus-mode-browser-menu");
        menu->setPosition({0.f, 0.f});

        auto* icon = CCSprite::createWithSpriteFrameName("GJ_versusBtn_001.png");
        if (!icon) {
            icon = ButtonSprite::create("VERSUS", 48, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 0.7f);
        } else {
            icon->setScale(0.55f);
        }

        auto* button = CCMenuItemSpriteExtra::create(icon, this, menu_selector(VersusLevelBrowserLayer::onVersusPressed));
        button->setID("versus-mode-button");
        button->setPosition({CCDirector::sharedDirector()->getWinSize().width / 2 - 42.f,
                             CCDirector::sharedDirector()->getWinSize().height / 2 - 18.f});
        menu->addChild(button);
        this->addChild(menu, 100);
        return true;
    }

    void onVersusPressed(CCObject*) {
        auto* layer = Versus::LobbyLayer::create();
        if (layer) {
            layer->setID("versus-lobby-layer");
            this->addChild(layer, 1000);
        }
    }
};
