#pragma once

// Local achievements derived only from this device's recorded match history.
// These are personal milestones, not globally verified or transferable rewards.

#include "MatchLogic.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace VersusAchievements {
    struct Achievement {
        char const* id;
        char const* title;
        char const* description;
        bool unlocked;
    };

    inline std::array<Achievement, 6> evaluate(
        std::vector<VersusMatch::MatchRecord> const& history
    ) {
        int wins = 0;
        uint32_t rankedModeMask = 0;
        int survivalWins = 0;
        int platformerWins = 0;

        for (auto const& record : history) {
            auto* mode = VersusModes::find(record.mode);
            if (!mode || !mode->ranked) continue;

            bool won = record.outcome == VersusMatch::Outcome::Win ||
                record.outcome == VersusMatch::Outcome::Forfeit;
            if (!won) continue;

            ++wins;
            if (record.mode >= 1 && record.mode <= 5) {
                rankedModeMask |= (1u << (record.mode - 1));
            }
            if (record.mode == static_cast<uint8_t>(VersusModes::Mode::Survival)) {
                ++survivalWins;
            }
            if (record.mode == static_cast<uint8_t>(VersusModes::Mode::PlatformerBattle)) {
                ++platformerWins;
            }
        }

        int matches = 0;
        for (auto const& record : history) {
            auto* mode = VersusModes::find(record.mode);
            if (mode && mode->ranked &&
                record.outcome != VersusMatch::Outcome::Unrecorded) {
                ++matches;
            }
        }

        int distinctModes = 0;
        for (uint32_t bits = rankedModeMask; bits != 0; bits >>= 1) {
            distinctModes += static_cast<int>(bits & 1u);
        }

        return {{
            {"first-match", "FIRST MATCH", "Complete your first recorded ranked match.", matches >= 1},
            {"first-win", "FIRST VICTORY", "Win your first recorded ranked match.", wins >= 1},
            {"ten-matches", "GETTING SERIOUS", "Complete 10 recorded ranked matches.", matches >= 10},
            {"mode-explorer", "MODE EXPLORER", "Win in four different ranked modes.", distinctModes >= 4},
            {"survival-win", "LAST ONE STANDING", "Win a Survival match.", survivalWins >= 1},
            {"platformer-win", "PLATFORMER ACE", "Win a Platformer Battle.", platformerWins >= 1},
        }};
    }

    inline size_t unlockedCount(std::array<Achievement, 6> const& achievements) {
        size_t count = 0;
        for (auto const& achievement : achievements) {
            if (achievement.unlocked) ++count;
        }
        return count;
    }
}
