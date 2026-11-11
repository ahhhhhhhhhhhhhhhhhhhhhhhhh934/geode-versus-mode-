# Versus Mode Roadmap

This document is the working feature plan for Versus Mode. A roadmap item is a goal, not a claim that the feature is already implemented.

## Product direction

Versus Mode should feel like a complete Geometry Dash competition hub: fast to enter, visually consistent with the game, useful with friends, and honest about online status. Keep the mod client focused on presentation, local preferences, match rules, and gameplay integration.

## Multiplayer foundation

- [x] Remove the Supabase dependency and its configuration.
- [x] Detect Globed installation, enabled state, connection state, and current room.
- [x] Show the current Globed room status in the Versus UI.
- [ ] Validate the Globed 2.3.0-beta session handoff on real Android devices.
- [ ] Define a versioned Versus match protocol with explicit lobby, ready, start, finish, and disconnect states.
- [ ] Add global matchmaking only when Globed's central-server layer supports a persistent cross-room queue and safe pairing. Do not disguise a same-room flow as global matchmaking.
- [ ] Add reconnect handling, match cancellation, host departure handling, and stale-lobby cleanup.
- [ ] Keep match outcomes authoritative and prevent clients from awarding online rank for unverified results.

## Game modes

### Core competitive modes
- [ ] Rank-matched level challenge — select randomized Geometry Dash levels from the player's current rank difficulty: Auto, Easy, Normal, Hard, Harder, Insane, and matching Demon tiers through Extreme Demon. Difficulty rises with rank. No wagers, paid entries, lost rank points, or stakes; progression is based on verified completion only. Requires a curated/searchable level pool and validation of each level's actual difficulty.
- [ ] 1v1 Race — players race through the same level; record finish time and completion state.
- [ ] Survival — stay alive as long as possible under a clearly defined shared ruleset.
- [ ] Dual Challenge — coordinated or competing dual-control challenges, with the exact variant selected per playlist.
- [ ] Platformer Battle — platformer levels with checkpoints, finish conditions, and tie handling.
- [ ] Classic level race — compare completion time and progress on classic levels.
- [ ] Practice / warm-up room — let players test controls before a scored match.

### Match quality
- [ ] Mode-specific rules and clear pre-match instructions.
- [ ] Fair level selection and difficulty filters.
- [ ] Match timer, pause/disconnect policy, rematch, and return-to-lobby flow.
- [ ] Results screen with verified outcome, personal best, and match history.
- [ ] Spectator support only after match state and privacy rules are stable.

### Rank-matched level challenge rules
- Current rank determines the level difficulty pool; do not mix easier/harder tiers unless the player explicitly selects a separate unranked practice option.
- Random selection must avoid recently played levels and support level length / mode filters.
- A level must be verified as the intended difficulty before it can award progression.
- The challenge is not gambling: no real-money purchases, wagers, betting, entry fees, or loss of rank points/items.
- [ ] Build a curated level catalog and a real level picker; the current UI is only a preview until this exists.

## Player progression

- [ ] Persistent online profile backed by a trusted service or supported Globed-side infrastructure.
- [ ] Ranked rating with placement matches, divisions, seasonal resets, and transparent rating changes.
- [ ] Match history, win/loss record, streaks, personal records, and per-mode statistics.
- [ ] Achievements, titles, badges, and unlockable profile cosmetics.
- [ ] Local practice statistics clearly separated from verified online records.

## Social and community

- [ ] Party invites and private friend matches.
- [ ] Room browser showing only real rooms and live player counts.
- [ ] Group parties and team-based playlists.
- [ ] Voice support through Globed where available, with mute, block, report, and privacy controls.
- [ ] Live match directory and spectating after safe, reliable synchronization exists.
- [ ] Community workshop for playlists and challenge presets, with moderation and reporting before public uploads.

## Creator and customization

- [ ] Custom match rules and rotating community playlists.
- [ ] Player-selected lobby themes, banners, icons, and profile presentation.
- [ ] Level filters for length, difficulty, game mode, and platformer/classic format.
- [ ] Shareable challenge codes and saved private playlists.
- [ ] Clear distinction between cosmetic customization and competitive advantages.

## Reliability, accessibility, and safety

- [ ] Scale layouts for different aspect ratios and screen sizes.
- [ ] Add reduced-motion and readable-text options.
- [ ] Handle missing or outdated Globed versions with actionable messages.
- [ ] Never show fake rooms, fake player counts, fake ranks, or simulated online results.
- [ ] Add mute/block/report flows before enabling Versus-managed social communication.
- [ ] Keep voice and community features subject to appropriate moderation and privacy controls.
- [ ] Add structured diagnostic logging for connection, lobby, session handoff, and match state transitions.

## Technical boundaries

- Globed remains the multiplayer networking and gameplay-session foundation.
- Versus Mode owns its game-mode rules, lobby presentation, local UI state, and match-specific client logic.
- Globed does not automatically supply a custom global Versus queue, ranked service, room directory, or authoritative Versus results.
- A true cross-room queue needs explicit support in Globed's central-server layer (upstream or a maintained Globed-compatible server extension). No unrelated gameplay server or Supabase dependency should be introduced.
- Every online feature must be tested end-to-end on Android with the supported Globed version before it is described as playable.

## Release gates

A feature is ready to be called playable only after:
1. The Android build succeeds.
2. The UI path works in Geometry Dash.
3. Two real clients can complete the flow through Globed.
4. Disconnects, retries, and invalid states are handled.
5. The feature's status is documented accurately.

Until then, label it as a preview, prototype, or planned feature.
