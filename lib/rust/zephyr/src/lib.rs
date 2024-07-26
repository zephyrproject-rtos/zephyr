// Copyright (c) 2024 Linaro LTD
// SPDX-License-Identifier: Apache-2.0

//! Zephyr application support for Rust
//!
//! This crates provides the core functionality for applications written in Rust that run on top of
//! Zephyr.

#![no_std]
#![allow(unexpected_cfgs)]

pub mod sys;

// Bring in the generated kconfig module
include!(concat!(env!("OUT_DIR"), "/kconfig.rs"));

// And the generated devicetree.
include!(concat!(env!("OUT_DIR"), "/devicetree.rs"));

// Ensure that Rust is enabled.
#[cfg(not(CONFIG_RUST))]
compile_error!("CONFIG_RUST must be set to build Rust in Zephyr");

// Printk is provided if it is configured into the build.
#[cfg(CONFIG_PRINTK)]
pub mod printk;

use core::panic::PanicInfo;

#[cfg(CONFIG_PRINTK)]
pub fn set_logger() {
    printk::set_printk_logger();
}

/// Override rust's panic.  This simplistic initial version just hangs in a loop.
#[panic_handler]
fn panic(_ :&PanicInfo) -> ! {
    loop {
    }
}

/// Re-export of zephyr-sys as zephyr::raw.
pub mod raw {
    pub use zephyr_sys::*;
}

/// Provide symbols used by macros in a crate-local namespace.
#[doc(hidden)]
pub mod _export {
    pub use core::format_args;
}

/// If allocation has been requested, provide the allocator.
#[cfg(CONFIG_RUST_ALLOC)]
mod alloc;

// If we have allocation, we can also support logging.
#[cfg(CONFIG_RUST_ALLOC)]
pub mod log {
    #[cfg(CONFIG_LOG)]
    compile_error!("Rust with CONFIG_LOG is not yet supported");

    mod log_printk;

    pub use log_printk::*;
}
