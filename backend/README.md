# Versus Mode matchmaking

The matchmaking service is separate from the multiplayer game server.

- **Globed 2.3.0 beta or newer** owns game sessions, player synchronization, and gameplay networking.
- Versus matchmaking only queues players by mode, pairs them, and coordinates readiness.
- A successful ready-check is not a playable match until the backend returns a validated Globed session descriptor and both clients hand off to that same session.

Do not deploy a second gameplay server or duplicate Globed's session transport. The current Worker implementation is only a queue/ready-check prototype and is not a complete online-match backend.
