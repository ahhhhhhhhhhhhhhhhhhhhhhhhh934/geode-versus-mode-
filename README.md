# Geode Versus Mode — 1.0.0

Version target: **v1.0.5** · Geometry Dash 2.2081 · Geode SDK 5.11.2 · Android64 and iOS

## Platform builds

- Android64 and iOS packages are built independently in GitHub Actions.
- The iOS target uses the official Geode iOS build target on a macOS runner.
- iOS packages are distributed as Geode `.geode` mod packages; they are not standalone Geometry Dash apps.
- The iOS build must pass CI and then be tested in Geometry Dash with the iOS Geode launcher and a compatible Globed installation before it is called playable.

## Multiplayer architecture

- **Globed 2.3.0 beta or newer:** connection, rooms, multiplayer sessions, player synchronization, and gameplay networking.
- **Versus Mode:** lobby, mode selection, match rules, and results UI.
- No Supabase, Cloudflare Worker, Render service, or separate gameplay server is required or configured.

## 1.0.0 status

- Level Complete-inspired Geometry Dash lobby styling.
- Globed connection detection and room status.
- Local rank/profile preview and Versus mode-selection UI.
- Private play uses Globed rooms directly.

## Important current limitations

Globed provides multiplayer networking and room infrastructure, but it does not automatically provide Versus Mode's custom cross-room matchmaking queue, ready-check, mode rules, or authoritative results. Those features must be implemented as a Globed-native protocol before they can be advertised as working. This release does not require external Supabase configuration and does not claim a global automatic queue is live.

The room/session handoff and synchronized gameplay rules still require runtime validation on Android with Globed 2.3.0 beta or newer.

## Development

The complete feature plan and release gates are tracked in [`docs/ROADMAP.md`](docs/ROADMAP.md).
