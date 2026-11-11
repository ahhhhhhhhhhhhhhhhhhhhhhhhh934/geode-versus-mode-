// Host unit tests for the pure ranked progression logic: seasons,
// divisions, and ranked challenges.

#include "core/GameModes.hpp"
#include "core/MatchLogic.hpp"
#include "core/Progression.hpp"

#include <cassert>
#include <cstdio>
#include <string>

using namespace VersusProgression;

static void testSeasons() {
    // The anchor itself is day one of season 1.
    auto s = seasonAt(kSeasonAnchor);
    assert(s.number == 1);
    assert(s.daysRemaining == 30);
    assert(s.endsAt == kSeasonAnchor + 30LL * 86400);

    // Day 29 is still season 1.
    s = seasonAt(kSeasonAnchor + 29LL * 86400);
    assert(s.number == 1);
    assert(s.daysRemaining == 1);

    // Day 30 starts season 2.
    s = seasonAt(kSeasonAnchor + 30LL * 86400);
    assert(s.number == 2);
    assert(s.daysRemaining == 30);

    // A timestamp before the anchor clamps to season 1.
    s = seasonAt(kSeasonAnchor - 100LL * 86400);
    assert(s.number == 1);
    assert(s.daysRemaining == 30);
}

static void testDivisions() {
    // Bronze covers the first three difficulty tiers.
    assert(divisionForTier(0)->name == std::string("BRONZE"));
    assert(divisionForTier(2)->name == std::string("BRONZE"));
    assert(divisionForTier(3)->name == std::string("SILVER"));
    assert(divisionForTier(5)->name == std::string("SILVER"));
    assert(divisionForTier(6)->name == std::string("GOLD"));
    assert(divisionForTier(8)->name == std::string("PLATINUM"));
    assert(divisionForTier(9)->name == std::string("DIAMOND"));
    assert(divisionForTier(10)->name == std::string("DIAMOND"));
    // Out-of-range indices have no division.
    assert(divisionForTier(-1) == nullptr);
    assert(divisionForTier(11) == nullptr);
}

static void testChallengeWindows() {
    // Day and week windows are deterministic floors.
    int64_t t = 1794000000;
    assert(dayFloor(t) == t - (t % 86400));
    assert(weekFloor(t) <= dayFloor(t));
    assert(dayFloor(t) - weekFloor(t) < 7LL * 86400);
}

static void testChallengeEvaluation() {
    int64_t now = 1794000000;
    std::vector<VersusMatch::MatchRecord> history;

    // Two ranked wins today complete the daily-wins challenge.
    for (int i = 0; i < 2; ++i) {
        history.push_back({now - 60, 0, 9001, 777, VersusMatch::Outcome::Win, 25, 0, 1000});
    }
    auto progress = evaluateChallenges(history, now);
    assert(progress.size() == kChallengeCount);
    for (auto const& p : progress) {
        if (p.challenge->id == std::string("daily-wins")) {
            assert(p.wins == 2);
            assert(p.complete);
        }
    }
    // Weekly challenge is not complete from two wins.
    for (auto const& p : progress) {
        if (p.challenge->id == std::string("weekly-wins")) {
            assert(!p.complete);
        }
    }

    // Losses and forfeits do not count toward challenges.
    std::vector<VersusMatch::MatchRecord> bad;
    bad.push_back({now - 60, 0, 9001, 777, VersusMatch::Outcome::Loss, -15, 3, 640});
    bad.push_back({now - 61, 0, 9001, 777, VersusMatch::Outcome::Forfeit, -15, 0, 0});
    for (auto const& p : evaluateChallenges(bad, now)) {
        if (p.challenge->daily) assert(p.wins == 0);
    }

    // Survival challenge requires the Survival mode (index 2).
    std::vector<VersusMatch::MatchRecord> survivalWin;
    survivalWin.push_back({now - 30, 2, 9001, 777, VersusMatch::Outcome::Win, 25, 0, 1000});
    for (auto const& p : evaluateChallenges(survivalWin, now)) {
        if (p.challenge->id == std::string("daily-survival")) assert(p.complete);
    }

    // Wins from before the daily window do not count.
    std::vector<VersusMatch::MatchRecord> old;
    old.push_back({dayFloor(now) - 86400, 0, 9001, 777, VersusMatch::Outcome::Win, 25, 0, 1000});
    for (auto const& p : evaluateChallenges(old, now)) {
        if (p.challenge->daily) assert(p.wins == 0);
    }

    // Completed challenges contribute their bonus points.
    int bonus = 0;
    for (auto const& p : progress) {
        if (p.complete) bonus += p.challenge->bonusPoints;
    }
    assert(bonus >= 10);
}

int main() {
    testSeasons();
    testDivisions();
    testChallengeWindows();
    testChallengeEvaluation();
    std::printf("test_progression: all tests passed\n");
    return 0;
}
