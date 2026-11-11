# Versus Leaderboard Specification

This document defines the complete leaderboard feature. Items here describe the intended feature; they are not claims that online ranking is already live.

## Player identity and ranking
- Use a stable, verified Versus player identity linked to the player's supported account identity.
- Show global placement and rank points only when they come from the same trusted, persistent ranking dataset.
- Support rank divisions and the existing Geometry Dash difficulty-inspired ladder.
- Show rating changes after verified matches, including a clear explanation of gains/losses.
- Keep local practice points and local statistics separate from online competitive records.

## Player statistics
Each eligible player profile should expose:
- Rank points and current division.
- Wins, losses, and total completed matches.
- Win rate, calculated from verified completed matches.
- Current win/loss streak and best streak.
- Per-mode records and performance.
- Personal bests and recent match history.
- Seasonal peak and current-season placement when seasons are supported.
- Region and platform labels only when verified and privacy-safe.

Do not count cancelled, abandoned, disconnected, or otherwise invalid matches as ordinary wins/losses. Define the treatment of disconnects and forfeits in the match protocol before using them in ranking calculations.

## Leaderboard views and filters
The finished interface should include:
- Global overall rankings.
- Per-mode rankings for each supported competitive mode.
- Friends-only rankings once party/friend identity is available.
- Current season and all-time views once season data exists.
- Region filters only when region is supplied by a reliable source and the player can understand/change the setting.
- Search by verified player name or ID.
- Sort options for rank points, wins, win rate, and streak, with minimum-match requirements for rate-based sorting.
- Pagination or incremental loading for large ranking lists.
- A player row that opens the corresponding profile.

Unavailable filters must be visibly disabled or marked unavailable; never display simulated options as if they work.

## Player profile and match history
- Selecting a leaderboard row opens the player's Versus profile.
- Show verified rank, mode records, achievements/titles, and supported cosmetics.
- Provide recent match history with mode, opponent, outcome, date, and rating change when those fields are available.
- Clearly label private or hidden profile fields and respect player privacy settings.

## Integrity and fair presentation
- Only verified match results may change online rank or populate online statistics.
- Do not use local-only rank points as global leaderboard entries.
- Do not fabricate player names, scores, placements, player counts, or match records.
- Do not punish a player based only on lag, desync, or an unverified client-side suspicion.
- Cheat detection and automatic kicking are explicitly out of scope for this leaderboard work and must be planned separately if requested later.

## Architecture constraints
- Globed remains the multiplayer networking and gameplay-session foundation.
- Do not introduce Supabase or an unrelated matchmaking/results backend.
- The current Globed Soft Link integration provides room/session handoff, not a persistent Versus profile database or authoritative cross-room results service.
- A real global leaderboard therefore requires an agreed, supported persistent identity/results capability in Globed-side infrastructure or an upstream-compatible extension. The client must not claim this exists until it is implemented and tested.
- Keep the UI ready to display real data, but show an honest disconnected/ unavailable state until the trusted data source is available.

## Release acceptance criteria
1. Android build succeeds with the supported Geode and Globed versions.
2. Leaderboard navigation and all available UI states work in Geometry Dash.
3. Two real clients can complete a supported match through Globed and produce one consistent verified result.
4. Verified results persist across sessions and devices.
5. Rankings, filters, sorting, pagination, profiles, and statistics all use the same authoritative dataset.
6. Disconnects, retries, duplicate result submissions, stale data, and invalid match states are handled.
7. Tests prove that unverified/local results cannot change online rank.
8. Documentation clearly distinguishes implemented features from previews and unavailable filters.
