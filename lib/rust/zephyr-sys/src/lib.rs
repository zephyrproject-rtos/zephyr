// Copyright (c) 2024 Linaro LTD
// SPDX-License-Identifier: Apache-2.0

//! Zephyr application support for Rust
//!
//! This crates provides the core functionality for applications written in Rust that run on top of
//! Zephyr.

#![no_std]

// Allow rust naming convention violations.
#![allow(non_snake_case)]
#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]

// Zephyr makes use of zero-sized structs, which Rustc considers invalid.  Suppress this warning.
// Note, however, that this suppresses any warnings in the bindings about improper C types.
#![allow(improper_ctypes)]

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
