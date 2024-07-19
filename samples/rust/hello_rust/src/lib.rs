// Copyright (c) 2024 Linaro LTD
// SPDX-License-Identifier: Apache-2.0

#![no_std]

// Reference the Zephyr crate so that the panic handler gets used.  This is only needed if no
// symbols from the crate are directly used.
extern crate zephyr;
extern crate alloc;

use zephyr::kernel::msleep;
use zephyr::drivers::gpio;
use zephyr::println;

#[no_mangle]
extern "C" fn rust_main() {
    println!("Hello, world! {}", zephyr::kconfig::CONFIG_BOARD);

    let pin = gpio::Pin::get_led();

    pin.configure(gpio::Flags::OutputActive).expect("Failed to configure pin.");

    loop {
        msleep(1000);
        pin.toggle().expect("Failed to toggle pin.");
    }
}
