# Geode Versus Mode — Android build project

Target: Geometry Dash 2.2081 / Geode SDK 5.11.2 / Android 64-bit.

## Current implementation status
- Creator menu Versus entry: hooked into CreatorLayer (the Create/Saved/Scores/Quests screen).
- Versus lobby screen: implemented in source.
- Local 1v1: UI placeholder; independent player controls are not implemented.
- Online 1v1: Globed status and room detection only; match protocol and synchronization are not implemented.
- Voice chat: not implemented. Globed's audio API requires its full link API; the current Soft Link wrapper does not expose microphone/audio methods.
- Workshop: not implemented; investigate Geode's mod portal/workshop integration separately.

## Build an actual .geode package
This repository includes a GitHub Actions workflow at .github/workflows/build-android.yml. Open **Actions → Build Android Geode mod → Run workflow** to start an Android64 build. A successful run uploads the .geode package as the Versus-Mode-Android64 artifact.

For a local build, install the Android NDK and Geode CLI/SDK 5.8.2, then run:
```sh
geode sdk install-binaries -p android64
geode build -p android64
```

A compiled .geode file is not included until a build succeeds.
