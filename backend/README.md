# Supabase matchmaking

Supabase replaces the previous Cloudflare Worker. It is matchmaking only; **Globed 2.3.0 beta or newer remains responsible for actual game sessions, player synchronization, and gameplay networking.**

## Deploy

1. Create a Supabase project.
2. Apply `supabase/migrations/20261110000000_versus_matchmaking.sql` in the SQL editor or with the Supabase CLI.
3. Deploy the Edge Function with `supabase functions deploy versus-matchmaking`.
4. The function uses the project's `SUPABASE_URL` and `SUPABASE_SERVICE_ROLE_KEY` secrets. Never put the service-role key in the mod.
5. Set Geode's **Supabase Matchmaking Function URL** setting to `https://YOUR_PROJECT_REF.supabase.co/functions/v1/versus-matchmaking`.

The client appends `/queue/join`, `/queue/status`, `/match/ready`, and `/queue/leave` to that base URL.

## Scope and limitations

- Race, Survival, Dual, and Platformer queues are supported.
- Random UUID tickets act as bearer credentials.
- The database serializes pairing and expires stale searching entries.
- Ready-check only confirms both clients accepted. It does **not** create a Globed room/session.
- A validated Globed session descriptor and automatic 2.3.0-beta handoff are still required before matches can start. The backend deliberately does not fabricate session IDs.
- Add per-install rate limits, authenticated identity or abuse controls, and monitoring before opening a large public queue.
