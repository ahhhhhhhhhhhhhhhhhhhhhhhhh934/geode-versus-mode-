# Geode Versus Mode — 1.0.1

Version target: **v1.0.1** · Geometry Dash 2.2081 · Geode SDK 5.11.2

## Platform support

- **Android64:** supported build target; Android CI produces the installable `.geode` artifact.
- **iOS arm64:** new experimental build target. iOS CI compiles the mod and publishes the resulting package/binary as a separate artifact. It is not called stable until installed and tested on a real iPhone/iPad with the current Geode iOS launcher.
- **Windows/macOS:** not currently configured as release targets.

The iOS build uses Geode's iOS cross-compilation toolchain. Geode's iOS launcher supports iOS/iPadOS 14 or later, but installation and JIT/JIT-less capabilities depend on the device, iOS version, and installation method.

## Multiplayer architecture

- **Globed 2.3.0 beta or newer:** connection, rooms, multiplayer sessions, player synchronization, and gameplay networking.
- **Versus Mode:** lobby, mode selection, match rules, and results UI.
- No Supabase, Cloudflare Worker, Render service, or separate gameplay server is required or configured.

## Current implementation

- Level Complete-inspired Geometry Dash lobby styling.
- Globed connection detection and room status.
- Local rank/profile preview and Versus mode-selection UI.
- Private play uses Globed rooms directly.
- Android build workflow.
- Experimental iOS arm64 build workflow.

## Important limitations

Globed provides multiplayer networking and room infrastructure, but it does not automatically provide Versus Mode's custom cross-room matchmaking queue, ready-check, mode rules, ranked service, or authoritative results. Those features still need a Globed-native protocol and server-side support before they can be advertised as working.

The room/session handoff and synchronized gameplay rules still require runtime validation with two real clients. The iOS build also requires device testing before it can be called supported/stable.

## Development

See [the Versus Mode roadmap](docs/ROADMAP.md) for the feature plan and release gates.
