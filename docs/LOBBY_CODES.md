# Room joining

Versus does not implement lobby-code entry, code sharing, or a separate room directory.

Players create or join rooms entirely through Globed's own UI. Versus detects the active Globed room through the Soft Link API (`room::isInRoom` and `room::getId`) and automatically uses that room for the Versus lobby. Friends do not need a second Versus-specific code or another joining flow.
