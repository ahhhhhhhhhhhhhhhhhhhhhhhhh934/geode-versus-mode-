// Host-side unit tests for the six Versus game modes.
#include "../src/core/GameModes.hpp"

#include <cstdio>

static int g_failures = 0;

#define CHECK(cond) do { \
    if (!(cond)) { std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); ++g_failures; } \
} while (0)

using namespace VersusModes;

static void testRegistry() {
    // All six protocol modes are registered, in id order.
    size_t seen = 0;
    for (uint8_t id = 1; id <= 6; ++id) {
        auto* m = find(id);
        CHECK(m != nullptr);
        if (m) {
            CHECK(m->id == id);
            CHECK(m->name != nullptr);
            CHECK(m->tagline != nullptr);
            CHECK(m->rules != nullptr);
            ++seen;
        }
    }
    CHECK(seen == kModeCount);

    // Unknown ids resolve to nothing and "UNKNOWN" names.
    CHECK(find(0) == nullptr);
    CHECK(find(7) == nullptr);
    CHECK(find(255) == nullptr);
    CHECK(std::string_view(name(0)) == "UNKNOWN");
}

static void testModeSemantics() {
    CHECK(find(Mode::Race1v1)->ranked);
    CHECK(find(Mode::Race1v1)->requiresPlatformer == false);

    CHECK(find(Mode::Survival)->elimination);
    CHECK(find(Mode::Survival)->ranked);

    CHECK(find(Mode::DualChallenge)->cooperative);
    CHECK(find(Mode::DualChallenge)->elimination == false);

    CHECK(find(Mode::PlatformerBattle)->requiresPlatformer);

    CHECK(find(Mode::ClassicRace)->ranked);
    CHECK(find(Mode::ClassicRace)->requiresPlatformer == false);

    // Warm-up is the only mode where nothing is recorded.
    CHECK(find(Mode::Warmup)->ranked == false);
    CHECK(isRanked(static_cast<uint8_t>(Mode::Warmup)) == false);
    for (auto m : {Mode::Race1v1, Mode::Survival, Mode::DualChallenge,
        Mode::PlatformerBattle, Mode::ClassicRace}) {
        CHECK(isRanked(static_cast<uint8_t>(m)));
    }
}

static void testDifficultyCategories() {
    // Every category from Auto through Extreme Demon exists, in order.
    CHECK(kDifficultyCount == 11);
    CHECK(std::string_view(kDifficulties[0].name) == "AUTO");
    CHECK(std::string_view(kDifficulties[10].name) == "EXTREME DEMON");

    // Thresholds are strictly increasing.
    for (size_t i = 1; i < kDifficultyCount; ++i) {
        CHECK(kDifficulties[i].minimumPoints > kDifficulties[i - 1].minimumPoints);
    }

    // Placement by points.
    CHECK(difficultyForPoints(0)->minimumPoints == 0);
    CHECK(std::string_view(difficultyForPoints(99)->name) == "AUTO");
    CHECK(std::string_view(difficultyForPoints(100)->name) == "EASY");
    CHECK(std::string_view(difficultyForPoints(3500)->name) == "EXTREME DEMON");
    CHECK(std::string_view(difficultyForPoints(999999)->name) == "EXTREME DEMON");
    CHECK(difficultyAt(kDifficultyCount) == nullptr);
    CHECK(difficultyAt(0) != nullptr);
}


static void testLevelChecks() {
    using LC = VersusModes::LevelCheck;
    // Unknown mode
    CHECK(VersusModes::checkLevelForMode(99, 500) == LC::UnknownMode);
    // Invalid ids
    CHECK(VersusModes::checkLevelForMode(1, 0) == LC::InvalidId);
    CHECK(VersusModes::checkLevelForMode(1, -5) == LC::InvalidId);
    // Plain race modes only need a valid id
    CHECK(VersusModes::checkLevelForMode(1, 500) == LC::Ok);
    CHECK(VersusModes::checkLevelForMode(2, 500) == LC::Ok);
    CHECK(VersusModes::checkLevelForMode(3, 500) == LC::Ok);
    // Platformer / classic expectations
    CHECK(VersusModes::checkLevelForMode(4, 500) == LC::NeedsPlatformerLevel);
    CHECK(VersusModes::checkLevelForMode(5, 500) == LC::NeedsClassicLevel);
    // Warm-up accepts anything with a valid id
    CHECK(VersusModes::checkLevelForMode(6, 500) == LC::WarmupAnyLevel);
    CHECK(VersusModes::checkLevelForMode(6, 0) == LC::InvalidId);
    // Hints exist for every verdict
    for (uint8_t c = 0; c <= 5; ++c) {
        CHECK(std::string_view(VersusModes::levelCheckHint(static_cast<VersusModes::LevelCheck>(c))).size() > 0);
    }
}

int main() {
    testRegistry();
    testModeSemantics();
    testDifficultyCategories();
    testLevelChecks();

    if (g_failures == 0) {
        std::printf("All game mode tests passed.\n");
        return 0;
    }
    std::printf("%d game mode test(s) failed.\n", g_failures);
    return 1;
}
