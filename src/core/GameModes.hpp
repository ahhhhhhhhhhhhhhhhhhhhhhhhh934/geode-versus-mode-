#pragma once

// The six Versus game modes and the Geometry Dash difficulty categories.
// Pure data + pure rules helpers: no Geode/Globed includes, so this header
// is unit-tested on the host exactly as it compiles on Android.

#include <cstdint>
#include <optional>
#include <string_view>

namespace VersusModes {
    // Mode ids are stable protocol values (VersusProto::validMode range).
    enum class Mode : uint8_t {
        Race1v1 = 1,          // 1v1 Race — two players race the same level
        Survival = 2,         // Survival — last player alive wins
        DualChallenge = 3,    // Dual Challenge — cooperative completion
        PlatformerBattle = 4, // Platformer Battle — race in platformer levels
        ClassicRace = 5,      // Classic Level Race — race through classic levels
        Warmup = 6,           // Warm-up Room — practice, nothing recorded
    };

    inline constexpr size_t kModeCount = 6;

    struct ModeInfo {
        uint8_t id;
        char const* name;
        char const* tagline;
        char const* rules;
        bool ranked;         // verified wins/losses and rank points
        bool cooperative;    // team verdict: both players share the result
        bool elimination;    // dying eliminates the player
        bool requiresPlatformer; // the level must be a platformer level
    };

    inline ModeInfo const kModes[kModeCount] = {
        {1, "1V1 RACE", "TWO PLAYERS  •  ONE LEVEL  •  FIRST TO FINISH",
            "Both players run the same level. The first player to complete it wins. Deaths do not eliminate you, but every death costs time.",
            true, false, false, false},
        {2, "SURVIVAL", "LAST PLAYER ALIVE WINS",
            "Dying eliminates you from the match. The last player alive wins. If everyone is eliminated, the player with the best progress wins.",
            true, false, true, false},
        {3, "DUAL CHALLENGE", "COOPERATIVE  •  FINISH IT TOGETHER",
            "A team mode: both players must complete the level. The match is won only when every player finishes; combined deaths decide ties.",
            true, true, false, false},
        {4, "PLATFORMER BATTLE", "RACE THROUGH PLATFORMER LEVELS",
            "A race in Geometry Dash platformer levels. The first player to reach the end of the platformer level wins.",
            true, false, false, true},
        {5, "CLASSIC LEVEL RACE", "RACE THROUGH CLASSIC LEVELS",
            "A race through classic (non-platformer) levels. The first player to complete the level wins.",
            true, false, false, false},
        {6, "WARM-UP ROOM", "PRACTICE  •  NOTHING RECORDED",
            "A casual room for practicing before real matches. No wins, losses or rank points are recorded.",
            false, false, false, false},
    };

    inline ModeInfo const* find(uint8_t id) {
        for (auto const& m : kModes) {
            if (m.id == id) return &m;
        }
        return nullptr;
    }

    inline ModeInfo const* find(Mode m) {
        return find(static_cast<uint8_t>(m));
    }

    inline char const* name(uint8_t id) {
        auto* m = find(id);
        return m ? m->name : "UNKNOWN";
    }

    inline bool isRanked(uint8_t id) {
        auto* m = find(id);
        return m && m->ranked;
    }

    // ---- difficulty categories (Auto through Extreme Demon) ---------------

    // The official Geometry Dash difficulty categories, aligned with the rank
    // ladder thresholds used by the ranked progression system.
    struct DifficultyTier {
        char const* name;
        int minimumPoints;
    };

    inline DifficultyTier const kDifficulties[] = {
        {"AUTO", 0},
        {"EASY", 100},
        {"NORMAL", 250},
        {"HARD", 450},
        {"HARDER", 700},
        {"INSANE", 1000},
        {"EASY DEMON", 1400},
        {"MEDIUM DEMON", 1850},
        {"HARD DEMON", 2350},
        {"INSANE DEMON", 2900},
        {"EXTREME DEMON", 3500},
    };

    inline constexpr size_t kDifficultyCount = sizeof(kDifficulties) / sizeof(kDifficulties[0]);

    inline DifficultyTier const* difficultyAt(size_t index) {
        if (index >= kDifficultyCount) return nullptr;
        return &kDifficulties[index];
    }

    // The difficulty category whose point threshold a player with this many
    // rank points is currently playing at. Unranked players start at Auto.
    inline DifficultyTier const* difficultyForPoints(int points) {
        DifficultyTier const* current = &kDifficulties[0];
        for (auto const& d : kDifficulties) {
            if (points >= d.minimumPoints) current = &d;
        }
        return current;
    }
}
