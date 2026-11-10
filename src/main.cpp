#include <Geode/Geode.hpp>
#include <Geode/modify/CreatorLayer.hpp>
#include <globed/soft-link/API.hpp>
#include <Geode/utils/web.hpp>

using namespace geode::prelude;

namespace Versus {
    static bool globedLoaded();
    // Probe only the public Soft Link API. Keep the exact dependency version
    // visible in diagnostics so an unavailable table is not misdiagnosed as a
    // network disconnect.
    static bool globedApiAvailable() {
        if (!globedLoaded()) return false;
        const bool available = globed::api::available();
        static bool loggedUnavailable = false;
        if (!available && !loggedUnavailable) {
            loggedUnavailable = true;
            auto* mod = geode::Loader::get()->getInstalledMod("dankmeme.globed2");
            auto version = mod ? mod->getVersion().toVString() : std::string("not installed");
            log::warn("Versus: Globed {} is loaded, but getRootTable() did not return an API table. This is an API-export/loader issue, not a server connection state.", version);
        }
        if (available) loggedUnavailable = false;
        return available;
    }

    static bool globedConnected() {
        return globedApiAvailable() && globed::api::net::isConnected();
    }

    static bool globedInRoom() {
        return globedApiAvailable() && globed::api::room::isInRoom();
    }


    // Globed 2.3.0 adds the session warp API. Keep this version check at
    // the integration boundary so older stable Globed builds never receive
    // a call to an unavailable function.
    static bool globedSessionHandoffAvailable() {
        return globedApiAvailable() && globed::api::isAtLeast("v2.3.0");
    }

    static bool warpToGlobedSession(globed::SessionId const& session) {
        if (!globedSessionHandoffAvailable()) {
            log::warn("Versus: Globed 2.3.0 or newer is required for session handoff.");
            return false;
        }
        globed::api::game::warpToSession(session);
        return true;
    }

    static uint32_t globedRoomId() {
        return globedApiAvailable() && globedInRoom()
            ? globed::api::room::getId() : 0;
    }

    enum class Screen { Home, Plaza, Ranked, Profile, PublicRooms, GroupRooms, Live, Rooms, Voice, Local, Online, Loading, Info };
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

    static char const* globedConnectionStateName() {
        if (!globedLoaded()) return "GLOBED NOT LOADED";
        switch (globed::api::net::getConnectionState()) {
            case globed::ConnectionState::Disconnected: return "DISCONNECTED";
            case globed::ConnectionState::DnsResolving: return "DNS RESOLVING";
            case globed::ConnectionState::Pinging: return "PINGING";
            case globed::ConnectionState::Connecting: return "CONNECTING";
            case globed::ConnectionState::Authenticating: return "AUTHENTICATING";
            case globed::ConnectionState::Connected: return "CONNECTED";
            case globed::ConnectionState::Closing: return "CLOSING";
            case globed::ConnectionState::Reconnecting: return "RECONNECTING";
            default: return "UNKNOWN";
        }
    }

    struct RankTier {
        char const* name;
        int minimumPoints;
        ccColor3B color;
    };

    // The ladder uses Geometry Dash's real classic difficulty names.
    // Points are local until a Versus match server exists to verify results.
    static RankTier const kRankTiers[] = {
        {"AUTO", 0, {155, 165, 180}},
        {"EASY", 100, {95, 220, 125}},
        {"NORMAL", 250, {95, 185, 245}},
        {"HARD", 450, {255, 205, 85}},
        {"HARDER", 700, {255, 145, 70}},
        {"INSANE", 1000, {220, 105, 245}},
        {"EASY DEMON", 1400, {255, 85, 95}},
        {"MEDIUM DEMON", 1850, {235, 70, 80}},
        {"HARD DEMON", 2350, {205, 55, 75}},
        {"INSANE DEMON", 2900, {175, 45, 85}},
        {"EXTREME DEMON", 3500, {255, 55, 65}}
    };

    static int rankPoints() {
        return std::max(0, Mod::get()->getSavedValue<int>("rank-points", 0));
    }

    static size_t rankTierIndex(int points) {
        size_t tier = 0;
        for (size_t i = 0; i < sizeof(kRankTiers) / sizeof(kRankTiers[0]); ++i) {
            if (points >= kRankTiers[i].minimumPoints) tier = i;
        }
        return tier;
    }

    static char const* rankName(int points) {
        return points <= 0 ? "UNRANKED" : kRankTiers[rankTierIndex(points)].name;
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
        async::TaskHolder<web::WebResponse> m_queueTask;
        std::string m_queueTicket;
        std::string m_queueMode = "race";
        std::string m_queueState;
        float m_queuePollTimer = 0.f;
        bool m_queueRequestPending = false;
        bool m_matchReady = false;

        float m_globedWait = 0.f;
        float m_loadingPulse = 0.f;
        CCLabelBMFont* m_loadingLabel = nullptr;
        CCLayerColor* m_loadingFill = nullptr;

        void update(float dt) override {
            CCLayer::update(dt);
            if (!m_queueTicket.empty() && !m_queueRequestPending && !m_matchReady) {
                m_queuePollTimer += dt;
                if (m_queuePollTimer >= 2.f) {
                    m_queuePollTimer = 0.f;
                    pollGlobalQueue();
                }
            }
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
                        auto roomId = globedRoomId();
                        auto status = fmt::format("ROOM {}", roomId);
                        showInfo("ONLINE VERSUS", "GLOBED ROOM READY", status.c_str(),
                            "Globed's Soft Link API is responding and your room is detected.",
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
            if (m_globedWait >= (globedLoaded() ? 3.f : 12.f)) {
                m_waitingForGlobed = false;
                if (!globedLoaded()) {
                    showInfo("ONLINE VERSUS", "GLOBED INITIALIZATION", "GLOBED DID NOT LOAD",
                        "Geode has not finished loading the Globed mod.",
                        "Enable Globed in Geode, then restart Geometry Dash.");
                } else {
                    log::warn("Versus: Globed loaded but its Soft Link API table could not be queried.");
                    showInfo("ONLINE VERSUS", "GLOBED INITIALIZATION", "GLOBED API UNAVAILABLE",
                        "Globed is loaded, but its Soft Link API could not be called.",
                        "Check that Globed 2.3.0 beta or newer is installed and enabled, then send the new log if this persists.");
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
            m_waitingForGlobed = false;
            basePage("VERSUS MODE", "COMPETE  •  CONNECT  •  CLIMB");
            auto w = m_size.width;
            auto h = m_size.height;

            auto* status = CCLayerColor::create({12, 25, 52, 248}, w * 0.88f, 42.f);
            status->setPosition({w * 0.06f, h - 112.f});
            m_page->addChild(status, 0);
            label("NETWORK", 0.22f, {w * 0.19f, h - 91.f}, {125,190,240});
            char const* network = !globedInstalled() ? "GLOBED NOT INSTALLED" :
                (!globedEnabled() ? "GLOBED DISABLED" :
                (!globedLoaded() ? "GLOBED STARTING" :
                globedConnectionStateName()));
            label(network, 0.27f, {w * 0.63f, h - 91.f},
                globedReady() ? ccColor3B{115,255,170} : ccColor3B{255,195,115});

            auto* hero = CCLayerColor::create({15, 38, 76, 248}, w * 0.86f, h * 0.39f);
            hero->setPosition({w * 0.07f, h * 0.34f});
            m_page->addChild(hero, 0);
            auto* heroLine = CCLayerColor::create({62,175,245,255}, w * 0.86f, 3.f);
            heroLine->setPosition({w * 0.07f, h * 0.34f + h * 0.39f - 3.f});
            m_page->addChild(heroLine, 1);
            label("YOUR NEXT MATCH STARTS HERE", 0.39f,
                {w * 0.50f, h * 0.67f}, {255,224,110});
            label("A new home for Geometry Dash competition", 0.27f,
                {w * 0.50f, h * 0.60f}, {195,218,245});
            label(fmt::format("{}  •  {} RP", rankName(rankPoints()), rankPoints()).c_str(),
                0.30f, {w * 0.50f, h * 0.51f}, {145,220,255});

            auto* menu = CCMenu::create();
            menu->setPosition({0,0});
            menu->addChild(button("OPEN PLAZA", menu_selector(LobbyLayer::onOpenPlaza),
                {w * 0.50f, h * 0.43f}, "GJ_button_05.png", 0.66f));
            menu->addChild(button("RANKS", menu_selector(LobbyLayer::onRanked),
                {w * 0.27f, h * 0.25f}, "GJ_button_02.png", 0.48f));
            menu->addChild(button("ROOMS", menu_selector(LobbyLayer::onPublicRooms),
                {w * 0.50f, h * 0.25f}, "GJ_button_04.png", 0.48f));
            menu->addChild(button("PROFILE", menu_selector(LobbyLayer::onProfile),
                {w * 0.73f, h * 0.25f}, "GJ_button_03.png", 0.48f));
            menu->addChild(button("EXIT", menu_selector(LobbyLayer::onClose),
                {w * 0.50f, 42.f}, "GJ_button_01.png", 0.45f));
            m_page->addChild(menu, 5);
            label("VERSUS MODE  •  GLOBED 2.3.0+", 0.20f, {w/2.f, 12.f}, {125,155,195});
        }

        void showPlaza() {
            s_screen = Screen::Plaza;
            basePage("PLAZA", "THE VERSUS COMMUNITY HUB");
            auto w = m_size.width;
            auto h = m_size.height;

            addCard({w * 0.50f, h * 0.72f}, {w * 0.88f, h * 0.25f}, {16, 39, 75, 250});
            label("WELCOME TO THE PLAZA", 0.42f,
                {w * 0.50f, h * 0.80f}, {255,224,110});
            label(fmt::format("{}  •  {} RANK POINTS", rankName(rankPoints()), rankPoints()).c_str(),
                0.28f, {w * 0.50f, h * 0.72f}, {145,220,255});
            std::string state = !globedInstalled() ? "GLOBED NOT INSTALLED" :
                (!globedEnabled() ? "GLOBED DISABLED" :
                (!globedLoaded() ? "GLOBED STARTING" :
                fmt::format("GLOBED {}", globedConnectionStateName())));
            label(state.c_str(), 0.23f, {w * 0.50f, h * 0.65f},
                globedReady() ? ccColor3B{115,255,170} : ccColor3B{255,195,115});

            auto* menu = CCMenu::create();
            menu->setPosition({0,0});
            menu->addChild(button("FIND MATCH", menu_selector(LobbyLayer::onOnline),
                {w * 0.50f, h * 0.53f}, "GJ_button_05.png", 0.56f));
            menu->addChild(button("RANKED", menu_selector(LobbyLayer::onRanked),
                {w * 0.27f, h * 0.41f}, "GJ_button_02.png", 0.44f));
            menu->addChild(button("PUBLIC ROOMS", menu_selector(LobbyLayer::onPublicRooms),
                {w * 0.73f, h * 0.41f}, "GJ_button_04.png", 0.42f));
            menu->addChild(button("GROUP ROOMS", menu_selector(LobbyLayer::onGroupRooms),
                {w * 0.27f, h * 0.30f}, "GJ_button_01.png", 0.42f));
            menu->addChild(button("LIVE", menu_selector(LobbyLayer::onLive),
                {w * 0.73f, h * 0.30f}, "GJ_button_03.png", 0.44f));
            menu->addChild(button("PROFILE", menu_selector(LobbyLayer::onProfile),
                {w * 0.50f, h * 0.19f}, "GJ_button_02.png", 0.42f));
            menu->addChild(button("HOME", menu_selector(LobbyLayer::onDashboard),
                {w * 0.50f, 38.f}, "GJ_button_01.png", 0.43f));
            m_page->addChild(menu, 5);
        }

        void showRanked() {
            s_screen = Screen::Ranked;
            basePage("RANKED", "GEOMETRY DASH DIFFICULTY LADDER");
            auto w = m_size.width;
            auto h = m_size.height;
            int points = rankPoints();
            addCard({w * 0.50f, h * 0.83f}, {w * 0.88f, h * 0.16f}, {16, 39, 75, 250});
            label(fmt::format("{}  •  {} RP", rankName(points), points).c_str(),
                0.34f, {w * 0.50f, h * 0.85f}, {255,224,110});
            label("Local ladder preview — online rating is not synced yet", 0.21f,
                {w * 0.50f, h * 0.80f}, {185,210,245});

            auto count = sizeof(kRankTiers) / sizeof(kRankTiers[0]);
            for (size_t i = 0; i < count; ++i) {
                auto column = i % 2;
                auto row = i / 2;
                float x = column == 0 ? w * 0.27f : w * 0.73f;
                float y = h * 0.68f - static_cast<float>(row) * h * 0.085f;
                auto const& tier = kRankTiers[i];
                bool active = points >= tier.minimumPoints &&
                    (i + 1 == count || points < kRankTiers[i + 1].minimumPoints);
                auto* tile = CCLayerColor::create(
                    active ? ccColor4B{tier.color.r, tier.color.g, tier.color.b, 255}
                           : ccColor4B{20, 34, 58, 245},
                    w * 0.43f, h * 0.065f
                );
                tile->setPosition({x - w * 0.215f, y - h * 0.0325f});
                m_page->addChild(tile, 1);
                // Use Geometry Dash's own difficulty-face sprite, not a custom imitation.
                int gdDifficulty = i == 0 ? -1 : static_cast<int>(i);
                auto* face = GJDifficultySprite::create(gdDifficulty, GJDifficultyName::Short);
                if (face) {
                    face->setScale(0.48f);
                    face->setPosition({x - w * 0.145f, y});
                    m_page->addChild(face, 3);
                }
                label(tier.name, 0.22f, {x + w * 0.035f, y + 2.f},
                    active ? ccColor3B{255,255,255} : tier.color);
                label(fmt::format("{} RP", tier.minimumPoints).c_str(),
                    0.16f, {x + w * 0.035f, y - h * 0.022f}, {185,205,230});
            }

            auto* menu = CCMenu::create();
            menu->setPosition({0,0});
            menu->addChild(button("PLAZA", menu_selector(LobbyLayer::onOpenPlaza),
                {w * 0.50f, 35.f}, "GJ_button_01.png", 0.43f));
            m_page->addChild(menu, 5);
        }

        void showProfile() {
            s_screen = Screen::Profile;
            basePage("PLAYER PROFILE", "YOUR VERSUS IDENTITY");
            auto w = m_size.width;
            auto h = m_size.height;
            addCard({w * 0.50f, h * 0.60f}, {w * 0.86f, h * 0.42f}, {18,42,91,255});
            label("LOCAL PLAYER", 0.38f, {w * 0.50f, h * 0.75f}, {255,224,110});
            label(fmt::format("RANK  {}", rankName(rankPoints())).c_str(),
                0.32f, {w * 0.50f, h * 0.65f}, {145,220,255});
            label(fmt::format("{} RANK POINTS", rankPoints()).c_str(),
                0.27f, {w * 0.50f, h * 0.56f}, {225,235,250});
            label("Online profiles, records and statistics need the Versus server.", 0.22f,
                {w * 0.50f, h * 0.46f}, {180,205,235});
            auto* menu = CCMenu::create();
            menu->setPosition({0,0});
            menu->addChild(button("RANKS", menu_selector(LobbyLayer::onRanked),
                {w * 0.30f, 38.f}, "GJ_button_02.png", 0.43f));
            menu->addChild(button("PLAZA", menu_selector(LobbyLayer::onOpenPlaza),
                {w * 0.70f, 38.f}, "GJ_button_01.png", 0.43f));
            m_page->addChild(menu, 5);
        }

        void showPublicRooms() {
            s_screen = Screen::PublicRooms;
            showInfo("PUBLIC ROOMS", "ROOM BROWSER",
                "ROOM DIRECTORY NOT CONNECTED",
                "Globed can show its own rooms, but Versus does not have a room-list service yet.",
                "This page will not invent rooms or show fake player counts.");
        }

        void showGroupRooms() {
            s_screen = Screen::GroupRooms;
            showInfo("GROUP ROOMS", "PLAY WITH MORE PEOPLE",
                "GROUP MATCHMAKING NOT CONNECTED",
                "The room interface is planned, but no Versus group-room server exists yet.",
                "Globed room data alone is not a Versus group match.");
        }

        void showLive() {
            s_screen = Screen::Live;
            showInfo("LIVE", "WATCH ACTIVE MATCHES",
                "LIVE FEED NOT CONNECTED",
                "Spectating needs a Versus match feed and spectator protocol.",
                "No live matches can be listed until that service is built.");
        }

        void showRooms() {
            s_screen = Screen::Rooms;
            basePage("ROOM STATUS", "GLOBED CONNECTION INFORMATION");
            auto w = m_size.width;
            auto h = m_size.height;
            auto mid = h * 0.57f;
            addCard({w * 0.50f, mid}, {w * 0.86f, h * 0.31f}, {18,42,91,255});

            char const* state = !globedInstalled() ? "GLOBED NOT INSTALLED" :
                (!globedEnabled() ? "GLOBED DISABLED" :
                (!globedLoaded() ? "GLOBED NOT LOADED" :
                (!globedConnected() ? "NOT CONNECTED" :
                (globedInRoom() ? "INSIDE A ROOM" : "CONNECTED  •  NO ROOM"))));
            label(state, 0.37f, {w * 0.50f, mid + h * 0.075f},
                globedReady() ? ccColor3B{115,255,170} : ccColor3B{255,195,115});

            if (globedLoaded() && globedApiAvailable() && globedConnected() && globedInRoom()) {
                auto roomText = fmt::format("ROOM ID  {}", globedRoomId());
                label(roomText.c_str(), 0.30f, {w * 0.50f, mid + h * 0.005f}, {225,235,250});
                label("ROOM DETECTED THROUGH GLOBED API", 0.24f,
                    {w * 0.50f, mid - h * 0.055f}, {170,210,250});
            } else {
                label("Join or create a room using Globed's own room menu.", 0.27f,
                    {w * 0.50f, mid}, {220,230,245});
                label("Versus room creation and matchmaking are not implemented yet.", 0.23f,
                    {w * 0.50f, mid - h * 0.055f}, {170,195,230});
            }

            auto* menu = CCMenu::create();
            menu->setPosition({0,0});
            menu->addChild(button("REFRESH", menu_selector(LobbyLayer::onRooms),
                {w * 0.32f, 48.f}, "GJ_button_05.png", 0.48f));
            menu->addChild(button("PLAZA", menu_selector(LobbyLayer::onOpenPlaza),
                {w * 0.68f, 48.f}, "GJ_button_01.png", 0.48f));
            m_page->addChild(menu, 5);
        }

        void onRooms(CCObject*) { showRooms(); }

        void onDashboard(CCObject*) { showHome(); }

        void onOpenPlaza(CCObject*) { showPlaza(); }
        void onRanked(CCObject*) { showRanked(); }
        void onProfile(CCObject*) { showProfile(); }
        void onPublicRooms(CCObject*) { showPublicRooms(); }
        void onGroupRooms(CCObject*) { showGroupRooms(); }
        void onLive(CCObject*) { showLive(); }

        void onWorkshop(CCObject*) {
            showInfo("VERSUS WORKSHOP", "COMMUNITY CONTENT",
                "WORKSHOP NOT AVAILABLE YET",
                "This button now opens the Workshop status page.",
                "A real catalog, safe uploads, moderation, and hosting are still needed.");
        }

        void onVoice(CCObject*) {
            s_screen = Screen::Voice;
            basePage("VOICE CHAT", "VOICE SETUP AND SAFETY");
            auto w = m_size.width;
            auto h = m_size.height;
            auto mid = h * 0.57f;
            addCard({w * 0.50f, mid}, {w * 0.86f, h * 0.34f}, {18,42,91,255});

            char const* state = !globedInstalled() ? "GLOBED IS REQUIRED FOR VOICE" :
                (!globedEnabled() ? "ENABLE GLOBED FIRST" :
                (!globedLoaded() ? "WAITING FOR GLOBED" :
                (!globedConnected() ? "CONNECT TO GLOBED" :
                    "VOICE IS PROVIDED BY GLOBED IN SHARED LEVELS")));
            label(state, 0.32f, {w * 0.50f, mid + h * 0.09f},
                globedReady() ? ccColor3B{115,255,170} : ccColor3B{255,195,115});
            label("Globed handles voice chat inside shared levels.", 0.27f,
                {w * 0.50f, mid + h * 0.025f}, {225,235,250});
            label("Set your push-to-talk and deafen keys in Globed settings.", 0.24f,
                {w * 0.50f, mid - h * 0.035f}, {190,215,245});
            label("Use mute / block / report controls when available.", 0.23f,
                {w * 0.50f, mid - h * 0.085f}, {170,205,240});

            auto* menu = CCMenu::create();
            menu->setPosition({0,0});
            menu->addChild(button("ROOM STATUS", menu_selector(LobbyLayer::onRooms),
                {w * 0.30f, 48.f}, "GJ_button_02.png", 0.43f));
            menu->addChild(button("PLAZA", menu_selector(LobbyLayer::onOpenPlaza),
                {w * 0.70f, 48.f}, "GJ_button_01.png", 0.43f));
            m_page->addChild(menu, 5);
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
            m_waitingForGlobed = false;
            s_screen = Screen::Info;
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
        void showQueueModes() {
            s_screen = Screen::Online;
            basePage("GLOBAL MATCHMAKING", "FIND A PLAYER FROM ANY GLOBED ROOM");
            auto w = m_size.width;
            auto h = m_size.height;
            addCard({w * 0.50f, h * 0.62f}, {w * 0.88f, h * 0.30f}, {16, 39, 75, 250});
            label("SELECT YOUR GAME MODE", 0.36f, {w * 0.50f, h * 0.72f}, {255,224,110});
            label("Casual queue  •  global pairing  •  ready check", 0.24f,
                {w * 0.50f, h * 0.64f}, {190,215,245});
            auto* menu = CCMenu::create();
            menu->setPosition({0,0});
            menu->addChild(button("RACE", menu_selector(LobbyLayer::onQueueRace),
                {w * 0.28f, h * 0.52f}, "GJ_button_05.png", 0.50f));
            menu->addChild(button("SURVIVAL", menu_selector(LobbyLayer::onQueueSurvival),
                {w * 0.72f, h * 0.52f}, "GJ_button_05.png", 0.50f));
            menu->addChild(button("DUAL", menu_selector(LobbyLayer::onQueueDual),
                {w * 0.28f, h * 0.40f}, "GJ_button_04.png", 0.50f));
            menu->addChild(button("PLATFORMER", menu_selector(LobbyLayer::onQueuePlatformer),
                {w * 0.72f, h * 0.40f}, "GJ_button_04.png", 0.43f));
            menu->addChild(button("BACK", menu_selector(LobbyLayer::onBack),
                {w * 0.50f, 42.f}, "GJ_button_01.png", 0.48f));
            m_page->addChild(menu, 5);
        }

        void showQueueStatus(char const* title, char const* detail, char const* extra) {
            s_screen = Screen::Online;
            basePage("GLOBAL MATCHMAKING", "CROSS-ROOM QUEUE");
            auto mid = m_size.height / 2.f;
            addCard({m_size.width/2.f, mid+8.f}, {m_size.width*0.86f, 154.f}, {18, 42, 91, 255});
            label(title, 0.40f, {m_size.width/2.f, mid+50.f}, {255,220,105});
            label(detail, 0.27f, {m_size.width/2.f, mid+12.f}, {225,235,250});
            label(extra, 0.23f, {m_size.width/2.f, mid-24.f}, {165,205,245});
            auto* menu = CCMenu::create();
            menu->setPosition({0,0});
            menu->addChild(button("CANCEL QUEUE", menu_selector(LobbyLayer::onCancelQueue),
                {m_size.width/2.f, 48.f}, "GJ_button_01.png", 0.55f));
            m_page->addChild(menu, 5);
        }

        std::string matchmakingEndpoint() {
            auto endpoint = Mod::get()->getSettingValue<std::string>("matchmaking-url");
            while (!endpoint.empty() && endpoint.back() == '/') endpoint.pop_back();
            return endpoint;
        }

        std::string matchmakingAnonKey() {
            return Mod::get()->getSettingValue<std::string>("matchmaking-anon-key");
        }

        void queueForMode(char const* mode) {
            if (!globedLoaded() || !globedApiAvailable() || !globedConnected()) {
                showInfo("GLOBAL MATCHMAKING", "GLOBED REQUIRED", "NOT CONNECTED",
                    "Connect to Globed before entering the global queue.",
                    "Your current private room does not limit who can be matched.");
                return;
            }
            auto endpoint = matchmakingEndpoint();
            if (endpoint.empty() || matchmakingAnonKey().empty()) {
                showInfo("GLOBAL MATCHMAKING", "SERVER NOT CONFIGURED", "SUPABASE SETTINGS MISSING",
                    "Set the Supabase function URL and project anon/publishable key in Geode settings.",
                    "Never enter the Supabase service-role key in the mod.");
                return;
            }
            m_queueMode = mode;
            m_queueTicket.clear();
            m_matchReady = false;
            m_queueState = "joining";
            m_queueRequestPending = true;
            showQueueStatus("JOINING GLOBAL QUEUE", "Contacting matchmaking service...", mode);
            auto req = web::WebRequest();
            req.header("Content-Type", "application/json");
            req.header("apikey", matchmakingAnonKey());
            req.timeout(std::chrono::seconds(12));
            req.bodyString(fmt::format(R"({{"mode":"{}","region":"global","ranked":false}})", mode));
            m_queueTask.spawn(req.post(endpoint + "/queue/join"),
                [this](web::WebResponse res) {
                    m_queueRequestPending = false;
                    if (!res.ok()) {
                        auto message = res.string().unwrapOr("No response body");
                        showInfo("GLOBAL MATCHMAKING", "QUEUE REQUEST FAILED",
                            fmt::format("HTTP {}", res.code()).c_str(), message.c_str(),
                            "Check the backend deployment and try again.");
                        return;
                    }
                    auto parsed = res.json();
                    if (!parsed) {
                        showInfo("GLOBAL MATCHMAKING", "INVALID SERVER RESPONSE", "BAD JSON",
                            "The matchmaking server returned unreadable data.",
                            "The queue was not started.");
                        return;
                    }
                    auto data = parsed.unwrap();
                    m_queueTicket = data["ticket"].asString().unwrapOr("");
                    auto state = data["state"].asString().unwrapOr("unknown");
                    if (m_queueTicket.empty()) {
                        showInfo("GLOBAL MATCHMAKING", "QUEUE ERROR", "NO TICKET",
                            "The service did not return a queue ticket.",
                            "Try again after checking the backend logs.");
                        return;
                    }
                    m_queueState = state;
                    m_queuePollTimer = 2.f;
                    if (state == "matched") {
                        confirmGlobalMatch();
                    } else {
                        showQueueStatus("SEARCHING FOR PLAYER",
                            "You're in the shared global queue.", m_queueMode.c_str());
                    }
                }
            );
        }

        void pollGlobalQueue() {
            auto endpoint = matchmakingEndpoint();
            if (endpoint.empty() || m_queueTicket.empty() || m_queueRequestPending) return;
            m_queueRequestPending = true;
            auto req = web::WebRequest();
            req.header("apikey", matchmakingAnonKey());
            req.header("Authorization", "Bearer " + m_queueTicket);
            req.timeout(std::chrono::seconds(10));
            m_queueTask.spawn(req.get(endpoint + "/queue/status"),
                [this](web::WebResponse res) {
                    m_queueRequestPending = false;
                    if (!res.ok()) {
                        if (res.code() == 404) {
                            m_queueTicket.clear();
                            showInfo("GLOBAL MATCHMAKING", "QUEUE EXPIRED", "SEARCH ENDED",
                                "Your queue ticket expired or the match was cleared.",
                                "Join the global queue again to search.");
                        }
                        return;
                    }
                    auto parsed = res.json();
                    if (!parsed) return;
                    auto data = parsed.unwrap();
                    auto state = data["state"].asString().unwrapOr("unknown");
                    if (state == "ready-check") {
                        m_queueState = state;
                        confirmGlobalMatch();
                    } else if (state == "ready") {
                        m_queueState = state;
                        m_matchReady = true;
                        showInfo("GLOBAL MATCHMAKING", "MATCH FOUND", "BOTH PLAYERS READY",
                            "The global service paired you and both players accepted.",
                            "Globed room/session handoff is the next integration step.");
                        m_queuePollTimer = 99999.f;
                    } else if (state == "cancelled" || state == "not_found") {
                        m_queueTicket.clear();
                        showInfo("GLOBAL MATCHMAKING", "MATCH CANCELLED", "QUEUE CLOSED",
                            "The other player left or the match expired.",
                            "Return to the plaza and search again.");
                    }
                }
            );
        }

        void confirmGlobalMatch() {
            if (m_queueTicket.empty() || m_queueRequestPending || m_queueState == "ready") return;
            auto endpoint = matchmakingEndpoint();
            if (endpoint.empty()) return;
            m_queueRequestPending = true;
            auto req = web::WebRequest();
            req.header("Authorization", "Bearer " + m_queueTicket);
            req.timeout(std::chrono::seconds(10));
            m_queueTask.spawn(req.post(endpoint + "/match/ready"),
                [this](web::WebResponse res) {
                    m_queueRequestPending = false;
                    if (!res.ok()) return;
                    auto parsed = res.json();
                    if (!parsed) return;
                    auto data = parsed.unwrap();
                    auto state = data["state"].asString().unwrapOr("ready-check");
                    m_queueState = state;
                    if (state == "ready") {
                        m_matchReady = true;
                        showInfo("GLOBAL MATCHMAKING", "MATCH FOUND", "BOTH PLAYERS READY",
                            "The global service paired you and both players accepted.",
                            "Globed room/session handoff is the next integration step.");
                        m_queuePollTimer = 99999.f;
                    } else {
                        showQueueStatus("MATCH FOUND",
                            "Waiting for the other player to accept...", m_queueMode.c_str());
                    }
                }
            );
        }

        void leaveGlobalQueue() {
            if (m_queueTicket.empty()) {
                showQueueModes();
                return;
            }
            auto endpoint = matchmakingEndpoint();
            auto ticket = m_queueTicket;
            m_queueTicket.clear();
            m_queueState.clear();
            m_matchReady = false;
            m_queuePollTimer = 0.f;
            if (!endpoint.empty()) {
                auto req = web::WebRequest();
                req.header("apikey", matchmakingAnonKey());
                req.header("Authorization", "Bearer " + ticket);
                req.timeout(std::chrono::seconds(8));
                m_queueTask.spawn(req.post(endpoint + "/queue/leave"), [](web::WebResponse) {});
            }
            showQueueModes();
        }

        void onQueueRace(CCObject*) { queueForMode("race"); }
        void onQueueSurvival(CCObject*) { queueForMode("survival"); }
        void onQueueDual(CCObject*) { queueForMode("dual"); }
        void onQueuePlatformer(CCObject*) { queueForMode("platformer"); }
        void onCancelQueue(CCObject*) { leaveGlobalQueue(); }

        void onOnline(CCObject*) {
            if (!globedInstalled() || !globedEnabled() || !globedLoaded()) {
                if (!globedInstalled()) {
                    showInfo("ONLINE VERSUS","GLOBED INTEGRATION","GLOBED NOT INSTALLED",
                        "Install and enable Globed to use online features.",
                        "Versus Mode can still be used without Globed.");
                } else if (!globedEnabled()) {
                    showInfo("ONLINE VERSUS","GLOBED INTEGRATION","GLOBED IS DISABLED",
                        "Enable Globed in the Geode mod list.","Then reopen Versus Mode.");
                } else showGlobedLoading();
                return;
            }
            if (!globedApiAvailable() || !globedConnected()) {
                showInfo("GLOBAL MATCHMAKING", "GLOBED NETWORK", "NOT CONNECTED",
                    "Connect to the Globed central server first.",
                    "The global queue works across rooms once connected.");
                return;
            }
            showQueueModes();
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
