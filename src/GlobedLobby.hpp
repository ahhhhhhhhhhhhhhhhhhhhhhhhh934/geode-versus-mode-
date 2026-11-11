#pragma once

// Versus room-lobby signaling built on Globed's public Soft Link Server Events.
// Events use the central server so players do not need to be in the same level
// session. The room ID in each packet scopes the lobby to one Globed room.
#include <globed/soft-link/API.hpp>
#include <globed/core/data/Messages.hpp>
#include <globed/core/net/MessageListener.hpp>

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace VersusLobby {
    // Version 2 adds an explicit schema byte; old clients ignore this event name.
    inline constexpr std::string_view kEventId = "versus.mode/lobby-v2";
    inline constexpr uint8_t kProtocolVersion = 2;
    inline constexpr uint8_t kMinimumMode = 1;
    inline constexpr uint8_t kMaximumMode = 4;
    inline constexpr size_t kPacketSize = 8;

    enum class Action : uint8_t { Join = 1, Ready = 2, Leave = 3, Heartbeat = 4 };

    inline bool validMode(uint8_t mode) { return mode >= kMinimumMode && mode <= kMaximumMode; }
    inline bool validAction(uint8_t action) {
        return action >= static_cast<uint8_t>(Action::Join) && action <= static_cast<uint8_t>(Action::Heartbeat);
    }

    struct RemotePlayer {
        int accountId = 0;
        uint8_t mode = 0;
        bool ready = false;
        float lastSeen = 0.f;
    };

    inline std::unordered_map<int, RemotePlayer> s_players;
    inline geode::ListenerHandle s_listener;
    inline uint32_t s_roomId = 0;
    inline uint8_t s_mode = 0;
    inline bool s_joined = false;
    inline bool s_ready = false;
    inline float s_heartbeat = 0.f;
    inline float s_clock = 0.f;
    inline bool s_registered = false;

    inline std::vector<uint8_t> encode(uint32_t roomId, Action action, uint8_t mode, bool ready) {
        return {
            static_cast<uint8_t>(roomId & 0xff),
            static_cast<uint8_t>((roomId >> 8) & 0xff),
            static_cast<uint8_t>((roomId >> 16) & 0xff),
            static_cast<uint8_t>((roomId >> 24) & 0xff),
            kProtocolVersion,
            static_cast<uint8_t>(action),
            mode,
            static_cast<uint8_t>(ready ? 1 : 0)
        };
    }

    inline void send(Action action) {
        if (!s_joined || !s_roomId || !validMode(s_mode) || !globed::api::available() || !globed::api::net::isConnected())
            return;
        globed::EventOptions options{};
        options.server = globed::EventServer::Central;
        options.reliable = true;
        globed::api::net::sendEvent(kEventId, encode(s_roomId, action, s_mode, s_ready), options);
    }

    inline void initialize() {
        globed::api::waitForGlobed([] {
            if (!globed::api::available() || s_registered) return;
            globed::api::net::registerEvent(kEventId, globed::EventServer::Central);
            s_listener = globed::MessageEvent<globed::msg::EventsMessage>{false}.listen(
                [](const globed::msg::EventsMessage& message) {
                    for (auto const& event : message.events) {
                        if (event.name != "versus.mode/lobby-v2"_spr || event.data.size() != kPacketSize) continue;
                        auto const& d = event.data;
                        if (d[4] != kProtocolVersion || !validAction(d[5]) || !validMode(d[6]) || d[7] > 1) continue;
                        uint32_t roomId = static_cast<uint32_t>(d[0]) |
                            (static_cast<uint32_t>(d[1]) << 8) |
                            (static_cast<uint32_t>(d[2]) << 16) |
                            (static_cast<uint32_t>(d[3]) << 24);
                        if (!roomId || roomId != s_roomId || event.options.sender <= 0) continue;

                        int sender = event.options.sender;
                        auto action = static_cast<Action>(d[5]);
                        if (action == Action::Leave) {
                            s_players.erase(sender);
                            continue;
                        }
                        if (action != Action::Join && action != Action::Ready &&
                            action != Action::Heartbeat) continue;

                        auto& player = s_players[sender];
                        player.accountId = sender;
                        player.mode = d[6];
                        player.ready = d[7] != 0;
                        player.lastSeen = s_clock;
                    }
                }
            );
            s_registered = true;
        });
    }

    inline void leave();

    inline void join(uint32_t roomId, uint8_t mode) {
        if (s_joined && s_roomId != roomId) leave();
        if (!roomId || !validMode(mode)) return;
        s_roomId = roomId;
        s_mode = mode;
        s_ready = false;
        s_joined = true;
        s_heartbeat = 0.f;
        send(Action::Join);
    }

    inline void setReady(bool ready) {
        if (!s_joined) return;
        s_ready = ready;
        send(Action::Ready);
    }

    inline void leave() {
        if (s_joined) send(Action::Leave);
        s_players.clear();
        s_roomId = 0;
        s_mode = 0;
        s_ready = false;
        s_joined = false;
        s_heartbeat = 0.f;
    }

    inline void update(float dt, uint32_t currentRoomId) {
        s_clock += dt;
        if (s_joined && currentRoomId != s_roomId) leave();
        if (!s_joined) return;
        s_heartbeat += dt;
        if (s_heartbeat >= 5.f) {
            s_heartbeat = 0.f;
            send(Action::Heartbeat);
        }
        for (auto it = s_players.begin(); it != s_players.end();) {
            if (s_clock - it->second.lastSeen > 15.f) it = s_players.erase(it);
            else ++it;
        }
    }

    inline bool joined() { return s_joined; }
    inline bool ready() { return s_ready; }
    inline uint32_t roomId() { return s_roomId; }
    inline uint8_t mode() { return s_mode; }
    inline std::vector<RemotePlayer> players(uint8_t mode) {
        std::vector<RemotePlayer> result;
        for (auto const& [id, player] : s_players) {
            if (player.mode == mode) result.push_back(player);
        }
        return result;
    }
}
