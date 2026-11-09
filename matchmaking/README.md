# Versus global matchmaking service

This is the first server-side slice of the global queue. It is deliberately separate from the Geode mod so the queue can coordinate players who are currently in different Globed rooms.

## Current protocol

- `GET /health` — service health.
- `POST /queue/join` — JSON body: `{"mode":"race","region":"global","ranked":false}`.
- `GET /queue/status` — requires `Authorization: Bearer <ticket>`.
- `POST /queue/leave` — requires the same bearer ticket.
- `POST /match/ready` — confirms a pending ready check.
- `POST /match/cancel` — cancels a pending match.

Supported casual modes: `race`, `survival`, `dual`, and `platformer`. The Durable Object serializes queue mutations and persists queue/match state. Queue tickets are random opaque capabilities, not player account identities.

## Important limitations (intentional)

1. Ranked matchmaking is rejected. The service does not yet authenticate a Geometry Dash account or verify match results, so it must not award rank points.
2. A successful pair is a global matchmaking result, not yet a playable match. Globed room creation/assignment and the in-game client protocol are separate integration work. The API explicitly returns no room assignment rather than pretending both clients were moved.
3. Casual tickets are temporary bearer credentials. Do not share them. This first slice is a prototype and is not a substitute for account authentication, abuse controls, moderation, or production security review.
4. No public endpoint is deployed by this repository. Deployment requires a Cloudflare account and Worker/Durable Object provisioning.

## Deploy

From this directory, with Wrangler authenticated to the project owner's Cloudflare account:

```sh
npx wrangler deploy
```

After deployment, verify `GET /health`. Do not configure the game client to use a URL until deployment and endpoint tests are complete.

## Next integration stages

- Add a verified identity handshake and per-account rate limits.
- Connect the Geode client to queue join/status/leave and ready-check endpoints.
- Integrate Globed room allocation and invite/join flow so both matched players enter the same match room.
- Add server-authoritative match lifecycle/results before enabling ranked.
- Add abuse reporting, blocking, and operational monitoring before public launch.

## One-click GitHub deployment

The repository includes a manual GitHub Actions deploy job. Before running it, add these repository Actions secrets:

- `CLOUDFLARE_API_TOKEN` — a Cloudflare API token with permission to deploy Workers and manage Durable Objects for this account.
- `CLOUDFLARE_ACCOUNT_ID` — the Cloudflare account ID.

Then open **Actions → Matchmaking Worker Validation → Run workflow** on the `feature/global-matchmaking-backend` branch. The deploy job publishes the Worker. The public Worker URL is shown in the deploy logs. Add that HTTPS URL to the Geode mod's **Global Matchmaking Server URL** setting.

Never commit the API token or paste it into source files. The deploy job is manual and does not run on ordinary pushes or pull requests.
