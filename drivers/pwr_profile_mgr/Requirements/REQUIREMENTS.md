# Requirements — pwr_profile_mgr

## Purpose

A Zephyr RTOS project targeting the **STM32F407 Discovery** board.
Demonstrates RTOS power-management concepts, board-level driver
integration, and disciplined failure-handling design.

Toolchain, Zephyr SDK/codebase, and the physical STM32F407 Discovery board
are already available and working.

## Functional requirements

**REQ-1 — Dual operating profile.**
The module shall expose exactly two externally visible operating
profiles: `Active` (full functionality) and `Sleep` (low power,
execution halted).

**REQ-2 — Button-triggered suspend.**
The Active → Sleep transition shall be triggerable by the onboard user
button (PA0/EXTI0).

**REQ-3 — CLI-triggered suspend.**
The Active → Sleep transition shall be triggerable by a shell CLI
command.

**REQ-4 — Button-triggered resume.**
The Sleep → Active transition shall be triggerable by the same onboard
button, toggling against REQ-2 (a second press wakes the system).

**REQ-5 — CLI-triggered resume.**
The Sleep → Active transition shall be triggerable by a shell CLI
command.

**REQ-6 — Unified trigger path.**
All trigger sources (REQ-2 through REQ-5) shall route through the same
underlying trigger functions. The state machine shall not depend on, or
have visibility into, which source initiated a given transition.

**REQ-7 — SRAM retention in Sleep.**
RAM contents shall be retained across the Active → Sleep → Active
cycle. Execution halts while in `Sleep`, but RAM state shall not be
lost.

**REQ-8 — Onboard-only peripherals.**
The module shall not require any external ICs or sensors. Only
peripherals already present on the STM32F407 Discovery board may be
used.

**REQ-9 — LED suspend/resume behavior.**
The onboard LED shall stop toggling while in `Sleep` and shall resume
at the correct blink phase upon wake.

**REQ-10 — UART shell suspend/resume behavior.**
The UART shell shall be silent while in `Sleep`. Upon resume, it shall
reflect application state retained from before suspend (e.g. a counter
value continuing from where it left off).

**REQ-11 — Accelerometer device-level power management.**
The onboard SPI-connected MEMS accelerometer shall be suspended and
resumed via device-level power-management operations over the live SPI
bus as part of the Active/Sleep transitions.

**REQ-12 — RTC wakeup timer.**
A real-time-clock wakeup timer shall function as an additional wake
source for the Sleep → Active transition, independent of the button
(REQ-4).

**REQ-13 — SoC-agnostic core.**
The module's core logic (state machine, public API, transition
orchestration) shall be implemented independent of any specific SoC.

**REQ-14 — Board-specific backend.**
An STM32F407-specific backend shall implement the SoC-specific behavior
required by REQ-13 for v1, including Stop-mode entry/exit and
per-peripheral suspend/resume operations.

**REQ-15 — Thread-safe trigger handling.**
Interrupt-context triggers (the button ISR from REQ-2/REQ-4) shall not
directly invoke suspend/resume operations. They shall defer execution
to thread context.

**REQ-16 — Naming namespace compliance.**
Public identifiers shall use the `pwr_profile_` prefix family and shall
not use a bare `PWR_` or `SYS_` prefix.

| Element | Required name |
|---|---|
| Module name | `pwr_profile_mgr` |
| Generic core | `pwr_profile_mgr` |
| Board-specific backend (example) | `pwr_profile_mgr_stm32f4` |
| States (enum) | `PWR_PROFILE_ACTIVE`, `PWR_PROFILE_SLEEPING`, `PWR_PROFILE_SLEEP`, `PWR_PROFILE_WAKING` |
| Commands (enum/macros) | `PWR_PROFILE_CMD_SUSPEND`, `PWR_PROFILE_CMD_RESUME` |
| Public function prefix | `pwr_profile_*` |
| Internal-only function | `pwr_profile_set_state()` |

**REQ-17 — Sleep-state hardware mapping documentation.**
`PWR_PROFILE_SLEEP` maps to STM32 **Stop mode**, not STM32's own
lighter "Sleep mode". This distinction shall be documented in code
comments and the project README to avoid confusion against the STM32
reference manual (RM0090).

## Scope exclusions

**REQ-18 — No external hardware.**
The module shall not require any hardware beyond the onboard STM32F407
Discovery peripherals covered by REQ-8 through REQ-12.

**REQ-19 — Upstream-quality robustness not required.**
Full Zephyr-upstream-quality items — Doxygen-style API documentation,
`ztest` unit/integration coverage, twister CI configs, and full
Kconfig/Devicetree binding polish — are not required for v1. REQ-15
(thread-safe trigger handling) is exempt from this exclusion and
remains mandatory.
