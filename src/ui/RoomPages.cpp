#include "LobbyLayer.hpp"

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
            VersusUI::label(m_page, "GLOBED ROOM DETECTED", 0.37f,
                {w * 0.50f, mid + h * 0.105f}, {145,255,175});
            auto roomText = fmt::format("ROOM ID  {}", globedRoomId());
            VersusUI::label(m_page, roomText.c_str(), 0.32f,
                {w * 0.50f, mid + h * 0.035f}, {255,225,135});
            VersusUI::label(m_page, "Your friends must join this same Globed room.", 0.25f,
                {w * 0.50f, mid - h * 0.035f}, {235,240,250});
            VersusUI::label(m_page, "Make the Globed room private or invite-only there.", 0.23f,
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
            showGroupRooms();
        } else {
            showInfo("ONLINE VERSUS", "GLOBED CONNECTED", "NO ROOM JOINED",
                "Your Globed connection is ready.",
                "Join or create a room through Globed, then open Private Versus.");
        }
    }
}
