# Geode Versus Mode — 1.0.2

Version target: **v1.0.2** · Geometry Dash 2.2081 · Geode SDK 5.11.2 · Android64

## Platform focus

- **Android64:** primary supported build target. The Android GitHub Actions workflow builds and uploads the installable `.geode` package.
- **iOS:** not a current release target. Android development and runtime validation take priority.
- **Windows/macOS:** not configured as release targets.

## Multiplayer architecture

- **Globed 2.3.0 beta or newer:** connection, rooms, multiplayer sessions, player synchronization, and gameplay networking.
- **Versus Mode:** lobby, mode selection, match rules, and results UI.
- No Supabase, Cloudflare Worker, Render service, or separate gameplay server is part of the intended architecture.

## Current implementation

- Level Complete-inspired Geometry Dash lobby styling.
- Globed installation, enabled-state, API, connection, and room detection.
- Private Versus room-status screen that reuses Globed rooms.
- Local rank/profile preview and Versus mode-selection UI.
- Android64 build and release workflow.

## Important limitations

Globed provides multiplayer networking and room infrastructure, but it does not automatically provide Versus Mode's custom cross-room matchmaking queue, ready-check, mode rules, ranked service, room directory, or authoritative results. Those features must be implemented and tested before they can be advertised as working.

The current room/session handoff is groundwork, not proof of a complete playable match. It still requires runtime validation on two Android clients with the supported Globed version. Local rank points are not an online rating.

## Development and release gates

See [the Versus Mode roadmap](docs/ROADMAP.md) for planned features and acceptance criteria.

A feature is not considered playable until the Android build succeeds, its UI works in Geometry Dash, two real Android clients complete the flow through Globed, and disconnect/retry states are tested.
