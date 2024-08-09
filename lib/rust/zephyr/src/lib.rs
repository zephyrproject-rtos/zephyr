// Copyright (c) 2024 Linaro LTD
// SPDX-License-Identifier: Apache-2.0

//! Zephyr application support for Rust
//!
//! This crates provides the core functionality for applications written in Rust that run on top of
//! Zephyr.

#![no_std]

extern crate alloc;

mod allocator;
mod panic;

pub mod errno;
pub mod kernel;
pub mod drivers;
pub mod devicetree;

// To me, it would feel more consistent if kconfig.rs was also generated using Python within the
// Zephyr build infrastructure instead of the zephyr-build crate. For the devicetree this was
// much easier as there is already the pickled EDT available.

// Bring in the generated kconfig module
include!(concat!(env!("OUT_DIR"), "/kconfig.rs"));

// Ensure that Rust is enabled.
#[cfg(not(CONFIG_RUST))]
compile_error!("CONFIG_RUST must be set to build Rust in Zephyr");
