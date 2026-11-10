# Geode Versus Mode

A Geometry Dash competition hub for Geode, targeting Geometry Dash **2.2081**, Geode **5.11.2**, Android64.

## v0.6.5 — Stable Globed 2.2.2

This release pins the required Globed dependency and public headers to stable **2.2.2**.

- Globed is a required dependency.
- CMake and Geode metadata both require Globed 2.2.2.
- The current build pipeline targets Android64.

**Important:** A successful CI build confirms compilation and packaging, not successful Soft Link API communication on a device. The previous device log reported that Globed 2.2.2 loaded and connected but did not return the exported Soft Link API table. That runtime compatibility issue still needs device verification.

## Honest feature status

- **Plaza / navigation / rank ladder / profile UI:** implemented in this branch.
- **Globed room lobby foundation:** room-scoped join/ready/leave/heartbeat events use Globed's central Server Events API; players must first join the same Globed room. Android device testing is still required.
- **Automatic public-room discovery, cross-room pairing, automatic room transfer, synchronized Versus gameplay, ranked results, local split controls, and server-backed profiles:** not implemented yet.
- **Voice:** Globed handles voice inside shared levels. Versus does not directly activate the microphone or manage voice sessions.

A successful CI build is not the same as on-device testing. Verify Globed status on the target phone before merging.


## Versus room lobby (development branch)

The lobby no longer needs a separate Cloudflare Worker. It uses Globed's central Server Events API to exchange small lobby-state packets between connected players. Each packet includes a Globed room ID; clients ignore events for other rooms. Players select Race, Survival, Dual, or Platformer, then publish ready state. Heartbeats expire stale lobby entries.

### Current boundaries

- Players must manually create or join the same Globed room using Globed's existing room UI before entering the Versus lobby.
- The lobby can coordinate presence and ready state among clients in that room.
- This does not yet create or transfer players into a shared gameplay session, start a level, or synchronize Race/Survival/Dual/Platformer rules.
- The feature needs an Android64 CI build and a two-device test with Globed 2.2.2 before it can be called working.
- Central Server Events are used only for low-frequency lobby messages, not frame-by-frame gameplay networking.
