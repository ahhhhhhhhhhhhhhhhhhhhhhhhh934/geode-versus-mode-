#include "LobbyLayer.hpp"
#include "../core/GameModes.hpp"
#include "../core/Matchmaking.hpp"
#include "../core/Protocol.hpp"

// Pages that describe the room / network state. These screens never invent
// data: every line comes from Globed's Soft Link API or states plainly that
// a feature is not available yet.
namespace Versus {
    void LobbyLayer::showPublicRooms() {
        s_screen = Screen::PublicRooms;
        showInfo("PUBLIC ROOMS", "ROOM BROWSER",
            "ROOM DIRECTORY NOT CONNECTED",
            "Globed can show its own rooms, but Versus does not have a room-list service yet.",
            "This page will not invent rooms or show fake player counts.");
    }

    void LobbyLayer::showGroupRooms() {
        s_screen = Screen::GroupRooms;
        basePage("PRIVATE VERSUS", "PLAY WITH YOUR FRIENDS");
        auto w = m_size.width;
        auto h = m_size.height;
        auto mid = h * 0.56f;
        VersusUI::addCard(m_page, {w * 0.50f, mid}, {w * 0.86f, h * 0.34f}, {42, 58, 91, 255});

        bool connected = globedLoaded() && globedApiAvailable() && globedConnected();
        bool inRoom = connected && globedInRoom();
        if (inRoom) {
            VersusUI::label(m_page, "GLOBED ROOM DETECTED", 0.34f,
                {w * 0.50f, mid + h * 0.115f}, {145,255,175});
            VersusUI::label(m_page, fmt::format("ROOM ID  {}", globedRoomId()).c_str(), 0.32f,
                {w * 0.50f, mid + h * 0.035f}, {255,225,135});
            VersusUI::label(m_page, "Open the Versus lobby to find players in this room.", 0.23f,
                {w * 0.50f, mid - h * 0.025f}, {235,240,250});
            VersusUI::label(m_page, "Make the Globed room private or invite-only there.", 0.20f,
                {w * 0.50f, mid - h * 0.095f}, {195,215,245});
        } else {
            VersusUI::label(m_page, connected ? "NO ROOM JOINED" : "CONNECT TO GLOBED FIRST", 0.37f,
                {w * 0.50f, mid + h * 0.105f}, {255,220,125});
            VersusUI::label(m_page, "Create or join a private room in Globed.", 0.27f,
                {w * 0.50f, mid + h * 0.035f}, {235,240,250});
            VersusUI::label(m_page, "Then return here to set up a Versus match.", 0.24f,
                {w * 0.50f, mid - h * 0.035f}, {195,215,245});
            VersusUI::label(m_page, "Room creation is handled by Globed, not a fake local room.", 0.20f,
                {w * 0.50f, mid - h * 0.095f}, {175,195,225});
        }

        auto* menu = CCMenu::create();
        menu->setPosition({0,0});
        if (inRoom) {
            menu->addChild(VersusUI::button("OPEN VERSUS LOBBY", this, menu_selector(LobbyLayer::onJoinLobby),
                {w * 0.50f, 92.f}, "GJ_button_05.png", 0.50f));
        }
        menu->addChild(VersusUI::button("REFRESH ROOM", this, menu_selector(LobbyLayer::onGroupRooms),
            {w * 0.28f, 48.f}, "GJ_button_05.png", 0.44f));
        menu->addChild(VersusUI::button("ROOM STATUS", this, menu_selector(LobbyLayer::onRooms),
            {w * 0.72f, 48.f}, "GJ_button_04.png", 0.44f));
        menu->addChild(VersusUI::button("PLAZA", this, menu_selector(LobbyLayer::onOpenPlaza),
            {w * 0.50f, 20.f}, "GJ_button_01.png", 0.36f));
        m_page->addChild(menu, 5);
    }

    void LobbyLayer::showLive() {
        s_screen = Screen::Live;
        showInfo("LIVE", "WATCH ACTIVE MATCHES",
            "LIVE FEED NOT CONNECTED",
            "Spectating needs a Versus match feed and spectator protocol.",
            "No live matches can be listed until that service is built.");
    }

    void LobbyLayer::showRooms() {
        s_screen = Screen::Rooms;
        basePage("ROOM STATUS", "GLOBED CONNECTION INFORMATION");
        auto w = m_size.width;
        auto h = m_size.height;
        auto mid = h * 0.57f;
        VersusUI::addCard(m_page, {w * 0.50f, mid}, {w * 0.86f, h * 0.31f}, {18,42,91,255});

        char const* state = !globedInstalled() ? "GLOBED NOT INSTALLED" :
            (!globedEnabled() ? "GLOBED DISABLED" :
            (!globedLoaded() ? "GLOBED NOT LOADED" :
            (!globedConnected() ? "NOT CONNECTED" :
            (globedInRoom() ? "INSIDE A ROOM" : "CONNECTED  •  NO ROOM"))));
        VersusUI::label(m_page, state, 0.37f, {w * 0.50f, mid + h * 0.075f},
            globedReady() ? ccColor3B{115,255,170} : ccColor3B{255,195,115});

        if (globedLoaded() && globedApiAvailable() && globedConnected() && globedInRoom()) {
            auto roomText = fmt::format("ROOM ID  {}", globedRoomId());
            VersusUI::label(m_page, roomText.c_str(), 0.30f, {w * 0.50f, mid + h * 0.005f}, {225,235,250});
            VersusUI::label(m_page, "ROOM DETECTED THROUGH GLOBED API", 0.24f,
                {w * 0.50f, mid - h * 0.055f}, {170,210,250});
        } else {
            VersusUI::label(m_page, "Join or create a room using Globed's own room menu.", 0.27f,
                {w * 0.50f, mid}, {220,230,245});
            VersusUI::label(m_page, "Versus matchmaking will hand off into this Globed room.", 0.23f,
                {w * 0.50f, mid - h * 0.055f}, {170,195,230});
        }

        auto* menu = CCMenu::create();
        menu->setPosition({0,0});
        menu->addChild(VersusUI::button("REFRESH", this, menu_selector(LobbyLayer::onRooms),
            {w * 0.32f, 48.f}, "GJ_button_05.png", 0.48f));
        menu->addChild(VersusUI::button("PLAZA", this, menu_selector(LobbyLayer::onOpenPlaza),
            {w * 0.68f, 48.f}, "GJ_button_01.png", 0.48f));
        m_page->addChild(menu, 5);
    }

    void LobbyLayer::onRooms(CCObject*) { showRooms(); }
    void LobbyLayer::onPublicRooms(CCObject*) { showPublicRooms(); }
    void LobbyLayer::onGroupRooms(CCObject*) { showGroupRooms(); }
    void LobbyLayer::onLive(CCObject*) { showLive(); }

    void LobbyLayer::showRoomLobby() {
        s_screen = Screen::Lobby;
        m_waitingForGlobed = false;
        auto w = m_size.width;
        auto h = m_size.height;

        // The lobby lives only while the Globed room does.
        if (!Versus::globedInRoom() || Versus::globedRoomId() != VersusLobby::roomId()) {
            VersusLobby::leave();
            showGroupRooms();
            return;
        }

        auto roomLine = fmt::format("GLOBED ROOM  {}", VersusLobby::roomId());
        basePage("VERSUS LOBBY", roomLine.c_str());
        VersusUI::label(m_page, fmt::format("MODE: {}", VersusModes::name(VersusLobby::mode())).c_str(),
            0.26f, {m_size.width * 0.50f, m_size.height - 108.f}, {145,220,255});

        // ---- match alerts (toasts) -------------------------------------------
        {
            auto live = VersusLobby::alerts();
            float ty = m_size.height - 132.f;
            for (auto it = live.rbegin(); it != live.rend() && ty > h * 0.80f; ++it) {
                VersusUI::label(m_page, it->text.c_str(), 0.19f,
                    {w * 0.50f, ty}, {255, 205, 130});
                ty -= 20.f;
            }
        }

        // ---- connection status -------------------------------------------------
        {
            char const* state = Versus::globedConnectionStateName();
            uint32_t ping = Versus::globedApiAvailable() ? globed::api::net::getCentralPingMs() : 0;
            bool online = Versus::globedConnected();
            std::string line = online
                ? fmt::format("{}  •  {} MS", state, ping)
                : std::string(state);
            if (!online) line += "  •  PACKETS PAUSED";
            VersusUI::label(m_page, line.c_str(), 0.20f,
                {w * 0.50f, m_size.height - 158.f},
                online ? ccColor3B{115,255,170} : ccColor3B{255,140,140});
        }

        int host = VersusLobby::hostAccountId();
        bool hostKnown = host != 0;

        // ---- player list -----------------------------------------------------
        auto players = VersusLobby::players();
        int self = VersusLobby::localAccountId();
        size_t rows = players.size() + 1; // + self
        float listTop = h * 0.78f;
        float rowHeight = h * 0.062f;
        float cardHeight = static_cast<float>(rows) * rowHeight + 26.f;
        if (cardHeight > h * 0.52f) cardHeight = h * 0.52f;
        VersusUI::addCard(m_page, {w * 0.50f, listTop - cardHeight / 2.f + 12.f},
            {w * 0.88f, cardHeight}, {18, 42, 91, 255});

        VersusUI::label(m_page, "PLAYERS", 0.24f, {w * 0.14f, listTop}, {125,190,240});
        VersusUI::label(m_page, "STATUS", 0.24f, {w * 0.86f, listTop}, {125,190,240});

        // Sort by account id so the list is stable between refreshes.
        std::sort(players.begin(), players.end(),
            [](auto const& a, auto const& b) { return a.accountId < b.accountId; });

        float y = listTop - rowHeight;
        bool hostInList = false;
        auto drawRow = [&](int accountId, char const* suffix, bool isHost, bool ready, bool versusActive) {
            if (isHost) hostInList = true;
            std::string name = fmt::format("PLAYER {}{}", accountId, suffix);
            VersusUI::label(m_page, name.c_str(), 0.24f, {w * 0.22f, y}, {235,240,250});
            char const* status = isHost ? "HOST" :
                (!versusActive ? "IN ROOM" : (ready ? "READY" : "NOT READY"));
            VersusUI::label(m_page, status, 0.24f, {w * 0.82f, y},
                isHost ? ccColor3B{255,210,110} :
                (!versusActive ? ccColor3B{150,190,235} :
                    (ready ? ccColor3B{115,255,170} : ccColor3B{185,205,230})));
            y -= rowHeight;
        };

        // Always draw the local player first so "you" are visible even
        // before anyone else has joined.
        bool selfListed = false;
        for (auto const& p : players) {
            if (p.accountId == self) {
                drawRow(p.accountId, "  (YOU)", p.accountId == host, p.ready, true);
                selfListed = true;
            } else {
                drawRow(p.accountId, "", p.accountId == host, p.ready, p.versusActive);
            }
        }
        if (!selfListed) drawRow(self, "  (YOU)", self == host, VersusLobby::ready(), true);

        if (hostKnown && !hostInList && self != host) {
            VersusUI::label(m_page, fmt::format("HOST PLAYER {} NOT IN LOBBY", host).c_str(),
                0.20f, {w * 0.50f, y + rowHeight * 0.5f}, {255,195,115});
        }

        // ---- match state banner ----------------------------------------------
        if (VersusLobby::phase() == VersusLobby::MatchPhase::Countdown) {
            int seconds = static_cast<int>(std::ceil(VersusLobby::s_countdown));
            if (seconds < 0) seconds = 0;
            VersusUI::label(m_page, fmt::format("MATCH STARTING IN {}", seconds).c_str(),
                0.40f, {w * 0.50f, h * 0.33f}, {255,120,120});
        } else if (VersusLobby::phase() == VersusLobby::MatchPhase::InGame) {
            VersusUI::label(m_page, "MATCH IN PROGRESS — PLAY THE LEVEL",
                0.28f, {w * 0.50f, h * 0.33f}, {255,160,120});
        } else {
            size_t readyCount = 0;
            for (auto const& p : players) if (p.ready) ++readyCount;
            if (VersusLobby::ready()) ++readyCount;
            VersusUI::label(m_page,
                fmt::format("{} / {} PLAYERS READY", readyCount, players.size() + 1).c_str(),
                0.28f, {w * 0.50f, h * 0.33f}, {145,220,255});
        }

        // ---- controls ----------------------------------------------------------
        auto* menu = CCMenu::create();
        menu->setPosition({0,0});
        menu->addChild(VersusUI::button(VersusLobby::ready() ? "UNREADY" : "READY",
            this, menu_selector(LobbyLayer::onToggleReady),
            {w * 0.27f, h * 0.155f}, "GJ_button_04.png", 0.44f));
        menu->addChild(VersusUI::button("CHOOSE MODE", this, menu_selector(LobbyLayer::onOpenModeSelect),
            {w * 0.73f, h * 0.155f}, "GJ_button_02.png", 0.44f));
        menu->addChild(VersusUI::button("MATCHMAKING", this, menu_selector(LobbyLayer::onOpenMatchmaking),
            {w * 0.50f, h * 0.105f}, "GJ_button_03.png", 0.42f));
        bool matchActive = VersusLobby::phase() == VersusLobby::MatchPhase::Countdown ||
            VersusLobby::phase() == VersusLobby::MatchPhase::InGame;
        if (VersusLobby::isHost() && matchActive) {
            menu->addChild(VersusUI::button("CANCEL MATCH", this, menu_selector(LobbyLayer::onHostCancelMatch),
                {w * 0.50f, h * 0.24f}, "GJ_button_06.png", 0.40f));
        } else if (VersusLobby::isHost()) {
            menu->addChild(VersusUI::button("MATCH SETUP", this, menu_selector(LobbyLayer::onOpenMatchSetup),
                {w * 0.50f, h * 0.24f}, "GJ_button_05.png", 0.40f));
        }
        menu->addChild(VersusUI::button("LEAVE LOBBY", this, menu_selector(LobbyLayer::onLeaveLobby),
            {w * 0.73f, h * 0.055f}, "GJ_button_01.png", 0.36f));
        menu->addChild(VersusUI::button("PLAZA", this, menu_selector(LobbyLayer::onOpenPlaza),
            {w * 0.50f, 30.f}, "GJ_button_01.png", 0.36f));
        m_page->addChild(menu, 5);

        if (hostKnown && self == host) {
            VersusUI::label(m_page, "YOU ARE THE HOST",
                0.19f, {w * 0.50f, 12.f}, {255,215,130});
        } else {
            VersusUI::label(m_page, "WAIT FOR THE HOST TO LAUNCH THE MATCH",
                0.19f, {w * 0.50f, 12.f}, {150,180,215});
        }
    }

    void LobbyLayer::showMatchmaking() {
        s_screen = Screen::Matchmaking;
        basePage("MATCHMAKING", "ROOM-SCOPED QUEUE PREFERENCES");
        auto w = m_size.width;
        auto h = m_size.height;

        // ---- queue type toggle -------------------------------------------------
        bool ranked = VersusLobby::queueType() == 1;
        VersusUI::addCard(m_page, {w * 0.50f, h * 0.72f}, {w * 0.88f, h * 0.14f}, {18, 42, 91, 255});
        VersusUI::label(m_page, ranked ? "QUEUE: RANKED" : "QUEUE: CASUAL", 0.30f,
            {w * 0.50f, h * 0.755f}, ranked ? ccColor3B{255,224,110} : ccColor3B{145,220,255});
        VersusUI::label(m_page, ranked
            ? "Ranked matches move your local rank points." :
            "Casual matches record nothing.", 0.19f,
            {w * 0.50f, h * 0.71f}, {195,215,245});
        auto* menu = CCMenu::create();
        menu->setPosition({0,0});
        menu->addChild(VersusUI::button("CASUAL", this, menu_selector(LobbyLayer::onQueueCasual),
            {w * 0.27f, h * 0.665f}, "GJ_button_04.png", 0.42f));
        menu->addChild(VersusUI::button("RANKED", this, menu_selector(LobbyLayer::onQueueRanked),
            {w * 0.73f, h * 0.665f}, "GJ_button_02.png", 0.42f));

        // ---- difficulty band ----------------------------------------------------
        VersusUI::label(m_page, "DIFFICULTY BAND", 0.24f, {w * 0.50f, h * 0.585f}, {125,190,240});
        uint8_t tier = VersusLobby::difficultyTier();
        char const* tierName = "ANY";
        if (tier > 0 && tier <= VersusModes::kDifficultyCount) {
            tierName = VersusModes::kDifficulties[tier - 1].name;
        }
        VersusUI::label(m_page, tierName, 0.36f, {w * 0.50f, h * 0.525f}, {255,225,135});
        menu->addChild(VersusUI::button("<", this, menu_selector(LobbyLayer::onTierPrev),
            {w * 0.27f, h * 0.525f}, "GJ_button_01.png", 0.46f));
        menu->addChild(VersusUI::button(">", this, menu_selector(LobbyLayer::onTierNext),
            {w * 0.73f, h * 0.525f}, "GJ_button_01.png", 0.46f));
        m_page->addChild(menu, 5);

        // ---- room preferences and suggestions -----------------------------------
        VersusUI::addCard(m_page, {w * 0.50f, h * 0.30f}, {w * 0.88f, h * 0.30f}, {18, 42, 91, 255});
        float y = h * 0.40f;
        std::vector<VersusMatchmaking::QueuePlayer> queue;
        auto players = VersusLobby::players();
        for (auto const& p : players) {
            if (!p.versusActive) continue;
            queue.push_back({p.accountId, p.queueType, p.difficultyTier});
        }
        auto self = VersusLobby::localAccountId();
        queue.push_back({self, VersusLobby::queueType(), VersusLobby::difficultyTier()});

        auto suggestions = VersusMatchmaking::suggestPairings(queue);
        if (suggestions.empty()) {
            VersusUI::label(m_page, "NO SUGGESTED PAIRING YET", 0.24f,
                {w * 0.50f, y}, {255,205,140});
            VersusUI::label(m_page, "Pairings appear when two players want the same queue.", 0.18f,
                {w * 0.50f, y - 22.f}, {195,215,245});
        } else {
            for (auto const& sug : suggestions) {
                if (sug.a != self && sug.b != self) continue; // show your own pairing
                int other = sug.a == self ? sug.b : sug.a;
                VersusUI::label(m_page,
                    fmt::format("SUGGESTED MATCH: PLAYER {}  ({})", other,
                        sug.crossTier ? "NEARBY BAND" : "SAME BAND").c_str(),
                    0.24f, {w * 0.50f, y}, {150,235,180});
                y -= 22.f;
            }
        }
        VersusUI::label(m_page,
            "The host launches matches from MATCH SETUP — this page only sets preferences.",
            0.17f, {w * 0.50f, h * 0.175f}, {175,195,225});
        VersusUI::label(m_page,
            "Room-scoped matchmaking only. Global queues need an upstream Globed feature.",
            0.16f, {w * 0.50f, h * 0.14f}, {150,175,210});

        auto* back = CCMenu::create();
        back->setPosition({0,0});
        back->addChild(VersusUI::button("BACK TO LOBBY", this, menu_selector(LobbyLayer::onJoinLobby),
            {w * 0.50f, 34.f}, "GJ_button_01.png", 0.42f));
        m_page->addChild(back, 5);
    }

    void LobbyLayer::onQueueCasual(CCObject*) {
        VersusLobby::setQueue(0, VersusLobby::difficultyTier());
        showMatchmaking();
    }

    void LobbyLayer::onQueueRanked(CCObject*) {
        VersusLobby::setQueue(1, VersusLobby::difficultyTier());
        showMatchmaking();
    }

    void LobbyLayer::onTierPrev(CCObject*) {
        uint8_t tier = VersusLobby::difficultyTier();
        if (tier > 0) VersusLobby::setQueue(VersusLobby::queueType(), tier - 1);
        showMatchmaking();
    }

    void LobbyLayer::onTierNext(CCObject*) {
        uint8_t tier = VersusLobby::difficultyTier();
        if (tier < VersusModes::kDifficultyCount) {
            VersusLobby::setQueue(VersusLobby::queueType(), tier + 1);
        }
        showMatchmaking();
    }

    void LobbyLayer::onOpenMatchmaking(CCObject*) {
        if (!VersusLobby::joined()) {
            showGroupRooms();
            return;
        }
        showMatchmaking();
    }

    void LobbyLayer::onJoinLobby(CCObject*) {
        if (!Versus::globedInRoom()) {
            showGroupRooms();
            return;
        }
        if (!Versus::globedConnected()) {
            // A room exists but the central server is unreachable: say so
            // plainly instead of showing an empty lobby.
            showInfo("CANNOT JOIN LOBBY", "CONNECTION",
                "NOT CONNECTED TO GLOBED",
                "The room is there, but the central server is unreachable.",
                fmt::format("CONNECTION STATE: {}", Versus::globedConnectionStateName()).c_str());
            return;
        }
        if (!VersusLobby::joined()) {
            VersusLobby::join(Versus::globedRoomId(), 1);
        }
        showRoomLobby();
    }

    void LobbyLayer::onToggleReady(CCObject*) {
        VersusLobby::setReady(!VersusLobby::ready());
        showRoomLobby();
    }


    void LobbyLayer::onHostCancelMatch(CCObject*) {
        VersusLobby::cancelMatch(VersusProto::CancelReason::HostCancelled);
        showRoomLobby();
    }

    void LobbyLayer::onLeaveLobby(CCObject*) {
        VersusLobby::leave();
        showGroupRooms();
    }

    void LobbyLayer::onWorkshop(CCObject*) {
        showInfo("VERSUS WORKSHOP", "COMMUNITY CONTENT",
            "WORKSHOP NOT AVAILABLE YET",
            "This button now opens the Workshop status page.",
            "A real catalog, safe uploads, moderation, and hosting are still needed.");
    }

    void LobbyLayer::showVoice() {
        s_screen = Screen::Voice;
        basePage("VOICE CHAT", "VOICE SETUP AND SAFETY");
        auto w = m_size.width;
        auto h = m_size.height;
        auto mid = h * 0.57f;
        VersusUI::addCard(m_page, {w * 0.50f, mid}, {w * 0.86f, h * 0.34f}, {18,42,91,255});

        char const* state = !globedInstalled() ? "GLOBED IS REQUIRED FOR VOICE" :
            (!globedEnabled() ? "ENABLE GLOBED FIRST" :
            (!globedLoaded() ? "WAITING FOR GLOBED" :
            (!globedConnected() ? "CONNECT TO GLOBED" :
                "VOICE IS PROVIDED BY GLOBED IN SHARED LEVELS")));
        VersusUI::label(m_page, state, 0.32f, {w * 0.50f, mid + h * 0.09f},
            globedReady() ? ccColor3B{115,255,170} : ccColor3B{255,195,115});
        VersusUI::label(m_page, "Globed handles voice chat inside shared levels.", 0.27f,
            {w * 0.50f, mid + h * 0.025f}, {225,235,250});
        VersusUI::label(m_page, "Set your push-to-talk and deafen keys in Globed settings.", 0.24f,
            {w * 0.50f, mid - h * 0.035f}, {190,215,245});
        VersusUI::label(m_page, "Use mute / block / report controls when available.", 0.23f,
            {w * 0.50f, mid - h * 0.085f}, {170,205,240});

        auto* menu = CCMenu::create();
        menu->setPosition({0,0});
        menu->addChild(VersusUI::button("ROOM STATUS", this, menu_selector(LobbyLayer::onRooms),
            {w * 0.30f, 48.f}, "GJ_button_02.png", 0.43f));
        menu->addChild(VersusUI::button("PLAZA", this, menu_selector(LobbyLayer::onOpenPlaza),
            {w * 0.70f, 48.f}, "GJ_button_01.png", 0.43f));
        m_page->addChild(menu, 5);
    }

    void LobbyLayer::onVoice(CCObject*) { showVoice(); }

    void LobbyLayer::onLocal(CCObject*) {
        s_screen = Screen::Local;
        showInfo("LOCAL VERSUS","SAME DEVICE MATCH","LOCAL MODE",
            "Local match setup is being built.",
            "Two independent player controls are still needed.");
    }

    // Globed owns network connectivity and rooms. Versus does not call an
    // external matchmaking service or pretend Globed has a cross-room queue.
    void LobbyLayer::onOnline(CCObject*) {
        if (!globedInstalled() || !globedEnabled() || !globedLoaded()) {
            showInfo("ONLINE VERSUS", "GLOBED REQUIRED", "GLOBED NOT READY",
                "Install and enable Globed 2.3.0 beta or newer.",
                "Versus uses Globed directly for multiplayer networking.");
            return;
        }
        if (!globedApiAvailable() || !globedConnected()) {
            showInfo("ONLINE VERSUS", "GLOBED NETWORK", "NOT CONNECTED",
                "Connect to the Globed central server first.",
                "Once connected, open Private Versus to use your Globed room.");
            return;
        }
        if (globedInRoom()) {
            onJoinLobby(nullptr);
        } else {
            showInfo("ONLINE VERSUS", "GLOBED CONNECTED", "NO ROOM JOINED",
                "Your Globed connection is ready.",
                "Join or create a room through Globed, then open Private Versus.");
        }
    }
}
