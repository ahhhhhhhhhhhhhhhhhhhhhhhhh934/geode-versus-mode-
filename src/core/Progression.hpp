#pragma once

// Ranked progression: seasons, division ladders and ranked challenges.
// Pure logic — unit-tested on the host exactly as compiled. The store that
// persists points and history lives in StatsStore.hpp (Geode side).
//
// Ranked challenges are objectives only: complete them to earn bonus rank
// points. They never involve gambling, wagers, real money, or losing rank
// points as stakes.

#include "GameModes.hpp"
#include "MatchLogic.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace VersusProgression {
    // ---- seasons ------------------------------------------------------------

    // A season is a fixed 30-day period counted from the mod's anchor date.
    // Seasons are local until a Versus service exists to run them globally.
    inline constexpr int64_t kSeasonLengthDays = 30;
    // Anchor: 2026-11-01T00:00:00Z
    inline constexpr int64_t kSeasonAnchor = 1793577600;

    struct SeasonInfo {
        int number = 1;
        int daysRemaining = 0;
        int64_t endsAt = 0;
    };

    inline SeasonInfo seasonAt(int64_t timestamp) {
        int64_t secondsPerDay = 86400;
        int64_t elapsed = timestamp - kSeasonAnchor;
        if (elapsed < 0) elapsed = 0;
        int64_t day = elapsed / secondsPerDay;
        int season = static_cast<int>(day / kSeasonLengthDays) + 1;
        int64_t seasonEndDay = (season - 1) * kSeasonLengthDays + kSeasonLengthDays;
        int64_t remaining = seasonEndDay - day;
        SeasonInfo info;
        info.number = season;
        info.daysRemaining = static_cast<int>(remaining);
        info.endsAt = kSeasonAnchor + seasonEndDay * secondsPerDay;
        return info;
    }

    // ---- divisions ------------------------------------------------------------

    // Divisions group the difficulty ladder into broader competitive bands.
    struct Division {
        char const* name;
        int minimumTier; // index into VersusModes::kDifficulties
        int maximumTier;
    };

    inline Division const kDivisions[] = {
        {"BRONZE", 0, 2},    // Auto, Easy, Normal
        {"SILVER", 3, 5},   // Hard, Harder, Insane
        {"GOLD", 6, 7},     // Easy Demon, Medium Demon
        {"PLATINUM", 8, 8}, // Hard Demon
        {"DIAMOND", 9, 10}, // Insane Demon, Extreme Demon
    };

    inline constexpr size_t kDivisionCount = sizeof(kDivisions) / sizeof(kDivisions[0]);

    inline Division const* divisionForTier(int tierIndex) {
        for (auto const& d : kDivisions) {
            if (tierIndex >= d.minimumTier && tierIndex <= d.maximumTier) return &d;
        }
        return nullptr;
    }

    // ---- ranked challenges ------------------------------------------------------

    // Challenges are computed from the recorded match history: no separate
    // progress store, no fabrication. All rewards are bonus rank points.
    struct Challenge {
        char const* id;
        char const* title;
        char const* description;
        int targetWins;
        int bonusPoints;
        bool daily; // daily resets each day; weekly each 7 days
    };

    inline Challenge const kChallenges[] = {
        {"daily-wins", "DAILY: WIN 2 RANKED MATCHES", "Win any two ranked matches today.", 2, 10, true},
        {"daily-survival", "DAILY: SURVIVOR", "Win one Survival match today.", 1, 5, true},
        {"weekly-wins", "WEEKLY: WIN 7 RANKED MATCHES", "Win seven ranked matches this week.", 7, 50, false},
    };

    inline constexpr size_t kChallengeCount = sizeof(kChallenges) / sizeof(kChallenges[0]);

    inline int64_t dayFloor(int64_t timestamp) {
        return timestamp - (timestamp % 86400);
    }

    inline int64_t weekFloor(int64_t timestamp) {
        return dayFloor(timestamp) - ((timestamp / 86400 % 7) * 86400);
    }

    struct ChallengeProgress {
        Challenge const* challenge = nullptr;
        int wins = 0;
        bool complete = false;
    };

    // Evaluate challenge progress from the local match history.
    inline std::vector<ChallengeProgress> evaluateChallenges(
        std::vector<VersusMatch::MatchRecord> const& history, int64_t now) {
        std::vector<ChallengeProgress> result;
        int64_t windowStart = dayFloor(now);
        for (auto const& c : kChallenges) {
            ChallengeProgress p;
            p.challenge = &c;
            int64_t start = c.daily ? windowStart : weekFloor(now);
            for (auto const& r : history) {
                if (r.timestamp < start) continue;
                if (r.outcome != VersusMatch::Outcome::Win) continue;
                if (std::string_view(c.id) == "daily-survival" && r.mode != 2) continue;
                if (std::string_view(c.id) != "daily-survival" && c.daily && r.mode == 6) continue;
                ++p.wins;
            }
            p.complete = p.wins >= c.targetWins;
            result.push_back(p);
        }
        return result;
    }

    // Bonus points earned from completed challenges, for the ranked page.
    inline int pendingBonusPoints(std::vector<ChallengeProgress> const& progress) {
        // Bonus points are granted when the challenge completes, once per
        // window; the grant itself is persisted by the Geode side.
        int total = 0;
        for (auto const& p : progress) {
            if (p.complete) total += p.challenge->bonusPoints;
        }
        return total;
    }
}
