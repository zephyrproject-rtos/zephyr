// Copyright (c) 2024 Linaro LTD
// SPDX-License-Identifier: Apache-2.0

//! Time types similar to `std::time` types.
//!
//! However, the rust-embedded world tends to use `fugit` for time.  This has a Duration and
//! Instant type, but they are parameterized by the slice used, and try hard to do the conversion
//! at compile time.
//!
//! std has two time types, a Duration which represents an elapsed amount of time, and Instant,
//! which represents a specific instance in time.
//!
//! Zephyr typically coordinates time in terms of a system tick interval, which comes from
//! `sys_clock_hw_cycles_per_sec()`.
//!
//! The Rust/std semantics require Instant to be monotonically increasing.
//!
//! Zephyr's `sys/time_units.h` header contains numerous optimized macros for manipulating time in
//! these units, specifically for converting between human time units and ticks, trying to avoid
//! division, especially division by non-constants.

use zephyr_sys::k_timeout_t;

// The system ticks, is mostly a constant, but there are some boards that use a dynamic tick
// frequency, and thus need to read this at runtime.
#[cfg(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME)]
compile_error!("Rust does not (yet) support dynamic frequency timer");

// Given the above not defined, the system time base comes from a kconfig.
/// The system time base.  The system clock has this many ticks per second.
pub const SYS_FREQUENCY: u32 = crate::kconfig::CONFIG_SYS_CLOCK_TICKS_PER_SEC as u32;

/// Zephyr can be configured for either 64-bit or 32-bit time values.  Use the appropriate type
/// internally to match.  This should end up the same size as `k_ticks_t`, but unsigned instead of
/// signed.
#[cfg(CONFIG_TIMEOUT_64BIT)]
pub type Tick = u64;
#[cfg(not(CONFIG_TIMEOUT_64BIT))]
pub type Tick = u32;

/// Duration appropraite for Zephyr calls that expect `k_timeout_t`.  The result will be a time
/// interval from "now" (when the call is made).
pub type Duration = fugit::Duration<Tick, 1, SYS_FREQUENCY>;

/// An Instant appropriate for Zephyr calls that expect a `k_timeout_t`.  The result will be an
/// absolute time in terms of system ticks.
#[cfg(CONFIG_TIMEOUT_64BIT)]
pub type Instant = fugit::Instant<Tick, 1, SYS_FREQUENCY>;

// The zephry k_timeout_t represents several different types of intervals, based on the range of
// the value.  It is a signed number of the same size as the Tick here, which effectively means it
// is one bit less.
//
// 0: K_NO_WAIT: indicates the operation should not wait.
// 1 .. Max: indicates a duration in ticks of a delay from "now".
// -1: K_FOREVER: a time that never expires.
// MIN .. -2: A wait for an absolute amount of ticks from the start of the system.
//
// The absolute time offset is only implemented when time is a 64 bit value.  This also means that
// "Instant" isn't available when time is defined as a 32-bit value.

// Wrapper around the timeout type, so we can implement From/Info.
pub struct Timeout(pub k_timeout_t);

// From allows methods to take a time of various types and convert it into a Zephyr timeout.
impl From<Duration> for Timeout {
    fn from(value: Duration) -> Timeout {
        Timeout(k_timeout_t { ticks: value.ticks() as i64 })
    }
}

#[cfg(CONFIG_TIMEOUT_64BIT)]
impl From<Instant> for Timeout {
    fn from(value: Instant) -> Timeout {
        Timeout(k_timeout_t { ticks: -1 - 1 - (value.ticks() as i64) })
    }
}

/// A sleep that waits forever.  This is its own type, that is `Into<Timeout>` and can be used
/// anywhere a timeout is needed.
pub struct Forever;

impl From<Forever> for Timeout {
    fn from(_value: Forever) -> Timeout {
        Timeout(crate::sys::K_FOREVER)
    }
}

/// A sleep that doesn't ever wait.  This is its own type, that is `Info<Timeout>` and can be used
/// anywhere a timeout is needed.
pub struct NoWait;

impl From<NoWait> for Timeout {
    fn from(_valued: NoWait) -> Timeout {
        Timeout(crate::sys::K_NO_WAIT)
    }
}

/// Put the current thread to sleep, for the given duration.  Uses `k_sleep` for the actual sleep.
/// Returns a duration roughly representing the remaining amount of time if the sleep was woken.
pub fn sleep<T>(timeout: T) -> Duration
    where T: Into<Timeout>,
{
    let timeout: Timeout = timeout.into();
    let rest = unsafe { crate::raw::k_sleep(timeout.0) };
    Duration::millis(rest as Tick)
}
