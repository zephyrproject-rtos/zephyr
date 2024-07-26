// Copyright (c) 2024 Linaro LTD
// SPDX-License-Identifier: Apache-2.0

#![no_std]

use log::warn;

use core::ffi::c_void;

use zephyr::sys::gpio::GPIO_OUTPUT_ACTIVE;

#[no_mangle]
extern "C" fn rust_main() {
    zephyr::set_logger();

    warn!("Starting blinky");
    // println!("Blinky!");
    // Invoke "blink" as a user thread.
    // blink();
    if false {
        unsafe {
            zephyr::raw::k_thread_user_mode_enter
                (Some(blink),
                core::ptr::null_mut(),
                core::ptr::null_mut(),
                core::ptr::null_mut());
        }
    } else {
        unsafe {
            blink(core::ptr::null_mut(),
                  core::ptr::null_mut(),
                  core::ptr::null_mut());
        }
    }
}

// fn blink() {
unsafe extern "C" fn blink(_p1: *mut c_void, _p2: *mut c_void, _p3: *mut c_void) {
    warn!("Inside of blinky");

    let mut led0 = zephyr::devicetree::aliases::led0::get_instance();

    if !led0.is_ready() {
        warn!("LED is not ready");
        loop {
        }
        // return;
    }

    led0.configure(GPIO_OUTPUT_ACTIVE);
    loop {
        led0.toggle_pin();
        zephyr::raw::k_msleep(500);
    }
}
