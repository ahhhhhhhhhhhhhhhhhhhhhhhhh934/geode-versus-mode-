#pragma once

#include <algorithm>
#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "../net/LobbyClient.hpp"
#include "../core/GameModes.hpp"

using namespace geode::prelude;

// In-game Versus sync: hooks into PlayLayer only when a Versus match is
// actually running, and an overlay that shows the live standings.
namespace VersusPlay {
    // A versus match is active for this level only when the launched match
    // points at this exact level.
    inline bool matchActiveForLevel(GJGameLevel* level) {
        return VersusLobby::phase() == VersusLobby::MatchPhase::InGame &&
            VersusLobby::matchId() != 0 &&
            level && level->m_levelID == VersusLobby::levelId();
    }

    // A small always-on-top overlay showing the live match standings.
    class StandingsOverlay : public CCLayer {
    public:
        static StandingsOverlay* create() {
            auto* ret = new StandingsOverlay();
            if (ret && ret->init()) { ret->autorelease(); return ret; }
            CC_SAFE_DELETE(ret);
            return nullptr;
        }

        bool init() override {
            if (!CCLayer::init()) return false;
            this->setID("versus-standings-overlay");
            auto size = CCDirector::sharedDirector()->getWinSize();

            m_title = CCLabelBMFont::create("VERSUS", "bigFont.fnt");
            m_title->setScale(0.22f);
            m_title->setColor({255, 224, 110});
            m_title->setPosition({size.width * 0.16f, size.height - 34.f});
            this->addChild(m_title, 10);

            for (auto& line : m_lines) {
                line = CCLabelBMFont::create("", "bigFont.fnt");
                line->setScale(0.16f);
                line->setAnchorPoint({0.f, 0.5f});
                line->setPosition({size.width * 0.05f, 0.f});
                this->addChild(line, 10);
            }
            this->scheduleUpdate();
            return true;
        }

        void update(float dt) override {
            CCLayer::update(dt);
            m_refresh += dt;
            if (m_refresh < 1.f) return;
            m_refresh = 0.f;

            auto* info = VersusModes::find(VersusLobby::mode());
            m_title->setString(info ? info->name : "VERSUS");

            auto players = VersusLobby::players();
            int self = VersusLobby::localAccountId();
            size_t lineIndex = 0;

            // The local line.
            auto localPercent = PlayLayer::get() ? PlayLayer::get()->m_percentage : 0;
            std::string localLine = fmt::format("YOU  {}%  {} DEATHS{}",
                localPercent, VersusLobby::s_localDeaths,
                VersusLobby::s_localFinished ? "  FINISHED" :
                (VersusLobby::s_localEliminated ? "  ELIMINATED" : ""));
            setLine(lineIndex++, localLine, {145, 255, 175});

            // Opponent lines, sorted by account id.
            std::sort(players.begin(), players.end(),
                [](auto const& a, auto const& b) { return a.accountId < b.accountId; });
            for (auto const& p : players) {
                if (p.accountId == self || lineIndex >= kMaxLines) continue;
                std::string line;
                ccColor3B color = {225, 235, 250};
                if (p.finished) {
                    line = fmt::format("PLAYER {}  FINISHED  {} DEATHS", p.accountId, p.deaths);
                    color = {255, 220, 120};
                } else if (p.eliminated) {
                    line = fmt::format("PLAYER {}  ELIMINATED  {}%", p.accountId, p.percentX10 / 10);
                    color = {255, 130, 130};
                } else {
                    line = fmt::format("PLAYER {}  {}%  {} DEATHS", p.accountId, p.percentX10 / 10, p.deaths);
                }
                setLine(lineIndex++, line, color);
            }
            for (size_t i = lineIndex; i < kMaxLines; ++i) {
                m_lines[i]->setString("");
            }
        }

    private:
        static constexpr size_t kMaxLines = 4;

        void setLine(size_t index, std::string const& text, ccColor3B color) {
            if (index >= kMaxLines) return;
            auto size = CCDirector::sharedDirector()->getWinSize();
            m_lines[index]->setString(text.c_str());
            m_lines[index]->setColor(color);
            m_lines[index]->setPosition({size.width * 0.05f,
                size.height - 56.f - static_cast<float>(index) * 16.f});
        }

        float m_refresh = 0.f;
        CCLabelBMFont* m_title = nullptr;
        CCLabelBMFont* m_lines[kMaxLines] = {};
    };
}

class $modify(VersusPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        if (VersusPlay::matchActiveForLevel(level)) {
            // Reset local match state and show the live standings.
            VersusLobby::beginLocalPlay();
            auto* overlay = VersusPlay::StandingsOverlay::create();
            if (overlay) this->addChild(overlay, 1000);
        }
        return true;
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        PlayLayer::destroyPlayer(player, object);
        if (!VersusPlay::matchActiveForLevel(m_level)) return;
        // Only the local player's deaths count for Versus.
        if (player != m_player1) return;
        VersusLobby::reportLocalDeath(static_cast<uint16_t>(m_percentage * 10));
    }

    void levelComplete() {
        PlayLayer::levelComplete();
        if (!VersusPlay::matchActiveForLevel(m_level)) return;
        VersusLobby::reportLocalFinish();
    }
};
