# Geode Versus Mode — 1.0.0 launch line

Version target: **v1.0.0** · Geometry Dash 2.2081 · Geode SDK 5.11.2 · Android64

## Globed requirement

Versus Mode targets **Globed 2.3.0 beta or newer**. The 2.3.0 API is the target for session-warp handoff. Do not downgrade to stable Globed 2.2.2.

## Multiplayer architecture

- **Globed 2.3.0 beta:** room membership, multiplayer sessions, player synchronization, and gameplay networking.
- **Versus Mode:** lobby, private-room flow, mode selection, match rules, and results UI.
- A true cross-room global queue still requires server-side support in Globed's central server. The existing Supabase prototype is not part of the intended final architecture.

## 1.0.0 launch-line status

- Level Complete-inspired Geometry Dash lobby styling.
- Dedicated Private Versus screen with Globed room detection.
- Globed connection diagnostics, ranks, profile, and mode-selection screens.
- **Still required before a fully functional public release:** validated Globed room creation/join flow, level picker and launch, server-authoritative matchmaking, synchronized gameplay rules/results, Android runtime testing, and Geode Index approval.

## Current implementation status

- Creator menu Versus entry and lobby UI.
- Globed Soft Link connection detection.
- Global queues for Race, Survival, Dual, and Platformer.
- Supabase Edge Function and SQL migration for pairing and ready-check.
- **Not yet complete:** automatic handoff to a real Globed session, gameplay rules/state synchronization, disconnect recovery, and server-validated results.
- Voice chat is provided by Globed where supported; Versus does not implement a separate voice transport.

## Configure matchmaking

Follow [backend/README.md](backend/README.md) to deploy Supabase. Set the Geode setting **Supabase Matchmaking Function URL** to the deployed Edge Function URL ending in `/functions/v1/versus-matchmaking`.

## Private rooms, level launching, and publishing

The implementation plan for private friend rooms, a level-picker button, and public distribution is documented in [docs/PRIVATE_ROOMS_LEVEL_LAUNCH_AND_PUBLISHING.md](docs/PRIVATE_ROOMS_LEVEL_LAUNCH_AND_PUBLISHING.md).

Globed already provides private rooms and invites, so Versus Mode should reuse those rather than create a separate room backend. The new Versus private-room flow and level-launch button are not yet implemented or verified. The intended distribution target is the **Geode Index**; this repository is not configured as a Steam Workshop project.
