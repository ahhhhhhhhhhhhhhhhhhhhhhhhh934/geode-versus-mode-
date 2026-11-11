#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

// Shared drawing helpers for every Versus screen. All pages use the same
// Geometry Dash-inspired visual language: dark blue backdrop, gold titles,
// cyan accents, and the game's own ButtonSprite artwork for every action.
namespace VersusUI {
    inline CCLabelBMFont* label(CCNode* page, char const* text, float scale, CCPoint pos,
        ccColor3B color = {255, 255, 255}) {
        auto* l = CCLabelBMFont::create(text, "bigFont.fnt");
        l->setScale(scale);
        l->setColor(color);
        l->setPosition(pos);
        page->addChild(l, 4);
        return l;
    }

    inline CCMenuItemSpriteExtra* button(char const* text, CCObject* target, SEL_MenuHandler cb,
        CCPoint pos, char const* frame = "GJ_button_01.png", float scale = 0.72f) {
        auto* sprite = ButtonSprite::create(text, 42, true, "goldFont.fnt", frame, 36.f, scale);
        auto* item = CCMenuItemSpriteExtra::create(sprite, target, cb);
        item->setPosition(pos);
        return item;
    }

    // Geometry Dash Level Complete-inspired card: soft shadow, warm gold
    // frame, and highlight edges on the top and bottom of the card.
    inline void addCard(CCNode* page, CCPoint center, CCSize size, ccColor4B tint) {
        auto* shadow = CCLayerColor::create({0, 0, 0, 105}, size.width + 6.f, size.height + 6.f);
        shadow->setPosition({center.x - size.width / 2.f + 3.f, center.y - size.height / 2.f - 4.f});
        page->addChild(shadow, -1);

        auto* frame = CCLayerColor::create({196, 139, 57, 255}, size.width + 4.f, size.height + 4.f);
        frame->setPosition({center.x - size.width / 2.f - 2.f, center.y - size.height / 2.f - 2.f});
        page->addChild(frame, 0);

        auto* card = CCLayerColor::create(tint, size.width, size.height);
        card->setPosition({center.x - size.width / 2.f, center.y - size.height / 2.f});
        page->addChild(card, 1);
        auto* edge = CCLayerColor::create({255, 220, 128, 255}, size.width, 2.f);
        edge->setPosition({0.f, size.height - 2.f});
        card->addChild(edge, 2);
        auto* lowerEdge = CCLayerColor::create({112, 73, 35, 255}, size.width, 2.f);
        card->addChild(lowerEdge, 2);
    }
}
