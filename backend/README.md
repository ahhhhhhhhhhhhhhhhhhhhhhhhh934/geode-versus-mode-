# Versus Mode Matchmaking Worker

This is the small server required by the in-mod global queue client.

## Deploy

1. Install Node.js and Wrangler: `npm install -g wrangler`.
2. From this directory, authenticate with Cloudflare: `wrangler login`.
3. Deploy: `wrangler deploy`.
4. Copy the deployed `https://....workers.dev` URL.
5. In Geometry Dash, open Geode settings for Versus Mode and set **Global Matchmaking Server URL** to that URL.

## API

- `POST /queue/join` JSON: `{"mode":"race"}` (also `survival`, `dual`, `platformer`). Returns an opaque bearer ticket.
- `GET /queue/status` with `Authorization: Bearer <ticket>`.
- `POST /match/ready` with the same authorization header.
- `POST /queue/leave` with the same authorization header.

The Worker pairs players by mode and requires both players to accept before reporting `ready`. It does not collect account passwords or Geometry Dash credentials.

## Important scope

This is the matchmaking and ready-check service only. A ready match is **not yet a playable synchronized match**: the mod still needs a game-session transport, level/mode handoff, player-state replication, latency handling, disconnect recovery, and result validation. Those must be implemented and tested before advertising online gameplay as complete.
