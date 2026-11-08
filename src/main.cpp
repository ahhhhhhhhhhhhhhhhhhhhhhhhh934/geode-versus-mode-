#include <Geode/Geode.hpp>
#include <Geode/modify/GJMoreGamesLayer.hpp>
#include <globed/soft-link/Wrappers.hpp>

using namespace geode::prelude;

namespace Versus {
    enum class Screen { Home, Local, Online };
    static Screen s_screen = Screen::Home;

    static bool globedReady() {
        return globed::api::available() && globed::api::net::isConnected();
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
            this->setKeypadEnabled(true);
            this->showHome();
            return true;
        }

        void keyBackClicked() override {
            if (s_screen != Screen::Home) showHome();
            else this->removeFromParentAndCleanup(true);
        }

    private:
        CCSize m_size;
        CCLayer* m_page = nullptr;

        void clearPage() {
            if (m_page) {
                m_page->removeFromParentAndCleanup(true);
                m_page = nullptr;
            }
        }

        CCLabelBMFont* label(char const* text, float scale, CCPoint pos, ccColor3B color = {255,255,255}) {
            auto* l = CCLabelBMFont::create(text, "bigFont.fnt");
            l->setScale(scale);
            l->setColor(color);
            l->setPosition(pos);
            m_page->addChild(l, 4);
            return l;
        }

        CCMenuItemSpriteExtra* button(char const* text, SEL_MenuHandler cb, CCPoint pos, char const* frame = "GJ_button_01.png", float scale = 0.72f) {
            auto* sprite = ButtonSprite::create(text, 42, true, "goldFont.fnt", frame, 36.f, scale);
            auto* item = CCMenuItemSpriteExtra::create(sprite, this, cb);
            item->setPosition(pos);
            return item;
        }

        void basePage(char const* title, char const* subtitle) {
            clearPage();
            m_size = CCDirector::sharedDirector()->getWinSize();
            m_page = CCLayer::create();
            this->addChild(m_page, 1);

            auto* shade = CCLayerColor::create({7, 12, 31, 245});
            m_page->addChild(shade, -5);
            auto* glow = CCLayerColor::create({28, 70, 132, 130}, m_size.width, 4.f);
            glow->setPosition({0.f, m_size.height - 5.f});
            m_page->addChild(glow, -4);

            label(title, 0.78f, {m_size.width/2, m_size.height - 42.f}, {255,224,110});
            label(subtitle, 0.34f, {m_size.width/2, m_size.height - 70.f}, {185,210,245});
        }

        void addCard(CCPoint center, CCSize size, ccColor4B tint) {
            auto* card = CCLayerColor::create(tint, size.width, size.height);
            card->setPosition({center.x-size.width/2, center.y-size.height/2});
            m_page->addChild(card, 0);
            auto* edge = CCLayerColor::create({100,190,255,255}, size.width, 2.f);
            edge->setPosition({0.f,size.height-2.f});
            card->addChild(edge, 1);
        }

        void showHome() {
            s_screen = Screen::Home;
            basePage("VERSUS MODE", "CHALLENGE A FRIEND  •  RACE FOR THE WIN");
            auto mid = m_size.height/2;
            addCard({m_size.width/2,mid+47.f},{m_size.width*0.72f,94.f},{22,55,105,245});
            addCard({m_size.width/2,mid-65.f},{m_size.width*0.72f,94.f},{70,36,104,245});

            label("LOCAL MATCH",0.48f,{m_size.width/2,mid+69.f},{120,235,255});
            label("TWO-PLAYER / SHARED DEVICE",0.28f,{m_size.width/2,mid+43.f});
            label("ONLINE MATCH",0.48f,{m_size.width/2,mid-43.f},{220,150,255});
            label(globedReady() ? "GLOBED CONNECTED" : "GLOBED NOT CONNECTED",0.30f,
                {m_size.width/2,mid-68.f}, globedReady() ? ccColor3B{120,255,170} : ccColor3B{255,175,145});

            auto* menu=CCMenu::create(); menu->setPosition({0,0});
            menu->addChild(button("LOCAL PLAY",menu_selector(LobbyLayer::onLocal),{m_size.width/2,mid+5.f},"GJ_button_02.png",0.68f));
            menu->addChild(button("ONLINE PLAY",menu_selector(LobbyLayer::onOnline),{m_size.width/2,mid-107.f},"GJ_button_05.png",0.68f));
            menu->addChild(button("CLOSE",menu_selector(LobbyLayer::onClose),{m_size.width/2,42.f},"GJ_button_01.png",0.58f));
            m_page->addChild(menu,5);
            label("BETA  •  v0.3.3",0.25f,{m_size.width/2,15.f},{130,150,190});
        }

        void showInfo(char const* title, char const* subtitle, char const* status, char const* detail, char const* extra) {
            basePage(title,subtitle);
            auto mid=m_size.height/2;
            addCard({m_size.width/2,mid},{m_size.width*0.84f,148.f},{18,38,72,250});
            label(status,0.45f,{m_size.width/2,mid+48.f},{255,220,115});
            label(detail,0.29f,{m_size.width/2,mid+15.f},{220,230,245});
            label(extra,0.27f,{m_size.width/2,mid-19.f},{160,205,255});
            auto* menu=CCMenu::create(); menu->setPosition({0,0});
            menu->addChild(button("BACK",menu_selector(LobbyLayer::onBack),{m_size.width/2,48.f},"GJ_button_01.png",0.62f));
            m_page->addChild(menu,5);
        }

        void onLocal(CCObject*) {
            s_screen=Screen::Local;
            showInfo("LOCAL VERSUS","SAME DEVICE MATCH","LOCAL MODE",
                "Local match setup is being built.",
                "Two independent player controls are still needed.");
        }
        void onOnline(CCObject*) {
            s_screen=Screen::Online;
            if (!globed::api::available()) {
                showInfo("ONLINE VERSUS","GLOBED INTEGRATION","GLOBED API NOT FOUND",
                    "Install and enable Globed 2.2.2 or newer.",
                    "Then connect to a Globed server.");
                return;
            }
            if (!globed::api::net::isConnected()) {
                showInfo("ONLINE VERSUS","GLOBED INTEGRATION","NOT CONNECTED",
                    "Globed is installed, but you are not connected.",
                    "Connect through Globed, then reopen this screen.");
                return;
            }
            if (!globed::api::room::isInRoom()) {
                showInfo("ONLINE VERSUS","GLOBED INTEGRATION","CONNECTED TO GLOBED",
                    "You are not inside a private room yet.",
                    "Join or create a Globed room, then return here.");
                return;
            }
            auto players = globed::api::game::getPlayerCount();
            auto roomId = globed::api::room::getId();
            auto status = fmt::format("ROOM {}  •  {} PLAYERS", roomId, players);
            auto detail = std::string("Globed room detected. Player sync can use its event API.");
            showInfo("ONLINE VERSUS","GLOBED ROOM DETECTED",status.c_str(),
                detail.c_str(),"Room creation and matchmaking are not exposed by Soft Link.");
        }
        void onBack(CCObject*) { showHome(); }
        void onClose(CCObject*) { this->removeFromParentAndCleanup(true); }
    };
}

class $modify(VersusMoreGamesLayer, GJMoreGamesLayer) {
    bool init() {
        // Preserve the original More Games grid; calling GJDropDownLayer::init
        // directly skips GJMoreGamesLayer's own initialization.
        if (!GJMoreGamesLayer::init()) return false;
        auto size=CCDirector::sharedDirector()->getWinSize();
        auto* menu=CCMenu::create();
        menu->setID("versus-mode-grid-entry");
        menu->setPosition({0.f,0.f});
        auto* tile=ButtonSprite::create("VERSUS",86,true,"goldFont.fnt","GJ_button_02.png",58.f,0.8f);
        auto* item=CCMenuItemSpriteExtra::create(tile,this,menu_selector(VersusMoreGamesLayer::onVersus));
        item->setID("versus-mode-button");
        item->setPosition({size.width*0.788f,size.height*0.808f});
        menu->addChild(item);
        this->addChild(menu,100);
        return true;
    }
    void onVersus(CCObject*) {
        auto* lobby=Versus::LobbyLayer::create();
        if (lobby) { lobby->setID("versus-mode-lobby"); this->addChild(lobby,1000); }
    }
};
