# Lobby codes

A Versus lobby is a Globed room. Its lobby code is the room passcode that
Globed itself reports through `globed::api::room::getPasscode()`.

## What works

- The lobby header and the Private Versus page show the passcode when Globed
  reports one, and fall back to the room id when it does not.
- The host can open **Share Code** in the lobby to see the code in large text.
- Friends join through Globed's own room browser and enter the code when
  Globed prompts for it. After they are in the room, they open the Versus
  lobby and join it with one button.

## What does not work, and why

Versus cannot join a room by code on a player's behalf. The Globed Soft Link
API exposes `room::isInRoom`, `getId`, `getOwner`, `getPasscode`,
`getPinnedLevel` and `makeSessionId`, but it has no call to join or create a
room. Room joining stays inside Globed's own UI (`RoomListingPopup`).

Upstream request: a Soft Link call such as `room::join(roomId, passcode)`
and `room::create(settings)` would let Versus offer a real "enter code" box.
Until it exists, Versus does not show a code-entry field, because a field
that cannot join anything would be a placeholder.

## Code format

The code is whatever Globed assigns. Versus does not generate its own
codes and does not keep a separate room directory.
