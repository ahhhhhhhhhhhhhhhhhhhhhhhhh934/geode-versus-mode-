#include "LobbyLayer.hpp"
#include "../core/GameModes.hpp"

// Pages that configure and describe matches: mode selection today, match
// setup and results in the following PRs of the stack.
namespace Versus {
    void LobbyLayer::showModeSelect() {
        s_screen = Screen::Info;
        basePage("MODE SELECT", "PICK THE RULES FOR THIS ROOM");
        auto w = m_size.width;
        auto h = m_size.height;

        // Two columns of three mode cards. Each card shows the mode name,
        // its tagline, its ranked status, and its core rule.
        float top = h * 0.80f;
        float cardH = h * 0.185f;
        float gap = h * 0.015f;
        for (size_t i = 0; i < VersusModes::kModeCount; ++i) {
            auto const& m = VersusModes::kModes[i];
            float column = i % 2 == 0 ? 0.27f : 0.73f;
            int row = static_cast<int>(i / 2);
            float y = top - cardH / 2.f - row * (cardH + gap);
            bool selected = VersusLobby::joined() && VersusLobby::mode() == m.id;
            VersusUI::addCard(m_page, {w * column, y}, {w * 0.44f, cardH},
                selected ? ccColor4B{26, 68, 108, 255} : ccColor4B{18, 42, 91, 255});
            VersusUI::label(m_page, m.name, 0.30f, {w * column, y + cardH * 0.32f},
                m.ranked ? ccColor3B{255,224,110} : ccColor3B{160,205,255});
            VersusUI::label(m_page, m.tagline, 0.15f, {w * column, y + cardH * 0.13f}, {185,210,245});
            char const* badge = m.ranked ? "RANKED" : "CASUAL";
            VersusUI::label(m_page, badge, 0.14f, {w * column, y - cardH * 0.08f},
                m.ranked ? ccColor3B{115,255,170} : ccColor3B{170,200,235});
            VersusUI::label(m_page, selected ? "SELECTED" : "TAP TO SELECT", 0.13f,
                {w * column, y - cardH * 0.26f},
                selected ? ccColor3B{255,215,130} : ccColor3B{150,180,215});

            auto* menu = CCMenu::create();
            menu->setPosition({0,0});
            // An invisible full-card tap target selects the mode.
            auto* hit = CCMenuItemSpriteExtra::create(
                CCNode::create(), this, menu_selector(LobbyLayer::onPickMode));
            hit->setContentSize({w * 0.44f, cardH});
            hit->setPosition({w * column, y});
            hit->setTag(m.id);
            menu->addChild(hit);
            m_page->addChild(menu, 6);
        }

        VersusUI::label(m_page,
            "RULES: DYING ELIMINATES YOU IN SURVIVAL  •  WARM-UP RECORDS NOTHING",
            0.16f, {w * 0.50f, h * 0.115f}, {160,190,225});

        auto* menu = CCMenu::create();
        menu->setPosition({0,0});
        menu->addChild(VersusUI::button("BACK TO LOBBY", this, menu_selector(LobbyLayer::onJoinLobby),
            {w * 0.50f, 40.f}, "GJ_button_01.png", 0.44f));
        m_page->addChild(menu, 7);
    }

    void LobbyLayer::onOpenModeSelect(CCObject*) {
        showModeSelect();
    }

    void LobbyLayer::onPickMode(CCObject* sender) {
        auto* item = static_cast<CCMenuItemSpriteExtra*>(sender);
        uint8_t mode = static_cast<uint8_t>(item->getTag());
        if (VersusModes::find(mode)) {
            VersusLobby::setMode(mode);
        }
        showRoomLobby();
    }
}
