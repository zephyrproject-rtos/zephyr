// Copyright (c) 2024 Linaro LTD
// SPDX-License-Identifier: Apache-2.0

//! Zephyr application support for Rust
//!
//! This crates provides the core functionality for applications written in Rust that run on top of
//! Zephyr.

#![no_std]

// Bring in the generated kconfig module
include!(concat!(env!("OUT_DIR"), "/kconfig.rs"));

// Ensure that Rust is enabled.
#[cfg(not(CONFIG_RUST))]
compile_error!("CONFIG_RUST must be set to build Rust in Zephyr");

use core::panic::PanicInfo;

/// Override rust's panic.  This simplistic initial version just hangs in a loop.
#[panic_handler]
fn panic(_ :&PanicInfo) -> ! {
    loop {
    }
}
