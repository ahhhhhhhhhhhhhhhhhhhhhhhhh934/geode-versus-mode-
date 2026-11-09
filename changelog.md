# Changelog

## v0.6.6 — Release pipeline correction
- Bumped the Geode mod, project, and in-game version to v0.6.6.
- Fixed the release pipeline so the published tag points at the exact commit that was built, instead of a stale pre-existing tag.
- The publish workflow now reads the version from mod.json, verifies the built .geode package (version and Globed 2.2.2 dependency) before publishing, and fails the run if the artifact is missing.
- Publishes the v0.6.6 release with the Android64 .geode package.

## v0.6.5 — Build and release correction
- Bumped the Geode mod and project version to v0.6.5.
- Corrected the Android64 build diagnostic that prevented compilation.
- Publishes the v0.6.5 release with the Android64 .geode package.


## v0.6.4 — Stable Globed 2.2.2 requirement
- Bumped the project and mod version to v0.6.4.
- Requires stable Globed 2.2.2.
- Pins the public Globed headers and external dependency declaration to 2.2.2.
- Android64 CI build passed for the v0.6.3 code baseline; v0.6.4 release build is being run.

## v0.6.3 — Globed dependency resolution
- Declared Globed as an external Geode mod dependency.
- Pinned public Globed headers to the v2.2.2 tag.
- Requires Globed 2.2.2 at runtime.

## v0.6.2 — Globed Soft Link API experiment
- Replaced direct Link API singleton calls with Soft Link API wrappers.
- This version targeted a later Globed API and was not compatible with the user's stable 2.2.2 installation.

## v0.6.0 — Plaza, ranks and integration overhaul
- Rebuilt dashboard and Plaza navigation.
- Added Ranked and Profile pages.
- Added a rank ladder using Geometry Dash difficulty names from Auto through Extreme Demon.
- Added Public Rooms, Group Rooms, and LIVE destinations with honest service-status messages.
- Improved Globed status diagnostics.
- Fixed the release workflow to attach the generated .geode package.

## v0.5.0 — Plaza and navigation overhaul
- Added a distinct Plaza page and room/voice status screens.
- Added Globed connection diagnostics.

## v0.4.13
- Globed Soft Link API integration and loading diagnostics.

## v0.4.12
- Replace custom Globed event dispatch with Globed's documented Soft Link API wrappers.
- Declare Globed as an optional dependency and retry API lookup after it loads.

## v0.3.0
- Initial Versus hub prototype.
