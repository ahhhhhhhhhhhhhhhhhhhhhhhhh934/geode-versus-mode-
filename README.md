# Geode Versus Mode

A Geometry Dash competition hub for Geode, targeting Geometry Dash **2.2081**, Geode **5.11.2**, Android64.

## v0.6.2 Globed integration repair

The supplied device log showed Globed **2.2.2** loaded and connected, but its Soft Link API table was not exported. Globed's upstream changelog identifies this as fixed in **2.3.0**. This branch now uses the documented Soft Link API and requires Globed 2.3.0 or newer (the dependency is pinned to 2.3.0 for reproducible builds).

- Uses Globed's Soft Link API wrappers for availability, connection state, room state, and room ID.
- Removes direct Link API singleton calls that compiled but failed at link time.
- Requires Globed 2.3.0 because 2.2.2 has the missing API export shown in the user's log.
- Keeps Globed connection diagnostics separate from Versus matchmaking.

**Device requirement:** the installed Globed mod must be updated from 2.2.2 to 2.3.0 or newer before testing this integration.

## Honest feature status

- **Plaza / navigation / rank ladder / profile UI:** implemented in this branch.
- **Globed connection and room status:** uses the Soft Link API, requiring Globed 2.3.0 or newer.
- **Ranked matchmaking, public/group room directories, LIVE match feed, synchronized Versus gameplay, local split controls, and server-backed profiles:** not implemented yet.
- **Voice:** Globed handles voice inside shared levels. Versus does not directly activate the microphone or manage voice sessions.

A successful CI build is not the same as on-device testing. Install the package and verify Globed status on the target phone before merging.
