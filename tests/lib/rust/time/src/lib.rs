// Copyright (c) 2024 Linaro LTD
// SPDX-License-Identifier: Apache-2.0

#![no_std]

use core::ffi::{
    c_char,
    CStr,
};

use zephyr::printkln;
use zephyr::time::{Duration, Instant, Tick, Timeout};
use zephyr::raw::k_timeout_t;

#[no_mangle]
extern "C" fn rust_main() {
    printkln!("Tick frequency: {}", zephyr::time::SYS_FREQUENCY);
    check_conversions();
    printkln!("All tests passed");
}

/// Verify that the conversions are correct.
fn check_conversions() {
    let mut index = 0;
    loop {
        // The entry returns is always valid, so is a valid reference.
        let entry = unsafe { &*get_time_entry(index) };
        if entry.name.is_null() {
            break;
        }
        let name = unsafe {
            CStr::from_ptr(entry.name).to_str().expect("Invalid C string")
        };
        printkln!("Testing: {}", name);

        // The units must match the enum in the C code.
        match entry.units {
            // UNIT_FOREVER
            0 => {
                assert_eq!(entry.value.ticks, zephyr::sys::K_FOREVER.ticks);
            }
            // UNIT_NO_WAIT
            1 => {
                assert_eq!(entry.value.ticks, zephyr::sys::K_NO_WAIT.ticks);
            }
            // UNIT_DUR_MS
            2 => {
                let value = Duration::millis_at_least(entry.uvalue as Tick);
                let value: Timeout = value.into();
                assert_eq!(entry.value.ticks, value.0.ticks);
            }
            // UNIT_INST_MS
            3 => {
                let base = Instant::from_ticks(0);
                let value = Duration::millis_at_least(entry.uvalue as Tick);
                let value = base + value;
                let value: Timeout = value.into();
                let c_value = unsafe { ms_to_abs_timeout(entry.uvalue) };
                if c_value.ticks != value.0.ticks {
                    printkln!("Mismatch C: {}, Rust: {}",
                        c_value.ticks, value.0.ticks);
                }
                assert_eq!(c_value.ticks, value.0.ticks);
            }
            _ => {
                panic!("Invalid unit enum");
            }
        }

        index += 1;
    }
}

/// The time entry information.
#[repr(C)]
struct TimeEntry {
    name: *const c_char,
    units: u32,
    uvalue: i64,
    value: k_timeout_t,
}

extern "C" {
    fn get_time_entry(index: usize) -> *const TimeEntry;
    fn ms_to_abs_timeout(ms: i64) -> k_timeout_t;
}
