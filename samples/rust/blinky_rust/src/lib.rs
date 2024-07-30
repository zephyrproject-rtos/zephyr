// Copyright (c) 2024 Linaro LTD
// SPDX-License-Identifier: Apache-2.0

#![no_std]

use log::warn;

use core::time::Duration;
use zephyr::sys::gpio::GPIO_OUTPUT_ACTIVE;
use zephyr::thread::Thread;
use zephyr::userspace::object_access_grant;

#[no_mangle]
extern "C" fn rust_main() {
    zephyr::set_logger();

    warn!("Starting blinky");

    let mut led0 = zephyr::devicetree::aliases::led0::get_instance();
    led0.configure(GPIO_OUTPUT_ACTIVE);

    object_access_grant(&led0, &Thread::current_get());

    Thread::user_mode_enter(move || {
        warn!("Inside of blinky");

        loop {
            led0.toggle_pin();
            Thread::sleep(Duration::from_millis(500));
        }
    });
}
