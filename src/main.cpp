#include <Geode/Geode.hpp>
#include <Geode/modify/CreatorLayer.hpp>
#include <globed/soft-link/API.hpp>

using namespace geode::prelude;

namespace Versus {
    // Use Globed's supported Soft Link API. Its table lookup retries while
    // Globed is installed/enabled, and its event export handles ABI-safe lookup.
    static bool globedApiAvailable() {
        return globed::api::available();
    }

    static bool globedConnected() {
        return globed::api::net::isConnected();
    }

    static bool globedInRoom() {
        return globed::api::room::isInRoom();
    }

    static uint32_t globedRoomId() {
        return globed::api::room::getId();
    }

    static size_t globedPlayerCount() {
        return globed::api::game::getPlayerCount();
    }

    enum class Screen { Home, Local, Online, Loading };
    static Screen s_screen = Screen::Home;

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
        return globedLoaded() && globedApiAvailable() &&
            globedConnected();
    }

    class LobbyLayer final : public CCLayer {
    public:
        static LobbyLayer* create() {
            auto* ret = new LobbyLayer();
            if (ret && ret->init()) { ret->autorelease(); return ret; }
            CC_SAFE_DELETE(ret);
            return nullptr;
        }

        static CCScene* scene() {
            auto* scene = CCScene::create();
            if (!scene) return nullptr;
            auto* layer = LobbyLayer::create();
            if (!layer) return nullptr;
            scene->addChild(layer);
            return scene;
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
            else CCDirector::sharedDirector()->popScene();
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
            if (globedLoaded() && globedApiAvailable()) {
                m_waitingForGlobed = false;
                if (globedConnected()) {
                    if (globedInRoom()) {
                        auto players = globedPlayerCount();
                        auto roomId = globedRoomId();
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
                if (!globedLoaded()) {
                    showInfo("ONLINE VERSUS", "GLOBED INITIALIZATION", "GLOBED DID NOT LOAD",
                        "Geode has not finished loading the Globed mod.",
                        "Check that Globed 2.2.2 is enabled and restart Geometry Dash.");
                } else {
                    auto mod = getGlobedMod();
                    auto version = mod ? "installed" : "unknown";
                    log::warn("Versus: Globed loaded ({}), but its exported Soft Link API table was not returned.", version);
                    showInfo("ONLINE VERSUS", "GLOBED INITIALIZATION", "SOFT LINK TABLE MISSING",
                        "Geode loaded Globed, but its exported API table was not returned.",
                        "Fully restart Geometry Dash with Globed 2.2.2 enabled, then retry.");
                }
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
            basePage("VERSUS", "THE GEOMETRY DASH COMPETITION HUB");
            auto w = m_size.width;
            auto h = m_size.height;
            auto mid = h * 0.51f;

            // Original plaza-inspired composition: identity bar, arena stage,
            // side activity panels, and a bottom navigation dock.
            auto* top = CCLayerColor::create({12, 25, 52, 248}, w * 0.92f, 46.f);
            top->setPosition({w * 0.04f, h - 100.f});
            m_page->addChild(top, 0);
            auto* topLine = CCLayerColor::create({70, 194, 255, 255}, w * 0.92f, 2.f);
            topLine->setPosition({w * 0.04f, h - 100.f});
            m_page->addChild(topLine, 1);

            label("PLAYER", 0.22f, {w * 0.13f, h - 70.f}, {125, 190, 240});
            label("YOUR PROFILE", 0.34f, {w * 0.29f, h - 82.f}, {255, 226, 145});

            char const* network = !globedInstalled() ? "GLOBED REQUIRED" :
                (!globedEnabled() ? "GLOBED DISABLED" :
                (!globedLoaded() ? "GLOBED STARTING" :
                (!globedApiAvailable() ? "API WAITING" :
                (globedConnected() ? "ONLINE" : "OFFLINE"))));
            label(network, 0.27f, {w * 0.75f, h - 82.f},
                globedReady() ? ccColor3B{115,255,170} : ccColor3B{255,195,115});

            auto* stage = CCLayerColor::create({15, 38, 76, 248}, w * 0.72f, h * 0.36f);
            stage->setPosition({w * 0.14f, mid - h * 0.10f});
            m_page->addChild(stage, 0);
            auto* stageTop = CCLayerColor::create({62, 175, 245, 255}, w * 0.72f, 3.f);
            stageTop->setPosition({w * 0.14f, mid + h * 0.26f - 3.f});
            m_page->addChild(stageTop, 1);

            auto* glowA = CCLayerColor::create({28, 91, 150, 100}, w * 0.52f, 3.f);
            glowA->setPosition({w * 0.24f, mid + h * 0.09f});
            m_page->addChild(glowA, 1);
            auto* glowB = CCLayerColor::create({105, 65, 160, 110}, w * 0.40f, 2.f);
            glowB->setPosition({w * 0.30f, mid - h * 0.03f});
            m_page->addChild(glowB, 1);

            label("ARENA PLAZA", 0.32f, {w * 0.50f, mid + h * 0.205f}, {150, 210, 255});
            label("READY FOR YOUR NEXT DUEL?", 0.36f, {w * 0.50f, mid + h * 0.105f}, {255, 235, 170});
            label("Find an opponent and compete head-to-head", 0.25f,
                {w * 0.50f, mid + h * 0.045f}, {195, 218, 245});

            auto* centerMark = CCSprite::createWithSpriteFrameName("GJ_versusBtn_001.png");
            if (centerMark) {
                centerMark->setPosition({w * 0.50f, mid - h * 0.035f});
                centerMark->setScale(0.72f);
                m_page->addChild(centerMark, 3);
            } else {
                label("VS", 0.72f, {w * 0.50f, mid - h * 0.035f}, {120, 220, 255});
            }

            auto* menu = CCMenu::create();
            menu->setPosition({0,0});
            menu->addChild(button("FIND MATCH", menu_selector(LobbyLayer::onOnline),
                {w * 0.50f, mid - h * 0.145f}, "GJ_button_05.png", 0.78f));
            m_page->addChild(menu, 5);

            auto* left = CCLayerColor::create({19, 48, 82, 245}, w * 0.34f, h * 0.13f);
            left->setPosition({w * 0.08f, h * 0.205f});
            m_page->addChild(left, 0);
            auto* right = CCLayerColor::create({42, 35, 82, 245}, w * 0.34f, h * 0.13f);
            right->setPosition({w * 0.58f, h * 0.205f});
            m_page->addChild(right, 0);
            label("ROOMS", 0.31f, {w * 0.25f, h * 0.285f}, {120, 220, 255});
            label("Browse / create", 0.22f, {w * 0.25f, h * 0.245f}, {190, 215, 245});
            label("LIVE", 0.31f, {w * 0.75f, h * 0.285f}, {220, 165, 255});
            label("Watch matches", 0.22f, {w * 0.75f, h * 0.245f}, {205, 190, 235});

            auto* dock = CCLayerColor::create({8, 17, 37, 250}, w * 0.92f, 42.f);
            dock->setPosition({w * 0.04f, 30.f});
            m_page->addChild(dock, 0);
            auto* dockLine = CCLayerColor::create({55, 112, 180, 255}, w * 0.92f, 2.f);
            dockLine->setPosition({w * 0.04f, 70.f});
            m_page->addChild(dockLine, 1);

            auto* dockMenu = CCMenu::create();
            dockMenu->setPosition({0,0});
            dockMenu->addChild(button("PLAZA", menu_selector(LobbyLayer::onHome),
                {w * 0.22f, 51.f}, "GJ_button_02.png", 0.38f));
            dockMenu->addChild(button("WORKSHOP", menu_selector(LobbyLayer::onWorkshop),
                {w * 0.50f, 51.f}, "GJ_button_04.png", 0.38f));
            dockMenu->addChild(button("VOICE", menu_selector(LobbyLayer::onVoice),
                {w * 0.78f, 51.f}, "GJ_button_01.png", 0.38f));
            dockMenu->addChild(button("BACK", menu_selector(LobbyLayer::onClose),
                {w * 0.075f, h - 76.f}, "GJ_button_01.png", 0.38f));
            m_page->addChild(dockMenu, 5);

            label("VERSUS MODE  •  v0.6.7", 0.20f, {w/2.f, 12.f}, {125,155,195});
        }

        void onHome(CCObject*) { showHome(); }

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
            m_globedWait = 0.f;
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
            if (!globedLoaded() || !globedApiAvailable()) {
                showGlobedLoading();
                return;
            }
            if (!globedConnected()) {
                showInfo("ONLINE VERSUS","GLOBED INTEGRATION","NOT CONNECTED",
                    "Globed is installed, but you are not connected.",
                    "Connect through Globed, then reopen this screen.");
                return;
            }
            if (!globedInRoom()) {
                showInfo("ONLINE VERSUS","GLOBED INTEGRATION","CONNECTED TO GLOBED",
                    "You are not inside a private room yet.",
                    "Join or create a Globed room, then return here.");
                return;
            }
            auto players = globedPlayerCount();
            auto roomId = globedRoomId();
            auto status = fmt::format("ROOM {}  •  {} PLAYERS", roomId, players);
            auto detail = std::string("Globed room detected. Player sync can use its event API.");
            showInfo("ONLINE VERSUS","GLOBED ROOM DETECTED",status.c_str(),
                detail.c_str(),"Room creation and matchmaking are not exposed by Soft Link.");
        }
        void onBack(CCObject*) { showHome(); }
        void onClose(CCObject*) { CCDirector::sharedDirector()->popScene(); }
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
        auto* scene = Versus::LobbyLayer::scene();
        if (scene) {
            scene->setID("versus-mode-scene");
            CCDirector::sharedDirector()->pushScene(
                CCTransitionFade::create(0.25f, scene)
            );
        }
    }
};
