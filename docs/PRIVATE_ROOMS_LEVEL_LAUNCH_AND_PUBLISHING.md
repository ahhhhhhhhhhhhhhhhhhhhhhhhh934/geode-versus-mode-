# Private rooms, level launch, and publishing plan

Date: 2026-11-11

This document records the next client features and the verified integration boundaries. It deliberately distinguishes existing Globed functionality from features Versus Mode still needs to implement.

## 1. Private Versus rooms with friends

### Reuse Globed's private-room system

Globed already supports public and private rooms, room passwords, invite-only rooms, player limits, room invites, and friend-oriented room discovery. Versus Mode should reuse those rooms instead of inventing a second room server or storing room passwords in a separate service.

Expected Versus flow:

1. Player opens **Private Versus** from the Versus Plaza.
2. If they are not in a Globed room, Versus directs them to Globed's native room UI to create or join a private room.
3. The room owner sets a room name, password/invite-only access, and player limit in Globed.
4. Friends join using Globed's invite or room-join flow.
5. Once everyone is in the same Globed room, the owner selects a Versus mode and level.
6. Versus validates that all participants are in that room, then starts the selected shared level/session.
7. The match HUD and mode rules are supplied by Versus; player synchronization and voice remain Globed responsibilities.

### API boundary

The documented Soft Link API exposes room state and session construction (including `isInRoom()`, `getId()`, and `makeSessionId(levelId)`). It does not document the complete room create/join/invite UI flow. The old Globed Link API exposes more internals, but Globed explicitly warns that it is not actively maintained and is not suitable for mods intended for the Geode Index.

Therefore the safe first integration is to call or open Globed's existing room UI through a supported public entry point, not to depend on undocumented private C++ classes. The current branch does not yet contain a verified public Soft Link function that opens the room creator. Until that entry point is confirmed against the pinned 2.3.0-beta headers, the Versus button must not pretend to create a room.

## 2. Button that opens a selected Geometry Dash level

Yes. Geode supports adding clickable menu items: its official button tutorial uses `CCMenuItemSpriteExtra` with a `menu_selector` callback, and its class-modification tutorial shows adding that item to a layer's `CCMenu`.

For a level-launch button, the callback should:

1. Resolve a real `GJGameLevel` object (from a saved level or a completed online-level search result).
2. Validate that the object exists and has usable level data.
3. Open the normal Geometry Dash level flow for that object, rather than fabricating a level ID or calling a session warp with a made-up ID.
4. If the user is in a Globed room, create the session using Globed's supported `makeSessionId(levelId)` and hand off through the 2.3.0+ `warpToSession` API only after the room and level are valid.

Geode's official button documentation verifies the button/callback part. It does not by itself prove a safe level-loading call signature. The level-launch implementation must be checked against the exact Geometry Dash 2.2081 bindings in this repository's build environment before shipping. The button should initially open a level picker, not silently launch a hard-coded level.

## 3. Workshop / mod distribution

There is no Steam Workshop target configured in this repository. Versus Mode is a Geode mod, so its intended public distribution route is the Geode Index, not a Steam Workshop upload.

Before submitting it to the Geode Index, the project needs:

- a successful Android build for the declared Geometry Dash / Geode target;
- a working, installable `.geode` package;
- verified runtime dependency metadata for Globed;
- no placeholder or nonfunctional matchmaking claims in the mod description;
- tested private-room and level-launch flows;
- screenshots, description, and release notes;
- a clean CI run and a reviewable release commit.

The current branch is not ready for an Index submission: the online queue still references the old Supabase prototype, the room-creation UI entry point is not verified, and level launching is not implemented or tested. Publishing before those are fixed would ship buttons that imply functionality the mod does not yet provide.

## 4. Implementation order

1. Replace the misleading Group Rooms placeholder with a clearly named **Private Versus** screen and room-status checks.
2. Verify a supported Globed 2.3.0-beta route into its native create/join room UI.
3. Add a level picker backed by actual saved/online `GJGameLevel` objects.
4. Add the validated launch callback and Globed session handoff.
5. Add mode selection and match start for everyone in the private room.
6. Remove the obsolete Supabase queue dependency only when the Globed-native replacement exists and is tested.
7. Build, test on Android, then prepare the Geode Index submission.

## Official references

- Geode button tutorial: https://docs.geode-sdk.org/tutorials/buttons/
- Geode class modification tutorial: https://docs.geode-sdk.org/tutorials/modify/
- Globed feature list (including public/private rooms): https://github.com/GlobedGD/globed2
- Globed Link API warning and room-manager examples: https://github.com/GlobedGD/globed2/blob/main/docs/link-api.md
- Globed 2.3.0 changelog and Soft Link API expansion: https://github.com/GlobedGD/globed2/blob/main/changelog.md
