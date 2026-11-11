# Globed-native matchmaking investigation

Date: 2026-11-11

## Decision

The public Globed central server's existing Server Events API is **not sufficient for a global matchmaking queue across separate Globed rooms**.

Versus Mode should not replace Supabase with a client-only event queue and claim that it is global. That would be incorrect and unreliable.

A genuinely Globed-native global queue requires a server-side matchmaking feature in the Globed central server (or an upstream Globed feature that provides the same capability). This would be an extension of Globed's own central server, not an unrelated matchmaking provider.

## What the inspected Globed 2.3.0-beta-target source provides

Inspected upstream Globed source at commit:

`737d2211b6ce00119f752dc07d0d76d2700b6d08`

Relevant source:

- [Soft Link API guide](https://github.com/GlobedGD/globed2/blob/737d2211b6ce00119f752dc07d0d76d2700b6d08/docs/soft-link-api.md)
- [Server Events guide](https://github.com/GlobedGD/globed2/blob/737d2211b6ce00119f752dc07d0d76d2700b6d08/docs/server-events.md)
- [EventOptions and EventServer](https://github.com/GlobedGD/globed2/blob/737d2211b6ce00119f752dc07d0d76d2700b6d08/include/globed/core/data/Event.hpp)
- [ServerEvent implementation](https://github.com/GlobedGD/globed2/blob/737d2211b6ce00119f752dc07d0d76d2700b6d08/include/globed/core/Event.hpp)
- [Soft Link API table](https://github.com/GlobedGD/globed2/blob/737d2211b6ce00119f752dc07d0d76d2700b6d08/include/globed/soft-link/Table.hpp)
- [Central server event handler](https://github.com/GlobedGD/central-server/blob/main/src/core/handler/misc.rs)
- [Central server event worker](https://github.com/GlobedGD/central-server/blob/main/src/core/event_worker.rs)

Confirmed API details:

- `globed::EventServer::Central` selects the central server.
- `globed::api::net::registerEvent(id, server)` registers a custom event.
- `globed::api::net::sendEvent(id, data, options)` sends it.
- `EventOptions` supports `targetPlayers`, `reliable`, `urgent`, and `sendBack`.
- The soft-link API supports custom event registration and sending; a mod can listen to `globed::msg::EventsMessage` through the exposed message listener wrappers.
- The central server forwards events to their recipients; it does not persist them as a queue.

## Critical routing limitation

The inspected public central-server implementation does the following:

1. It resolves the sender's current room, using the global room if the sender is not in a private room.
2. If an event has no explicit target list, it rejects the event when the sender is in the global room (`UnscopedGlobalEvent`).
3. If an event has explicit target player IDs, it looks up those online clients and filters them to clients in the sender's same room.
4. It forwards the event to those eligible recipients.

Consequences:

- A client cannot broadcast a matchmaking request to every online Globed user from the global room.
- A targeted event cannot reach a player in a different private room.
- The event API has no persistent queue, atomic pairing, ready-check coordinator, or authoritative match record.
- A client-only queue built on these events would not provide reliable global matchmaking.

These constraints are implemented in `ConnectionHandler::handle_event` in the central server's `src/core/handler/misc.rs`. The event worker in `src/core/event_worker.rs` only batches and forwards events to already-selected recipients; it is not a matchmaking service.

## Session handoff findings

The selected Globed source exposes the 2.3.0 session warp API in the Soft Link table:

- `GameSubtable::warpToSession(const WarpContext&)` is marked as since v2.3.0.
- `RoomSubtable::makeSessionId(int levelId)` builds a session ID from the current room and selected game server.
- `SessionId::fromParts(serverId, roomId, levelId)` encodes the server, room, and level.
- The central server validates that a joining session belongs to the client's current room and that the referenced game server exists.

Important: constructing a session ID does not create a room, select a match host, or move users into a different room. The existing Versus client must not claim that calling `warpToSession` alone creates a playable match.

## Required Globed-native design

To support a true global queue while retaining Globed for all networking:

1. Add a matchmaking request/response protocol to the Globed central server.
2. The central server owns queue state, pairs users atomically by mode, expires stale entries, and performs ready checks.
3. The server returns a validated match descriptor containing the chosen Globed room, host, game server/session details, and expiry.
4. Versus clients use Globed's supported room/session flow to enter that match.
5. Globed game servers continue to provide gameplay synchronization; Versus events synchronize only the additional match rules and results.
6. Server-side identity comes from the authenticated Globed connection, never a client-supplied account ID.
7. Apply rate limits, queue cancellation, disconnect cleanup, and match expiry on the central server.

This needs either an upstream Globed feature or a maintained Globed central-server fork. If a fork is used, it must be deployed as the central server users connect to; this is still Globed infrastructure, but it is no longer the public main Globed central server.

## Current Versus Mode repository status

The current feature branch still contains Supabase queue calls and Supabase settings in `src/main.cpp` and `mod.json`. The existing Supabase implementation should not be described as Globed-native matchmaking. It is also not proof that a real Globed match can be created: room creation/joining and validated session handoff remain separate integration work.

Do not delete the current backend implementation until a replacement server-side Globed matchmaking endpoint/protocol exists and the client migration can be tested end-to-end. Removing it prematurely would leave the online queue nonfunctional.

## Acceptance tests for a future implementation

- Two authenticated clients in different Globed rooms can queue for the same mode and be paired.
- Clients queuing for different modes are never paired.
- Concurrent queue joins cannot pair one player twice.
- Disconnecting or cancelling removes the player and safely updates the opponent.
- A ready-check expires if either player does not accept.
- A returned match descriptor is validated against an actual Globed room and game server.
- Both clients join the same real Globed session and see one another in-game.
- No Supabase, Cloudflare, Render, or unrelated gameplay server is required.
