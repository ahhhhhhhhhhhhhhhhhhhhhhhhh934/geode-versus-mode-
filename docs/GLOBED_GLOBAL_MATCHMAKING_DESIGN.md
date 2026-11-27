# Globed Global Matchmaking Design

Status: integration design; not yet operational
Target: Versus Mode PR #40
Transport: Globed central server and Globed sessions
No separate gameplay backend (no Supabase, Render, or Cloudflare worker)

## Goal

Allow players to enter a Versus queue from any Globed room and be paired globally. The queue coordinator must be authoritative and shared by all connected clients. A client-only queue cannot provide this: Globed server events are forwarded within their supported routing scope and do not create a durable, cross-room matchmaking service.

## Required Globed-side capability

This feature requires a Globed central-server extension or an upstream Globed feature. The existing public client Server Events API is not, by itself, a global queue. The coordinator must run inside the Globed central server's authenticated connection/event handling path so it can identify the sender from the authenticated connection rather than trusting an account ID in a packet.

The Versus mod repository can contain the client protocol, state machine, tests, and integration adapter. It cannot deploy or change the public Globed central server merely by shipping a Geode mod. Until the central-server capability is available, the UI must report global matchmaking as unavailable (or offer the room-based fallback); it must never display a fake global queue.

## Client-to-central-server protocol

Use a dedicated, versioned namespace, separate from room-scoped match gameplay events:

- `versus.queue.join`: queue mode, game mode, skill estimate, skill uncertainty, party size, client protocol version.
- `versus.queue.leave`: idempotent cancellation.
- `versus.queue.status`: authoritative ticket state and estimated wait band.
- `versus.queue.ready`: accept/reject a proposed match.
- `versus.queue.match`: server-issued match descriptor and expiry.
- `versus.queue.result`: match completion acknowledgement / failure reason.

The central server derives the player's account identity from its authenticated session. It must ignore any client-supplied account ID as an identity assertion. Argon-backed Globed authentication must be required for ranked queueing; unauthenticated users may be limited to casual queueing only if the server operator explicitly enables that policy.

## Authoritative ticket lifecycle

1. Validate authenticated account, protocol version, mode, queue type, and rate limits.
2. Enforce one live ticket per account; a new join replaces or rejects the prior ticket deterministically.
3. Store ticket with server-side enqueue time, rating, rating uncertainty, region/latency cohort, and recent-opponent history.
4. Match only compatible modes and queue types. Ranked starts with a narrow rating window and widens gradually as wait time grows; casual prioritizes wait time.
5. Atomically reserve both tickets in one transaction/critical section so concurrent workers cannot double-pair a player.
6. Send a ready check with a short deadline. Remove non-responders and requeue eligible responders with original wait credit.
7. Create or allocate a real Globed session and issue the same server-generated session descriptor to both clients.
8. Expire descriptors, clean up disconnects, and release reservations on every failure path.
9. Record the authoritative result only after the match protocol reports a valid completion.

## Match quality

- Never mix ranked and casual tickets.
- Never mix incompatible game modes or incompatible client protocol versions.
- Prefer closest rating and lowest uncertainty; widen the rating window gradually with queue age.
- Use latency/region as a soft preference, not a hard exclusion.
- Avoid immediate repeat opponents when another equally suitable pairing exists.
- Preserve queue age as a fairness factor so new arrivals do not indefinitely leapfrog waiting players.
- Support cancellation, disconnect cleanup, ready-check expiry, and idempotent retries.
- Enforce per-account queue limits and server-side rate limits.

## Session handoff

Globed's session-warp capability can be used only after the central coordinator has created/selected a valid session and assigned a host/level. A warp call alone does not create a matchmaking service, choose a host, or atomically pair players. Both clients must receive the same validated session descriptor from the coordinator and verify its expiry and protocol version before warping.

## Security and abuse controls

- Use the authenticated Globed connection identity, never packet-provided identity.
- Require Argon verification for ranked play if enabled by the central server.
- Validate all enums, sizes, timestamps, and descriptor fields server-side.
- Apply join/cancel/ready rate limits and queue-ticket expiry.
- Make join, leave, and ready operations idempotent.
- Do not trust client-reported rating, result, deaths, or completion as authoritative ranked evidence without server-side validation.
- Keep private user data out of public queue broadcasts.

## Acceptance tests

1. Two clients in different Globed rooms can be paired into one match.
2. A casual ticket never pairs with ranked; incompatible modes never pair.
3. Concurrent queue joins cannot reserve the same account twice.
4. Duplicate join/leave/ready messages are idempotent.
5. Cancellation and disconnect release tickets and reservations.
6. A ready-check timeout requeues or removes players according to policy.
7. Both players receive the same valid session descriptor and reach the same session.
8. Expired or malformed descriptors are rejected.
9. A client cannot impersonate another account by changing packet fields.
10. Ranked queueing is rejected when required Argon verification is absent.

## Delivery boundary

This repository can implement and test the client integration and keep the room-based queue as a fallback. The global queue is not complete until the required Globed central-server extension is merged and deployed by the Globed server operator. Do not claim global matchmaking works before the cross-room acceptance tests pass against that server.
