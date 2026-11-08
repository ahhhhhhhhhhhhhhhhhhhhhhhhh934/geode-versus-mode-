# Changelog

## v0.5.0 — Plaza and navigation overhaul
- Rebuilt the main dashboard and added a distinct Plaza page.
- Made the Plaza button navigate to a real page.
- Added a room-status page with refresh and live Globed room/player information when the API is available.
- Reworked the Voice button into a dedicated voice setup/status page, with clear explanation that Globed voice runs inside shared levels.
- Added a direct Soft Link API failure explanation when Globed is loaded but its API table is unavailable.
- Made Globed an optional runtime dependency so the Plaza and non-online UI can still open without it.
- Clearly documented which multiplayer, local-play, voice-control, and Workshop features are not implemented.

## v0.4.13
- Globed Soft Link API integration and loading diagnostics.

## v0.4.12
- Replace custom Globed event dispatch with Globed's documented Soft Link API wrappers.
- Declare Globed as an optional dependency and retry API lookup after it loads.

## v0.3.0
- Align metadata with Geode SDK 5.8.2 and Geometry Dash 2.2081.
- Add Android64 GitHub Actions build workflow.
- Clarify implemented prototype UI versus planned multiplayer features.

## v0.3.1
- Disable background menu items while the Versus lobby is open and restore them when it closes.
- Put lobby buttons inside a touch-enabled CCMenu.
- Use Geometry Dash's native Versus button artwork where available, with a fallback.
