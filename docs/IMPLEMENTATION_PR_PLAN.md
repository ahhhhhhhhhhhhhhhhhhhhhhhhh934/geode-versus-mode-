# Versus Mode implementation PR plan

This is the execution order for completing Versus Mode. Each item is a separate pull request (or a small stack of dependent PRs). A feature is not marked complete until its code, Android build, and required in-game/two-client validation are complete.

## Rules for every PR

- Keep the Android64 / Geometry Dash 2.2081 / Geode 5.11.2 target working.
- Use Globed as the multiplayer transport and room/session foundation.
- Do not add Supabase, a Cloudflare Worker, a Render service, or a separate matchmaking/results backend.
- Do not fake online players, rooms, ratings, results, or match history.
- Keep local preview data separate from verified online data.
- Include build validation and update the feature status in the roadmap.
- Do not merge a PR until its dependencies are merged and required checks pass.
- Cheat detection and automatic match kicking are postponed and excluded from this implementation batch.

## PR stack and dependency order

### Foundation and release reliability
1. **PR-A — Build and integration baseline**
   - Audit existing GitHub Actions workflows, Geode SDK/toolchain versions, Android package output, and dependency pins.
   - Ensure pull requests receive a reproducible Android64 build check and useful artifacts/logs.
   - Add lightweight source/protocol tests where practical.
   - Acceptance: clean PR build; package metadata and target versions agree.

2. **PR-B — Versus protocol and state machine**
   - Define a versioned match/lobby protocol with explicit states: idle, joining, waiting, ready, starting, playing, finishing, completed, cancelled, disconnected, and recovering.
   - Add message validation, session/room scoping, timeouts, duplicate handling, and diagnostic logging.
   - Use only supported Globed APIs; do not invent cross-room or persistence capabilities.
   - Acceptance: deterministic state transitions and tests for invalid/out-of-order messages.

3. **PR-C — Private lobby lifecycle**
   - Implement real same-Globed-room lobby membership, player list, ready/unready, host rules, leave/cancel, and stale-lobby cleanup.
   - Add reconnect/host-departure behavior.
   - Acceptance: two Android clients can join the same room, ready, cancel, and recover from disconnects.

### Playable match modes
4. **PR-D — Shared level launch and match start**
   - Complete Globed session handoff for a selected Geometry Dash level.
   - Add a ready countdown, shared match settings, and clear failure/return paths.
   - Acceptance: two clients enter the same intended level/session and see consistent start/cancel behavior.

5. **PR-E — 1v1 Race**
   - Track validated level start, progress/completion, elapsed time, and tie rules for the race.
   - Keep results provisional until both clients agree on a valid completion state.
   - Acceptance: two-client race flow, ties, retries, and disconnect cases tested.

6. **PR-F — Survival**
   - Define the survival rules and consistent end conditions; implement match state and result presentation.
   - Acceptance: deterministic end-of-round behavior on two clients.

7. **PR-G — Dual Challenge**
   - Implement the chosen dual challenge variant, input ownership/rules, and match result conditions.
   - Acceptance: both players understand the same rules and the round resolves consistently.

8. **PR-H — Platformer Battle**
   - Add platformer-specific level selection, checkpoints, finish/tie rules, and result flow.
   - Acceptance: two clients can complete a platformer round with consistent results.

9. **PR-I — Rank-matched level catalog**
   - Build a curated level pool with verified difficulty, format/length filters, anti-repeat selection, and rank-tier mapping.
   - Connect the existing rank challenge preview to actual level selection only when the catalog is valid.
   - No wagers, paid entries, stakes, or rank-point loss.
   - Acceptance: every selected level passes metadata/difficulty checks and launches for both clients.

10. **PR-J — Practice, rematch, and match quality**
    - Add warm-up/practice flow, rematch, pause policy, return-to-lobby, clear rules, and graceful cancellation.
    - Acceptance: complete round lifecycle without stranded clients.

### Results, progression, and discovery
11. **PR-K — Verified result pipeline**
    - Define what Globed-side supported capability can safely carry match outcomes.
    - Persist nothing and award no online rating unless an authoritative, supported Globed-side result/identity facility exists.
    - If Globed does not expose this capability, prepare an upstream-compatible Globed contribution rather than adding a separate service.
    - Acceptance: duplicate/forged/local-only results cannot change online records.

12. **PR-L — Profiles, statistics, and leaderboard**
    - Finish the existing leaderboard PR after verified identity/results are available.
    - Add global and per-mode rankings, friends/season/region filters only when their data exists, search, sorting, pagination, profiles, and history.
    - Acceptance: every row and statistic comes from the same verified dataset.

13. **PR-M — Public rooms and live directory**
    - Show real Globed rooms and live activity only through supported APIs; add filters and refresh states.
    - Spectating comes later, after synchronized match state and privacy rules.
    - Acceptance: no fake rooms/counts; stale entries expire.

14. **PR-N — Parties, groups, and social safety**
    - Add party invites and friend/private match flow where identity support exists.
    - Before Versus-managed social communication, provide mute, block, report, moderation, and privacy controls.
    - Acceptance: privacy and abuse-report flows are testable.

15. **PR-O — Voice integration and safety**
    - Integrate only supported Globed voice controls/status; provide clear mute/deafen/push-to-talk guidance and safety controls.
    - Do not claim Versus owns or records voice if Globed owns the voice session.
    - Acceptance: controls and safety affordances are verified on device.

### Customization, accessibility, and release
16. **PR-P — Workshop and custom playlists**
    - Add playlist creation/sharing and level presets only with safe validation, moderation, and reporting for public content.
    - Acceptance: invalid or unsafe submissions are rejected and private sharing works.

17. **PR-Q — Achievements and cosmetics**
    - Add achievements, titles, badges, banners, and cosmetic unlocks without competitive advantages.
    - Online unlocks require verified progression; local unlocks are clearly local.
    - Acceptance: persistence and ownership rules are consistent.

18. **PR-R — Accessibility and device polish**
    - Add responsive layouts, readable text, reduced motion, touch target checks, and actionable dependency/version errors.
    - Acceptance: tested across supported Android screen sizes/aspect ratios.

19. **PR-S — Release hardening**
    - Run the full Android build/test matrix, update README/changelog/version, collect device test results, and publish a release only after the release gates pass.
    - Acceptance: fresh install/update, two-client multiplayer, reconnect, and core modes tested.

## Existing pull requests to fold into the stack

- **#12 Rank-matched level challenge preview**: keep as preview until PR-I connects a real catalog.
- **#13 Leaderboard page**: keep honest UI/spec groundwork; finish the live feature in PR-K and PR-L.
- **#11 Android release docs / obsolete deployment cleanup**: retain its Globed-only architecture decisions.
- **#10 experimental iOS workflow**: do not block Android-first completion; keep iOS experimental until independently verified.

## Explicitly deferred

- Cheat detection and automatic kicking: postponed until separately requested and designed around reliable evidence and appeal/recovery behavior.
- A separate matchmaking/results backend: not allowed in this project architecture.
- Spectating: deferred until synchronized match state, privacy, and reliable observer support exist.
- The Map: separate future project; do not start it before Versus is complete.

## Merge policy

Merge in dependency order, not by PR number. A dependent PR should target the preceding feature branch when necessary, then be retargeted to the integration branch after its parent merges. Do not merge previews as if they were playable features. Keep the main branch buildable throughout the stack.
