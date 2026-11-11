#pragma once

// Local persistence for Versus match results, statistics and history.
//
// Everything here is stored through Geode saved values and is explicitly
// LOCAL: it records what this client verified about its own matches.
// There is no Versus server, so nothing here is a global ranking.
// Record keeping uses the one-line record codec from MatchLogic.hpp.

#include <Geode/Geode.hpp>
#include "MatchLogic.hpp"

#include <string>
#include <vector>

using namespace geode::prelude;

namespace VersusStats {
    inline constexpr size_t kMaxHistoryEntries = 100;

    // ---- rank points ----------------------------------------------------------

    inline int rankPoints() {
        return std::max(0, Mod::get()->getSavedValue<int>("rank-points", 0));
    }

    inline void setRankPoints(int points) {
        Mod::get()->setSavedValue("rank-points", std::max(0, points));
    }

    inline int applyRankDelta(int delta) {
        int next = VersusMatch::applyRankDelta(rankPoints(), delta);
        setRankPoints(next);
        return next;
    }

    // ---- match history ----------------------------------------------------------

    inline std::vector<VersusMatch::MatchRecord> history() {
        std::string blob = Mod::get()->getSavedValue<std::string>("match-history", "");
        std::vector<VersusMatch::MatchRecord> records;
        size_t start = 0;
        while (start <= blob.size() && records.size() < kMaxHistoryEntries) {
            size_t newline = blob.find('\n', start);
            std::string line = newline == std::string::npos
                ? blob.substr(start) : blob.substr(start, newline - start);
            if (auto r = VersusMatch::decodeRecord(line)) records.push_back(*r);
            if (newline == std::string::npos) break;
            start = newline + 1;
        }
        return records;
    }

    inline void appendRecord(VersusMatch::MatchRecord const& record) {
        auto records = history();
        records.push_back(record);
        // Keep only the newest entries.
        if (records.size() > kMaxHistoryEntries) {
            records.erase(records.begin(), records.end() - kMaxHistoryEntries);
        }
        std::string blob;
        for (size_t i = 0; i < records.size(); ++i) {
            if (i > 0) blob += '\n';
            blob += VersusMatch::encodeRecord(records[i].timestamp, records[i].mode,
                records[i].levelId, records[i].opponentId, records[i].outcome,
                records[i].rankDelta, records[i].deaths, records[i].percentX10);
        }
        Mod::get()->setSavedValue("match-history", blob);
    }

    // ---- aggregate statistics ------------------------------------------------------

    struct PlayerStats {
        int wins = 0;
        int losses = 0;
        int forfeits = 0;
        int matches = 0;
        int rankPoints = 0;
        std::vector<int> winsPerMode = std::vector<int>(VersusModes::kModeCount + 1, 0);
        std::vector<int> matchesPerMode = std::vector<int>(VersusModes::kModeCount + 1, 0);
    };

    inline PlayerStats stats() {
        PlayerStats s;
        s.rankPoints = rankPoints();
        for (auto const& r : history()) {
            if (!VersusModes::find(r.mode)) continue;
            s.matches++;
            s.matchesPerMode[std::min<size_t>(r.mode, VersusModes::kModeCount)]++;
            switch (r.outcome) {
                case VersusMatch::Outcome::Win:
                    s.wins++;
                    s.winsPerMode[std::min<size_t>(r.mode, VersusModes::kModeCount)]++;
                    break;
                case VersusMatch::Outcome::Loss:
                    s.losses++;
                    break;
                case VersusMatch::Outcome::Forfeit:
                    s.forfeits++;
                    s.wins++; // the opponent left; the win is still verified
                    break;
                default:
                    break;
            }
        }
        return s;
    }

    inline int winRatePercent(PlayerStats const& s) {
        if (s.matches == 0) return 0;
        return static_cast<int>((s.wins * 100) / s.matches);
    }
}
