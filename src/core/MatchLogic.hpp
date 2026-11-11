#pragma once

// The pure Versus match engine: verdict computation per game mode, rank
// point deltas, and the match-record codec used by the history store.
// No Geode/Globed includes — unit-tested on the host exactly as compiled.

#include "GameModes.hpp"
#include "Protocol.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace VersusMatch {
    // A player's state at the moment a verdict is computed.
    struct PlayerResult {
        int accountId = 0;
        bool finished = false;
        bool eliminated = false;
        bool disconnected = false;
        uint32_t deaths = 0;
        uint16_t percentX10 = 0;
        uint32_t finishTimeMs = 0;
    };

    // Everything the engine needs to judge a match. Built by the transport
    // from the packet stream when the match ends.
    struct MatchSnapshot {
        uint8_t mode = 0;
        int levelId = 0;
        int localAccountId = 0;
        std::vector<PlayerResult> players;
    };

    enum class Outcome : uint8_t {
        Win = 1,
        Loss = 2,
        Draw = 3,
        Forfeit = 4,     // opponent disconnected before the match resolved
        Unrecorded = 5,  // warm-up, or not enough data to judge
    };

    struct Verdict {
        Outcome localOutcome = Outcome::Unrecorded;
        int32_t winner = 0;
        bool recordable = false;
        int rankDelta = 0;
        char const* summary = "";
    };

    // Rank progression: fixed, transparent deltas. No wagering, no losing
    // points as stakes — a loss simply costs progression, floored at zero.
    inline constexpr int kWinPoints = 25;
    inline constexpr int kLossPoints = -15;
    inline constexpr int kDrawPoints = 0;

    inline int applyRankDelta(int currentPoints, int delta) {
        int next = currentPoints + delta;
        return next < 0 ? 0 : next;
    }

    // ---- verdict computation ------------------------------------------------

    inline Verdict computeVerdict(MatchSnapshot const& snapshot) {
        Verdict v;
        auto* info = VersusModes::find(snapshot.mode);
        if (!info) {
            v.summary = "unknown mode";
            return v;
        }

        // Warm-up rooms record nothing, by design.
        if (!info->ranked) {
            v.summary = "warm-up: nothing recorded";
            return v;
        }

        // A match needs at least one opponent (even a disconnected one —
        // leaving mid-match is handled by each mode's rules below).
        size_t opponents = 0;
        for (auto const& p : snapshot.players) {
            if (p.accountId != snapshot.localAccountId) ++opponents;
        }
        if (opponents < 1) {
            v.summary = "not enough participants";
            return v;
        }

        auto byId = [&](int id) -> PlayerResult const* {
            for (auto const& p : snapshot.players) {
                if (p.accountId == id) return &p;
            }
            return nullptr;
        };
        auto local = byId(snapshot.localAccountId);

        // ---- cooperative modes: Dual Challenge --------------------------------
        if (info->cooperative) {
            bool allFinished = true;
            for (auto const& p : snapshot.players) {
                if (!p.disconnected && !p.finished) allFinished = false;
            }
            if (allFinished) {
                v.localOutcome = Outcome::Win;
                v.winner = 0; // team result
                v.recordable = true;
                v.rankDelta = kWinPoints;
                v.summary = "team completed the challenge";
            } else {
                v.localOutcome = Outcome::Loss;
                v.recordable = false;
                v.summary = "team did not finish";
            }
            return v;
        }

        // ---- elimination modes: Survival --------------------------------------
        if (info->elimination) {
            std::vector<PlayerResult const*> alive;
            for (auto const& p : snapshot.players) {
                if (!p.disconnected && !p.eliminated) alive.push_back(&p);
            }
            if (alive.size() == 1) {
                v.winner = alive[0]->accountId;
                v.localOutcome = alive[0]->accountId == snapshot.localAccountId
                    ? Outcome::Win : Outcome::Loss;
                v.recordable = true;
                v.rankDelta = alive[0]->accountId == snapshot.localAccountId
                    ? kWinPoints : kLossPoints;
                v.summary = "last player standing";
            } else if (alive.empty()) {
                // Everyone eliminated: best progress wins.
                PlayerResult const* best = nullptr;
                for (auto const& p : snapshot.players) {
                    if (p.disconnected) continue;
                    if (!best || p.percentX10 > best->percentX10 ||
                        (p.percentX10 == best->percentX10 && p.deaths < best->deaths)) {
                        best = &p;
                    }
                }
                if (best) {
                    v.winner = best->accountId;
                    v.localOutcome = best->accountId == snapshot.localAccountId
                        ? Outcome::Win : Outcome::Loss;
                    v.recordable = true;
                    v.rankDelta = best->accountId == snapshot.localAccountId
                        ? kWinPoints : kLossPoints;
                    v.summary = "all eliminated: best progress wins";
                } else {
                    v.summary = "no valid results";
                }
            } else {
                v.summary = "match still running";
            }
            return v;
        }

        // ---- race modes: 1v1 Race, Platformer Battle, Classic Level Race -------
        std::vector<PlayerResult const*> finishers;
        for (auto const& p : snapshot.players) {
            if (!p.disconnected && p.finished) finishers.push_back(&p);
        }
        if (!finishers.empty()) {
            std::sort(finishers.begin(), finishers.end(),
                [](PlayerResult const* a, PlayerResult const* b) {
                    if (a->finishTimeMs != b->finishTimeMs) return a->finishTimeMs < b->finishTimeMs;
                    return a->deaths < b->deaths;
                });
            v.winner = finishers[0]->accountId;
            v.localOutcome = finishers[0]->accountId == snapshot.localAccountId
                ? Outcome::Win : Outcome::Loss;
            v.recordable = true;
            v.rankDelta = finishers[0]->accountId == snapshot.localAccountId
                ? kWinPoints : kLossPoints;
            v.summary = "first to finish";
        } else {
            // No finishers: if every opponent disconnected while the local
            // player was still active, the local player wins by forfeit.
            bool opponentLeft = true;
            for (auto const& p : snapshot.players) {
                if (p.accountId == snapshot.localAccountId) continue;
                if (!p.disconnected) opponentLeft = false;
            }
            bool localWasActive = local && !local->disconnected;
            if (opponentLeft && localWasActive) {
                v.localOutcome = Outcome::Forfeit;
                v.winner = snapshot.localAccountId;
                v.recordable = true;
                v.rankDelta = kWinPoints;
                v.summary = "opponent left: win by forfeit";
            } else {
                v.summary = "no finishers";
            }
        }
        return v;
    }

    // ---- match record codec (history store) ---------------------------------
    //
    // Records are single lines of pipe-separated fields so they can be kept
    // in one Geode saved string without a JSON dependency:
    //   timestamp|mode|levelId|opponentId|outcome|rankDelta|deaths|percentX10
    inline std::string encodeRecord(int64_t timestamp, uint8_t mode, int levelId,
        int opponentId, Outcome outcome, int rankDelta, uint32_t deaths, uint16_t percentX10) {
        auto n = [](auto v) {
            std::string s;
            bool negative = v < 0;
            auto u = static_cast<uint64_t>(negative ? -v : v);
            if (u == 0) s = "0";
            while (u > 0) {
                s = std::string(1, static_cast<char>('0' + static_cast<int>(u % 10))) + s;
                u /= 10;
            }
            return negative ? "-" + s : s;
        };
        return n(timestamp) + "|" + n(mode) + "|" + n(levelId) + "|" + n(opponentId) + "|" +
            n(static_cast<int>(outcome)) + "|" + n(rankDelta) + "|" + n(deaths) + "|" + n(percentX10);
    }

    struct MatchRecord {
        int64_t timestamp = 0;
        uint8_t mode = 0;
        int levelId = 0;
        int opponentId = 0;
        Outcome outcome = Outcome::Unrecorded;
        int rankDelta = 0;
        uint32_t deaths = 0;
        uint16_t percentX10 = 0;
    };

    inline std::optional<MatchRecord> decodeRecord(std::string const& line) {
        if (line.empty()) return std::nullopt;

        // Exactly 8 fields means exactly 7 separators — no more, no fewer.
        size_t separators = 0;
        for (char c : line) if (c == '|') ++separators;
        if (separators != 7) return std::nullopt;

        // Split on '|' into exactly 8 numeric fields.
        int64_t fields[8];
        int seen = 0;
        size_t start = 0;
        while (seen < 8) {
            size_t bar = line.find('|', start);
            std::string field = bar == std::string::npos
                ? line.substr(start) : line.substr(start, bar - start);
            if (field.empty()) return std::nullopt;
            bool negative = field[0] == '-';
            int64_t value = 0;
            for (size_t i = negative ? 1 : 0; i < field.size(); ++i) {
                if (field[i] < '0' || field[i] > '9') return std::nullopt;
                value = value * 10 + (field[i] - '0');
            }
            fields[seen++] = negative ? -value : value;
            if (bar == std::string::npos) break;
            start = bar + 1;
        }
        if (seen != 8) return std::nullopt;

        MatchRecord r;
        r.timestamp = fields[0];
        r.mode = static_cast<uint8_t>(fields[1]);
        r.levelId = static_cast<int>(fields[2]);
        r.opponentId = static_cast<int>(fields[3]);
        r.outcome = static_cast<Outcome>(fields[4]);
        r.rankDelta = static_cast<int>(fields[5]);
        r.deaths = static_cast<uint32_t>(fields[6]);
        r.percentX10 = static_cast<uint16_t>(fields[7]);
        if (r.mode < 1 || r.mode > 6) return std::nullopt;
        if (static_cast<int>(r.outcome) < 1 || static_cast<int>(r.outcome) > 5) return std::nullopt;
        return r;
    }
}
