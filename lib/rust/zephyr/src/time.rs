// Copyright (c) 2024 Linaro LTD
// SPDX-License-Identifier: Apache-2.0

//! Time types designed for Zephyr, inspired by `std::time`.
//!
//! In `std`, there are two primary time types: `Duration`, representing a span of time, and
//! `Instant`, representing a specific point in time. Both have nanosecond precision, which is
//! well-suited to more powerful machines. However, on embedded systems like Zephyr, this precision
//! can lead to performance issues, often requiring divisions whenever timeouts are used.
//!
//! In the Rust embedded ecosystem, the `fugit` crate is commonly used for handling time. It
//! provides both `Duration` and `Instant` types, but with parameters that allow the representation
//! to match the time slice used, enabling compile-time conversion and storing time directly as
//! tick counts.
//!
//! Zephyr manages time in terms of system tick intervals, derived from
//! `sys_clock_hw_cycles_per_sec()`.  This model aligns well with `fugit`, especially when the
//! types are properly parameterized.
//!
//! It's important to note that Rust’s `std::Instant` requires time to be monotonically increasing.
//!
//! Zephyr’s `sys/time_units.h` provides a variety of optimized macros for manipulating time
//! values, converting between human-readable units and ticks, and minimizing divisions (especially
//! by non-constant values).  Similarly, the `fugit` crate offers constructors that aim to result
//! in constants when possible, avoiding costly division operations.

use zephyr_sys::{k_timeout_t, k_ticks_t};

use core::fmt::Debug;

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
        let ticks: k_ticks_t = checked_cast(value.ticks());
        debug_assert_ne!(ticks, crate::sys::K_FOREVER.ticks);
        debug_assert_ne!(ticks, crate::sys::K_NO_WAIT.ticks);
        Timeout(k_timeout_t { ticks })
    }
}

#[cfg(CONFIG_TIMEOUT_64BIT)]
impl From<Instant> for Timeout {
    fn from(value: Instant) -> Timeout {
        let ticks: k_ticks_t = checked_cast(value.ticks());
        debug_assert_ne!(ticks, crate::sys::K_FOREVER.ticks);
        debug_assert_ne!(ticks, crate::sys::K_NO_WAIT.ticks);
        Timeout(k_timeout_t { ticks: -1 - 1 - ticks })
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

/// Convert from the Tick time type, which is unsigned, to the `k_ticks_t` type. When debug
/// assertions are enabled, it will panic on overflow.
fn checked_cast<I, O>(tick: I) -> O
where
    I: TryInto<O>,
    I::Error: Debug,
    O: Default,
{
    if cfg!(debug_assertions) {
        tick.try_into().expect("Overflow in time conversion")
    } else {
        tick.try_into().unwrap_or(O::default())
    }
}
