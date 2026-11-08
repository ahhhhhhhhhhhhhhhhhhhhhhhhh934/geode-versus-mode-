#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

using namespace geode::prelude;

namespace Versus {
    enum class MatchType { Local, Online };

    struct LobbyState {
        MatchType type = MatchType::Local;
        bool inRoom = false;
        std::string roomCode;
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
            this->addChild(CCLayerColor::create({8, 12, 34, 220}), -1);

            auto* title = CCLabelBMFont::create("VERSUS MODE", "goldFont.fnt");
            title->setScale(0.8f);
            title->setPosition({winSize.width / 2, winSize.height - 48.f});
            this->addChild(title);

            auto* subtitle = CCLabelBMFont::create("CHOOSE YOUR MATCH TYPE", "bigFont.fnt");
            subtitle->setScale(0.45f);
            subtitle->setPosition({winSize.width / 2, winSize.height - 82.f});
            this->addChild(subtitle);

            this->addChild(makeButton("LOCAL 1V1", menu_selector(LobbyLayer::onLocal), {winSize.width / 2, winSize.height / 2 + 35.f}));
            this->addChild(makeButton("ONLINE 1V1", menu_selector(LobbyLayer::onOnline), {winSize.width / 2, winSize.height / 2 - 25.f}));
            this->addChild(makeButton("BACK", menu_selector(LobbyLayer::onBack), {winSize.width / 2, 45.f}));

            auto* note = CCLabelBMFont::create("ONLINE MATCHMAKING REQUIRES A SERVER", "chatFont.fnt");
            note->setScale(0.35f);
            note->setColor({190, 200, 225});
            note->setPosition({winSize.width / 2, 18.f});
            this->addChild(note);
            return true;
        }

    private:
        CCMenuItemSpriteExtra* makeButton(char const* text, SEL_MenuHandler callback, CCPoint position) {
            auto* sprite = ButtonSprite::create(text, 40, true, "goldFont.fnt", "GJ_button_01.png", 28.f, 0.65f);
            auto* item = CCMenuItemSpriteExtra::create(sprite, this, callback);
            item->setPosition(position);
            return item;
        }
        void onLocal(CCObject*) {
            s_lobby.type = MatchType::Local;
            FLAlertLayer::create("LOCAL VERSUS", "Local lobby selected.<cr><cr>Next gameplay step: bind two independent input sources and spawn a second player in the active level. This starter does not yet implement a second controllable icon.", "OK")->show();
        }
        void onOnline(CCObject*) {
            s_lobby.type = MatchType::Online;
            FLAlertLayer::create("ONLINE VERSUS", "Online lobby selected.<cr><cr>Online races need a matchmaking/relay service to exchange room state and player progress. No server is configured yet.", "OK")->show();
        }
        void onBack(CCObject*) { this->removeFromParentAndCleanup(true); }
    };
}

class $modify(VersusMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        auto* bottomMenu = this->getChildByID("bottom-menu");
        if (!bottomMenu) { log::warn("Versus Mode: main menu button container not found"); return true; }

        auto* sprite = ButtonSprite::create("VERSUS", 48, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 0.7f);
        auto* button = CCMenuItemSpriteExtra::create(sprite, this, menu_selector(VersusMenuLayer::onVersusPressed));
        button->setID("versus-mode-button");
        bottomMenu->addChild(button);
        bottomMenu->updateLayout();
        return true;
    }
    void onVersusPressed(CCObject*) {
        auto* layer = Versus::LobbyLayer::create();
        if (!layer) return;
        layer->setID("versus-lobby-layer");
        this->getParent()->addChild(layer, 100);
    }
};
