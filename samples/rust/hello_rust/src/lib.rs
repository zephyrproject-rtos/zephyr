// Copyright (c) 2024 Linaro LTD
// SPDX-License-Identifier: Apache-2.0

#![no_std]

// Reference the Zephyr crate so that the panic handler gets used.  This is only needed if no
// symbols from the crate are directly used.
extern crate zephyr;

use zephyr::gpio::{sleep, toggle};

#[no_mangle]
extern "C" fn rust_main() {
    // Until we have allocation, converting a Rust string into a C string is a bit awkward.
    // This works because the kconfig values are const.
    const BOARD_SRC: &'static str = zephyr::kconfig::CONFIG_BOARD;
    let mut board = [0u8; BOARD_SRC.len() + 1];
    board[..BOARD_SRC.len()].clone_from_slice(BOARD_SRC.as_bytes());
    board[BOARD_SRC.len()] = 0;

    unsafe {
        printk("Hello world from Rust on %s\n\0".as_ptr(), board.as_ptr());
    }

    loop {
        toggle();
        sleep();
    }
}

extern "C" {
    fn printk(msg: *const u8, arg1: *const u8);
}
