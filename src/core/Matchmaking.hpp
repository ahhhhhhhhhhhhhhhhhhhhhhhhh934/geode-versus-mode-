#pragma once

// Room-scoped matchmaking: pairing suggestions from the queue preferences
// that players broadcast with their lobby packets. Pure logic, unit-tested
// on the host exactly as compiled.
//
// Scope note: Globed's central server forwards events only inside the
// sender's room, so there is no global queue. Everything here operates on
// the players already in the same room — see
// docs/GLOBED_NATIVE_MATCHMAKING_INVESTIGATION.md.

#include <cstdint>
#include <optional>
#include <vector>

namespace VersusMatchmaking {
    // A player's broadcast queue preference.
    struct QueuePlayer {
        int accountId = 0;
        uint8_t queueType = 0;     // 0 casual, 1 ranked
        uint8_t difficultyTier = 0; // 0..10, difficulty ladder index
    };

    // A suggested 1v1 pairing.
    struct Pairing {
        int a = 0;
        int b = 0;
        uint8_t difficultyTier = 0; // shared band to play
        bool crossTier = false;     // players were one tier apart
    };

    // Maximum tier distance for a ranked suggestion.
    inline constexpr uint8_t kMaxTierDistance = 1;

    // Suggest 1v1 pairings from the room's preferences. Casual players
    // pair with anyone casual (tier ignored); ranked players pair within
    // the maximum tier distance, closest tiers first. Every player is
    // used at most once. Players without a partner are simply omitted.
    inline std::vector<Pairing> suggestPairings(std::vector<QueuePlayer> const& players) {
        std::vector<Pairing> result;

        std::vector<QueuePlayer> casual;
        std::vector<QueuePlayer> ranked;
        for (auto const& p : players) {
            if (p.accountId == 0) continue;
            if (p.queueType == 1) ranked.push_back(p); else casual.push_back(p);
        }

        // Casual: first-come pairing, difficulty ignored.
        for (size_t i = 0; i + 1 < casual.size(); i += 2) {
            result.push_back({casual[i].accountId, casual[i + 1].accountId,
                casual[i].difficultyTier, false});
        }

        // Ranked: pair closest tiers first.
        std::vector<size_t> remaining(ranked.size());
        for (size_t i = 0; i < ranked.size(); ++i) remaining[i] = i;
        bool matched = true;
        while (remaining.size() >= 2 && matched) {
            matched = false;
            for (size_t i = 0; i < remaining.size() && !matched; ++i) {
                for (size_t j = i + 1; j < remaining.size(); ++j) {
                    uint8_t ta = ranked[remaining[i]].difficultyTier;
                    uint8_t tb = ranked[remaining[j]].difficultyTier;
                    uint8_t distance = ta > tb ? static_cast<uint8_t>(ta - tb)
                                               : static_cast<uint8_t>(tb - ta);
                    if (distance <= kMaxTierDistance) {
                        result.push_back({ranked[remaining[i]].accountId,
                            ranked[remaining[j]].accountId,
                            static_cast<uint8_t>((ta + tb) / 2),
                            distance == 1});
                        remaining.erase(remaining.begin() + j);
                        remaining.erase(remaining.begin() + i);
                        matched = true;
                        break;
                    }
                }
            }
        }
        return result;
    }

    // The difficulty band name for a suggestion, when the caller wants to
    // render it (the ladder itself lives in GameModes.hpp).
    inline uint8_t suggestMode() {
        // 1v1 Race is the default suggested mode for pairings.
        return 1;
    }
}
