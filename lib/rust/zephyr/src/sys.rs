// Copyright (c) 2024 Linaro LTD
// SPDX-License-Identifier: Apache-2.0

//! Zephyr 'sys' module.
//!
//! The `zephyr-sys` crate contains the direct C bindings to the Zephyr API.  All of these are
//! unsafe.
//!
//! This module `zephyr::sys` contains thin wrappers to these C bindings, that can be used without
//! unsafe, but as unchanged as possible.

use zephyr_sys::k_timeout_t;

// These two constants are not able to be captured by bindgen.  It is unlikely that these values
// would change in the Zephyr headers, but there will be an explicit test to make sure they are
// correct.
pub const K_FOREVER: k_timeout_t = k_timeout_t { ticks: -1 };
pub const K_NO_WAIT: k_timeout_t = k_timeout_t { ticks: 0 };
