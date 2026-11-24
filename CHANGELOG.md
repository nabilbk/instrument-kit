# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog 1.1.0](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- NOTICE file listing third-party licences (LVGL MIT, Qt LGPLv3).
- This changelog.

## [0.1.0] - 2026-11-24

### Added
- Four instruments with the same API in both engines (Qt Quick 6 and LVGL 9.2.2):
  RoundGauge, LinearTape, AttitudeIndicator, Annunciator.
- Instrument data described once in `spec/instruments.json`, with a shared
  C99 core (ADR 0001).
- Two demos in each engine: a car instrument cluster and a flight display (PFD).
- 14 reviewed screenshots at 800 x 480 (four of them in the README).
- Tests: 9/9 for LVGL, 7/7 for Qt; clean under ASan and UBSan; no warning
  with `-Werror` and `qmllint`.
- English and French READMEs, SPEC, API and VERIFICATION documents.
- MIT licence.

### Known limitations
- Measurements were taken on a PC only (408 to 472 bytes per LVGL widget,
  0.5 to 1.4 ms per frame at 800 x 480); nothing has been run on target
  hardware yet, so 60 fps is not verified.
- The SDL simulator is not built, Qt GPU rendering is not tried, and
  clang-tidy, clazy and coverage have not been run.

[Unreleased]: https://github.com/nabilbk/instrument-kit/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/nabilbk/instrument-kit/releases/tag/v0.1.0
