# Geode Versus Mode — Android build project

Target: Geometry Dash 2.2081 / Geode SDK latest stable / Android 64-bit.

## Current implementation status
- Creator menu Versus entry: hooked into CreatorLayer (the Create/Saved/Scores/Quests screen).
- Versus lobby screen: implemented in source.
- Local 1v1: UI placeholder; independent player controls are not implemented.
- Global matchmaking: queue client for Race, Survival, Dual, and Platformer, with a Cloudflare Durable Object worker for cross-room pairing and two-player ready checks.
- Online gameplay synchronization: not implemented yet. A successful ready check does not launch or synchronize a Geometry Dash match.
- Voice chat: not implemented; Globed's audio API needs a separate audio integration.
- Workshop: not implemented; Geode mod portal integration remains a separate task.

## Build an actual .geode package
This repository includes a GitHub Actions workflow at .github/workflows/build-android.yml. Open **Actions → Build Android Geode mod → Run workflow** to start an Android64 build. A successful run uploads the .geode package as the Versus-Mode-Android64 artifact.

For a local build, install the Android NDK and Geode CLI/SDK 5.11.2, then run:
```sh
geode sdk install-binaries -p android64
geode build -p android64
```

A compiled .geode file is not included until a build succeeds.


Build pipeline notes: Android builds use the latest stable Geode SDK to avoid pinning a broken release metadata lookup. iOS builds run on macOS using the official Geode build action, not a nonexistent Docker image.


## Global matchmaking backend

The deployable Cloudflare Worker is in `backend/`. Follow `backend/README.md` to deploy it, then set the **Global Matchmaking Server URL** in Geode's Versus Mode settings to the Worker URL. Until a server URL is configured, the queue will explain that the backend is not configured rather than pretending to find players.

The queue pairs players by game mode and requires both players to accept. This is a matchmaking milestone, not yet a complete playable multiplayer session: gameplay transport, level handoff, player-state synchronization, disconnect recovery, and result validation remain separate work.
