#include "../src/core/Achievements.hpp"

#include <cassert>
#include <vector>

int main() {
    using namespace VersusMatch;
    using namespace VersusAchievements;

    std::vector<MatchRecord> history;
    auto empty = evaluate(history);
    assert(unlockedCount(empty) == 0);

    history.push_back({1, 1, 100, 22, Outcome::Win, 25, 0, 1000});
    auto first = evaluate(history);
    assert(first[0].unlocked);
    assert(first[1].unlocked);
    assert(!first[2].unlocked);
    assert(unlockedCount(first) == 2);

    history.push_back({2, 2, 101, 23, Outcome::Forfeit, 25, 1, 750});
    history.push_back({3, 3, 102, 24, Outcome::Win, 25, 0, 1000});
    history.push_back({4, 4, 103, 25, Outcome::Win, 25, 0, 1000});
    auto explorer = evaluate(history);
    assert(explorer[3].unlocked);
    assert(explorer[4].unlocked);
    assert(explorer[5].unlocked);
    assert(unlockedCount(explorer) == 6);

    history.push_back({5, 5, 104, 26, Outcome::Loss, -15, 3, 420});
    history.push_back({6, 1, 105, 27, Outcome::Draw, 0, 2, 800});
    history.push_back({7, 6, 106, 28, Outcome::Unrecorded, 0, 0, 0});
    auto stillSix = evaluate(history);
    assert(stillSix[2].unlocked);
    assert(unlockedCount(stillSix) == 6);

    return 0;
}
