# pwr_profile_mgr

Custom Zephyr module implementing a two-state power-profile manager
(`Active` / `Sleep`) for the **STM32F407 Discovery** board, with a
generic SoC-agnostic core and a board-specific backend.

Portfolio project — see `Requirements/`, `Design/`, and `Architecture/`
for the full design record before any implementation code is written.
Source (`pwr_profile_mgr.c`, backend, Kconfig, CMakeLists.txt, sample
app) is added once Phase 0 (board/toolchain confirmation) and the open
points below are resolved.

## Docs in this folder

- [`Requirements/REQUIREMENTS.md`](Requirements/REQUIREMENTS.md) — what the module must do, naming rules, explicitly deferred scope.
- [`Design/DESIGN.md`](Design/DESIGN.md) — state machine, public/internal API, failure-handling (fail-stuck) design.
- [`Architecture/ARCHITECTURE.md`](Architecture/ARCHITECTURE.md) — core/backend split, ops-struct, file layout, implementation phases.

## Implementation phase sequencing

0. Setup/confirmation — confirm board revision and accelerometer part
   (physical inspection), confirm toolchain builds/flashes a known-good
   baseline sample.
1. State machine defined conceptually (done — see `Design/DESIGN.md`).
2. Baseline functional mode — GPIO/LED and UART shell working standalone,
   no PM involved. Reference point for later comparison.
3. Basic suspend capability (system-level) — reliably enter/exit Stop
   mode on a simple trigger (button) before wrapping in the module.
4. Build `pwr_profile_mgr` around it.
5. Integrate peripherals one at a time — LED, then UART, then
   accelerometer last (most complex). Re-validate after each addition.
6. Add RTC wakeup timer as a second, independent wake source.
7. Validation — repeated suspend/resume cycles, confirm state survives
   every time; produce evidence (logs, demo video, before/after).
8. Documentation and packaging.

## Status

Design phase complete (per the source design discussion). Implementation
not started. Hardware (STM32F407 Discovery) is physically available.

## Open points to resolve before/during coding

Tracked in full in `Design/DESIGN.md` §Open Points and
`Architecture/ARCHITECTURE.md` §Open Points. Summary:

| # | Open point | Resolve when |
|---|---|---|
| 1 | Accelerometer part (LIS302DL vs LIS3DSH) — physical board check | Phase 0 |
| 2 | `pwr_profile_backend_ops` struct signatures | Scaffolding |
| 3 | `pwr_profile_set_state()` exact signature/bookkeeping | Scaffolding |
| 4 | Recoverable vs. unrecoverable error-code contract | Scaffolding |
| 5 | `max_retries` value (example: 3) | Scaffolding, Kconfig |
| 6 | Per-step timeout value (example: 100ms) | Scaffolding, Kconfig |
| 7 | Thread-safety: ISR → `k_work` flow, state locking | Scaffolding (v1 correctness, not deferred) |
| 8 | Kconfig / Devicetree overlay structure | Scaffolding |
| 9 | File/directory layout | Scaffolding (see Architecture doc) |
| 10 | CLI binary/app name (`state_manager` working name) | Any time before sample app |
| 11 | `status` shell subcommand spec | Alongside suspend/resume |

## Next step

Phase 0: confirm board revision / onboard accelerometer part number by
physical inspection, and confirm the Zephyr SDK/WSL toolchain builds
and flashes a known-good baseline sample on this board. Then resolve
open points 2, 3, 4, 7, 8, 9 as part of initial scaffolding.
