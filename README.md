# Geode Versus Mode — Android build project

Target: Geometry Dash 2.2081 / Geode SDK 5.11.2 / Android 64-bit.

## Globed requirement

Versus Mode targets **Globed 2.3.0 beta or newer**. The 2.3.0 API adds the session-warp handoff needed for a matchmaking result to send both players into the same Globed session. The build fetches the current Globed nightly headers because the stable v2.2.2 headers do not contain that API.

Install a compatible Globed 2.3.0 beta build before testing online integration. Do not downgrade to Globed 2.2.2; it does not provide the required handoff API.

## Current implementation status
- Creator menu Versus entry and lobby UI.
- Globed connection detection through its Soft Link API.
- Compile-time integration with the Globed 2.3.0 session-warp API, guarded by a runtime version check.
- Global matchmaking queue client for Race, Survival, Dual, and Platformer.
- **Not yet complete:** the matchmaking response must supply a validated Globed `SessionId`; both clients must receive the same session handoff; gameplay rules/state synchronization, disconnect recovery, and server-validated results remain to be implemented.
- Voice chat is provided by Globed where supported; Versus does not implement a separate voice transport.

## Build an Android64 package

Open **Actions → Build Android Geode mod → Run workflow**. A successful run uploads the `.geode` package as the Versus-Mode-Android64 artifact.

For a local build, install the Android NDK and Geode CLI/SDK, then run:
```sh
geode sdk install-binaries -p android64
geode build -p android64
```

## Matchmaking

The existing queue client still speaks to the current matchmaking API. The previous Cloudflare Worker is not being treated as a gameplay server. Matchmaking only finds and pairs players; Globed remains responsible for multiplayer sessions and gameplay networking. The backend must return a trusted Globed session descriptor before Versus can automatically call the 2.3.0 session-warp API.
