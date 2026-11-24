# ADR 0001 — One C core and a generated table for both engines

*Status: accepted — 2026-11-18 — Viktor*

## Context

The kit must give Qt Quick and LVGL the same data model: bounds, units, zones, thresholds,
damping, and the same behaviour (hysteresis, invalid data, blink rules). LVGL targets
microcontrollers (C, no exceptions, little RAM, no file system); Qt targets Linux/Windows with
a GPU. Business logic must stay testable without any GUI.

## Options

1. **Two implementations** (C for LVGL, C++/QML for Qt) following a written spec.
   Cheap to start; behaviours drift, every fix is done twice.
2. **Parse the JSON at run time** in both engines. Flexible, but needs a JSON parser and RAM on
   the MCU, and errors appear on the device instead of at build time.
3. **One portable C99 core + a configuration table generated from JSON at build time**, used
   by the LVGL widgets directly and by thin C++ `QObject` wrappers in the QML module.

## Decision

Option 3. `tools/gen_instruments.py` validates `spec/instruments.json` and emits
`idev_instruments_cfg.c` (`const` data). `idev_core` holds all logic. QML files only draw,
binding to properties of the C++ types (`QML_ELEMENT`, no context properties).

## Consequences

- Configuration lives in flash (0 B of RAM) and invalid specs fail the build.
- One unit-test suite (`core/tests`) covers the behaviour of both engines.
- Changing a configuration needs a rebuild; run-time reconfiguration is out of scope for v0.1.
- Python 3 is required at build time (already present in ESP-IDF and most CI images).
- The Qt module links a C library: no Qt types leak into the core, the core never includes Qt.
