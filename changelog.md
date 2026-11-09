# Changelog

## v0.6.2 — Globed 2.3.0 Soft Link API repair
- Replaced the direct Link API singleton calls that failed at link time with Globed's documented Soft Link API wrappers.
- Requires Globed 2.3.0, which fixes the missing exported API table reported by the user's 2.2.2 log.
- Reads connection state and room ID through the Soft Link API.
- Removed the unnecessary direct Link API dependency include path.

## v0.6.0 — Plaza, ranks and integration overhaul
- Rebuilt dashboard and Plaza navigation as separate screens with clear navigation state.
- Added Ranked and Profile pages.
- Added a rank ladder based on Geometry Dash difficulty names from Auto through Extreme Demon.
- Added Public Rooms, Group Rooms, and LIVE destinations with honest service-status messages.
- Added a fallback button sprite for the Creator screen entry if a texture-pack frame is missing.
- Improved Globed status messaging so missing Soft Link API exports are distinct from server disconnections.
- Updated Globed headers to the upstream commit containing the Soft Link API export fix.
- Fixed release workflow to attach the actual generated .geode package.

## v0.5.0 — Plaza and navigation overhaul
- Added a distinct Plaza page and room/voice status screens.
- Added Globed connection and Soft Link API diagnostics.

## v0.4.13
- Globed Soft Link API integration and loading diagnostics.

## v0.4.12
- Replace custom Globed event dispatch with Globed's documented Soft Link API wrappers.
- Declare Globed as an optional dependency and retry API lookup after it loads.

## v0.3.0
- Initial Versus hub prototype.
