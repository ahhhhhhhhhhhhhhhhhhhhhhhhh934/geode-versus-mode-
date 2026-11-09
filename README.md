# Geode Versus Mode

A Geometry Dash competition hub for Geode, targeting Geometry Dash **2.2081**, Geode **5.11.2**, Android64.

## v0.6.0 interface overhaul

- Reworked the dashboard and Plaza navigation with separate screens.
- Added Ranked, Profile, Public Rooms, Group Rooms, and LIVE destinations.
- Added a rank ladder using Geometry Dash's classic difficulty names: Auto, Easy, Normal, Hard, Harder, Insane, Easy Demon, Medium Demon, Hard Demon, Insane Demon, and Extreme Demon.
- Added a local rank-points display. These points are a local UI/profile placeholder; they are not a verified online rating and are not awarded for online wins yet.
- Added distinct Globed states for installed, enabled, loaded, Soft Link API available, and connected.
- Added a safe fallback for the Creator screen entry if a texture-pack sprite is missing.
- Updated the Globed headers to the upstream commit containing the Soft Link API export fix.

## Globed compatibility

The uploaded device log shows Globed **2.2.2** loading and connecting while Versus **0.4.13** cannot obtain the Soft Link API table. Globed's upstream v2.3.0 changelog lists a fix for API functions not being exported. This project now builds against the upstream commit containing that fix and reports the missing API separately from network connection state.

The later log also shows a separate server connection timeout after Globed had connected. Versus can report connected/offline status when the Soft Link API is available, but it cannot repair a timeout inside Globed or its server connection.

## Honest implementation status

- **Plaza / navigation / rank ladder / profile UI:** implemented in this branch.
- **Globed status:** reads the documented Soft Link API when the installed Globed build exports it.
- **Ranked matchmaking, public/group room directories, LIVE match feed, synchronized Versus gameplay, local split controls, and server-backed profiles:** not implemented yet. Those screens explain their status instead of showing fake rooms, players, or results.
- **Voice:** Globed handles voice inside shared levels. Versus does not directly activate the microphone or manage voice sessions.

## Build

The GitHub Actions Android64 workflow builds the .geode package for Geometry Dash 2.2081 / Geode 5.11.2. A successful CI build is not the same as on-device testing; the new package must still be installed and checked in Geometry Dash.
