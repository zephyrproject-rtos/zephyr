// Copyright (c) 2024 Zühlke Engineering AG
// SPDX-License-Identifier: Apache-2.0

use core::time::Duration;

const FOREVER: zephyr_sys::k_timeout_t = zephyr_sys::k_timeout_t { ticks: -1 as zephyr_sys::k_ticks_t };
const NO_WAIT: zephyr_sys::k_timeout_t = zephyr_sys::k_timeout_t { ticks: 0 };

// Not sure if it makes more sense to use core::time::Duration or to create a Rust wrapper for
// k_timeout_t to use on Rust APIs.

pub(crate) fn into_timeout(value: Duration) -> zephyr_sys::k_timeout_t {
    if value == Duration::MAX {
        return FOREVER;
    }
    if value == Duration::ZERO {
        return NO_WAIT;
    }

    const NANOS_PER_SEC: u128 = 1_000_000_000;
    const TICKS_PER_SEC: u128 = crate::kconfig::CONFIG_SYS_CLOCK_TICKS_PER_SEC as u128;

    let ticks = (value.as_nanos() * TICKS_PER_SEC).div_ceil(NANOS_PER_SEC);
    zephyr_sys::k_timeout_t { ticks: ticks as zephyr_sys::k_ticks_t }
}
