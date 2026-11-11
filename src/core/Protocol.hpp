#pragma once

// Versus lobby protocol v4 — a pure, dependency-free packet codec.
//
// Transport: Globed Soft Link Server Events (see src/net/LobbyClient.hpp).
// The packet header is fixed-size and little-endian:
//
//   byte 0: magic 'V' (0x56)
//   byte 1: protocol version (4)
//   byte 2: action
//   byte 3: payload length (bytes that follow the 8-byte header)
//   bytes 4-7: room id (u32 LE) — every packet is scoped to one Globed room
//
// This header must stay free of Geode/Globed includes so it compiles in the
// host unit-test harness (tests/) exactly as it compiles on Android.
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace VersusProto {
    inline constexpr std::string_view kEventId = "versus.mode/lobby-v4";
    inline constexpr uint8_t kProtocolVersion = 4;
    inline constexpr uint8_t kMagic = 0x56;
    inline constexpr size_t kHeaderSize = 8;

    enum class Action : uint8_t {
        Join = 1,
        Ready = 2,
        Leave = 3,
        Heartbeat = 4,
        Launch = 5,
        Countdown = 6,
        Cancel = 7,
        Progress = 8,
        Finish = 9,
        Results = 10,
        Rematch = 11,
    };
    inline constexpr uint8_t kMinimumAction = 1;
    inline constexpr uint8_t kMaximumAction = 11;

    // Game mode ids carried by lobby packets (see src/core/GameModes.hpp).
    inline constexpr uint8_t kMinimumMode = 1;
    inline constexpr uint8_t kMaximumMode = 6;
    inline bool validMode(uint8_t mode) {
        return mode >= kMinimumMode && mode <= kMaximumMode;
    }

    // Progress flags
    inline constexpr uint8_t kFlagEliminated = 0x01;
    inline constexpr uint8_t kFlagFinished = 0x02;
    inline constexpr uint8_t kFlagDead = 0x04;

    // Cancel reasons
    enum class CancelReason : uint8_t {
        HostCancelled = 1,
        PlayerLeft = 2,
        Disconnect = 3,
        Timeout = 4,
        RoomClosed = 5,
        LevelInvalid = 6,
    };

    // Verdicts carried by Results packets
    enum class Verdict : uint8_t {
        Win = 1,
        Loss = 2,
        Draw = 3,
        Forfeit = 4,
        Unrecorded = 5,
    };

    // A decoded packet. Only the fields relevant to `action` are set.
    struct Decoded {
        Action action = Action::Heartbeat;
        uint32_t roomId = 0;
        // lobby fields
        uint8_t mode = 0;
        bool ready = false;
        uint8_t queueType = 0;     // 0 casual, 1 ranked
        uint8_t difficultyTier = 0; // 0..10, index into the difficulty ladder
        // match fields
        uint32_t matchId = 0;
        int levelId = 0;
        uint8_t countdownSeconds = 0;
        // progress fields
        uint16_t percentX10 = 0; // percent * 10, so 421 == 42.1%
        uint32_t deaths = 0;
        uint8_t flags = 0;
        uint32_t timeMs = 0;
        // cancel fields
        uint8_t reason = 0;
        // results fields
        uint8_t verdict = 0;
        int32_t winner = 0;
        // rematch fields
        uint8_t rematchVote = 0;
    };

    // ---- encoding helpers -------------------------------------------------

    inline void putU16(std::vector<uint8_t>& out, uint16_t v) {
        out.push_back(static_cast<uint8_t>(v & 0xff));
        out.push_back(static_cast<uint8_t>((v >> 8) & 0xff));
    }

    inline void putU32(std::vector<uint8_t>& out, uint32_t v) {
        out.push_back(static_cast<uint8_t>(v & 0xff));
        out.push_back(static_cast<uint8_t>((v >> 8) & 0xff));
        out.push_back(static_cast<uint8_t>((v >> 16) & 0xff));
        out.push_back(static_cast<uint8_t>((v >> 24) & 0xff));
    }

    inline uint16_t getU16(std::vector<uint8_t> const& d, size_t at) {
        return static_cast<uint16_t>(d[at]) | (static_cast<uint16_t>(d[at + 1]) << 8);
    }

    inline uint32_t getU32(std::vector<uint8_t> const& d, size_t at) {
        return static_cast<uint32_t>(d[at]) |
            (static_cast<uint32_t>(d[at + 1]) << 8) |
            (static_cast<uint32_t>(d[at + 2]) << 16) |
            (static_cast<uint32_t>(d[at + 3]) << 24);
    }

    inline std::vector<uint8_t> makeHeader(Action action, uint32_t roomId, size_t payloadSize) {
        std::vector<uint8_t> out;
        out.reserve(kHeaderSize + payloadSize);
        out.push_back(kMagic);
        out.push_back(kProtocolVersion);
        out.push_back(static_cast<uint8_t>(action));
        out.push_back(static_cast<uint8_t>(payloadSize));
        putU32(out, roomId);
        return out;
    }

    // ---- per-action encoders ----------------------------------------------

    inline std::vector<uint8_t> encodeLobby(uint32_t roomId, Action action,
        uint8_t mode, bool ready, uint8_t queueType, uint8_t difficultyTier) {
        // Join / Ready / Heartbeat share a 4-byte payload: mode, ready,
        // queue type (casual / ranked) and difficulty tier.
        auto out = makeHeader(action, roomId, 4);
        out.push_back(mode);
        out.push_back(ready ? 1 : 0);
        out.push_back(queueType);
        out.push_back(difficultyTier);
        return out;
    }

    inline std::vector<uint8_t> encodeLeave(uint32_t roomId) {
        return makeHeader(Action::Leave, roomId, 0);
    }

    inline std::vector<uint8_t> encodeLaunch(uint32_t roomId, uint32_t matchId,
        int levelId, uint8_t mode, uint8_t countdownSeconds) {
        auto out = makeHeader(Action::Launch, roomId, 10);
        putU32(out, matchId);
        putU32(out, static_cast<uint32_t>(levelId));
        out.push_back(mode);
        out.push_back(countdownSeconds);
        return out;
    }

    inline std::vector<uint8_t> encodeCountdown(uint32_t roomId, uint32_t matchId,
        uint8_t secondsLeft, int levelId, uint8_t mode) {
        auto out = makeHeader(Action::Countdown, roomId, 10);
        putU32(out, matchId);
        out.push_back(secondsLeft);
        putU32(out, static_cast<uint32_t>(levelId));
        out.push_back(mode);
        return out;
    }

    inline std::vector<uint8_t> encodeCancel(uint32_t roomId, uint32_t matchId, CancelReason reason) {
        auto out = makeHeader(Action::Cancel, roomId, 5);
        putU32(out, matchId);
        out.push_back(static_cast<uint8_t>(reason));
        return out;
    }

    inline std::vector<uint8_t> encodeProgress(uint32_t roomId, uint32_t matchId,
        uint16_t percentX10, uint32_t deaths, uint8_t flags) {
        auto out = makeHeader(Action::Progress, roomId, 11);
        putU32(out, matchId);
        putU16(out, percentX10);
        putU32(out, deaths);
        out.push_back(flags);
        return out;
    }

    inline std::vector<uint8_t> encodeFinish(uint32_t roomId, uint32_t matchId,
        uint32_t timeMs, uint32_t deaths, uint16_t percentX10) {
        auto out = makeHeader(Action::Finish, roomId, 14);
        putU32(out, matchId);
        putU32(out, timeMs);
        putU32(out, deaths);
        putU16(out, percentX10);
        return out;
    }

    inline std::vector<uint8_t> encodeResults(uint32_t roomId, uint32_t matchId,
        Verdict verdict, int32_t winner) {
        auto out = makeHeader(Action::Results, roomId, 9);
        putU32(out, matchId);
        out.push_back(static_cast<uint8_t>(verdict));
        putU32(out, static_cast<uint32_t>(winner));
        return out;
    }

    inline std::vector<uint8_t> encodeRematch(uint32_t roomId, uint32_t matchId, uint8_t vote) {
        auto out = makeHeader(Action::Rematch, roomId, 5);
        putU32(out, matchId);
        out.push_back(vote);
        return out;
    }

    // ---- decoding ----------------------------------------------------------

    // Exact payload size each action must carry.
    inline std::optional<size_t> payloadSizeFor(uint8_t action) {
        switch (static_cast<Action>(action)) {
            case Action::Join:
            case Action::Ready:
            case Action::Heartbeat: return 4;
            case Action::Leave: return 0;
            case Action::Launch:
            case Action::Countdown: return 10;
            case Action::Cancel:
            case Action::Rematch: return 5;
            case Action::Progress: return 11;
            case Action::Finish: return 14;
            case Action::Results: return 9;
            default: return std::nullopt;
        }
    }

    // Decode and validate a raw packet. Returns nullopt for any malformed
    // input: wrong magic, unknown version, unknown action, or wrong length.
    inline std::optional<Decoded> decode(std::vector<uint8_t> const& bytes) {
        if (bytes.size() < kHeaderSize) return std::nullopt;
        if (bytes[0] != kMagic) return std::nullopt;
        if (bytes[1] != kProtocolVersion) return std::nullopt;

        uint8_t actionByte = bytes[2];
        if (actionByte < kMinimumAction || actionByte > kMaximumAction) return std::nullopt;
        auto expected = payloadSizeFor(actionByte);
        if (!expected) return std::nullopt;
        if (bytes[3] != *expected) return std::nullopt;
        if (bytes.size() != kHeaderSize + *expected) return std::nullopt;

        Decoded d;
        d.action = static_cast<Action>(actionByte);
        d.roomId = getU32(bytes, 4);

        switch (d.action) {
            case Action::Join:
            case Action::Ready:
            case Action::Heartbeat:
                d.mode = bytes[kHeaderSize];
                d.ready = bytes[kHeaderSize + 1] != 0;
                d.queueType = bytes[kHeaderSize + 2];
                d.difficultyTier = bytes[kHeaderSize + 3];
                break;
            case Action::Leave:
                break;
            case Action::Launch:
                d.matchId = getU32(bytes, kHeaderSize);
                d.levelId = static_cast<int>(getU32(bytes, kHeaderSize + 4));
                d.mode = bytes[kHeaderSize + 8];
                d.countdownSeconds = bytes[kHeaderSize + 9];
                break;
            case Action::Countdown:
                d.matchId = getU32(bytes, kHeaderSize);
                d.countdownSeconds = bytes[kHeaderSize + 4];
                d.levelId = static_cast<int>(getU32(bytes, kHeaderSize + 5));
                d.mode = bytes[kHeaderSize + 9];
                break;
            case Action::Cancel:
                d.matchId = getU32(bytes, kHeaderSize);
                d.reason = bytes[kHeaderSize + 4];
                break;
            case Action::Progress:
                d.matchId = getU32(bytes, kHeaderSize);
                d.percentX10 = getU16(bytes, kHeaderSize + 4);
                d.deaths = getU32(bytes, kHeaderSize + 6);
                d.flags = bytes[kHeaderSize + 10];
                break;
            case Action::Finish:
                d.matchId = getU32(bytes, kHeaderSize);
                d.timeMs = getU32(bytes, kHeaderSize + 4);
                d.deaths = getU32(bytes, kHeaderSize + 8);
                d.percentX10 = getU16(bytes, kHeaderSize + 12);
                break;
            case Action::Results:
                d.matchId = getU32(bytes, kHeaderSize);
                d.verdict = bytes[kHeaderSize + 4];
                d.winner = static_cast<int32_t>(getU32(bytes, kHeaderSize + 5));
                break;
            case Action::Rematch:
                d.matchId = getU32(bytes, kHeaderSize);
                d.rematchVote = bytes[kHeaderSize + 4];
                break;
        }
        return d;
    }
}
