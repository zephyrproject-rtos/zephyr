// Copyright (c) 2024 Linaro LTD
// SPDX-License-Identifier: Apache-2.0

#![no_std]

// Reference the Zephyr crate so that the panic handler gets used.  This is only needed if no
// symbols from the crate are directly used.
extern crate zephyr;
extern crate alloc;

use zephyr::*;
use zephyr::drivers::gpio;

#[no_mangle]
extern "C" fn rust_main() {
    println!("Hello, world! {}", kconfig::CONFIG_BOARD);

    let pin = gpio::Pin::new(gpio_dt_spec_get!(dt_alias!(led0), gpios));

    pin.configure(gpio::Flags::OutputActive).expect("Failed to configure pin.");

    loop {
        kernel::msleep(1000);
        pin.toggle().expect("Failed to toggle pin.");
    }
}
