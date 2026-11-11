#pragma once

#include <Geode/Geode.hpp>
#include "LobbyClient.hpp"

using namespace geode::prelude;

// The Versus lobby needs to keep running (heartbeats, countdowns, match
// progress) while the player is inside a level, where the hub scene is no
// longer the active scene and its update() stops being called.
//
// SessionDriver is a tiny always-alive node scheduled directly on the
// director's scheduler, so lobby traffic continues for the whole session.
// It is created on first use and intentionally never released.
namespace Versus {
    class SessionDriver : public CCObject {
    public:
        static SessionDriver* ensure() {
            static SessionDriver* instance = nullptr;
            if (instance) return instance;
            instance = new SessionDriver();
            instance->retain(); // lives for the whole app session
            CCDirector::sharedDirector()->getScheduler()->scheduleSelector(
                schedule_selector(SessionDriver::tick), instance, 0.f, false);
            return instance;
        }

        void tick(float dt) {
            // Drive the lobby transport from any scene, including in-level.
            VersusLobby::update(dt, globedInRoom() ? globedRoomId() : 0);
        }
    };
}
