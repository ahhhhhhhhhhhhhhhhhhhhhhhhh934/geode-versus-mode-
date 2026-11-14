#pragma once

#include <Geode/Geode.hpp>
#include <globed/soft-link/API.hpp>
#include <globed/core/data/Messages.hpp>
#include <globed/core/net/MessageListener.hpp>
#include <Geode/binding/GJAccountManager.hpp>
#include "../core/GameModes.hpp"
#include "../core/MatchLogic.hpp"
#include "../core/Protocol.hpp"
#include "../core/StatsStore.hpp"

#include <ctime>
#include "GlobedClient.hpp"

#include <cmath>
#include <deque>
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
    inline void recordMatchOutcome();

    struct RemotePlayer {
        int accountId = 0;
        uint8_t mode = 0;
        bool ready = false;
        uint8_t queueType = 0;     // 0 casual, 1 ranked
        uint8_t difficultyTier = 0; // 0..10, difficulty ladder index
        float lastSeen = 0.f;
        // live match data (updated by Progress/Finish packets)
        uint16_t percentX10 = 0;
        uint32_t deaths = 0;
        bool finished = false;
        bool eliminated = false;
        bool disconnected = false;
        uint32_t finishTimeMs = 0;
        // True only when this player has joined the Versus protocol.
        bool versusActive = false;
    };

    // ---- shared state -----------------------------------------------------

    inline std::unordered_map<int, RemotePlayer> s_players;
    inline geode::ListenerHandle s_listener;
    inline geode::ListenerHandle s_roomStateListener;
    inline geode::ListenerHandle s_roomPlayersListener;
    inline bool s_registered = false;
    inline uint32_t s_globedRoomId = 0;
    inline std::unordered_map<int, RemotePlayer> s_roomMembers;

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
    inline bool s_levelModeValid = true;
    inline std::unordered_map<int, bool> s_rematchVotes;

    // connection bookkeeping for reconnection
    inline bool s_wasConnected = false;

    // local player state during a match
    inline uint32_t s_localDeaths = 0;
    inline bool s_localFinished = false;
    inline bool s_localEliminated = false;

    // ---- match alerts (toasts shown by the UI) -------------------------------
    struct Alert {
        std::string text;
        float age = 0.f;
    };
    inline std::deque<Alert> s_alerts;
    inline uint8_t s_lastCancelReason = 0;
    // Host migration: the Globed room owner at launch, plus a grace period
    // before a migrated host starts resolving the match.
    inline int s_hostAtLaunch = 0;
    inline float s_hostGrace = 0.f;

    // Queue preferences: casual or ranked, plus the difficulty band the
    // player wants to play. Room-scoped only — Globed's central server
    // forwards events inside one room, so there is no global queue.
    inline uint8_t s_queueType = 0;
    inline uint8_t s_difficultyTier = 0;

    inline void pushAlert(std::string const& text) {
        s_alerts.push_back({text, 0.f});
        while (s_alerts.size() > 6) s_alerts.pop_front();
    }

    inline std::vector<Alert> alerts() {
        std::vector<Alert> live;
        for (auto const& a : s_alerts) {
            if (a.age < 8.f) live.push_back(a);
        }
        return live;
    }
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

    inline char const* cancelReasonText(uint8_t reason) {
        switch (reason) {
            case 1: return "host cancelled";
            case 2: return "player left";
            case 3: return "disconnect";
            case 4: return "timeout";
            case 5: return "room closed";
            case 6: return "level invalid";
            default: return "unknown";
        }
    }

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
                p.versusActive = true;
                p.mode = d.mode;
                p.ready = d.ready;
                if (sender == hostAccountId() && VersusProto::validMode(d.mode)) s_mode = d.mode;
                p.queueType = d.queueType;
                p.difficultyTier = d.difficultyTier;
                p.lastSeen = s_clock;
                break;
            }
            case VersusProto::Action::Leave:
                s_players.erase(sender);
                s_rematchVotes.erase(sender);
                break;
            case VersusProto::Action::Launch: {
                // Only the room owner may launch a match.
                if (sender != hostAccountId() || !VersusProto::validMode(d.mode)) return;
                s_matchId = d.matchId;
                s_levelId = d.levelId;
                s_mode = d.mode;
                s_countdownTotal = d.countdownSeconds;
                s_countdown = static_cast<float>(d.countdownSeconds);
                s_phase = MatchPhase::Countdown;
                s_resultsRecorded = false;
                s_levelModeValid = true;
                s_rematchVotes.clear();
                s_hostAtLaunch = hostAccountId();
                s_hostGrace = 0.f;
                for (auto& [id, p] : s_players) {
                    p.percentX10 = 0;
                    p.deaths = 0;
                    p.finished = false;
                    p.eliminated = false;
                    p.disconnected = false;
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
                s_lastCancelReason = d.reason;
                pushAlert(fmt::format("MATCH CANCELLED  ({})", cancelReasonText(d.reason)));
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
                // Every client records the match exactly once, from the
                // host-computed verdict carried in the packet stream.
                if (!s_resultsRecorded && s_matchId != 0) {
                    recordMatchOutcome();
                }
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
            s_roomStateListener = globed::MessageEvent<globed::msg::RoomStateMessage>{false}.listen(
                [](const globed::msg::RoomStateMessage& message) {
                    s_globedRoomId = message.roomId;
                    s_roomMembers.clear();
                    for (auto const& member : message.players) {
                        int id = member.accountData.accountId;
                        if (id <= 0 || id == localAccountId()) continue;
                        RemotePlayer player;
                        player.accountId = id;
                        player.lastSeen = s_clock;
                        s_roomMembers[id] = player;
                    }
                }
            );
            s_roomPlayersListener = globed::MessageEvent<globed::msg::RoomPlayersMessage>{false}.listen(
                [](const globed::msg::RoomPlayersMessage& message) {
                    if (!globed::api::room::isInRoom()) {
                        s_globedRoomId = 0;
                        s_roomMembers.clear();
                        return;
                    }
                    s_globedRoomId = globed::api::room::getId();
                    s_roomMembers.clear();
                    for (auto const& member : message.players) {
                        int id = member.accountData.accountId;
                        if (id <= 0 || id == localAccountId()) continue;
                        RemotePlayer player;
                        player.accountId = id;
                        player.lastSeen = s_clock;
                        s_roomMembers[id] = player;
                    }
                }
            );
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
        send(VersusProto::encodeLobby(s_roomId, action, s_mode, s_ready,
            s_queueType, s_difficultyTier));
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
        if (!s_joined || !isHost() || !VersusProto::validMode(mode)) return;
        s_mode = mode;
        sendLobby(VersusProto::Action::Heartbeat);
    }

    // Queue preferences broadcast with every lobby packet.
    inline void setQueue(uint8_t queueType, uint8_t difficultyTier) {
        if (!s_joined) return;
        if (queueType > 1) return;
        if (difficultyTier > 10) return;
        s_queueType = queueType;
        s_difficultyTier = difficultyTier;
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
        s_levelModeValid = true;
        s_queueType = 0;
        s_difficultyTier = 0;
    }

    // ---- verdict recording ------------------------------------------------------

    // Build the pure-engine snapshot from the live packet state.
    inline VersusMatch::MatchSnapshot buildSnapshot() {
        VersusMatch::MatchSnapshot snapshot;
        snapshot.mode = s_mode;
        snapshot.levelId = s_levelId;
        snapshot.localAccountId = localAccountId();
        int self = localAccountId();
        bool selfPresent = false;
        for (auto const& [id, p] : s_players) {
            VersusMatch::PlayerResult r;
            r.accountId = p.accountId;
            r.finished = p.finished;
            r.eliminated = p.eliminated;
            r.disconnected = p.disconnected;
            r.deaths = p.deaths;
            r.percentX10 = p.percentX10;
            r.finishTimeMs = p.finishTimeMs;
            snapshot.players.push_back(r);
            if (p.accountId == self) selfPresent = true;
        }
        if (!selfPresent) {
            // The local player is always part of the snapshot.
            VersusMatch::PlayerResult r;
            r.accountId = self;
            r.finished = s_localFinished;
            r.eliminated = s_localEliminated;
            r.deaths = s_localDeaths;
            r.finishTimeMs = 0; // the engine only compares opponents' times
            snapshot.players.push_back(r);
        }
        return snapshot;
    }

    // Record the match locally: verdict, rank delta and history entry.
    // Runs exactly once per match on every client.
    inline void recordMatchOutcome() {
        if (s_resultsRecorded || s_matchId == 0) return;
        s_resultsRecorded = true;

        auto* info = VersusModes::find(s_mode);
        if (!info || !info->ranked || !s_levelModeValid) return; // warm-up or invalid level/mode

        auto snapshot = buildSnapshot();
        auto verdict = VersusMatch::computeVerdict(snapshot);
        if (!verdict.recordable) return;

        // Apply the rank delta locally (floored at zero by the engine).
        VersusStats::applyRankDelta(verdict.rankDelta);

        // History entry: opponent is the first other player in the snapshot.
        int opponentId = 0;
        for (auto const& p : snapshot.players) {
            if (p.accountId != localAccountId()) { opponentId = p.accountId; break; }
        }
        VersusMatch::MatchRecord record;
        record.timestamp = static_cast<int64_t>(std::time(nullptr));
        record.mode = s_mode;
        record.levelId = s_levelId;
        record.opponentId = opponentId;
        record.outcome = verdict.localOutcome;
        record.rankDelta = verdict.rankDelta;
        record.deaths = s_localDeaths;
        record.percentX10 = static_cast<uint16_t>(PlayLayer::get()
            ? static_cast<uint16_t>(PlayLayer::get() ? PlayLayer::get()->getCurrentPercentInt() : 0) : 0);
        VersusStats::appendRecord(record);
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
        if (s_phase != MatchPhase::InGame || s_localEliminated) return;
        s_localDeaths++;
        // Survival eliminates on death; other modes only cost time.
        if (auto* mode = VersusModes::find(s_mode); mode && mode->elimination) {
            s_localEliminated = true;
        }
        sendProgress(percentX10, s_localDeaths,
            VersusProto::kFlagDead | localProgressFlags());
    }

    inline void reportLocalProgress(uint16_t percentX10) {
        if (s_phase != MatchPhase::InGame || s_localFinished || s_localEliminated) return;
        sendProgress(percentX10, s_localDeaths, localProgressFlags());
    }

    inline void reportLocalFinish() {
        if (s_phase != MatchPhase::InGame || s_localFinished || s_localEliminated) return;
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
        if (!s_joined || !isHost() || !VersusProto::validMode(mode) || levelId <= 0) return;
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

        // Reconnection: re-announce after Globed reconnects, and keep the
        // player informed about what is happening with the transport.
        bool connected = Versus::globedApiAvailable() && globed::api::net::isConnected();
        if (!connected && s_wasConnected) {
            pushAlert("CONNECTION LOST — RECONNECTING");
        } else if (connected && !s_wasConnected) {
            pushAlert("RECONNECTED — REJOINING LOBBY");
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

        // ---- match resolution ---------------------------------------------------
        if (s_phase == MatchPhase::InGame) {
            // A match that cannot resolve is ended by timeout (10 minutes).
            float elapsed = s_clock - s_matchStartClock;
            bool timeout = elapsed > 600.f;
            if (isHost() && s_hostGrace <= 0.f) {
                auto snapshot = buildSnapshot();
                bool resolved = VersusMatch::computeVerdict(snapshot).recordable;
                if (!resolved && timeout) {
                    // End the match with whatever data exists.
                    s_phase = MatchPhase::Results;
                    sendResults(VersusProto::Verdict::Unrecorded, 0);
                    recordMatchOutcome();
                } else if (resolved) {
                    auto verdict = VersusMatch::computeVerdict(snapshot);
                    s_phase = MatchPhase::Results;
                    sendResults(static_cast<VersusProto::Verdict>(verdict.localOutcome),
                        verdict.winner);
                    recordMatchOutcome();
                }
            } else if (timeout) {
                // Non-host safety: never stay stuck in a dead match. Record
                // only what the engine can verify from the packet stream
                // (e.g. a forfeit win if every opponent disconnected).
                s_phase = MatchPhase::Results;
                recordMatchOutcome();
            }
        }

        // ---- rematch ------------------------------------------------------------
        if (s_phase == MatchPhase::Results && isHost()) {
            // When every connected player has voted yes, relaunch the same
            // level and mode with a fresh match id.
            bool allYes = !s_players.empty();
            for (auto const& [id, vote] : s_rematchVotes) {
                if (!vote) allYes = false;
            }
            for (auto const& [id, player] : s_players) {
                if (!s_rematchVotes.count(id)) allYes = false;
            }
            if (allYes && s_levelId > 0) {
                launchMatch(s_levelId, s_mode, 5);
            }
        }

        // ---- host migration ------------------------------------------------
        // If the room owner changes mid-match, the new owner takes over
        // match resolution after a short grace period, so it does not race
        // the old host's final packets. Verdicts are deterministic from the
        // shared packet stream, so any client computes the same result.
        if ((s_phase == MatchPhase::Countdown || s_phase == MatchPhase::InGame) &&
            s_hostAtLaunch != 0) {
            int currentHost = hostAccountId();
            if (currentHost != 0 && currentHost != s_hostAtLaunch) {
                s_hostAtLaunch = currentHost;
                s_hostGrace = 3.f;
                if (isHost()) {
                    pushAlert("HOST LEFT — YOU ARE NOW HOST");
                } else {
                    pushAlert(fmt::format("HOST MIGRATED TO PLAYER {}", currentHost));
                }
            }
        }
        if (s_hostGrace > 0.f) s_hostGrace -= dt;

        // Age out alerts.
        for (auto& a : s_alerts) a.age += dt;
        while (!s_alerts.empty() && s_alerts.front().age > 8.f) s_alerts.pop_front();

        // Drop players whose heartbeats expired. During an active match a
        // disconnected player stays in the snapshot (flagged) so the engine
        // can judge the match; outside a match they are removed outright.
        bool matchActive = s_matchId != 0 &&
            (s_phase == MatchPhase::Countdown || s_phase == MatchPhase::InGame);
        for (auto it = s_players.begin(); it != s_players.end();) {
            if (s_clock - it->second.lastSeen > 15.f) {
                if (matchActive && !it->second.disconnected) {
                    it->second.disconnected = true;
                    pushAlert(fmt::format("PLAYER {} DISCONNECTED", it->first));
                    ++it;
                    continue;
                }
                if (!matchActive) {
                    s_rematchVotes.erase(it->first);
                    it = s_players.erase(it);
                    continue;
                }
                ++it;
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
        std::unordered_map<int, RemotePlayer> combined;
        if (s_globedRoomId == s_roomId && s_roomId != 0) combined = s_roomMembers;
        for (auto const& [id, player] : s_players) {
            combined[id] = player;
            combined[id].versusActive = true;
        }
        int self = localAccountId();
        std::vector<RemotePlayer> result;
        result.reserve(combined.size());
        for (auto const& [id, player] : combined) {
            if (id != self) result.push_back(player);
        }
        return result;
    }

    inline uint8_t lastCancelReason() { return s_lastCancelReason; }
    inline uint8_t queueType() { return s_queueType; }
    inline uint8_t difficultyTier() { return s_difficultyTier; }

    inline size_t playerCount(uint8_t mode) {
        size_t count = 0;
        for (auto const& [id, player] : s_players) {
            if (player.mode == mode) ++count;
        }
        return count;
    }
}
