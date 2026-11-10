# Versus Mode Matchmaking Worker

The mod's global queue needs this Worker. The Android mod cannot connect until it has been deployed and its public URL is entered in Geode settings.

## One-time Cloudflare setup

1. Create a Cloudflare account and a **Workers** API token with permission to deploy Workers and manage Workers resources.
2. In the GitHub repository, open **Settings → Secrets and variables → Actions → New repository secret**.
3. Name the secret `CLOUDFLARE_API_TOKEN` and paste the token there. Do not put the token in a source file or share it in chat.
4. Open **Actions → Deploy Versus Matchmaking Worker → Run workflow**.
5. Open the successful deployment log and copy the `workers.dev` URL Wrangler reports. The Worker name is `versus-mode-matchmaker`.
6. In Geometry Dash, open **Geode → Versus Mode → Settings** and paste that HTTPS URL into **Global Matchmaking Server URL**.

The deployment workflow is `.github/workflows/deploy-matchmaker.yml`. It deploys the Worker from this directory using the repository secret; it does not need a local Wrangler installation.

## Manual deployment alternative

1. Install Node.js and Wrangler: `npm install -g wrangler`.
2. From this directory, authenticate with Cloudflare: `wrangler login`.
3. Deploy: `wrangler deploy`.
4. Copy the deployed `https://....workers.dev` URL and enter it in the Geode setting described above.

## API

- `POST /queue/join` JSON: `{"mode":"race"}` (also `survival`, `dual`, `platformer`). Returns an opaque bearer ticket.
- `GET /queue/status` with `Authorization: Bearer <ticket>`.
- `POST /match/ready` with the same authorization header.
- `POST /queue/leave` with the same authorization header.

The Worker pairs players by mode and requires both players to accept before reporting `ready`. It does not collect account passwords or Geometry Dash credentials.

## Important scope

This is the matchmaking and ready-check service only. A ready match is **not yet a playable synchronized match**: the mod still needs a game-session transport, level/mode handoff, player-state replication, latency handling, disconnect recovery, and result validation. Those must be implemented and tested before advertising online gameplay as complete.
