// Copyright (c) 2024 Linaro LTD
// SPDX-License-Identifier: Apache-2.0

#![no_std]

use zephyr::println;

#[no_mangle]
extern "C" fn rust_main() {
    println!("Blinky!");
    blink();
}

fn blink() {
    println!("Inside of blinky");

    let mut led0 = zephyr::devicetree::aliases::led0::get_instance();

    if !led0.is_ready() {
        println!("LED is not ready");
        return;
    }

    led0.configure((1 << 17) | (1 << 19) | (1 << 20));
    loop {
        led0.toggle_pin();
        zephyr::sys::k_msleep(500);
    }
}
