# Geode Versus Mode

A Geometry Dash competition hub for Geode, targeting Geometry Dash **2.2081**, Geode **5.11.2**, Android64.

## v0.6.6 — Stable Globed 2.2.2

This release pins the required Globed dependency and public headers to stable **2.2.2**.

- Globed is a required dependency.
- CMake and Geode metadata both require Globed 2.2.2.
- The current build pipeline targets Android64.

**Important:** A successful CI build confirms compilation and packaging, not successful Soft Link API communication on a device. The previous device log reported that Globed 2.2.2 loaded and connected but did not return the exported Soft Link API table. That runtime compatibility issue still needs device verification.

## Honest feature status

- **Plaza / navigation / rank ladder / profile UI:** implemented in this branch.
- **Globed connection and room status:** integration is under compatibility testing against 2.2.2.
- **Ranked matchmaking, public/group room directories, LIVE match feed, synchronized Versus gameplay, local split controls, and server-backed profiles:** not implemented yet.
- **Voice:** Globed handles voice inside shared levels. Versus does not directly activate the microphone or manage voice sessions.

A successful CI build is not the same as on-device testing. Verify Globed status on the target phone before merging.
