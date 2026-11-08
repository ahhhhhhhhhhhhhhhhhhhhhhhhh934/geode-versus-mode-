# Geode Versus Mode — Android build project

Target: Geometry Dash 2.2081 / Geode SDK 5.8.2 / Android 64-bit.

## Current implementation status
- Main-menu Versus button: implemented in source.
- Versus lobby screen: implemented in source.
- Local 1v1 and Online 1v1: UI placeholders only. There is no second player, independent input, matchmaking, server, or synchronization yet.
- Voice chat and the approved feature wishlist: planned, not implemented.

## Build an actual .geode package
This repository includes a GitHub Actions workflow at .github/workflows/build-android.yml. Open **Actions → Build Android Geode mod → Run workflow** to start an Android64 build. A successful run uploads the .geode package as the Versus-Mode-Android64 artifact.

For a local build, install the Android NDK and Geode CLI/SDK 5.8.2, then run:
```sh
geode sdk install-binaries -p android64
geode build -p android64
```

A compiled .geode file is not included until a build succeeds.
