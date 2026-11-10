# Geode Versus Mode — Android build project

Target: Geometry Dash 2.2081 / Geode SDK 5.11.2 / Android 64-bit.

## Globed requirement

Versus Mode targets **Globed 2.3.0 beta or newer**. The 2.3.0 API is the target for session-warp handoff. Do not downgrade to stable Globed 2.2.2.

## Multiplayer architecture

- **Globed 2.3.0 beta:** actual multiplayer sessions, player synchronization, and gameplay networking.
- **Supabase:** matchmaking queue, pairing, and ready-check only.
- **Versus Mode:** selected mode, queue UI, match rules, and the eventual handoff into a validated Globed session.

No Cloudflare Worker, Render service, or separate gameplay server is used.

## Current implementation status

- Creator menu Versus entry and lobby UI.
- Globed Soft Link connection detection.
- Global queues for Race, Survival, Dual, and Platformer.
- Supabase Edge Function and SQL migration for pairing and ready-check.
- **Not yet complete:** automatic handoff to a real Globed session, gameplay rules/state synchronization, disconnect recovery, and server-validated results.
- Voice chat is provided by Globed where supported; Versus does not implement a separate voice transport.

## Configure matchmaking

Follow [backend/README.md](backend/README.md) to deploy Supabase. Set the Geode setting **Supabase Matchmaking Function URL** to the deployed Edge Function URL ending in `/functions/v1/versus-matchmaking`.
