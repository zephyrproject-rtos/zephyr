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
    use zephyr::sys::{device, gpio_dt_spec};

    println!("Inside of blinky");

    let led0 = unsafe {
        extern "C" {
            static __device_dts_ord_11: device;
        }
        gpio_dt_spec {
            port: &__device_dts_ord_11,
            pin: 13,
            dt_flags: 1,
        }
    };

    if led0.is_ready() {
        println!("LED is ready");
    } else {
        println!("LED is not ready");
        return;
    }

    led0.configure((1 << 17) | (1 << 19) | (1 << 20));
    loop {
        led0.toggle_pin();
        zephyr::sys::k_msleep(500);
    }
}
