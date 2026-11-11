#include "LobbyLayer.hpp"

namespace Versus {
    LobbyLayer::RankTier const LobbyLayer::kRankTiers[] = {
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

    size_t LobbyLayer::rankTierCount() {
        return sizeof(kRankTiers) / sizeof(kRankTiers[0]);
    }

    int LobbyLayer::rankPoints() {
        return std::max(0, Mod::get()->getSavedValue<int>("rank-points", 0));
    }

    size_t LobbyLayer::rankTierIndex(int points) {
        size_t tier = 0;
        for (size_t i = 0; i < rankTierCount(); ++i) {
            if (points >= kRankTiers[i].minimumPoints) tier = i;
        }
        return tier;
    }

    char const* LobbyLayer::rankName(int points) {
        return points <= 0 ? "UNRANKED" : kRankTiers[rankTierIndex(points)].name;
    }

    LobbyLayer* LobbyLayer::create() {
        auto* ret = new LobbyLayer();
        if (ret && ret->init()) { ret->autorelease(); return ret; }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    CCScene* LobbyLayer::scene() {
        auto* scene = CCScene::create();
        if (!scene) return nullptr;
        auto* layer = LobbyLayer::create();
        if (!layer) return nullptr;
        scene->addChild(layer);
        return scene;
    }

    bool LobbyLayer::init() {
        if (!CCLayer::init()) return false;
        this->setKeypadEnabled(true);
        this->scheduleUpdate();
        this->showHome();
        return true;
    }

    void LobbyLayer::keyBackClicked() {
        if (s_screen != Screen::Home) showHome();
        else CCDirector::sharedDirector()->popScene();
    }

    void LobbyLayer::update(float dt) {
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

    void LobbyLayer::clearPage() {
        if (m_page) {
            m_page->removeFromParentAndCleanup(true);
            m_page = nullptr;
        }
    }

    // Geometry Dash Level Complete-inspired presentation:
    // darkened backdrop, warm gold frame, bright completion header,
    // and the game's own ButtonSprite artwork for every action.
    void LobbyLayer::basePage(char const* title, char const* subtitle) {
        clearPage();
        m_size = CCDirector::sharedDirector()->getWinSize();
        m_page = CCLayer::create();
        this->addChild(m_page, 1);

        auto* shade = CCLayerColor::create({13, 17, 35, 250});
        m_page->addChild(shade, -5);
        auto* topGlow = CCLayerColor::create({73, 47, 22, 255}, m_size.width, 5.f);
        topGlow->setPosition({0.f, m_size.height - 5.f});
        m_page->addChild(topGlow, -4);
        auto* bottomGlow = CCLayerColor::create({73, 47, 22, 255}, m_size.width, 4.f);
        bottomGlow->setPosition({0.f, 0.f});
        m_page->addChild(bottomGlow, -4);

        // A gold-trimmed title plate echoes the celebratory completion panel.
        auto* titlePlate = CCLayerColor::create({36, 48, 75, 255},
            m_size.width * 0.92f, 54.f);
        titlePlate->setPosition({m_size.width * 0.04f, m_size.height - 88.f});
        m_page->addChild(titlePlate, -1);
        auto* titleTop = CCLayerColor::create({255, 204, 83, 255},
            m_size.width * 0.92f, 3.f);
        titleTop->setPosition({0.f, 51.f});
        titlePlate->addChild(titleTop, 1);
        auto* titleBottom = CCLayerColor::create({173, 111, 40, 255},
            m_size.width * 0.92f, 2.f);
        titlePlate->addChild(titleBottom, 1);

        VersusUI::label(m_page, title, 0.72f, {m_size.width/2, m_size.height - 55.f}, {255,224,125});
        VersusUI::label(m_page, subtitle, 0.29f, {m_size.width/2, m_size.height - 75.f}, {224,232,248});
    }

    void LobbyLayer::showHome() {
        s_screen = Screen::Home;
        m_waitingForGlobed = false;
        basePage("VERSUS MODE", "COMPETE  •  CONNECT  •  CLIMB");
        auto w = m_size.width;
        auto h = m_size.height;

        auto* status = CCLayerColor::create({12, 25, 52, 248}, w * 0.88f, 42.f);
        status->setPosition({w * 0.06f, h - 112.f});
        m_page->addChild(status, 0);
        VersusUI::label(m_page, "NETWORK", 0.22f, {w * 0.19f, h - 91.f}, {125,190,240});
        VersusUI::label(m_page, globedStatusLine(), 0.27f, {w * 0.63f, h - 91.f},
            globedReady() ? ccColor3B{115,255,170} : ccColor3B{255,195,115});

        auto* hero = CCLayerColor::create({15, 38, 76, 248}, w * 0.86f, h * 0.39f);
        hero->setPosition({w * 0.07f, h * 0.34f});
        m_page->addChild(hero, 0);
        auto* heroLine = CCLayerColor::create({62,175,245,255}, w * 0.86f, 3.f);
        heroLine->setPosition({w * 0.07f, h * 0.34f + h * 0.39f - 3.f});
        m_page->addChild(heroLine, 1);
        VersusUI::label(m_page, "YOUR NEXT MATCH STARTS HERE", 0.39f,
            {w * 0.50f, h * 0.67f}, {255,224,110});
        VersusUI::label(m_page, "A new home for Geometry Dash competition", 0.27f,
            {w * 0.50f, h * 0.60f}, {195,218,245});
        VersusUI::label(m_page, fmt::format("{}  •  {} RP", rankName(rankPoints()), rankPoints()).c_str(),
            0.30f, {w * 0.50f, h * 0.51f}, {145,220,255});

        auto* menu = CCMenu::create();
        menu->setPosition({0,0});
        menu->addChild(VersusUI::button("OPEN PLAZA", this, menu_selector(LobbyLayer::onOpenPlaza),
            {w * 0.50f, h * 0.43f}, "GJ_button_05.png", 0.66f));
        menu->addChild(VersusUI::button("RANKS", this, menu_selector(LobbyLayer::onRanked),
            {w * 0.27f, h * 0.25f}, "GJ_button_02.png", 0.48f));
        menu->addChild(VersusUI::button("ROOMS", this, menu_selector(LobbyLayer::onPublicRooms),
            {w * 0.50f, h * 0.25f}, "GJ_button_04.png", 0.48f));
        menu->addChild(VersusUI::button("PROFILE", this, menu_selector(LobbyLayer::onProfile),
            {w * 0.73f, h * 0.25f}, "GJ_button_03.png", 0.48f));
        menu->addChild(VersusUI::button("EXIT", this, menu_selector(LobbyLayer::onClose),
            {w * 0.50f, 42.f}, "GJ_button_01.png", 0.45f));
        m_page->addChild(menu, 5);
        VersusUI::label(m_page, "VERSUS MODE  •  GLOBED 2.3.0+", 0.20f, {w/2.f, 12.f}, {125,155,195});
    }

    void LobbyLayer::showPlaza() {
        s_screen = Screen::Plaza;
        basePage("PLAZA", "THE VERSUS COMMUNITY HUB");
        auto w = m_size.width;
        auto h = m_size.height;

        VersusUI::addCard(m_page, {w * 0.50f, h * 0.72f}, {w * 0.88f, h * 0.25f}, {16, 39, 75, 250});
        VersusUI::label(m_page, "WELCOME TO THE PLAZA", 0.42f,
            {w * 0.50f, h * 0.80f}, {255,224,110});
        VersusUI::label(m_page, fmt::format("{}  •  {} RANK POINTS", rankName(rankPoints()), rankPoints()).c_str(),
            0.28f, {w * 0.50f, h * 0.72f}, {145,220,255});
        VersusUI::label(m_page, fmt::format("GLOBED {}", globedStatusLine()).c_str(), 0.23f,
            {w * 0.50f, h * 0.65f},
            globedReady() ? ccColor3B{115,255,170} : ccColor3B{255,195,115});

        auto* menu = CCMenu::create();
        menu->setPosition({0,0});
        menu->addChild(VersusUI::button("FIND MATCH", this, menu_selector(LobbyLayer::onOnline),
            {w * 0.50f, h * 0.53f}, "GJ_button_05.png", 0.56f));
        menu->addChild(VersusUI::button("RANKED", this, menu_selector(LobbyLayer::onRanked),
            {w * 0.27f, h * 0.41f}, "GJ_button_02.png", 0.44f));
        menu->addChild(VersusUI::button("PUBLIC ROOMS", this, menu_selector(LobbyLayer::onPublicRooms),
            {w * 0.73f, h * 0.41f}, "GJ_button_04.png", 0.42f));
        menu->addChild(VersusUI::button("GROUP ROOMS", this, menu_selector(LobbyLayer::onGroupRooms),
            {w * 0.27f, h * 0.30f}, "GJ_button_01.png", 0.42f));
        menu->addChild(VersusUI::button("LIVE", this, menu_selector(LobbyLayer::onLive),
            {w * 0.73f, h * 0.30f}, "GJ_button_03.png", 0.44f));
        menu->addChild(VersusUI::button("PROFILE", this, menu_selector(LobbyLayer::onProfile),
            {w * 0.50f, h * 0.19f}, "GJ_button_02.png", 0.42f));
        menu->addChild(VersusUI::button("HOME", this, menu_selector(LobbyLayer::onDashboard),
            {w * 0.50f, 38.f}, "GJ_button_01.png", 0.43f));
        m_page->addChild(menu, 5);
    }

    void LobbyLayer::showRanked() {
        s_screen = Screen::Ranked;
        basePage("RANKED", "GEOMETRY DASH DIFFICULTY LADDER");
        auto w = m_size.width;
        auto h = m_size.height;
        int points = rankPoints();
        VersusUI::addCard(m_page, {w * 0.50f, h * 0.83f}, {w * 0.88f, h * 0.16f}, {16, 39, 75, 250});
        VersusUI::label(m_page, fmt::format("{}  •  {} RP", rankName(points), points).c_str(),
            0.34f, {w * 0.50f, h * 0.85f}, {255,224,110});
        VersusUI::label(m_page, "Local ladder preview — online rating is not synced yet", 0.21f,
            {w * 0.50f, h * 0.80f}, {185,210,245});

        auto count = rankTierCount();
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
            VersusUI::label(m_page, tier.name, 0.22f, {x + w * 0.035f, y + 2.f},
                active ? ccColor3B{255,255,255} : tier.color);
            VersusUI::label(m_page, fmt::format("{} RP", tier.minimumPoints).c_str(),
                0.16f, {x + w * 0.035f, y - h * 0.022f}, {185,205,230});
        }

        auto* menu = CCMenu::create();
        menu->setPosition({0,0});
        menu->addChild(VersusUI::button("PLAZA", this, menu_selector(LobbyLayer::onOpenPlaza),
            {w * 0.50f, 35.f}, "GJ_button_01.png", 0.43f));
        m_page->addChild(menu, 5);
    }

    void LobbyLayer::showProfile() {
        s_screen = Screen::Profile;
        basePage("PLAYER PROFILE", "YOUR VERSUS IDENTITY");
        auto w = m_size.width;
        auto h = m_size.height;
        VersusUI::addCard(m_page, {w * 0.50f, h * 0.60f}, {w * 0.86f, h * 0.42f}, {18,42,91,255});
        VersusUI::label(m_page, "LOCAL PLAYER", 0.38f, {w * 0.50f, h * 0.75f}, {255,224,110});
        VersusUI::label(m_page, fmt::format("RANK  {}", rankName(rankPoints())).c_str(),
            0.32f, {w * 0.50f, h * 0.65f}, {145,220,255});
        VersusUI::label(m_page, fmt::format("{} RANK POINTS", rankPoints()).c_str(),
            0.27f, {w * 0.50f, h * 0.56f}, {225,235,250});
        VersusUI::label(m_page, "Online profiles, records and statistics need the Versus server.", 0.22f,
            {w * 0.50f, h * 0.46f}, {180,205,235});
        auto* menu = CCMenu::create();
        menu->setPosition({0,0});
        menu->addChild(VersusUI::button("RANKS", this, menu_selector(LobbyLayer::onRanked),
            {w * 0.30f, 38.f}, "GJ_button_02.png", 0.43f));
        menu->addChild(VersusUI::button("PLAZA", this, menu_selector(LobbyLayer::onOpenPlaza),
            {w * 0.70f, 38.f}, "GJ_button_01.png", 0.43f));
        m_page->addChild(menu, 5);
    }

    void LobbyLayer::showGlobedLoading() {
        s_screen = Screen::Loading;
        m_waitingForGlobed = true;
        m_globedWait = 0.f;
        m_loadingPulse = 0.f;
        basePage("ONLINE VERSUS", "GLOBED CONNECTION");
        auto mid = m_size.height / 2.f;
        VersusUI::addCard(m_page, {m_size.width/2.f, mid+15.f}, {m_size.width*0.82f, 142.f}, {18, 42, 91, 255});
        VersusUI::label(m_page, "PLEASE WAIT", 0.34f, {m_size.width/2.f, mid+58.f}, {255,220,105});
        m_loadingLabel = VersusUI::label(m_page, "CONNECTING TO GLOBED", 0.38f,
            {m_size.width/2.f, mid+18.f}, {255,255,255});
        auto* track = CCLayerColor::create({10,18,44,255}, m_size.width*0.58f, 9.f);
        track->setPosition({m_size.width*0.21f, mid-20.f});
        m_page->addChild(track, 2);
        m_loadingFill = CCLayerColor::create({80,225,255,255}, 2.f, 9.f);
        m_loadingFill->setPosition({0.f,0.f});
        track->addChild(m_loadingFill, 3);
        VersusUI::label(m_page, "CHECKING MOD API  •  12 SECOND TIMEOUT", 0.22f,
            {m_size.width/2.f, mid-48.f}, {165,195,235});
        auto* menu = CCMenu::create();
        menu->setPosition({0,0});
        menu->addChild(VersusUI::button("CANCEL", this, menu_selector(LobbyLayer::onBack),
            {m_size.width/2.f, 48.f}, "GJ_button_01.png", 0.62f));
        m_page->addChild(menu, 5);
    }

    void LobbyLayer::showInfo(char const* title, char const* subtitle, char const* status,
        char const* detail, char const* extra) {
        m_waitingForGlobed = false;
        s_screen = Screen::Info;
        basePage(title, subtitle);
        auto mid = m_size.height/2;
        VersusUI::addCard(m_page, {m_size.width/2,mid},{m_size.width*0.84f,148.f},{18,38,72,250});
        VersusUI::label(m_page, status, 0.45f, {m_size.width/2,mid+48.f},{255,220,115});
        VersusUI::label(m_page, detail, 0.29f, {m_size.width/2,mid+15.f},{220,230,245});
        VersusUI::label(m_page, extra, 0.27f, {m_size.width/2,mid-19.f},{160,205,255});
        auto* menu=CCMenu::create(); menu->setPosition({0,0});
        menu->addChild(VersusUI::button("BACK", this, menu_selector(LobbyLayer::onBack),
            {m_size.width/2,48.f}, "GJ_button_01.png", 0.62f));
        m_page->addChild(menu,5);
    }

    void LobbyLayer::onDashboard(CCObject*) { showHome(); }
    void LobbyLayer::onOpenPlaza(CCObject*) { showPlaza(); }
    void LobbyLayer::onRanked(CCObject*) { showRanked(); }
    void LobbyLayer::onProfile(CCObject*) { showProfile(); }
    void LobbyLayer::onBack(CCObject*) { showHome(); }
    void LobbyLayer::onClose(CCObject*) { CCDirector::sharedDirector()->popScene(); }
}
