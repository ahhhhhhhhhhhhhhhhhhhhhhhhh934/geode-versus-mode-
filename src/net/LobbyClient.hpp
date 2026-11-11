#pragma once

#include <Geode/Geode.hpp>
#include <globed/soft-link/API.hpp>
#include <globed/core/data/Messages.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <Geode/binding/GJAccountManager.hpp>
#include "../core/GameModes.hpp"
#include "../core/Protocol.hpp"
#include "GlobedClient.hpp"

#include <cmath>
#include <unordered_map>
#include <vector>

// The Versus lobby transport. Everything travels over Globed's public
// Soft Link Server Events, sent to the central server and scoped to the
// sender's current room by the room id in every packet header.
//
// What this layer does:
//   - registers the versus.mode/lobby-v3 event and decodes every packet
//   - tracks room players, ready states, heartbeats and timeouts
//   - drives the match lifecycle packets (launch, countdown, cancel,
//     progress, finish, results, rematch)
//   - reconnects (re-join) after a Globed reconnect or room re-entry
//
// What this layer deliberately does NOT do:
//   - cross-room or global matchmaking (the central server only forwards
//     events inside the sender's room — see
//     docs/GLOBED_NATIVE_MATCHMAKING_INVESTIGATION.md)
//   - server-side validation of results (there is no Versus server)
using namespace geode::prelude;

namespace VersusLobby {
    // Match lifecycle phase, mirrored from the packet stream.
    enum class MatchPhase { None, Countdown, InGame, Results };

    // Defined further down in this file; declared early because the match
    // lifecycle functions above their definitions call them.
    inline void sendProgress(uint16_t percentX10, uint32_t deaths, uint8_t flags);

    struct RemotePlayer {
        int accountId = 0;
        uint8_t mode = 0;
        bool ready = false;
        float lastSeen = 0.f;
        // live match data (updated by Progress/Finish packets)
        uint16_t percentX10 = 0;
        uint32_t deaths = 0;
        bool finished = false;
        bool eliminated = false;
        uint32_t finishTimeMs = 0;
    };

    // ---- shared state -----------------------------------------------------

    inline std::unordered_map<int, RemotePlayer> s_players;
    inline geode::ListenerHandle s_listener;
    inline bool s_registered = false;

    inline uint32_t s_roomId = 0;
    inline uint8_t s_mode = 0;
    inline bool s_joined = false;
    inline bool s_ready = false;
    inline float s_heartbeat = 0.f;
    inline float s_clock = 0.f;

    // match state
    inline MatchPhase s_phase = MatchPhase::None;
    inline uint32_t s_matchId = 0;
    inline int s_levelId = 0;
    inline float s_countdown = 0.f;
    inline uint8_t s_countdownTotal = 0;
    inline uint32_t s_matchCounter = 0;
    inline bool s_resultsRecorded = false;
    inline std::unordered_map<int, bool> s_rematchVotes;

    // connection bookkeeping for reconnection
    inline bool s_wasConnected = false;

    // local player state during a match
    inline uint32_t s_localDeaths = 0;
    inline bool s_localFinished = false;
    inline bool s_localEliminated = false;
    inline float s_matchStartClock = 0.f;
    inline float s_progressTimer = 0.f;

    inline int localAccountId() {
        auto* am = GJAccountManager::sharedState();
        return am ? am->m_accountID : 0;
    }

    // The Globed room owner is the Versus host. This is authoritative room
    // state from Globed, not a Versus invention.
    inline int hostAccountId() {
        if (!Versus::globedApiAvailable() || !Versus::globedInRoom()) return 0;
        return globed::api::room::getOwner();
    }

    inline bool isHost() {
        auto host = hostAccountId();
        return host != 0 && host == localAccountId();
    }

    // ---- sending ----------------------------------------------------------

    inline bool transportReady() {
        return s_joined && s_roomId != 0 &&
            Versus::globedApiAvailable() && globed::api::net::isConnected();
    }

    inline void send(std::vector<uint8_t>&& packet) {
        if (!transportReady()) return;
        globed::EventOptions options{};
        options.server = globed::EventServer::Central;
        options.reliable = true;
        globed::api::net::sendEvent(VersusProto::kEventId, std::move(packet), options);
    }

    // ---- receiving ---------------------------------------------------------

    // Last verdict seen from a Results packet (host-computed).
    inline uint8_t s_lastVerdict = 0;
    inline int32_t s_lastWinner = 0;

    inline void leave(); // defined below; join() needs it

    inline void applyPacket(VersusProto::Decoded const& d, int sender) {
        // Every packet must be scoped to the lobby's room.
        if (d.roomId == 0 || d.roomId != s_roomId || sender <= 0) return;
        // A match packet must belong to the current match.
        bool isMatchPacket = d.action == VersusProto::Action::Launch ||
            d.action == VersusProto::Action::Countdown ||
            d.action == VersusProto::Action::Cancel ||
            d.action == VersusProto::Action::Progress ||
            d.action == VersusProto::Action::Finish ||
            d.action == VersusProto::Action::Results ||
            d.action == VersusProto::Action::Rematch;
        if (isMatchPacket && s_matchId != 0 && d.matchId != s_matchId && d.action != VersusProto::Action::Launch) return;

        switch (d.action) {
            case VersusProto::Action::Join:
            case VersusProto::Action::Ready:
            case VersusProto::Action::Heartbeat: {
                auto& p = s_players[sender];
                p.accountId = sender;
                p.mode = d.mode;
                p.ready = d.ready;
                p.lastSeen = s_clock;
                break;
            }
            case VersusProto::Action::Leave:
                s_players.erase(sender);
                s_rematchVotes.erase(sender);
                break;
            case VersusProto::Action::Launch: {
                // Only the room owner may launch a match.
                if (sender != hostAccountId()) return;
                s_matchId = d.matchId;
                s_levelId = d.levelId;
                s_mode = d.mode;
                s_countdownTotal = d.countdownSeconds;
                s_countdown = static_cast<float>(d.countdownSeconds);
                s_phase = MatchPhase::Countdown;
                s_resultsRecorded = false;
                s_rematchVotes.clear();
                for (auto& [id, p] : s_players) {
                    p.percentX10 = 0;
                    p.deaths = 0;
                    p.finished = false;
                    p.eliminated = false;
                    p.finishTimeMs = 0;
                }
                break;
            }
            case VersusProto::Action::Countdown: {
                if (sender != hostAccountId()) return;
                // Stay in sync with the host's clock.
                s_countdown = static_cast<float>(d.countdownSeconds);
                break;
            }
            case VersusProto::Action::Cancel: {
                if (sender != hostAccountId() && sender != localAccountId()) return;
                s_phase = MatchPhase::None;
                s_countdown = 0.f;
                break;
            }
            case VersusProto::Action::Progress: {
                auto& p = s_players[sender];
                p.accountId = sender;
                p.percentX10 = d.percentX10;
                p.deaths = d.deaths;
                if (d.flags & VersusProto::kFlagEliminated) p.eliminated = true;
                if (d.flags & VersusProto::kFlagFinished) p.finished = true;
                p.lastSeen = s_clock;
                break;
            }
            case VersusProto::Action::Finish: {
                auto& p = s_players[sender];
                p.accountId = sender;
                p.finished = true;
                p.finishTimeMs = d.timeMs;
                p.deaths = d.deaths;
                p.percentX10 = d.percentX10;
                p.lastSeen = s_clock;
                break;
            }
            case VersusProto::Action::Results: {
                s_phase = MatchPhase::Results;
                s_lastVerdict = d.verdict;
                s_lastWinner = d.winner;
                break;
            }
            case VersusProto::Action::Rematch: {
                s_rematchVotes[sender] = d.rematchVote != 0;
                break;
            }
        }
    }

    inline void initialize() {
        if (s_registered) return;
        globed::api::waitForGlobed([] {
            if (!globed::api::available() || s_registered) return;
            globed::api::net::registerEvent(VersusProto::kEventId, globed::EventServer::Central);
            s_listener = globed::MessageEvent<globed::msg::EventsMessage>{false}.listen(
                [](const globed::msg::EventsMessage& message) {
                    for (auto const& event : message.events) {
                        if (event.name != VersusProto::kEventId) continue;
                        auto decoded = VersusProto::decode(event.data);
                        if (!decoded) continue;
                        applyPacket(*decoded, event.options.sender);
                    }
                }
            );
            s_registered = true;
        });
    }

    // ---- lobby lifecycle ----------------------------------------------------

    inline void sendLobby(VersusProto::Action action) {
        send(VersusProto::encodeLobby(s_roomId, action, s_mode, s_ready));
    }

    inline void join(uint32_t roomId, uint8_t mode) {
        if (!roomId) return;
        if (s_joined && s_roomId != roomId) leave();
        if (!VersusProto::validMode(mode)) return;
        s_roomId = roomId;
        s_mode = mode;
        s_ready = false;
        s_joined = true;
        s_heartbeat = 0.f;
        s_phase = MatchPhase::None;
        sendLobby(VersusProto::Action::Join);
    }

    inline void setReady(bool ready) {
        if (!s_joined) return;
        s_ready = ready;
        sendLobby(VersusProto::Action::Ready);
    }

    inline void setMode(uint8_t mode) {
        if (!s_joined || !VersusProto::validMode(mode)) return;
        s_mode = mode;
        sendLobby(VersusProto::Action::Heartbeat);
    }

    inline void leave() {
        if (s_joined) sendLobby(VersusProto::Action::Leave);
        s_players.clear();
        s_rematchVotes.clear();
        s_roomId = 0;
        s_mode = 0;
        s_ready = false;
        s_joined = false;
        s_heartbeat = 0.f;
        s_phase = MatchPhase::None;
        s_countdown = 0.f;
        s_resultsRecorded = false;
    }

    // ---- local play tracking -------------------------------------------------

    inline uint32_t elapsedMatchMs() {
        return s_phase == MatchPhase::InGame
            ? static_cast<uint32_t>((s_clock - s_matchStartClock) * 1000.f) : 0u;
    }

    // Called when the level starts for a match level.
    inline void beginLocalPlay() {
        s_localDeaths = 0;
        s_localFinished = false;
        s_localEliminated = false;
        s_progressTimer = 0.f;
        s_matchStartClock = s_clock;
    }

    inline uint8_t localProgressFlags() {
        uint8_t flags = 0;
        if (s_localEliminated) flags |= VersusProto::kFlagEliminated;
        if (s_localFinished) flags |= VersusProto::kFlagFinished;
        return flags;
    }

    inline void reportLocalDeath(uint16_t percentX10) {
        if (s_phase != MatchPhase::InGame) return;
        s_localDeaths++;
        // Survival eliminates on death; other modes only cost time.
        if (auto* mode = VersusModes::find(s_mode); mode && mode->elimination) {
            s_localEliminated = true;
        }
        sendProgress(percentX10, s_localDeaths,
            VersusProto::kFlagDead | localProgressFlags());
    }

    inline void reportLocalProgress(uint16_t percentX10) {
        if (s_phase != MatchPhase::InGame || s_localFinished) return;
        sendProgress(percentX10, s_localDeaths, localProgressFlags());
    }

    inline void reportLocalFinish() {
        if (s_phase != MatchPhase::InGame || s_localFinished) return;
        s_localFinished = true;
        // 1000 == 100.0 percent, x10 encoding.
        send(VersusProto::encodeFinish(s_roomId, s_matchId, elapsedMatchMs(),
            s_localDeaths, 1000));
    }

    // ---- match lifecycle -----------------------------------------------------

    inline uint32_t nextMatchId() {
        ++s_matchCounter;
        return (s_roomId * 31u) ^ static_cast<uint32_t>(localAccountId()) ^ (s_matchCounter * 7919u);
    }

    inline void launchMatch(int levelId, uint8_t mode, uint8_t countdownSeconds) {
        if (!s_joined || !isHost()) return;
        s_matchId = nextMatchId();
        send(VersusProto::encodeLaunch(s_roomId, s_matchId, levelId, mode, countdownSeconds));
        // Apply locally too, so the host's own countdown runs.
        VersusProto::Decoded d{};
        d.action = VersusProto::Action::Launch;
        d.roomId = s_roomId;
        d.matchId = s_matchId;
        d.levelId = levelId;
        d.mode = mode;
        d.countdownSeconds = countdownSeconds;
        applyPacket(d, hostAccountId());
    }

    inline void broadcastCountdown(uint8_t secondsLeft) {
        if (!isHost() || s_matchId == 0) return;
        send(VersusProto::encodeCountdown(s_roomId, s_matchId, secondsLeft, s_levelId, s_mode));
    }

    inline void cancelMatch(VersusProto::CancelReason reason) {
        if (s_matchId == 0) return;
        send(VersusProto::encodeCancel(s_roomId, s_matchId, reason));
        s_phase = MatchPhase::None;
        s_countdown = 0.f;
    }

    inline void sendProgress(uint16_t percentX10, uint32_t deaths, uint8_t flags) {
        if (s_phase != MatchPhase::InGame || s_matchId == 0) return;
        send(VersusProto::encodeProgress(s_roomId, s_matchId, percentX10, deaths, flags));
    }

    inline void sendFinish(uint32_t timeMs, uint32_t deaths, uint16_t percentX10) {
        if (s_matchId == 0) return;
        send(VersusProto::encodeFinish(s_roomId, s_matchId, timeMs, deaths, percentX10));
    }

    inline void sendResults(VersusProto::Verdict verdict, int32_t winner) {
        if (s_matchId == 0) return;
        send(VersusProto::encodeResults(s_roomId, s_matchId, verdict, winner));
        s_lastVerdict = static_cast<uint8_t>(verdict);
        s_lastWinner = winner;
        s_phase = MatchPhase::Results;
    }

    inline void voteRematch(bool yes) {
        if (s_matchId == 0) return;
        s_rematchVotes[localAccountId()] = yes;
        send(VersusProto::encodeRematch(s_roomId, s_matchId, yes ? 1 : 0));
    }

    // ---- per-frame update ---------------------------------------------------

    inline void update(float dt, uint32_t currentRoomId) {
        s_clock += dt;
        if (!s_joined) return;

        // Room changed or left: the lobby is over.
        if (currentRoomId != s_roomId) {
            leave();
            return;
        }

        // Reconnection: re-announce after Globed reconnects.
        bool connected = Versus::globedApiAvailable() && globed::api::net::isConnected();
        if (connected && !s_wasConnected) {
            sendLobby(VersusProto::Action::Join);
        }
        s_wasConnected = connected;

        // Heartbeat so others know we are alive.
        s_heartbeat += dt;
        if (s_heartbeat >= 5.f) {
            s_heartbeat = 0.f;
            sendLobby(VersusProto::Action::Heartbeat);
        }

        // ---- countdown and level warp ----------------------------------------
        if (s_phase == MatchPhase::Countdown) {
            float before = s_countdown;
            s_countdown -= dt;
            // The host keeps everyone else's clock aligned.
            if (isHost()) {
                int beforeWhole = static_cast<int>(std::ceil(before));
                if (beforeWhole < 0) beforeWhole = 0;
                int nowWhole = static_cast<int>(std::ceil(s_countdown < 0.f ? 0.f : s_countdown));
                if (beforeWhole != nowWhole) broadcastCountdown(static_cast<uint8_t>(nowWhole));
            }
            if (s_countdown <= 0.f) {
                s_countdown = 0.f;
                s_phase = MatchPhase::InGame;
                // Enter the level session through Globed's warp API (2.3.0+).
                if (Versus::globedApiAvailable() && Versus::globedInRoom() && s_levelId > 0) {
                    globed::api::game::warpToSession(s_levelId);
                }
            }
        }

        // Drop players whose heartbeats expired.
        for (auto it = s_players.begin(); it != s_players.end();) {
            if (s_clock - it->second.lastSeen > 15.f) {
                s_rematchVotes.erase(it->first);
                it = s_players.erase(it);
            } else {
                ++it;
            }
        }
    }

    // ---- accessors -----------------------------------------------------------

    inline bool joined() { return s_joined; }
    inline bool ready() { return s_ready; }
    inline uint32_t roomId() { return s_roomId; }
    inline uint8_t mode() { return s_mode; }
    inline MatchPhase phase() { return s_phase; }
    inline uint32_t matchId() { return s_matchId; }
    inline int levelId() { return s_levelId; }

    inline std::vector<RemotePlayer> players() {
        std::vector<RemotePlayer> result;
        result.reserve(s_players.size());
        for (auto const& [id, player] : s_players) result.push_back(player);
        return result;
    }

    inline size_t playerCount(uint8_t mode) {
        size_t count = 0;
        for (auto const& [id, player] : s_players) {
            if (player.mode == mode) ++count;
        }
        return count;
    }
}
