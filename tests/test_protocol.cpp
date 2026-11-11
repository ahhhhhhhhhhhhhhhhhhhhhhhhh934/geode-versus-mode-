// Host-side unit tests for the Versus lobby protocol codec.
// Compiled and run by .github/workflows/unit-tests.yml on every push/PR.
// This file deliberately avoids Geode/Globed headers so it builds anywhere.
#include "../src/core/Protocol.hpp"

#include <cstdio>
#include <vector>

static int g_failures = 0;

#define CHECK(cond) do { \
    if (!(cond)) { std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); ++g_failures; } \
} while (0)

using namespace VersusProto;

static void testHeaderShape() {
    auto p = encodeLeave(0xAABBCCDD);
    CHECK(p.size() == kHeaderSize);
    CHECK(p[0] == kMagic);
    CHECK(p[1] == kProtocolVersion);
    CHECK(p[2] == static_cast<uint8_t>(Action::Leave));
    CHECK(p[3] == 0);
    CHECK(getU32(p, 4) == 0xAABBCCDD);
}

static void testLobbyRoundtrip() {
    for (auto action : {Action::Join, Action::Ready, Action::Heartbeat}) {
        auto p = encodeLobby(123456789, action, 6, true, 1, 7);
        auto d = decode(p);
        CHECK(d.has_value());
        CHECK(d->action == action);
        CHECK(d->roomId == 123456789);
        CHECK(d->mode == 6);
        CHECK(d->ready);
        CHECK(d->queueType == 1);
        CHECK(d->difficultyTier == 7);
    }
}

static void testLeaveRoundtrip() {
    auto p = encodeLeave(42);
    auto d = decode(p);
    CHECK(d.has_value());
    CHECK(d->action == Action::Leave);
    CHECK(d->roomId == 42);
}

static void testLaunchCountdownRoundtrip() {
    auto p = encodeLaunch(777, 0xDEADBEAF, 90123012, 3, 5);
    auto d = decode(p);
    CHECK(d.has_value());
    CHECK(d->action == Action::Launch);
    CHECK(d->matchId == 0xDEADBEAF);
    CHECK(d->levelId == 90123012);
    CHECK(d->mode == 3);
    CHECK(d->countdownSeconds == 5);

    auto c = encodeCountdown(777, 0xDEADBEAF, 3, 90123012, 3);
    auto cd = decode(c);
    CHECK(cd.has_value());
    CHECK(cd->action == Action::Countdown);
    CHECK(cd->matchId == 0xDEADBEAF);
    CHECK(cd->countdownSeconds == 3);
    CHECK(cd->levelId == 90123012);
    CHECK(cd->mode == 3);
}

static void testCancelRematchRoundtrip() {
    auto p = encodeCancel(9, 111, CancelReason::PlayerLeft);
    auto d = decode(p);
    CHECK(d.has_value());
    CHECK(d->action == Action::Cancel);
    CHECK(d->matchId == 111);
    CHECK(d->reason == static_cast<uint8_t>(CancelReason::PlayerLeft));

    auto r = encodeRematch(9, 111, 1);
    auto rd = decode(r);
    CHECK(rd.has_value());
    CHECK(rd->action == Action::Rematch);
    CHECK(rd->matchId == 111);
    CHECK(rd->rematchVote == 1);
}

static void testProgressFinishRoundtrip() {
    auto p = encodeProgress(5, 1000, 421, 17, kFlagDead);
    auto d = decode(p);
    CHECK(d.has_value());
    CHECK(d->action == Action::Progress);
    CHECK(d->matchId == 1000);
    CHECK(d->percentX10 == 421);
    CHECK(d->deaths == 17);
    CHECK(d->flags == kFlagDead);

    auto f = encodeFinish(5, 1000, 9500, 3, 1000);
    auto fd = decode(f);
    CHECK(fd.has_value());
    CHECK(fd->action == Action::Finish);
    CHECK(fd->timeMs == 9500);
    CHECK(fd->deaths == 3);
    CHECK(fd->percentX10 == 1000);
}

static void testResultsRoundtrip() {
    auto p = encodeResults(5, 1000, Verdict::Win, 555001);
    auto d = decode(p);
    CHECK(d.has_value());
    CHECK(d->action == Action::Results);
    CHECK(d->verdict == static_cast<uint8_t>(Verdict::Win));
    CHECK(d->winner == 555001);
}

static void testRejectsGarbage() {
    // Too short
    CHECK(!decode({}).has_value());
    CHECK(!decode(std::vector<uint8_t>(7, 0)).has_value());
    // Wrong magic
    auto p = encodeLeave(1);
    p[0] = 'X';
    CHECK(!decode(p).has_value());
    // Unknown version
    auto p2 = encodeLeave(1);
    p2[1] = 99;
    CHECK(!decode(p2).has_value());
    // Unknown action
    auto p3 = encodeLeave(1);
    p3[2] = 200;
    CHECK(!decode(p3).has_value());
    // Truncated / extended payload
    auto p4 = encodeLeave(1);
    p4.push_back(0);
    CHECK(!decode(p4).has_value());
    // Wrong declared payload length for action
    auto p5 = encodeLeave(1);
    p5[3] = 9;
    CHECK(!decode(p5).has_value());
    // Unknown cancel reason still decodes (reason is data, not structure)
    auto p6 = encodeCancel(1, 1, static_cast<CancelReason>(250));
    CHECK(decode(p6).has_value());
}

static void testRoomIdZero() {
    // A zero room id can never be a valid lobby scope.
    auto p = encodeLobby(0, Action::Join, 1, false, 0, 0);
    auto d = decode(p);
    CHECK(d.has_value());
    CHECK(d->roomId == 0); // codec passes it through; LobbyClient must reject it
}

static void testModeValidation() {
    CHECK(VersusProto::validMode(1));
    CHECK(VersusProto::validMode(6));
    CHECK(!VersusProto::validMode(0));
    CHECK(!VersusProto::validMode(7));
    CHECK(!VersusProto::validMode(255));
}

int main() {
    testHeaderShape();
    testLobbyRoundtrip();
    testLeaveRoundtrip();
    testLaunchCountdownRoundtrip();
    testCancelRematchRoundtrip();
    testProgressFinishRoundtrip();
    testResultsRoundtrip();
    testRejectsGarbage();
    testRoomIdZero();
    testModeValidation();

    if (g_failures == 0) {
        std::printf("All protocol tests passed.\n");
        return 0;
    }
    std::printf("%d protocol test(s) failed.\n", g_failures);
    return 1;
}
