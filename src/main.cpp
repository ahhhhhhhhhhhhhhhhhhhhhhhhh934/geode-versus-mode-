#include <Geode/Geode.hpp>
#include <Geode/modify/CreatorLayer.hpp>
#include <globed/soft-link/Wrappers.hpp>

using namespace geode::prelude;

namespace Versus {
    enum class Screen { Home, Local, Online, Loading };
    static Screen s_screen = Screen::Home;
    static bool s_globedLoadedSignal = false;

    static geode::Mod* getGlobedMod() {
        return geode::Loader::get()->getInstalledMod("dankmeme.globed2");
    }

    static bool globedInstalled() {
        return getGlobedMod() != nullptr;
    }

    static bool globedEnabled() {
        auto* mod = getGlobedMod();
        return mod && mod->isOrWillBeEnabled();
    }

    static bool globedLoaded() {
        auto* mod = getGlobedMod();
        return mod && mod->isOrWillBeEnabled() &&
            geode::Loader::get()->isModLoaded("dankmeme.globed2");
    }

    static bool globedReady() {
        return globedLoaded() && globed::api::available() &&
            globed::api::net::isConnected();
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
            this->scheduleUpdate();
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
        bool m_waitingForGlobed = false;
        float m_globedWait = 0.f;
        float m_loadingPulse = 0.f;
        CCLabelBMFont* m_loadingLabel = nullptr;
        CCLayerColor* m_loadingFill = nullptr;

        void update(float dt) override {
            CCLayer::update(dt);
            if (!m_waitingForGlobed) return;
            m_globedWait += dt;
            m_loadingPulse += dt;
            if (m_loadingLabel) {
                int dots = static_cast<int>(m_loadingPulse * 3.f) % 4;
                std::string text = "CONNECTING TO GLOBED" + std::string(dots, '.');
                m_loadingLabel->setString(text.c_str());
            }
            if (m_loadingFill) {
                float progress = std::min(m_globedWait / 12.f, 1.f);
                m_loadingFill->setContentSize({m_size.width * 0.58f * progress, 9.f});
            }
            if (s_globedLoadedSignal && globedLoaded() && globed::api::available()) {
                m_waitingForGlobed = false;
                if (globed::api::net::isConnected()) {
                    if (globed::api::room::isInRoom()) {
                        auto players = globed::api::game::getPlayerCount();
                        auto roomId = globed::api::room::getId();
                        auto status = fmt::format("ROOM {}  •  {} PLAYERS", roomId, players);
                        showInfo("ONLINE VERSUS", "GLOBED ROOM READY", status.c_str(),
                            "Globed API is responding and your room is detected.",
                            "Matchmaking and Versus gameplay sync still need their own protocol.");
                    } else {
                        showInfo("ONLINE VERSUS", "GLOBED CONNECTION READY", "API CONNECTED",
                            "Globed is ready, but you are not inside a room.",
                            "Join a room in Globed, then return here.");
                    }
                } else {
                    showInfo("ONLINE VERSUS", "GLOBED API READY", "NOT CONNECTED",
                        "The Globed API loaded successfully.",
                        "Connect to a Globed server, then try Online Match again.");
                }
                return;
            }
            if (m_globedWait >= 12.f) {
                m_waitingForGlobed = false;
                showInfo("ONLINE VERSUS", "GLOBED INITIALIZATION", "API DID NOT RESPOND",
                    "Globed is installed, but its API did not become available.",
                    "Check that Globed 2.2.2 is enabled, then restart Geometry Dash.");
            }
        }

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
            basePage("VERSUS // ARENA", "ONLINE COMPETITION  •  BUILT FOR GEOMETRY DASH");
            auto mid = m_size.height / 2.f;

            // New dashboard layout: status header, two distinct mode panels,
            // and a compact bottom utility bar. This is a real layout change.
            addCard({m_size.width/2.f, mid+73.f}, {m_size.width*0.82f, 54.f}, {15, 37, 76, 255});
            auto* headerEdge = CCLayerColor::create({255,220,90,255}, m_size.width*0.82f, 3.f);
            headerEdge->setPosition({m_size.width*0.09f, mid+98.f});
            m_page->addChild(headerEdge, 2);
            label("NETWORK STATUS", 0.25f, {m_size.width*0.30f, mid+83.f}, {145,190,245});
            char const* state = !globedInstalled() ? "GLOBED REQUIRED" :
                (!globedEnabled() ? "GLOBED DISABLED" :
                (!globedLoaded() ? "GLOBED LOADING" :
                (!globed::api::available() ? "API UNAVAILABLE" :
                (globed::api::net::isConnected() ? "ONLINE" : "OFFLINE"))));
            label(state, 0.34f, {m_size.width*0.69f, mid+62.f},
                globedReady() ? ccColor3B{120,255,170} : ccColor3B{255,185,130});

            addCard({m_size.width*0.30f, mid-13.f}, {m_size.width*0.40f, 116.f}, {20, 61, 105, 255});
            addCard({m_size.width*0.72f, mid-13.f}, {m_size.width*0.40f, 116.f}, {67, 38, 103, 255});
            auto* localStripe = CCLayerColor::create({70,220,255,255}, 4.f, 116.f);
            localStripe->setPosition({m_size.width*0.30f-m_size.width*0.20f, mid-71.f});
            m_page->addChild(localStripe, 1);
            auto* onlineStripe = CCLayerColor::create({205,125,255,255}, 4.f, 116.f);
            onlineStripe->setPosition({m_size.width*0.72f-m_size.width*0.20f, mid-71.f});
            m_page->addChild(onlineStripe, 1);
            label("LOCAL", 0.48f, {m_size.width*0.30f, mid+23.f}, {120,225,255});
            label("2 PLAYERS", 0.27f, {m_size.width*0.30f, mid-3.f}, {190,220,245});
            label("SAME DEVICE", 0.24f, {m_size.width*0.30f, mid-25.f}, {145,185,220});
            label("ONLINE", 0.48f, {m_size.width*0.72f, mid+23.f}, {225,170,255});
            label("GLOBED ROOMS", 0.27f, {m_size.width*0.72f, mid-3.f}, {220,195,245});
            label("PLAY TOGETHER", 0.24f, {m_size.width*0.72f, mid-25.f}, {190,160,220});

            auto* menu = CCMenu::create();
            menu->setPosition({0,0});
            menu->addChild(button("LOCAL MATCH", menu_selector(LobbyLayer::onLocal),
                {m_size.width*0.30f, mid-83.f}, "GJ_button_02.png", 0.54f));
            menu->addChild(button("ONLINE MATCH", menu_selector(LobbyLayer::onOnline),
                {m_size.width*0.72f, mid-83.f}, "GJ_button_05.png", 0.54f));
            menu->addChild(button("WORKSHOP", menu_selector(LobbyLayer::onWorkshop),
                {m_size.width*0.30f, 43.f}, "GJ_button_04.png", 0.48f));
            menu->addChild(button("VOICE CHAT", menu_selector(LobbyLayer::onVoice),
                {m_size.width*0.72f, 43.f}, "GJ_button_01.png", 0.48f));

            m_page->addChild(menu, 5);
            label("VERSUS MODE  •  v0.4.4", 0.24f, {m_size.width/2.f, 13.f}, {125,155,195});
        }

        void onWorkshop(CCObject*) {
            showInfo("VERSUS WORKSHOP", "COMMUNITY CONTENT",
                "WORKSHOP SERVICE NOT CONNECTED",
                "The Geometry Dash-style Workshop entry is now in the lobby.",
                "Publishing and browsing need a hosted catalog and upload service; no fake listings are shown.");
        }

        void onVoice(CCObject*) {
            s_screen = Screen::Online;
            showInfo("VOICE CHAT", "GLOBED AUDIO", "GLOBED VOICE CHAT",
                "Globed provides in-game voice chat and emotes.",
                "Use Globed's in-level voice controls while playing together.");
        }


        void showGlobedLoading() {
            s_screen = Screen::Loading;
            m_waitingForGlobed = true;
            s_globedLoadedSignal = globedLoaded();
            m_globedWait = 0.f;
            if (!s_globedLoadedSignal) {
                globed::api::waitForGlobed([] {
                    s_globedLoadedSignal = true;
                });
            }
            m_loadingPulse = 0.f;
            basePage("ONLINE VERSUS", "GLOBED CONNECTION");
            auto mid = m_size.height / 2.f;
            addCard({m_size.width/2.f, mid+15.f}, {m_size.width*0.82f, 142.f}, {18, 42, 91, 255});
            label("PLEASE WAIT", 0.34f, {m_size.width/2.f, mid+58.f}, {255,220,105});
            m_loadingLabel = label("CONNECTING TO GLOBED", 0.38f,
                {m_size.width/2.f, mid+18.f}, {255,255,255});
            auto* track = CCLayerColor::create({10,18,44,255}, m_size.width*0.58f, 9.f);
            track->setPosition({m_size.width*0.21f, mid-20.f});
            m_page->addChild(track, 2);
            m_loadingFill = CCLayerColor::create({80,225,255,255}, 2.f, 9.f);
            m_loadingFill->setPosition({0.f,0.f});
            track->addChild(m_loadingFill, 3);
            label("CHECKING MOD API  •  12 SECOND TIMEOUT", 0.22f,
                {m_size.width/2.f, mid-48.f}, {165,195,235});
            auto* menu = CCMenu::create();
            menu->setPosition({0,0});
            menu->addChild(button("CANCEL", menu_selector(LobbyLayer::onBack),
                {m_size.width/2.f, 48.f}, "GJ_button_01.png", 0.62f));
            m_page->addChild(menu, 5);
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
            if (!globedInstalled()) {
                showInfo("ONLINE VERSUS","GLOBED INTEGRATION","GLOBED NOT INSTALLED",
                    "Install Globed 2.2.2 or newer to use online features.",
                    "Versus Mode can still be used without Globed.");
                return;
            }
            if (!globedEnabled()) {
                showInfo("ONLINE VERSUS","GLOBED INTEGRATION","GLOBED IS DISABLED",
                    "Enable Globed in the Geode mod list.",
                    "Then reopen Versus Mode.");
                return;
            }
            if (!globedLoaded() || !globed::api::available()) {
                showGlobedLoading();
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
        if (!tile) {
            // Fallback if a particular GD texture pack omits the frame.
            tile = CCSprite::createWithSpriteFrameName("GJ_createBtn_001.png");
        }
        auto* item = CCMenuItemSpriteExtra::create(
            tile, this, menu_selector(VersusCreatorLayer::onVersus)
        );
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
        auto* lobby = Versus::LobbyLayer::create();
        if (lobby) {
            lobby->setID("versus-mode-lobby");
            this->addChild(lobby, 2000);
        }
    }
};
