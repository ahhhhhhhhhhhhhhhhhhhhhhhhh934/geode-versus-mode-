// Host-side unit tests for the Versus match engine.
#include "../src/core/MatchLogic.hpp"

#include <cstdio>

static int g_failures = 0;

#define CHECK(cond) do { \
    if (!(cond)) { std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); ++g_failures; } \
} while (0)

using namespace VersusMatch;

static PlayerResult player(int id, bool finished, uint32_t timeMs, uint32_t deaths,
    uint16_t percent, bool eliminated = false, bool disconnected = false) {
    PlayerResult p;
    p.accountId = id;
    p.finished = finished;
    p.finishTimeMs = timeMs;
    p.deaths = deaths;
    p.percentX10 = percent;
    p.eliminated = eliminated;
    p.disconnected = disconnected;
    return p;
}

static void testRaceFirstFinishWins() {
    for (uint8_t mode : {1, 4, 5}) { // 1v1 Race, Platformer Battle, Classic Race
        MatchSnapshot s;
        s.mode = mode;
        s.localAccountId = 10;
        s.players = {player(10, true, 30000, 3, 1000), player(20, true, 25000, 5, 1000)};
        auto v = computeVerdict(s);
        CHECK(v.winner == 20);
        CHECK(v.localOutcome == Outcome::Loss);
        CHECK(v.recordable);
        CHECK(v.rankDelta == kLossPoints);

        s.players = {player(10, true, 20000, 0, 1000), player(20, false, 0, 7, 430)};
        v = computeVerdict(s);
        CHECK(v.winner == 10);
        CHECK(v.localOutcome == Outcome::Win);
        CHECK(v.rankDelta == kWinPoints);
    }
}

static void testRaceTieBrokenByDeaths() {
    MatchSnapshot s;
    s.mode = 1;
    s.localAccountId = 10;
    s.players = {player(10, true, 30000, 2, 1000), player(20, true, 30000, 4, 1000)};
    auto v = computeVerdict(s);
    CHECK(v.winner == 10);
    CHECK(v.localOutcome == Outcome::Win);
}

static void testRaceNoFinishersUnrecorded() {
    MatchSnapshot s;
    s.mode = 1;
    s.localAccountId = 10;
    s.players = {player(10, false, 0, 2, 500), player(20, false, 0, 1, 600)};
    auto v = computeVerdict(s);
    CHECK(!v.recordable);
    CHECK(v.localOutcome == Outcome::Unrecorded);
}

static void testForfeitOnDisconnect() {
    MatchSnapshot s;
    s.mode = 1;
    s.localAccountId = 10;
    s.players = {player(10, false, 0, 0, 350), player(20, false, 0, 3, 400, false, true)};
    auto v = computeVerdict(s);
    CHECK(v.localOutcome == Outcome::Forfeit);
    CHECK(v.recordable);
    CHECK(v.rankDelta == kWinPoints);
    CHECK(v.winner == 10);

    // The local player leaving is NOT a win.
    s.players = {player(10, false, 0, 0, 350, false, true), player(20, false, 0, 3, 400)};
    v = computeVerdict(s);
    CHECK(!v.recordable);
}

static void testSurvivalLastAlive() {
    MatchSnapshot s;
    s.mode = 2;
    s.localAccountId = 10;
    s.players = {player(10, false, 0, 0, 700), player(20, false, 0, 1, 400, true)};
    auto v = computeVerdict(s);
    CHECK(v.winner == 10);
    CHECK(v.localOutcome == Outcome::Win);
    CHECK(v.recordable);

    // Local eliminated, opponent alive.
    s.players = {player(10, false, 0, 1, 400, true), player(20, false, 0, 0, 700)};
    v = computeVerdict(s);
    CHECK(v.winner == 20);
    CHECK(v.localOutcome == Outcome::Loss);
}

static void testSurvivalAllEliminatedProgressWins() {
    MatchSnapshot s;
    s.mode = 2;
    s.localAccountId = 10;
    s.players = {player(10, false, 0, 2, 610, true), player(20, false, 0, 5, 590, true)};
    auto v = computeVerdict(s);
    CHECK(v.winner == 10);
    CHECK(v.localOutcome == Outcome::Win);
    CHECK(v.summary == std::string("all eliminated: best progress wins"));
}

static void testDualChallenge() {
    MatchSnapshot s;
    s.mode = 3;
    s.localAccountId = 10;
    // Both finished: team win.
    s.players = {player(10, true, 60000, 4, 1000), player(20, true, 58000, 2, 1000)};
    auto v = computeVerdict(s);
    CHECK(v.recordable);
    CHECK(v.localOutcome == Outcome::Win);
    CHECK(v.rankDelta == kWinPoints);

    // One player did not finish: not recordable as a win.
    s.players = {player(10, true, 60000, 4, 1000), player(20, false, 0, 2, 800)};
    v = computeVerdict(s);
    CHECK(!v.recordable);
    CHECK(v.localOutcome == Outcome::Loss);
}

static void testWarmupRecordsNothing() {
    MatchSnapshot s;
    s.mode = 6;
    s.localAccountId = 10;
    s.players = {player(10, true, 10000, 0, 1000), player(20, true, 11000, 0, 1000)};
    auto v = computeVerdict(s);
    CHECK(!v.recordable);
    CHECK(v.localOutcome == Outcome::Unrecorded);
}

static void testNotEnoughPlayers() {
    MatchSnapshot s;
    s.mode = 1;
    s.localAccountId = 10;
    s.players = {player(10, false, 0, 0, 100)};
    auto v = computeVerdict(s);
    CHECK(!v.recordable);

    // Everyone disconnected: no verdict.
    s.players = {player(10, false, 0, 0, 100, false, true), player(20, false, 0, 0, 100, false, true)};
    v = computeVerdict(s);
    CHECK(!v.recordable);
}

static void testUnknownMode() {
    MatchSnapshot s;
    s.mode = 99;
    s.localAccountId = 10;
    s.players = {player(10, true, 1, 0, 1000), player(20, true, 2, 0, 1000)};
    auto v = computeVerdict(s);
    CHECK(!v.recordable);
}

static void testRankDeltas() {
    CHECK(applyRankDelta(100, kWinPoints) == 125);
    CHECK(applyRankDelta(100, kLossPoints) == 85);
    CHECK(applyRankDelta(5, kLossPoints) == 0); // floored at zero
    CHECK(kDrawPoints == 0);
}

static void testRecordCodec() {
    auto line = encodeRecord(1731000000, 2, 90123012, 555001, Outcome::Win, 25, 3, 421);
    auto r = decodeRecord(line);
    CHECK(r.has_value());
    CHECK(r->timestamp == 1731000000);
    CHECK(r->mode == 2);
    CHECK(r->levelId == 90123012);
    CHECK(r->opponentId == 555001);
    CHECK(r->outcome == Outcome::Win);
    CHECK(r->rankDelta == 25);
    CHECK(r->deaths == 3);
    CHECK(r->percentX10 == 421);

    // Negative deltas round-trip.
    line = encodeRecord(1, 1, 2, 3, Outcome::Loss, -15, 9, 100);
    r = decodeRecord(line);
    CHECK(r.has_value());
    CHECK(r->rankDelta == -15);

    // Malformed lines are rejected.
    CHECK(!decodeRecord("").has_value());
    CHECK(!decodeRecord("1|2|3").has_value());
    CHECK(!decodeRecord("a|b|c|d|e|f|g|h").has_value());
    CHECK(!decodeRecord("1|2|3|4|5|6|7|8|9").has_value());
    CHECK(!decodeRecord("1|99|3|4|5|6|7|8").has_value()); // invalid mode
    CHECK(!decodeRecord("1|1|3|4|0|6|7|8").has_value());  // invalid outcome
}

int main() {
    testRaceFirstFinishWins();
    testRaceTieBrokenByDeaths();
    testRaceNoFinishersUnrecorded();
    testForfeitOnDisconnect();
    testSurvivalLastAlive();
    testSurvivalAllEliminatedProgressWins();
    testDualChallenge();
    testWarmupRecordsNothing();
    testNotEnoughPlayers();
    testUnknownMode();
    testRankDeltas();
    testRecordCodec();

    if (g_failures == 0) {
        std::printf("All match logic tests passed.\n");
        return 0;
    }
    std::printf("%d match logic test(s) failed.\n", g_failures);
    return 1;
}
