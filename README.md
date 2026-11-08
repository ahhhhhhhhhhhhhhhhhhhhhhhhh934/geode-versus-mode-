# Geode Versus Mode

A Geometry Dash competition hub prototype for Geode. This overhaul focuses on reliable navigation, clearer Globed diagnostics, and an honest feature-status UI.

## What this version actually does

- Adds the Versus entry to the Geometry Dash Creator screen.
- Provides a dashboard and a separate **Plaza** page. The Plaza button now opens its own page instead of simply redrawing the dashboard.
- Adds dedicated room-status and voice-setup pages with refresh and navigation actions.
- Detects whether Globed is installed, enabled, loaded, connected, and whether the player is in a room / active shared level.
- Reports a missing Globed Soft Link API table immediately once Globed is loaded, rather than making the player wait through a fake connection timeout.
- Keeps the lobby accessible without Globed; online and voice information explains what dependency is missing.

## Important limitations (not represented as working features)

- **Online 1v1:** this build detects Globed connection and room state. It does not yet implement matchmaking, a Versus match protocol, or synchronized race results.
- **Voice:** voice is provided by Globed inside shared levels. The Versus voice page is a setup/status guide; it cannot directly activate the microphone because the Globed Soft Link API does not expose a voice-session control.
- **Local 1v1:** two independent player-control systems are not implemented.
- **Workshop:** no catalog, upload service, or moderation backend is connected yet.

## Globed compatibility

The mod uses Globed's documented Soft Link API. The uploaded v0.4.13 log showed Globed 2.2.2 loading and connecting while its exported API table was unavailable. Globed's upstream 2.3.0 changelog lists a fix for API functions not being exported. This mod cannot repair a missing export inside Globed itself; it now displays that failure clearly. Keep Globed optional so the Plaza and local UI can still load.

## Build

The project targets Geometry Dash 2.2081 and Geode SDK 5.11.2. For Android64, use the repository's GitHub Actions workflow:

1. Open **Actions**.
2. Run **Android64 Overhaul Build** (or push to the overhaul branch).
3. Download the `Versus-Mode-Android64-Overhaul` artifact after a successful run.

No release is published by the overhaul build workflow. The package is not considered tested until it is installed and checked in Geometry Dash on-device.
