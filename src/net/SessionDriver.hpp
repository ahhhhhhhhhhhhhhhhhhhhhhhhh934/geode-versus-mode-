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

            // Report local match progress once per second while playing a
            // Versus match level.
            if (VersusLobby::phase() == VersusLobby::MatchPhase::InGame &&
                VersusLobby::matchId() != 0) {
                m_progressTimer += dt;
                if (m_progressTimer >= 1.f) {
                    m_progressTimer = 0.f;
                    auto* playLayer = PlayLayer::get();
                    if (playLayer && playLayer->m_level &&
                        playLayer->m_level->m_levelID == VersusLobby::levelId()) {
                        VersusLobby::reportLocalProgress(static_cast<uint16_t>(
                            playLayer->getCurrentPercentInt() * 10));
                    }
                }
            } else {
                m_progressTimer = 0.f;
            }
        }

    private:
        float m_progressTimer = 0.f;
    };
}
