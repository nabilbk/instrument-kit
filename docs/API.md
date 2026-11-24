# IDEV Instrument Kit — data model and API (v0.1)

*Architecture: Viktor — 2026-11-18. Decision record: [adr/0001-shared-c-core.md](adr/0001-shared-c-core.md).*

The kit has three layers. Only the bottom one knows what an instrument *means*;
the two front ends only draw it.

```
spec/instruments.json ──(tools/gen_instruments.py, at build time)──► idev_instruments_cfg.c
                                                                          │
                     core/  idev_core (C99, no GUI): config table, damping, alerts,
                            zones, formatting, blink logic, demo scenarios
                         ┌────────────────────┴─────────────────────┐
     lvgl/  idev_lvgl (C, LVGL 9)                   qml/  IdevInstruments (Qt 6 QML module)
     lv_obj + custom draw callbacks                 C++ types (QML_ELEMENT) wrap idev_core,
                                                    QML files are declarative drawing only
```

## 1. The data model (described once)

`spec/instruments.json` holds a `palette` and a list of `instruments`. Each instrument:

| Field | Type | Meaning |
|---|---|---|
| `id` | string | Unique key, e.g. `car.rpm`. Used by both engines (`configId`). |
| `kind` | `RoundGauge` \| `LinearTape` \| `AttitudeAxis` \| `Annunciator` \| `Value` | Which widget the entry is meant for. `Value` = not drawn (drives an annunciator). |
| `label`, `unit` | string (UTF-8) | Shown on the instrument. |
| `min`, `max` | number | Scale bounds. The display is clamped; alerts use the raw value. |
| `decimals` | int (default 0) | Digits of the numeric readout. |
| `dampingMs` | number (default 120) | Time constant τ of the first-order filter. 0 = no damping. |
| `zones[]` | `{from, to, severity}` | Coloured arcs/bars; `severity` ∈ `normal` (green), `caution` (amber), `warning` (red). Sorted, non-overlapping, inside `[min, max]`. |
| `alerts` | `{cautionLow, warningLow, cautionHigh, warningHigh, hysteresis}` | Any threshold may be omitted. |
| `gauge` | `{startAngle, sweepAngle, majorStep, minorDivisions, labelScale, scaleCaption}` | RoundGauge only. |
| `tape` | `{visibleRange, majorStep, minorDivisions, labelStep}` | LinearTape only. |
| `attitude` | `{pitchVisibleRange, ladderStep}` | Pitch axis of an AttitudeIndicator. |
| `annunciator` | `{blinkHz}` | Annunciator only (default 2 Hz). |

The generator validates the file (unique ids, zone order and bounds, threshold order,
gauge/tape parameters) and fails the build with the offending id.

### Conventions

- **Angles**: degrees, clockwise, 0 = 12 o'clock (so `startAngle: -135, sweepAngle: 270`
  is the classic dial with the gap at the bottom). Both engines convert internally.
- **Damping**: `display += (target − display) × (1 − e^(−dt/τ))`, stepped at 60 Hz while
  the display is not at the target; the timer stops when it settles. The first valid sample
  (after start or after loss) is shown immediately.
- **Alert level** (`normal` / `caution` / `warning`) is evaluated on every new raw value.
  It escalates immediately; it de-escalates only once the value has moved `hysteresis`
  past the threshold toward the safe side.
- **Invalid data**: send `NaN`. The instrument shows `---`, hides the needle / tape scale /
  horizon, and reports `normal` (no false alarm).
- **Annunciator**: `normal` = dark, or steady green if `active`; `caution` = steady amber;
  `warning` = red, flashing at `blinkHz` (lit in the first half of each period, all lights in
  phase) until acknowledged, then steady. A higher level re-arms the flashing.
- **High/low alerts** (`alertHigh` / `alertLow`): the alert level, split by whether the raw
  value is above or below the middle of `[min, max]`. Used for STALL vs OVERSPEED.

### Adding an instrument

1. Add an entry to `spec/instruments.json`.
2. Rebuild (the table is regenerated automatically).
3. Qt: `Instrument { configId: "my.id"; value: … }` + a component. LVGL:
   `idev_round_gauge_create(parent, "my.id")` (or tape / attitude / annunciator).

## 2. Core C API (`core/include/idev/instrument.h`)

| Function | Purpose |
|---|---|
| `idev_cfg_find(id)` | Look up a configuration (NULL if unknown). |
| `idev_instrument_init / _set_value / _step / _settling` | Runtime state: raw value, damped display, validity, alert. No allocation. |
| `idev_alert_eval(alerts, value, previous)` | Alert level with hysteresis. |
| `idev_alert_high / idev_alert_low` | Alert split by scale side. |
| `idev_zone_at`, `idev_ratio` | Zone severity at a value; 0…1 position on the scale. |
| `idev_format`, `idev_format_scale_label` | Readout text (`---` when invalid) and scale labels. |
| `idev_annunciator_lit(level, active, acked, t_ms, blink_hz)` | Whether a light is lit at time *t*. |
| `idev_palette`, `idev_level_color(level)` | Colours from the JSON palette (0xRRGGBB). |
| `idev_sim_car(t, &frame)`, `idev_sim_pfd(t, &frame)` (`idev/sim.h`) | Deterministic demo scenarios (40 s loops), shared by both demos. |

## 3. LVGL API (`lvgl/include/idev_lvgl.h`)

```c
lv_obj_t *g = idev_round_gauge_create(parent, "car.rpm");
lv_obj_set_pos(g, 446, 40);
lv_obj_set_size(g, 330, 330);
idev_instrument_widget_set_value(g, rpm);          /* call whenever a sample arrives */
idev_annunciator_set_level(light, idev_widget_instrument(g, 0)->alert, false);
```

| Function | Notes |
|---|---|
| `idev_round_gauge_create(parent, id)` | Default 240 × 240. Layout switches to "small" below 240 px. |
| `idev_linear_tape_create(parent, id, mirrored)` | `mirrored = false`: ticks on the right edge (speed tape); `true`: left edge (altitude). |
| `idev_attitude_create(parent, roll_id, pitch_id)` | Two `AttitudeAxis` entries. Set with `idev_attitude_set(obj, roll, pitch)`. |
| `idev_annunciator_create(parent, id)` | `idev_annunciator_set_level(obj, level, active)`; a click acknowledges (`idev_annunciator_acknowledge`). |
| `idev_widget_instrument(obj, axis)` | Read back state (axis 0 = value/roll, 1 = pitch). |

Each widget is an `lv_obj` with styles removed, a `LV_EVENT_DRAW_MAIN` callback that draws
with `lv_draw_arc / line / triangle / rect / label`, a small state block (`lv_malloc`) and a
16 ms `lv_timer` that is paused whenever nothing moves. It is freed on `LV_EVENT_DELETE`.
All calls must come from the LVGL thread.

## 4. QML API (module `IdevInstruments`)

C++ types (registered with `QML_ELEMENT`, logic in C++):

| Type | Key properties |
|---|---|
| `Instrument` | in: `configId`, `value` (NaN = invalid). out: `displayValue`, `displayText`, `ratio`, `valid`, `alertLevel`, `alertHigh`, `alertLow`, `alertColor`; config: `label`, `unit`, `minimum`, `maximum`, `zones` (list of `{from, to, level, color}`), gauge/tape/attitude/annunciator parameters. Enum `Instrument.Normal / Caution / Warning`. Invokables `ratioOf(v)`, `scaleLabel(v)`. |
| `InstrumentPalette` (singleton) | Palette colours; `levelColor(level)`. |
| `AnnunciatorState` | in: `level`, `active`, `blinkHz`; out: `lit`, `acknowledged`; `acknowledge()`. |

QML components (declarative drawing only — Qt Quick Shapes for arcs and triangles,
rectangles and `Repeater` for ticks, no `Canvas`, no imperative JavaScript):

```qml
import IdevInstruments

Instrument { id: rpm; configId: "car.rpm"; value: ecu.rpm }
RoundGauge { width: 330; height: 330; instrument: rpm }
Annunciator { configId: "car.ann.temp"; level: coolant.alertLevel }
LinearTape { instrument: airspeed }                     // mirrored: true for altitude
AttitudeIndicator { roll: rollAxis; pitch: pitchAxis }
```

## 5. Same concepts, two spellings

| Concept | QML | LVGL |
|---|---|---|
| Configure | `Instrument { configId: "car.rpm" }` | `idev_round_gauge_create(p, "car.rpm")` |
| Push a value | `value: source.rpm` (binding) | `idev_instrument_widget_set_value(obj, rpm)` |
| Invalid data | `value: NaN` | `idev_instrument_widget_set_value(obj, NAN)` |
| Alert level | `rpm.alertLevel` | `idev_widget_instrument(obj, 0)->alert` |
| Light | `Annunciator { level: …; active: … }` | `idev_annunciator_set_level(obj, level, active)` |
| Acknowledge | tap the item / `AnnunciatorState.acknowledge()` | click / `idev_annunciator_acknowledge(obj)` |
| Colours | `InstrumentPalette.caution` | `idev_palette.caution` |
