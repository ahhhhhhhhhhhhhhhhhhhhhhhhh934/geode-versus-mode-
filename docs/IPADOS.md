# iPadOS support

## How the Apple build works

Versus Mode is a Geode mod, not a standalone app. Geode's Apple mobile build target is **iOS arm64**. iPadOS uses the same Apple mobile platform binary, so the existing versus.mode.ios.geode package is the package to test on a compatible iPad.

There is no separate Geode iPadOS target to compile in this project. Renaming the iOS package to versus.mode.ipados.geode would only create a duplicate filename, not a distinct build. Keep the published iOS asset name and document that it is intended for compatible iPhone and iPad installs.

The manifest's gd.ios compatibility entry is intentional; do not invent a gd.ipados key unless Geode's manifest schema adds support for it.

## What is prepared

- The project already builds an Apple mobile .geode package using the iOS target.
- The lobby screens position most content relative to Geometry Dash's runtime window size.
- The iOS package remains the candidate package for the iPad test.

## Required iPad test

iPadOS support is **not verified** until a real iPad test is completed. On a supported iPad with compatible Geometry Dash, the Geode iOS launcher, and Globed installed:

1. Install the existing versus.mode.ios.geode package through the supported Geode workflow.
2. Confirm the Versus entry appears in the Creator screen and opens.
3. Check portrait and landscape orientations where the game supports them.
4. Check that buttons and full-card mode selection respond accurately to touch.
5. Check text, cards, and menus at the iPad's available display sizes and window modes.
6. Join a Globed room and verify the lobby, ready state, mode selection, and return-to-lobby flow.
7. Capture the Geode log and record the iPad model, iPadOS version, Geometry Dash version, and Geode/Globed versions used.

Do not call the iPad build tested or release it as iPad-verified until those checks pass.

## Release naming

Keep the existing versus.mode.ios.geode asset. It is the shared iOS/iPadOS package; do not publish a renamed duplicate as if it were a separate compiled platform target.
