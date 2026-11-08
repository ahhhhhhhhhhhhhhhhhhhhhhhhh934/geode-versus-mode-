#include <Geode/Geode.hpp>
#include <Geode/modify/GJMoreGamesLayer.hpp>

using namespace geode::prelude;

namespace Versus {
    enum class Screen { Home, Local, Online };
    static Screen s_screen = Screen::Home;

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

            auto* shade = CCLayerColor::create({7, 12, 31, 238});
            m_page->addChild(shade, -5);
            auto* glow = CCLayerColor::create({28, 70, 132, 90}, m_size.width, 4.f);
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
            label("SAME DEVICE / LOCAL PLAY",0.28f,{m_size.width/2,mid+43.f});
            label("ONLINE MATCH",0.48f,{m_size.width/2,mid-43.f},{220,150,255});
            label("ROOMS  •  INVITES  •  MATCHMAKING",0.27f,{m_size.width/2,mid-68.f});

            auto* menu=CCMenu::create(); menu->setPosition({0,0});
            menu->addChild(button("PLAY LOCAL",menu_selector(LobbyLayer::onLocal),{m_size.width/2,mid+5.f},"GJ_button_02.png",0.68f));
            menu->addChild(button("PLAY ONLINE",menu_selector(LobbyLayer::onOnline),{m_size.width/2,mid-107.f},"GJ_button_05.png",0.68f));
            menu->addChild(button("CLOSE",menu_selector(LobbyLayer::onClose),{m_size.width/2,42.f},"GJ_button_01.png",0.58f));
            m_page->addChild(menu,5);
            label("VERSUS MODE  •  BETA",0.25f,{m_size.width/2,15.f},{130,150,190});
        }

        void showMode(char const* title, char const* subtitle, char const* status, char const* detail) {
            basePage(title,subtitle);
            auto mid=m_size.height/2;
            addCard({m_size.width/2,mid},{m_size.width*0.82f,130.f},{18,38,72,250});
            label(status,0.48f,{m_size.width/2,mid+35.f},{255,220,115});
            label(detail,0.30f,{m_size.width/2,mid+3.f},{220,230,245});
            label("MATCH GAMEPLAY IS NOT CONNECTED YET",0.28f,{m_size.width/2,mid-29.f},{255,170,145});
            auto* menu=CCMenu::create(); menu->setPosition({0,0});
            menu->addChild(button("BACK",menu_selector(LobbyLayer::onBack),{m_size.width/2,48.f},"GJ_button_01.png",0.62f));
            m_page->addChild(menu,5);
        }

        void onLocal(CCObject*) {
            s_screen=Screen::Local;
            showMode("LOCAL VERSUS","PLAY TOGETHER ON ONE DEVICE","LOCAL MODE SELECTED","Two-player input and shared-device match rules are next.");
        }
        void onOnline(CCObject*) {
            s_screen=Screen::Online;
            showMode("ONLINE VERSUS","PLAY WITH FRIENDS OVER THE INTERNET","ONLINE MODE SELECTED","Room codes, matchmaking, and a relay server are required.");
        }
        void onBack(CCObject*) { showHome(); }
        void onClose(CCObject*) { this->removeFromParentAndCleanup(true); }
    };
}

class $modify(VersusMoreGamesLayer, GJMoreGamesLayer) {
    bool init() {
        if (!GJDropDownLayer::init("VERSUS MODE")) return false;
        auto size=CCDirector::sharedDirector()->getWinSize();
        auto* menu=CCMenu::create();
        menu->setID("versus-mode-grid-entry");
        menu->setPosition({0.f,0.f});
        auto* tile=ButtonSprite::create("VERSUS",86,true,"goldFont.fnt","GJ_button_01.png",58.f,0.8f);
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
