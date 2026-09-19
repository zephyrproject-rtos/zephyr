# Design — pwr_profile_mgr

Unit-level design of the generic core: state machine, exposed types, and
API. For "what must this do" see
[../Requirements/REQUIREMENTS.md](../Requirements/REQUIREMENTS.md). For
"how the code is split and what interfaces exist between layers" see
[../Architecture/ARCHITECTURE.md](../Architecture/ARCHITECTURE.md).

Items marked **(proposed)** are not finalized and are tracked in
[Open points](#open-points) at the end of this file.

## 1. State machine

```
PWR_PROFILE_ACTIVE
        │  (PWR_PROFILE_CMD_SUSPEND)
        ▼
PWR_PROFILE_SLEEPING   ← internal/transient — NOT exposed to external callers
        │
        ▼
PWR_PROFILE_SLEEP
        │  (PWR_PROFILE_CMD_RESUME)
        ▼
PWR_PROFILE_WAKING     ← internal/transient — NOT exposed to external callers
        │
        ▼
PWR_PROFILE_ACTIVE     (cycle repeats)
```

**Why 4 states, not 2:** a 2-state model (`ACTIVE`/`SLEEP` only, guarded
by a busy boolean for re-entrancy) was seriously considered and nearly
adopted. It was reversed once the transient states were reframed not as
progress-indicators but as the **fail-stuck fault-latch mechanism**
(see `pwr_profile_set_state()` under [Functions](#functions-1)) — a
stronger justification than the original "peripherals take time to
suspend" reasoning. The 4-state model is final.

**Public exposure:** external callers only ever observe
`PWR_PROFILE_ACTIVE` or `PWR_PROFILE_SLEEP` via `pwr_profile_get_state()`.
Transient states are an internal implementation detail used for
fault-latching, never requested or observed directly by external code.

## Implementation phases

See [../README.md](../README.md#implementation-phase-sequencing)
for the full sequencing (phases 0–8).

## List

### Datatypes

- None. No `typedef`s are exposed; all exposed types are `enum`s and
  `struct`s listed below.

### Structs

- `struct pwr_profile_ctx` **(proposed)** — internal state record:
  current state, fault-latch flag, retry count.

### Enums

- `enum pwr_profile_state` — the four profile states.
- `enum pwr_profile_cmd` — the two trigger commands.

### Functions

- `pwr_profile_get_state()` — public; query current stable state.
- `pwr_profile_suspend()` — public; request Active → Sleep.
- `pwr_profile_resume()` — public; request Sleep → Active.
- `pwr_profile_set_state()` — internal (`static`); single implementation
  of all transition logic.

## Explanation

### Datatypes

None exposed. See [Structs](#structs-1) and [Enums](#enums-1).

### Structs

#### `struct pwr_profile_ctx` (proposed)

```c
struct pwr_profile_ctx {
	enum pwr_profile_state state;
	bool transition_failed;
	uint8_t retry_count;
};
```

**Explanation:**
Holds the module's runtime state. The same `state` enum value
(`SLEEPING`/`WAKING`) is used both transiently during a normal fast
transition and as the permanent fault-latch value when a rollback fails
after retries; `transition_failed` and `retry_count` disambiguate these
so debuggers, logs and callers can tell them apart:

- `state` — direction of the in-flight or failed transition (suspend vs.
  resume) when in a transient value.
- `transition_failed` — severity/certainty: `false` = normal in-progress
  or stable; `true` = latched fault, retries exhausted.
- `retry_count` — attempt history: number of rollback retries made
  before giving up (0 if none were needed).

A heavier parallel fault-reason enum was proposed and explicitly
rejected in favor of this lightweight form.

Internal to the core. Exposure to callers (e.g. through an extended
query API) is an open point.

**Boundary values:**

| Field | Lower | Upper |
|---|---|---|
| `state` | `PWR_PROFILE_ACTIVE` | `PWR_PROFILE_WAKING` |
| `transition_failed` | `false` | `true` |
| `retry_count` | `0` | `CONFIG_PWR_PROFILE_MGR_MAX_RETRIES` (proposed default 3; type ceiling 255) |

**Valid values:**

- `transition_failed == true` is valid only when `state` is
  `PWR_PROFILE_SLEEPING` or `PWR_PROFILE_WAKING`.
- `retry_count > 0` is valid only when a rollback has been attempted.
- `transition_failed == false` with `retry_count == 0` is the only valid
  combination for `ACTIVE` and `SLEEP`.

### Enums

#### `enum pwr_profile_state`

```c
enum pwr_profile_state {
	PWR_PROFILE_ACTIVE,
	PWR_PROFILE_SLEEPING,
	PWR_PROFILE_SLEEP,
	PWR_PROFILE_WAKING,
};
```

**Explanation:**
The four power-profile states (see [State machine](#1-state-machine)).
`ACTIVE` is normal operation. `SLEEP` is the low-power, SRAM-retained
state and maps to STM32 **Stop mode**, not STM32's own lighter "Sleep
mode" (RM0090). `SLEEPING` and `WAKING` are internal transient states
that double as the fault-latch value when a transition cannot be rolled
back.

**Boundary values:**

- Lower: `PWR_PROFILE_ACTIVE`
- Upper: `PWR_PROFILE_WAKING`

Explicit integer values are not assigned in the design (proposed:
implicit `0`–`3` in the order shown).

**Valid values:**

- Internally: all four.
- As returned to external callers by `pwr_profile_get_state()`: only
  `PWR_PROFILE_ACTIVE` and `PWR_PROFILE_SLEEP`.
- As the `target` of `pwr_profile_set_state()`: only
  `PWR_PROFILE_ACTIVE` and `PWR_PROFILE_SLEEP` (proposed; transients are
  sequenced internally).

#### `enum pwr_profile_cmd`

```c
enum pwr_profile_cmd {
	PWR_PROFILE_CMD_SUSPEND,
	PWR_PROFILE_CMD_RESUME,
};
```

**Explanation:**
The two trigger commands. Every trigger source (button, shell) reduces
to one of these, so the state machine never knows which source fired
(REQ-6). Shell subcommands exposed to the user are plain verbs
(`suspend`, `resume`) and do not carry this prefix.

**Boundary values:**

- Lower: `PWR_PROFILE_CMD_SUSPEND`
- Upper: `PWR_PROFILE_CMD_RESUME`

**Valid values:** both.

### Functions

#### `pwr_profile_get_state()`

```c
enum pwr_profile_state pwr_profile_get_state(void);
```

**Description:**
Returns the current profile state. Named with the `_get_` accessor
suffix to match Zephyr convention. Never returns a transient value to
external callers: while a transition is in flight, or after a fault
latch, the result is defined by the open points below. Bounded by the
per-step timeout like every other hardware-touching path (the "get"
path is covered by the same timeout rule as "set").

**Arguments:**

| Name | Direction | Type | Boundary values |
|---|---|---|---|
| — | — | `void` | n/a |

**Return values:**

Returns an `enum pwr_profile_state` directly, not an errno.

| Value | Meaning |
|---|---|
| `PWR_PROFILE_ACTIVE` | Module is in normal operation. |
| `PWR_PROFILE_SLEEP` | Module is in the low-power state. |

**Thread-safe:** YES (proposed) — reads `state` under the same lock that
guards writes.

#### `pwr_profile_suspend()`

```c
int pwr_profile_suspend(void);
```

**Description:**
Requests the Active → Sleep transition. A thin, intent-expressing
wrapper that calls `pwr_profile_set_state(PWR_PROFILE_SLEEP)`; it must
not duplicate any transition logic. Must be called from thread context
only — never from an ISR (Stop-mode entry and SPI/UART operations are
unsafe in interrupt context). The button ISR therefore submits a
`k_work` item, which calls this function from thread context.

**Arguments:**

| Name | Direction | Type | Boundary values |
|---|---|---|---|
| — | — | `void` | n/a |

**Return values (proposed errno contract):**

| Macro | Meaning |
|---|---|
| `0` | Suspended; state is now `PWR_PROFILE_SLEEP`. |
| `-EALREADY` | Already in `PWR_PROFILE_SLEEP`; nothing done. |
| `-EBUSY` | Another transition is in flight. |
| `-EAGAIN` | Recoverable failure (incl. timeout); all drivers rolled back, state reverted to `PWR_PROFILE_ACTIVE`. |
| `-EIO` | Unrecoverable failure or rollback failed after retries; state latched at `PWR_PROFILE_SLEEPING`, `transition_failed == true`. Target must be reset. |

**Thread-safe:** YES (proposed) — serialized against `resume()` and
concurrent callers by the state lock. **Not ISR-safe.**

#### `pwr_profile_resume()`

```c
int pwr_profile_resume(void);
```

**Description:**
Requests the Sleep → Active transition. Thin wrapper calling
`pwr_profile_set_state(PWR_PROFILE_ACTIVE)`, with the same thread-context
restriction as `suspend()`. The wake sources (button GPIO/EXTI, RTC
wakeup timer) reach it through deferred work, not directly from their
ISRs.

**Arguments:**

| Name | Direction | Type | Boundary values |
|---|---|---|---|
| — | — | `void` | n/a |

**Return values (proposed errno contract):**

| Macro | Meaning |
|---|---|
| `0` | Resumed; state is now `PWR_PROFILE_ACTIVE`. |
| `-EALREADY` | Already in `PWR_PROFILE_ACTIVE`; nothing done. |
| `-EBUSY` | Another transition is in flight. |
| `-EAGAIN` | Recoverable failure (incl. timeout); rolled back, state reverted to `PWR_PROFILE_SLEEP`. |
| `-EIO` | Unrecoverable failure or rollback failed after retries; state latched at `PWR_PROFILE_WAKING`, `transition_failed == true`. Target must be reset. |

**Thread-safe:** YES (proposed) — same as `suspend()`. **Not ISR-safe.**

#### `pwr_profile_set_state()` (internal)

```c
static int pwr_profile_set_state(enum pwr_profile_state target);
```

**Description:**
The single source of truth for every transition; `suspend()` and
`resume()` are thin callers. This mirrors how Zephyr's own PM subsystem
is structured (a general transition function with named actions as
callers). The design is a deliberate **fail-stuck** model — not
fail-closed, not silent rollback-and-pretend — analogous to automotive
ECU graceful degradation and DTC-style fault reporting.

Forward path:

1. Enter the matching transient state (`SLEEPING` for target `SLEEP`,
   `WAKING` for target `ACTIVE`).
2. Make a **single** attempt, no retry on the forward path. Every
   hardware step (each driver's suspend/resume, and Stop-mode
   entry/exit) is bounded by a per-step timeout. Expiry is a failure
   and feeds the same rollback logic, not a special case.
3. On success, set the stable target state and return `0`.

On failure, classify the backend's error first:

- **Recoverable** (e.g. timeout, `-EAGAIN`-style): roll back.
- **Unrecoverable** (e.g. `-EIO`-style, bus lockup): rollback may itself
  be unsafe — skip straight to the fault latch.

Rollback:

- Each driver (LED, UART shell, accelerometer) owns its own rollback
  routine.
- **Walk every driver in the rollback set; do not stop at the first
  failure.** Stopping early would sacrifice diagnostic completeness —
  the goal is that the user can see which driver is causing the error.
  The cost is negligible with three lightweight onboard drivers.
- If a driver's rollback fails, retry **that driver only**, up to
  `max_retries`. Retry is per-driver; drivers that already rolled back
  are not restarted.
- Log each failing driver and its retry outcome.

Terminal outcomes:

- **All rollbacks succeed:** revert to the previous stable state and
  return the original failure to the caller. Hardware is known-good;
  the transition simply did not happen.
- **Any rollback still fails after retries:** do **not** revert. Latch
  `state` at the transient value already reached, set
  `transition_failed = true`, record `retry_count`, log full per-driver
  detail, and tell the caller the target must be reset. No further
  hardware operations are attempted, since hardware state is no longer
  trustworthy. Once latched, later calls return `-EIO` immediately.

**Arguments:**

| Name | Direction | Type | Boundary values |
|---|---|---|---|
| `target` | in | `enum pwr_profile_state` | `PWR_PROFILE_ACTIVE` or `PWR_PROFILE_SLEEP` only (proposed). Transient values are invalid input. |

**Return values (proposed errno contract):**

| Macro | Meaning |
|---|---|
| `0` | Transition succeeded. |
| `-EINVAL` | `target` is a transient or out-of-range value. |
| `-EALREADY` | Already in `target`. |
| `-EBUSY` | Another transition is in flight. |
| `-EAGAIN` | Recoverable failure; rolled back, prior state restored. |
| `-EIO` | Unrecoverable failure or fault latched. |

**Thread-safe:** YES (proposed) — must run under the state lock; the
lock is held across the whole transition so callers cannot interleave.
**Not ISR-safe.** Internal; not callable from outside the core.

## Open points

1. **`pwr_profile_set_state()` signature and bookkeeping** — direction is
   set (stable-state-only `target`, transients sequenced internally) but
   the exact signature and internal bookkeeping are not drafted in code.
2. **Return-code contract** — the recoverable vs. unrecoverable
   distinction between backend and core needs an exact negative-errno
   convention. All macros above are **proposed**.
3. **`max_retries` value** — example 3; to be a Kconfig constant.
4. **Per-step timeout value** — example 100 ms; to be a Kconfig
   constant. Applies to both "set" and "get" paths.
5. **Thread-safety mechanism** — mutex vs. atomic for `state`,
   `transition_failed`, `retry_count`; and the exact ISR → `k_work`
   flow for the button handler. A v1 correctness requirement (REQ-15),
   not deferred.
6. **`struct pwr_profile_ctx` exposure** — whether `transition_failed`
   and `retry_count` are visible to callers (e.g. an extended query or
   the `status` shell subcommand), and what `pwr_profile_get_state()`
   returns while a fault is latched.
7. **`status` shell subcommand** — recommended (prints current state),
   not yet specified.
