# Versus Mode Feature PR Rollout

This is the implementation and merge sequence for completing Versus Mode. Each feature is delivered in a focused pull request, built and tested before it is merged. A checklist item is not complete merely because a screen or specification exists.

## Working rules

- Android64 is the primary release target; keep other supported targets intact unless a verified blocker requires a separate decision.
- Globed is the multiplayer transport and gameplay-session foundation.
- Do not add Supabase, a separate matchmaking service, Cloudflare Worker, or another standalone backend.
- Do not represent same-room messaging as global matchmaking.
- Never fabricate online players, rooms, ratings, match results, or statistics.
- Cheat detection and automatic match kicking are postponed and are not part of this rollout until explicitly requested again.
- No wagers, entry fees, or loss of rank points as stakes.
- A PR is merge-ready only after its dependencies are merged, Android CI passes, and its behavior is tested to the extent the available environment permits. Clearly identify any required two-device/manual test.
- Merge in dependency order. Do not merge a PR just because it is open or marked mergeable.

## PR sequence

### Track 0 — Existing work and integration
- [ ] PR #11: Android release cleanup and removal of obsolete matchmaking deployment. Review current checks and merge only after required checks pass.
- [ ] PR #12: rank-matched challenge preview. Keep the preview clearly distinguished from a connected level catalog.
- [ ] PR #13: leaderboard page/specification. Keep online data unavailable until a trusted identity and result source exists.
- [ ] Rebase/retarget dependent PRs as their prerequisites merge; verify the resulting diff contains only the intended feature.

### Track 1 — Multiplayer protocol and lobby lifecycle
1. **Protocol foundation** — versioned message types, validation, room-scoped player identity, bounded payloads, explicit states, compatibility checks, and diagnostic logging.
2. **Lobby lifecycle** — join/leave, ready/unready, host ownership, stale member cleanup, cancellation, duplicate-message handling, and reconnect behavior.
3. **Match launch** — deterministic level/rules selection, session creation and handoff through supported Globed APIs, synchronized start, and return-to-lobby.
4. **Match completion** — explicit finish/forfeit/cancel/disconnect states, result reconciliation, and no client-only ranked awards.

### Track 2 — Playable core modes
5. **1v1 Race** — shared level launch, synchronized start, finish/progress comparison, tie handling, and rematch.
6. **Platformer Battle** — platformer-specific level validation, checkpoint/progress tracking, finish and timeout rules.
7. **Survival** — defined survival objective, elimination/timeout semantics, and result resolution.
8. **Dual Challenge** — defined dual playlist variants, player role/control rules, and result resolution.
9. **Classic Level Race** — classic-mode progress and completion timing.
10. **Warm-up / practice room** — unranked practice with clear transition into a match.

Each mode PR must include mode-specific rules, invalid-state handling, and a test plan. A mode is not marked playable until two clients complete a real match.

### Track 3 — Ranked progression and leaderboard
11. **Verified result pipeline** — one consistent match result per match, duplicate protection, disconnect/forfeit policy, and a trusted source of truth.
12. **Player identity and profiles** — stable supported identity, privacy-safe profile fields, and local-vs-online data separation.
13. **Rating and divisions** — placement, rating changes, division thresholds, and season rules derived only from verified results.
14. **Leaderboard data integration** — global and per-mode lists, pagination, search, sorting, friends/region/season filters only when their data sources exist.
15. **Match history and statistics** — wins/losses, win rate, streaks, personal bests, and per-mode records calculated from verified matches.
16. **Ranked challenge catalog** — curated difficulty-matched levels, anti-repeat selection, launch integration, and verified completion.

**Architecture gate:** current Globed room/session handoff does not itself provide a persistent cross-room queue, account database, or authoritative Versus results store. Before implementing global matchmaking or persistent global rankings, identify and validate a supported Globed-side/upstream capability. If it does not exist, prepare the required upstream Globed contribution/protocol proposal; do not silently introduce a prohibited separate service or pretend the feature is live.

### Track 4 — Social, rooms, and community
17. **Public room directory** — real room discovery and live counts from a supported source.
18. **Parties and friend matches** — invites, party lifecycle, private match setup, and privacy controls.
19. **Groups and team playlists** — group membership, team rules, and reliable room state.
20. **Spectating and live match directory** — only after gameplay state, privacy, and synchronization are stable.
21. **Community playlists/workshop** — saved challenge presets, sharing, moderation, reporting, and safe upload constraints.

### Track 5 — Player experience and release quality
22. **Achievements and cosmetics** — earned/unlocked presentation; cosmetics must not affect competitive gameplay.
23. **Voice experience** — use Globed-supported voice capabilities; include mute, block, report, moderation, and privacy controls.
24. **Accessibility and responsive UI** — readable text, reduced motion, layout scaling, and input clarity.
25. **Reliability and diagnostics** — reconnect UX, version mismatch handling, structured logs, crash/failure fixes, and performance.
26. **Release integration** — changelog, version bump, Android64 package build, installation smoke test, two-client test matrix, and release notes.

## Merge gates

For every PR:
1. Confirm the base branch and dependency PRs are correct.
2. Review the complete diff for unrelated changes and accidental secrets.
3. Run the relevant checks; require Android64 build success for code changes.
4. Test in Geometry Dash when the feature has an in-game path.
5. For online flows, test at least two clients in a real Globed room and record what was/was not verified.
6. Update the roadmap and PR description to distinguish implemented, preview, blocked, and planned behavior.
7. Merge only after the gates pass; then use the merged commit as the base for the next dependent PR.

## Status vocabulary

- **Implemented:** code exists and the stated behavior is verified.
- **Preview:** UI or local-only demonstration; not an online feature.
- **Blocked:** implementation depends on a missing upstream/API capability.
- **Planned:** not implemented yet.

This plan intentionally treats the existing leaderboard and rank-challenge PRs as groundwork, not as finished online systems.
