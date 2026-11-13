# Geode Versus Mode — 1.0.0

Next release target: **v1.0.6** · Current package version: **v1.0.5** · Geometry Dash 2.2081 · Geode SDK 5.11.2 · Android64, iOS, and iPadOS

## Platform builds

- Android64 and Apple mobile builds are built independently in GitHub Actions.
- The Apple mobile build uses Geode's official **iOS arm64** target on a macOS runner. This is the shared iOS/iPadOS mod binary; Geode does not use a separate iPadOS target for .geode mods.
- The same versus.mode.ios.geode package is intended for compatible iPhone and iPad installations. A separate renamed copy would not be a separate iPad build.
- iPad compatibility still needs real-device verification with Geometry Dash, the Geode iOS launcher, and a compatible Globed installation before it can be called tested or playable on iPad.
- See [iPadOS support and test plan](docs/IPADOS.md).
- Apple platform packages are distributed as Geode .geode mod packages; they are not standalone Geometry Dash apps.

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

The room/session handoff and synchronized gameplay rules still require runtime validation on Android with Globed 2.3.0 beta or newer. iPadOS compatibility likewise remains pending a real iPad test.

## Development

The complete feature plan and release gates are tracked in [docs/ROADMAP.md](docs/ROADMAP.md).
