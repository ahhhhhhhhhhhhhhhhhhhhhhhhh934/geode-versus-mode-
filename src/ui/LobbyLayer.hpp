#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/CreatorLayer.hpp>
#include "../net/GlobedClient.hpp"
#include "../net/LobbyClient.hpp"
#include "../net/SessionDriver.hpp"
#include "Theme.hpp"

using namespace geode::prelude;

namespace Versus {
    enum class Screen { Home, Plaza, Ranked, Profile, PublicRooms, GroupRooms, Live, Rooms, Voice, Local, Online, Loading, Info, Lobby, MatchSetup };
    inline Screen s_screen = Screen::Home;

    // The single Versus hub scene. Every page is rendered into a mutable
    // `m_page` layer so the back button and navigation stay consistent.
    class LobbyLayer final : public CCLayer {
    public:
        static LobbyLayer* create();
        static CCScene* scene();

        bool init() override;
        void keyBackClicked() override;
        void update(float dt) override;

        // Navigation + screen state (public so RoomPages can drive it)
        void showHome();
        void showPlaza();
        void showRanked();
        void showProfile();
        void showPublicRooms();
        void showGroupRooms();
        void showLive();
        void showRooms();
        void showVoice();
        void showRoomLobby();
        void showModeSelect();
        void showMatchSetup();
        void showInfo(char const* title, char const* subtitle, char const* status,
            char const* detail, char const* extra);
        void showGlobedLoading();

        void onRooms(CCObject*);
        void onDashboard(CCObject*);
        void onOpenPlaza(CCObject*);
        void onRanked(CCObject*);
        void onProfile(CCObject*);
        void onPublicRooms(CCObject*);
        void onGroupRooms(CCObject*);
        void onLive(CCObject*);
        void onWorkshop(CCObject*);
        void onVoice(CCObject*);
        void onJoinLobby(CCObject*);
        void onOpenModeSelect(CCObject*);
        void onPickMode(CCObject*);
        void onOpenMatchSetup(CCObject*);
        void onUsePinnedLevel(CCObject*);
        void onLaunchMatch(CCObject*);
        void onToggleReady(CCObject*);
        void onLeaveLobby(CCObject*);
        void onLocal(CCObject*);
        void onOnline(CCObject*);
        void onBack(CCObject*);
        void onClose(CCObject*);

        // Rank ladder shared by the home, ranked and profile pages.
        struct RankTier {
            char const* name;
            int minimumPoints;
            ccColor3B color;
        };
        static RankTier const kRankTiers[];
        static size_t rankTierCount();
        static int rankPoints();
        static size_t rankTierIndex(int points);
        static char const* rankName(int points);

        CCSize m_size;
        CCLayer* m_page = nullptr;
        geode::TextInput* m_levelInput = nullptr;
        bool m_waitingForGlobed = false;
        float m_lobbyRefresh = 0.f;

        float m_globedWait = 0.f;
        float m_loadingPulse = 0.f;
        CCLabelBMFont* m_loadingLabel = nullptr;
        CCLayerColor* m_loadingFill = nullptr;

        void basePage(char const* title, char const* subtitle);
        void clearPage();
    };
}
