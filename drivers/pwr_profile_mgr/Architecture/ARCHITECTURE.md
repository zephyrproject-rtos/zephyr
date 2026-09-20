# Architecture — pwr_profile_mgr

Covers the core/backend split, the backend ops interface, file layout,
and Kconfig/Devicetree plan. For "what" see
[../Requirements/REQUIREMENTS.md](../Requirements/REQUIREMENTS.md); for
state machine/API/failure-model "how it behaves" see
[../Design/DESIGN.md](../Design/DESIGN.md).

All open points for this document are consolidated in one place: see
[Open points](#open-points) at the end of this file.

## 1. Layering: generic core + board-specific backend

To keep the module portable beyond STM32F407 (even though only
STM32F407 ships in v1), split into two layers, mirroring how Zephyr
itself separates generic subsystem code from SoC-specific backends:

- **Generic core** (`pwr_profile_mgr`) — owns the state enum, the
  public API (`get_state`/`suspend`/`resume`), the internal
  `set_state()` orchestration logic (sequencing, timeout, rollback-walk,
  retry, fault-latch decision), and calls out to the backend via
  function pointers / an ops-struct pattern.
- **Board-specific backend** (`pwr_profile_mgr_stm32f4`) — implements
  the actual "how": entering/exiting Stop mode on STM32F407
  specifically, and the per-driver suspend/resume/rollback routines for
  LED, UART shell, and the onboard accelerometer.

```
 ┌─────────────────────────────┐
 │   sample app / shell cmds    │
 └───────────────┬───────────────┘
                  │ pwr_profile_suspend()/resume()/get_state()
 ┌───────────────▼───────────────┐
 │  pwr_profile_mgr (core)        │
 │  - state + transition_failed   │
 │    + retry_count                │
 │  - set_state() orchestration    │
 │    (timeout, rollback, retry,   │
 │     fault-latch)                │
 └───────────────┬───────────────┘
                  │ pwr_profile_backend_ops (fn ptrs)
 ┌───────────────▼───────────────┐
 │  pwr_profile_mgr_stm32f4       │
 │  (backend)                     │
 │  - enter_sleep()/exit_sleep()  │
 │  - led   suspend/resume/rollback│
 │  - uart  suspend/resume/rollback│
 │  - accel suspend/resume/rollback│
 └─────────────────────────────────┘
```

### Diagram walkthrough

The diagram shows three boxes stacked vertically, connected by two
downward arrows. Each box is a distinct layer of the module; each arrow
is a call interface — what the layer above is allowed to call on the
layer below, and nothing more. Nothing in this diagram flows sideways
or upward: it's a strict one-directional call stack, top to bottom.

**Box 1 — sample app / shell cmds (top).**
This is not part of the `pwr_profile_mgr` module itself — it's whatever
calls into it: the shell sample application (the `state_manager
suspend`/`resume` CLI commands from REQ-3/REQ-5) and, indirectly, the
button-press path (REQ-2/REQ-4), once it's been deferred from ISR to
thread context per REQ-15. This box represents *every* external caller
of the module, not a single component.

**Arrow 1 — `pwr_profile_suspend()` / `resume()` / `get_state()`.**
This is the module's entire public API (three functions, per
Design/DESIGN.md §2). It is the *only* way anything outside the module
is allowed to interact with it. The caller never sees the state enum's
internal bookkeeping directly except through `get_state()`, and never
calls `set_state()` — that function is internal to the core box below.

**Box 2 — `pwr_profile_mgr` (core), the middle layer.**
This is the SoC-agnostic generic logic described in REQ-13. It owns:
- The state data itself: the current state enum value, plus the
  `transition_failed` flag and `retry_count` used for fault-latching
  (Design/DESIGN.md §3).
- `set_state()` — the single internal function that does all the actual
  work of a transition: sequencing through the transient states,
  applying the per-step timeout, walking the rollback set on failure,
  retrying a failed driver's rollback, and deciding whether to revert
  cleanly or latch into a permanent fault state.

Critically, this box contains **no STM32-specific code**. It doesn't
know how to enter Stop mode, and it doesn't know how to talk to the
accelerometer over SPI. It only knows *that* those things need to
happen, in what order, and what to do if one of them fails. All of the
actual hardware manipulation is delegated downward through Arrow 2.

**Arrow 2 — `pwr_profile_backend_ops` (function pointers).**
This is the seam that makes the core SoC-agnostic (REQ-13) while still
letting it drive real hardware. It's a struct of function pointers —
conceptually identical to how Zephyr's own driver subsystems (e.g.
`gpio_driver_api`) decouple generic API calls from a specific driver's
implementation. The core calls through this struct without knowing
which backend is on the other end; today there's only one backend
(STM32F407), but a second board would only need to provide a second
implementation of this same struct — no core code would change. The
exact fields of this struct are not yet drafted; see
[Open points](#open-points) item 1.

**Box 3 — `pwr_profile_mgr_stm32f4` (backend), the bottom layer.**
This is where all the STM32-specific work actually happens (REQ-14):
- `enter_sleep()` / `exit_sleep()` — the actual STM32 Stop-mode entry
  and exit sequence (register writes, HAL/LL calls).
- Three driver-specific callback groups — LED, UART shell, and
  accelerometer — each exposing `suspend()`, `resume()`, and
  `rollback()`. These map directly to the three peripheral integrations
  required by REQ-9, REQ-10, and REQ-11. The core's rollback-walk logic
  (Design/DESIGN.md §3) iterates over these three sets generically; it
  doesn't have LED/UART/accelerometer-specific logic itself, since that
  would break the SoC-agnostic boundary at Arrow 2.

**What the diagram deliberately does not show:** the RTC wakeup timer
(REQ-12) and the button GPIO/EXTI interrupt are wake *sources*, not
part of this call stack — they asynchronously cause the top box (via
`k_work`, per REQ-15) to call `pwr_profile_resume()`, re-entering the
diagram at Arrow 1 rather than being drawn as a fourth box. The
Kconfig/Devicetree layer is also not shown — it configures values used
inside Box 2 and Box 3 (retry count, timeout, pin/peripheral bindings)
rather than being part of the runtime call path.

## 2. File/directory layout

Code follows Zephyr driver conventions: flat, one `.c` per backend named
`<module>_<soc>.c` (as `hwinfo_stm32.c`), private headers beside the
sources, per-module `Kconfig` and `CMakeLists.txt`. Unlike in-tree
drivers, tests, sample and design docs stay inside the module folder so
the whole project reviews and pushes as one unit.

```
drivers/pwr_profile_mgr/
├── CMakeLists.txt
├── Kconfig                          # CONFIG_PWR_PROFILE_MGR, retries, timeout
├── pwr_profile_mgr.h                # public API + enums
├── pwr_profile_mgr.c                # generic core: public API, set_state()
├── pwr_profile_backend.h            # private: backend ops / driver table
├── pwr_profile_mgr_stm32f4.c        # board backend (planned)
├── README.md
├── Requirements/REQUIREMENTS.md
├── Design/DESIGN.md
├── Architecture/ARCHITECTURE.md
├── tests/
│   └── core/                        # ztest on native_sim, stub backend
└── samples/
    └── state_manager/               # shell sample app (planned, name TBD)
```

### Mapping to the in-tree Zephyr layout

If the module is ever prepared for upstream, the pieces move to the
locations Zephyr uses for other drivers:

| Item | Now | Upstream-style location |
|---|---|---|
| Public API header | `drivers/pwr_profile_mgr/pwr_profile_mgr.h` | `include/zephyr/drivers/pwr_profile_mgr.h` |
| Tests | `drivers/pwr_profile_mgr/tests/core/` | `tests/drivers/pwr_profile_mgr/core/` |
| Sample | `drivers/pwr_profile_mgr/samples/state_manager/` | `samples/drivers/pwr_profile_mgr/` |
| Docs | `drivers/pwr_profile_mgr/{Requirements,Design,Architecture}/` | `doc/hardware/peripherals/pwr_profile_mgr.rst` |
| DT bindings (if any) | not needed yet | `dts/bindings/<class>/` |

## 3. Kconfig / Devicetree plan

Not yet drafted in detail. Expected shape based on the requirements:

**Kconfig**
- `CONFIG_PWR_PROFILE_MGR` — enable the module.
- `CONFIG_PWR_PROFILE_MGR_MAX_RETRIES` (default 3, TBD) — per-driver
  rollback retry bound.
- `CONFIG_PWR_PROFILE_MGR_STEP_TIMEOUT_MS` (default 100, TBD) —
  per-step timeout for hardware operations in `set_state()`.
- Backend selection likely implicit (only `stm32f4` backend exists in
  v1) rather than a Kconfig choice, until a second backend exists.

**Devicetree**
- Accelerometer binding depends on Phase 0 physical confirmation
  (LIS302DL vs. LIS3DSH) — different compatible strings/driver classes
  in-tree. Must not write the overlay until that's confirmed.
- Button (PA0/EXTI0) and LED likely reference the board's existing
  `sw0`/`led0` devicetree aliases already defined in
  `boards/st/stm32f4_disco/stm32f4_disco.dts`, rather than new bindings.
- RTC wakeup timer — uses STM32's RTC peripheral binding already
  in-tree (`st,stm32-rtc` or similar); confirm exact compatible/API
  (`rtc_alarm_set_time()` or counter-based) during Phase 6.

Not yet finalized — see [Open points](#open-points) item 3.

## Implementation phases

See [../README.md](../README.md#implementation-phase-sequencing).
This architecture (ops-struct, file layout, Kconfig/DT) should be
finalized as part of the scaffolding pass, before Phase 3 (basic Stop-mode
suspend) is wrapped into the module in Phase 4.

## Open points

All architecture-level open points, in one place:

1. **`pwr_profile_backend_ops` struct** (§1) — exact function-pointer
   signatures not yet drafted. Must cover at minimum: `enter_sleep()` /
   `exit_sleep()`; per-driver `suspend()`/`resume()`/`rollback()` for
   LED, UART shell, accelerometer (likely as an array/table of driver
   descriptors so the core's rollback-walk can iterate generically);
   and a return convention that encodes the recoverable-vs-unrecoverable
   distinction from Design/DESIGN.md.
2. **Module placement** (§2) — layout inside the folder is decided
   (see §2). Still open: whether this ships as an in-tree `drivers/`
   module (current location) or is better suited as `subsys/pm/` or an
   out-of-tree module, given it's closer to a PM-policy subsystem than a
   hardware-facing driver (no `DEVICE_DT_INST_DEFINE`-style
   instantiation planned, since there's exactly one instance). Kept
   under `drivers/` for now; revisit if it causes friction with Zephyr's
   driver-model conventions.
3. **Kconfig / Devicetree structure** (§3):
   - `CONFIG_PWR_PROFILE_MGR*` exact names and defaults.
   - Devicetree overlay for the accelerometer — blocked on Phase 0
     physical board inspection (LIS302DL vs. LIS3DSH).
   - RTC binding — compatible string/API (alarm-based vs.
     counter-based) to confirm during Phase 6.
