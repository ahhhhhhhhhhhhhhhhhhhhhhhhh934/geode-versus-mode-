// Host unit tests for room-scoped matchmaking pairing.

#include "core/Matchmaking.hpp"

#include <cstdio>

static int g_failures = 0;
#define CHECK(cond) do { \
    if (!(cond)) { std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); ++g_failures; } \
} while (0)

using namespace VersusMatchmaking;

static void testCasualPairing() {
    std::vector<QueuePlayer> players = {
        {1, 0, 2}, {2, 0, 7}, // casual, very different tiers — still paired
        {3, 1, 5},
    };
    auto pairs = suggestPairings(players);
    CHECK(pairs.size() == 1);
    if (!pairs.empty()) {
        CHECK(pairs[0].a == 1 && pairs[0].b == 2);
        CHECK(!pairs[0].crossTier);
    }
}

static void testRankedCloseTiers() {
    std::vector<QueuePlayer> players = {
        {10, 1, 4}, {11, 1, 4}, // same tier
        {12, 1, 5},             // one tier away
        {20, 1, 9}, {21, 1, 9},
    };
    auto pairs = suggestPairings(players);
    CHECK(pairs.size() == 2);
    bool foundSame = false, foundCross = false, foundFar = false;
    for (auto const& p : pairs) {
        if (p.a == 10 && p.b == 11) foundSame = true;
        if (p.a == 12 && (p.b == 10 || p.b == 11)) foundCross = true;
        if (p.a == 12 && (p.b == 20 || p.b == 21)) foundFar = true;
    }
    CHECK(foundSame);
    CHECK(!foundCross); // 10/11 pair first; 12 pairs with nobody close
    CHECK(!foundFar);
}

static void testNoCrossQueuePairing() {
    // A casual player never pairs with a ranked player.
    std::vector<QueuePlayer> players = {
        {1, 0, 3}, {2, 1, 3},
    };
    auto pairs = suggestPairings(players);
    CHECK(pairs.empty());
}

static void testOddCounts() {
    std::vector<QueuePlayer> players = {
        {1, 1, 5}, {2, 1, 5}, {3, 1, 5},
    };
    auto pairs = suggestPairings(players);
    CHECK(pairs.size() == 1);
    int seen[4] = {0, 0, 0, 0};
    for (auto const& p : pairs) { seen[p.a] = 1; seen[p.b] = 1; }
    CHECK(seen[1] + seen[2] + seen[3] == 2); // exactly two of the three
}

static void testEmptyAndInvalid() {
    CHECK(suggestPairings({}).empty());
    CHECK(suggestPairings({{0, 1, 5}, {0, 1, 5}}).empty()); // invalid ids
    CHECK(suggestMode() == 1);
}

int main() {
    testCasualPairing();
    testEmptyAndInvalid();
    testRankedCloseTiers();
    testNoCrossQueuePairing();
    testOddCounts();
    if (g_failures == 0) {
        std::printf("All matchmaking tests passed.\n");
        return 0;
    }
    std::printf("%d matchmaking test(s) failed.\n", g_failures);
    return 1;
}
