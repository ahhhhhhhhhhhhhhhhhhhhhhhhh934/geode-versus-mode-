#include "LobbyLayer.hpp"
#include "../core/GameModes.hpp"
#include "../core/StatsStore.hpp"
#include <Geode/ui/TextInput.hpp>

// Pages that configure and describe matches: mode selection today, match
// setup and results in the following PRs of the stack.
namespace Versus {
    void LobbyLayer::showModeSelect() {
        s_screen = Screen::Info;
        basePage("MODE SELECT", "PICK THE RULES FOR THIS ROOM");
        auto w = m_size.width;
        auto h = m_size.height;

        // Two columns of three mode cards. Each card shows the mode name,
        // its tagline, its ranked status, and its core rule.
        float top = h * 0.80f;
        float cardH = h * 0.185f;
        float gap = h * 0.015f;
        for (size_t i = 0; i < VersusModes::kModeCount; ++i) {
            auto const& m = VersusModes::kModes[i];
            float column = i % 2 == 0 ? 0.27f : 0.73f;
            int row = static_cast<int>(i / 2);
            float y = top - cardH / 2.f - row * (cardH + gap);
            bool selected = VersusLobby::joined() && VersusLobby::mode() == m.id;
            VersusUI::addCard(m_page, {w * column, y}, {w * 0.44f, cardH},
                selected ? ccColor4B{26, 68, 108, 255} : ccColor4B{18, 42, 91, 255});
            VersusUI::label(m_page, m.name, 0.30f, {w * column, y + cardH * 0.32f},
                m.ranked ? ccColor3B{255,224,110} : ccColor3B{160,205,255});
            VersusUI::label(m_page, m.tagline, 0.15f, {w * column, y + cardH * 0.13f}, {185,210,245});
            char const* badge = m.ranked ? "RANKED" : "CASUAL";
            VersusUI::label(m_page, badge, 0.14f, {w * column, y - cardH * 0.08f},
                m.ranked ? ccColor3B{115,255,170} : ccColor3B{170,200,235});
            VersusUI::label(m_page, selected ? "SELECTED" : "TAP TO SELECT", 0.13f,
                {w * column, y - cardH * 0.26f},
                selected ? ccColor3B{255,215,130} : ccColor3B{150,180,215});

            auto* menu = CCMenu::create();
            menu->setPosition({0,0});
            // An invisible full-card tap target selects the mode.
            auto* hit = CCMenuItemSpriteExtra::create(
                CCNode::create(), this, menu_selector(LobbyLayer::onPickMode));
            hit->setContentSize({w * 0.44f, cardH});
            hit->setPosition({w * column, y});
            hit->setTag(m.id);
            menu->addChild(hit);
            m_page->addChild(menu, 6);
        }

        VersusUI::label(m_page,
            VersusLobby::isHost() ? "HOST SELECTS THE MODE FOR EVERYONE" : "ONLY THE ROOM HOST CAN SELECT THE MODE",
            0.17f, {w * 0.50f, h * 0.145f}, {255,215,130});
        VersusUI::label(m_page,
            "RULES: DYING ELIMINATES YOU IN SURVIVAL  •  WARM-UP RECORDS NOTHING",
            0.16f, {w * 0.50f, h * 0.115f}, {160,190,225});

        auto* menu = CCMenu::create();
        menu->setPosition({0,0});
        menu->addChild(VersusUI::button("BACK TO LOBBY", this, menu_selector(LobbyLayer::onJoinLobby),
            {w * 0.50f, 40.f}, "GJ_button_01.png", 0.44f));
        m_page->addChild(menu, 7);
    }

    void LobbyLayer::showMatchSetup() {
        s_screen = Screen::MatchSetup;
        basePage("MATCH SETUP", "HOST CONTROLS  •  PICK THE LEVEL");
        auto w = m_size.width;
        auto h = m_size.height;

        auto* info = VersusModes::find(VersusLobby::mode());
        if (info) {
            VersusUI::label(m_page, fmt::format("MODE: {}", info->name).c_str(), 0.32f,
                {w * 0.50f, h * 0.76f}, {255,224,110});
            VersusUI::label(m_page, info->tagline, 0.20f,
                {w * 0.50f, h * 0.70f}, {185,210,245});
        }

        VersusUI::addCard(m_page, {w * 0.50f, h * 0.52f}, {w * 0.86f, h * 0.24f}, {18, 42, 91, 255});
        VersusUI::label(m_page, "LEVEL ID", 0.24f, {w * 0.50f, h * 0.60f}, {125,190,240});

        // Numeric level-id entry with Geode's TextInput.
        auto* input = geode::TextInput::create(w * 0.60f, "ENTER LEVEL ID", "bigFont.fnt");
        input->setCommonFilter(geode::CommonFilter::Uint);
        input->setScale(0.9f);
        input->setPosition({w * 0.50f, h * 0.51f});
        input->setID("versus-level-id-input");
        m_page->addChild(input, 5);
        m_levelInput = input;

        auto* menu = CCMenu::create();
        menu->setPosition({0,0});
        menu->addChild(VersusUI::button("USE PINNED LEVEL", this, menu_selector(LobbyLayer::onUsePinnedLevel),
            {w * 0.27f, h * 0.38f}, "GJ_button_04.png", 0.40f));
        menu->addChild(VersusUI::button("LAUNCH MATCH", this, menu_selector(LobbyLayer::onLaunchMatch),
            {w * 0.73f, h * 0.38f}, "GJ_button_05.png", 0.46f));
        menu->addChild(VersusUI::button("BACK TO LOBBY", this, menu_selector(LobbyLayer::onJoinLobby),
            {w * 0.50f, 36.f}, "GJ_button_01.png", 0.42f));
        m_page->addChild(menu, 6);

        VersusUI::label(m_page,
            "5 SECOND COUNTDOWN  •  EVERYONE WARPS INTO THE LEVEL SESSION",
            0.17f, {w * 0.50f, h * 0.155f}, {160,195,230});

        // Mode-aware level hints: the pure check says what can and cannot
        // be verified from the id alone, and warm-up says plainly that
        // nothing is recorded.
        int previewId = 0;
        if (m_levelInput) {
            try { previewId = std::stoi(m_levelInput->getString()); } catch (...) {}
        }
        auto check = VersusModes::checkLevelForMode(VersusLobby::mode(), previewId);
        auto* hint = VersusModes::levelCheckHint(check);
        VersusUI::label(m_page, hint, 0.17f, {w * 0.50f, h * 0.11f},
            check == VersusModes::LevelCheck::Ok ? ccColor3B{150,235,180} : ccColor3B{255,205,140});
        if (auto* m = VersusModes::find(VersusLobby::mode()); m && !m->ranked) {
            VersusUI::label(m_page, "WARM-UP ROOM: NO WINS, LOSSES OR RANK POINTS",
                0.16f, {w * 0.50f, h * 0.075f}, {160,220,255});
        }
    }

    void LobbyLayer::onOpenMatchSetup(CCObject*) {
        if (!VersusLobby::isHost()) {
            showInfo("MATCH SETUP", "HOST ONLY",
                "ONLY THE HOST CAN LAUNCH",
                "Ask the room owner (host) to configure and launch the match.",
                "You will see the countdown when it starts.");
            return;
        }
        showMatchSetup();
    }

    void LobbyLayer::onUsePinnedLevel(CCObject*) {
        if (!Versus::globedApiAvailable()) return;
        // The Globed room's pinned level, when the owner set one.
        auto pinned = globed::api::room::getPinnedLevel();
        int levelId = pinned.levelId();
        if (levelId <= 0 || pinned == globed::SessionId{}) {
            showInfo("MATCH SETUP", "PINNED LEVEL", "NO PINNED LEVEL",
                "The room owner can pin a level in the Globed room.",
                "Pin it there, or type a level ID above.");
            return;
        }
        if (m_levelInput) m_levelInput->setString(fmt::format("{}", levelId));
    }

    void LobbyLayer::onLaunchMatch(CCObject*) {
        if (!VersusLobby::isHost()) {
            onOpenMatchSetup(nullptr);
            return;
        }
        int levelId = 0;
        if (m_levelInput) {
            try {
                levelId = std::stoi(m_levelInput->getString());
            } catch (...) {
                levelId = 0;
            }
        }
        if (levelId <= 0) {
            showInfo("MATCH SETUP", "LEVEL ID REQUIRED", "ENTER A VALID LEVEL ID",
                "Type the ID of the level to race, or use the pinned level button.",
                "Level IDs are numbers, like 90123012.");
            return;
        }
        // Mode-level validation from the pure rules layer. Platformer and
        // classic mismatches cannot be verified from the id alone, so they
        // warn here and are verified against the real level when it opens.
        auto check = VersusModes::checkLevelForMode(VersusLobby::mode(), levelId);
        if (check == VersusModes::LevelCheck::UnknownMode) {
            showInfo("MATCH SETUP", "UNKNOWN MODE", "PICK A MODE FIRST",
                "Choose a game mode in the lobby before launching.", "");
            return;
        }
        if (check == VersusModes::LevelCheck::NeedsPlatformerLevel ||
            check == VersusModes::LevelCheck::NeedsClassicLevel) {
            // Confirm-first: warn once, launch on the second press for the
            // same level. The real check runs when the level opens.
            if (m_confirmLaunchLevelId != levelId) {
                m_confirmLaunchLevelId = levelId;
                showInfo("MATCH SETUP", "CHECK THE LEVEL TYPE",
                    check == VersusModes::LevelCheck::NeedsPlatformerLevel
                        ? "PLATFORMER LEVEL EXPECTED" : "CLASSIC LEVEL EXPECTED",
                    "Press LAUNCH MATCH again to launch this level anyway.",
                    VersusModes::levelCheckHint(check));
                return;
            }
        }
        m_confirmLaunchLevelId = 0;
        VersusLobby::launchMatch(levelId, VersusLobby::mode(), 5);
        showRoomLobby();
    }

    void LobbyLayer::showResults() {
        s_screen = Screen::Results;
        basePage("MATCH RESULTS", "VERIFIED FROM THE MATCH PACKETS");
        auto w = m_size.width;
        auto h = m_size.height;

        auto* info = VersusModes::find(VersusLobby::mode());
        auto verdict = static_cast<VersusMatch::Outcome>(VersusLobby::s_lastVerdict);

        // Verdict banner.
        char const* banner = "MATCH OVER";
        ccColor3B bannerColor = {225, 235, 250};
        switch (verdict) {
            case VersusMatch::Outcome::Win:
                banner = "VICTORY"; bannerColor = {115, 255, 170}; break;
            case VersusMatch::Outcome::Loss:
                banner = "DEFEAT"; bannerColor = {255, 130, 130}; break;
            case VersusMatch::Outcome::Forfeit:
                banner = "WIN BY FORFEIT"; bannerColor = {255, 210, 110}; break;
            case VersusMatch::Outcome::Draw:
                banner = "DRAW"; bannerColor = {255, 220, 120}; break;
            default:
                banner = "NOT RECORDED"; break;
        }
        VersusUI::addCard(m_page, {w * 0.50f, h * 0.72f}, {w * 0.86f, h * 0.17f}, {18, 42, 91, 255});
        VersusUI::label(m_page, banner, 0.52f, {w * 0.50f, h * 0.76f}, bannerColor);
        if (info) {
            VersusUI::label(m_page, fmt::format("{}  •  LEVEL {}", info->name, VersusLobby::levelId()).c_str(),
                0.22f, {w * 0.50f, h * 0.685f}, {185, 210, 245});
        }

        // Verified record summary.
        VersusUI::addCard(m_page, {w * 0.50f, h * 0.50f}, {w * 0.86f, h * 0.22f}, {18, 42, 91, 255});
        auto stats = VersusStats::stats();
        VersusUI::label(m_page, fmt::format("RANK POINTS  {}", VersusStats::rankPoints()).c_str(),
            0.30f, {w * 0.50f, h * 0.56f}, {145, 220, 255});
        VersusUI::label(m_page, fmt::format("{} WINS  •  {} LOSSES  •  {}% WIN RATE",
            stats.wins, stats.losses, VersusStats::winRatePercent(stats)).c_str(),
            0.22f, {w * 0.50f, h * 0.485f}, {225, 235, 250});
        VersusUI::label(m_page,
            "Warm-up matches record nothing. Results are computed from the shared match packets.",
            0.16f, {w * 0.50f, h * 0.44f}, {170, 200, 235});

        // Live standings from the packet stream.
        auto players = VersusLobby::players();
        int self = VersusLobby::localAccountId();
        float y = h * 0.35f;
        auto drawLine = [&](std::string const& text, ccColor3B color) {
            VersusUI::label(m_page, text.c_str(), 0.20f, {w * 0.50f, y}, color);
            y -= h * 0.04f;
        };
        drawLine(fmt::format("YOU  {} DEATHS{}", VersusLobby::s_localDeaths,
            VersusLobby::s_localFinished ? "  •  FINISHED" : ""), {145, 255, 175});
        for (auto const& p : players) {
            if (p.accountId == self || !p.versusActive) continue;
            if (p.finished) {
                drawLine(fmt::format("PLAYER {}  FINISHED  •  {} DEATHS", p.accountId, p.deaths),
                    {255, 220, 120});
            } else if (p.eliminated) {
                drawLine(fmt::format("PLAYER {}  ELIMINATED", p.accountId), {255, 130, 130});
            } else {
                drawLine(fmt::format("PLAYER {}  {}%  •  {} DEATHS",
                    p.accountId, p.percentX10 / 10, p.deaths), {225, 235, 250});
            }
        }

        // Controls.
        auto* menu = CCMenu::create();
        menu->setPosition({0,0});
        bool voted = VersusLobby::s_rematchVotes.count(VersusLobby::localAccountId()) &&
            VersusLobby::s_rematchVotes[VersusLobby::localAccountId()];
        menu->addChild(VersusUI::button(voted ? "REMATCH VOTED" : "REMATCH",
            this, menu_selector(LobbyLayer::onVoteRematch),
            {w * 0.27f, 40.f}, voted ? "GJ_button_04.png" : "GJ_button_02.png", 0.44f));
        menu->addChild(VersusUI::button("BACK TO LOBBY", this, menu_selector(LobbyLayer::onJoinLobby),
            {w * 0.73f, 40.f}, "GJ_button_01.png", 0.44f));
        m_page->addChild(menu, 6);
        VersusUI::label(m_page, "REMATCH STARTS WHEN EVERY PLAYER VOTES",
            0.16f, {w * 0.50f, 14.f}, {150, 180, 215});
    }

    void LobbyLayer::onVoteRematch(CCObject*) {
        bool voted = VersusLobby::s_rematchVotes.count(VersusLobby::localAccountId()) &&
            VersusLobby::s_rematchVotes[VersusLobby::localAccountId()];
        VersusLobby::voteRematch(!voted);
        showResults();
    }

    void LobbyLayer::onOpenModeSelect(CCObject*) {
        showModeSelect();
    }

    void LobbyLayer::onPickMode(CCObject* sender) {
        if (!VersusLobby::isHost()) {
            showInfo("MODE SELECT", "HOST CONTROLS", "ONLY THE HOST CAN CHANGE THE MODE",
                "The room owner chooses the game mode for everyone.",
                "Ask the host to select the mode you want to play.");
            return;
        }
        auto* item = static_cast<CCMenuItemSpriteExtra*>(sender);
        uint8_t mode = static_cast<uint8_t>(item->getTag());
        if (VersusModes::find(mode)) {
            VersusLobby::setMode(mode);
        }
        showRoomLobby();
    }
}
