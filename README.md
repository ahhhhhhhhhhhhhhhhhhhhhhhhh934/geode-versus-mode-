# Geode Versus Mode

A Geometry Dash competition hub for Geode, targeting Geometry Dash **2.2081**, Geode **5.11.2**, Android64.

## v0.6.1 Globed integration repair

The supplied device log showed Globed **2.2.2** loaded and connected, but the Soft Link API table was not exported by that build. The upstream Globed changelog lists an API-export fix in v2.3.0. Rather than requiring the user to update Globed, this version switches to Globed's direct Link API, which is available in the user's existing v2.2.2 build.

- Uses the version-matched Globed 2.2.2 headers through Geode's dependency include directory.
- Declares Globed 2.2.2 as a required dependency because the Link API requires it.
- Reads actual connection state and room ID from Globed's NetworkManager and RoomManager.
- Removes Soft Link table lookup, the misleading API export warning, and unsupported player-count calls.
- Keeps Globed connection diagnostics separate from Versus matchmaking.

**Important:** Globed documents its Link API as ABI-sensitive and not actively maintained. This build pins the dependency to 2.2.2 to match the linked API. Do not change the Globed dependency version without reviewing the API headers and rebuilding/testing.

## Honest feature status

- **Plaza / navigation / rank ladder / profile UI:** implemented in this branch.
- **Globed connection and room status:** uses the direct Link API for Globed 2.2.2.
- **Ranked matchmaking, public/group room directories, LIVE match feed, synchronized Versus gameplay, local split controls, and server-backed profiles:** not implemented yet.
- **Voice:** Globed handles voice inside shared levels. Versus does not directly activate the microphone or manage voice sessions.

A successful CI build is not the same as on-device testing. Install the package and verify Globed status on the target phone before merging.
