#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <vector>

using namespace geode::prelude;

namespace Versus {
    enum class MatchType { Local, Online };

    struct LobbyState {
        MatchType type = MatchType::Local;
        bool inRoom = false;
        std::string roomCode;
    };
    static LobbyState s_lobby;
    static std::vector<CCMenuItem*> s_disabledBackgroundItems;

    static void disableMenuItems(CCNode* node) {
        if (!node) return;
        auto* children = node->getChildren();
        if (!children) return;

        for (unsigned int i = 0; i < children->count(); ++i) {
            auto* child = static_cast<CCNode*>(children->objectAtIndex(i));
            if (auto* item = typeinfo_cast<CCMenuItem*>(child)) {
                item->setEnabled(false);
                s_disabledBackgroundItems.push_back(item);
            }
            disableMenuItems(child);
        }
    }

    static void restoreBackgroundButtons() {
        for (auto* item : s_disabledBackgroundItems) {
            if (item) item->setEnabled(true);
        }
        s_disabledBackgroundItems.clear();
    }

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

            // Dim the game screen while the lobby is open.
            this->addChild(CCLayerColor::create({8, 12, 34, 235}), -1);

            auto* title = CCLabelBMFont::create("VERSUS MODE", "goldFont.fnt");
            title->setScale(0.8f);
            title->setPosition({winSize.width / 2, winSize.height - 48.f});
            this->addChild(title);

            auto* subtitle = CCLabelBMFont::create("CHOOSE YOUR MATCH TYPE", "bigFont.fnt");
            subtitle->setScale(0.45f);
            subtitle->setPosition({winSize.width / 2, winSize.height - 82.f});
            this->addChild(subtitle);

            // CCMenu is required for the menu items to receive taps.
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

        void keyBackClicked() override {
            restoreBackgroundButtons();
            this->removeFromParentAndCleanup(true);
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
            FLAlertLayer::create("LOCAL VERSUS", "Local lobby selected.<cr><cr>Two-player controls and race gameplay are still being developed.", "OK")->show();
        }
        void onOnline(CCObject*) {
            s_lobby.type = MatchType::Online;
            FLAlertLayer::create("ONLINE VERSUS", "Online matchmaking needs a server and player synchronization. Those features are not implemented yet.", "OK")->show();
        }
        void onBack(CCObject*) {
            restoreBackgroundButtons();
            this->removeFromParentAndCleanup(true);
        }
    };
}

class $modify(VersusMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        auto* bottomMenu = this->getChildByID("bottom-menu");
        if (!bottomMenu) {
            log::warn("Versus Mode: main menu button container not found");
            return true;
        }

        // Geometry Dash ships with its own unused Versus-mode button artwork.
        auto* icon = CCSprite::createWithSpriteFrameName("GJ_versusBtn_001.png");
        if (!icon) {
            log::warn("Versus Mode: native Versus icon frame was not found; using a fallback button");
            icon = ButtonSprite::create("VERSUS", 48, true, "goldFont.fnt", "GJ_button_01.png", 30.f, 0.7f);
        } else {
            icon->setScale(0.65f);
        }

        auto* button = CCMenuItemSpriteExtra::create(icon, this, menu_selector(VersusMenuLayer::onVersusPressed));
        button->setID("versus-mode-button");
        bottomMenu->addChild(button);
        bottomMenu->updateLayout();
        return true;
    }

    void onVersusPressed(CCObject*) {
        Versus::disableMenuItems(this);
        auto* layer = Versus::LobbyLayer::create();
        if (!layer) {
            Versus::restoreBackgroundButtons();
            return;
        }
        layer->setID("versus-lobby-layer");
        this->getParent()->addChild(layer, 100);
    }
};
