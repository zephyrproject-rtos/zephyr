// Copyright (c) 2024 Linaro LTD
// SPDX-License-Identifier: Apache-2.0

#![no_std]

// Reference the Zephyr crate so that the panic handler gets used.  This is only needed if no
// symbols from the crate are directly used.
extern crate zephyr;

#[no_mangle]
extern "C" fn rust_main() {
    unsafe {
        printk("Hello world from Rust\n\0".as_ptr());
    }
}

extern "C" {
    fn printk(msg: *const u8);
}
