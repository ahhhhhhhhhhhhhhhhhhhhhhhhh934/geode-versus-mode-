# Versus Feature Parity Plan

This is a design and implementation checklist for bringing the feature breadth of the user-provided Versus reference package into **our own Versus Mode codebase**. It is a plan, not a statement that any item is implemented or tested.

## Source and originality boundary

The reference package is `tipp7.versus` v1.1.1, targeting Geometry Dash 2.2081 with Geode 5.7.1. Its package metadata, `about.md`, and changelog were inspected to identify feature categories.

We will implement these capabilities independently in this repository. Do **not** copy the reference package's compiled code, source, artwork, sounds, branding, or other assets. Reuse only high-level feature ideas and behavior descriptions; create our own implementation and original assets, and respect any third-party licenses for dependencies.

## Feature parity checklist

### Rooms and match setup
- [ ] Public and private rooms.
- [ ] Private-room invite/code flow.
- [ ] One-versus-one and group rooms, with configurable player limits.
- [ ] Host-selected levels using Geometry Dash's normal level search flow.
- [ ] Player ready state and a synchronized start countdown.
- [ ] Custom match settings, including attempts, percentage, timed rounds, speed, and practice rules.
- [ ] Explicit handling for classic and platformer level compatibility.
- [ ] Rematch, leave, forfeit, host departure, and room cleanup flows.

### Map pools and selection
- [ ] Shared map pool with per-player contribution limits.
- [ ] Host controls for who can add maps.
- [ ] Sequential and randomized pool playback.
- [ ] Filters for difficulty, stars/rating, length, game mode, and other supported level metadata.
- [ ] Random-map selection and clear indication of the next map.
- [ ] Map download retries, failure states, and song-download status.

### Ranked competition
- [ ] Ranked matchmaking with visible search status and fair search expansion.
- [ ] Seasonal competition, divisions/cups, placement matches, and rating changes.
- [ ] Map selection/ban phase where appropriate.
- [ ] Player ranks, leaderboards, and per-season standings.
- [ ] Verified match outcomes only; never award rank from client-only or unverified results.
- [ ] Transparent handling of draws, disconnects, forfeits, and invalid matches.

### Match experience
- [ ] Live match HUD for both players and group standings.
- [ ] Synchronized progress, attempts, pause/practice state, and finish state.
- [ ] Match timer and last-attempt behavior for timed mode.
- [ ] Clear loading, countdown, connection, and music-status feedback.
- [ ] Results with outcome explanation, player stats, and rematch/return actions.
- [ ] Match history and replay support, including collected user-coin information where applicable.
- [ ] Spectating and live-match directory only after synchronization and privacy behavior are reliable.

### Profiles, progression, and cosmetics
- [ ] Player profiles and match records.
- [ ] Wins, losses, streaks, personal records, and per-mode statistics.
- [ ] Achievements, titles, badges, and seasonal progression.
- [ ] Original cosmetic system: auras, finishers, emotes, and profile presentation.
- [ ] Cosmetic effects remain non-pay-to-win and do not affect competitive outcomes.

### Social and lobby experience
- [ ] Versus lobby/plaza with responsive layout.
- [ ] Player presence and real room/player counts.
- [ ] Emotes and optional social interactions.
- [ ] Any chat or voice feature must include mute, block, report, moderation, and privacy controls before release.

### Reliability and integrity
- [ ] Robust reconnect and connection-loss handling.
- [ ] Explicit protocol versioning and match state transitions.
- [ ] Anti-cheat integration that does not falsely claim detection capabilities.
- [ ] Compatibility checks for supported Geometry Dash, Geode, and Globed versions.
- [ ] Structured diagnostic logs for room, network, and match transitions.
- [ ] Layout testing on narrow phones and different aspect ratios.
- [ ] Never display fake ranks, rooms, players, matchmaking, or match results.

## Our additions
These remain our own product direction, separate from reference parity:
- [ ] 1v1 Race playlist.
- [ ] Survival playlist.
- [ ] Dual Challenge playlist.
- [ ] Platformer Battle playlist.
- [ ] Improved onboarding and match explanations.
- [ ] Better match recap and personal-stat dashboards.
- [ ] More original cosmetics and unlockable lobby customization.
- [ ] Developer/test tools that are restricted to the authenticated developer account and cannot alter a player's real Geometry Dash save data.

## Implementation order

1. Audit current code and preserve existing working behavior.
2. Establish a tested Globed-native room/session protocol and lifecycle.
3. Finish room creation, joining, ready-up, countdown, and cleanup.
4. Implement and validate core match rules and results.
5. Add map pools and ranked flows only after the match foundation is reliable.
6. Add profiles, progression, cosmetics, and social features.
7. Run Android end-to-end tests with two real clients before calling online features playable.

## Release gate

A checklist item is not complete until its code exists, the Android build succeeds, the UI path is exercised in Geometry Dash, and any multiplayer behavior is tested with two real clients through the supported Globed version. Mark features as planned, preview, or implemented-and-tested accurately.
