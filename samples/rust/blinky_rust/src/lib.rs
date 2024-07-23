// Copyright (c) 2024 Linaro LTD
// SPDX-License-Identifier: Apache-2.0

#![no_std]

use core::ffi::c_void;

use zephyr::println;
use zephyr::sys::GPIO_OUTPUT_ACTIVE;

#[no_mangle]
extern "C" fn rust_main() {
    // println!("Blinky!");
    // Invoke "blink" as a user thread.
    // blink();
    unsafe {
        k_thread_user_mode_enter(blink,
                                 core::ptr::null_mut(),
                                 core::ptr::null_mut(),
                                 core::ptr::null_mut());
    }
}

// fn blink() {
extern "C" fn blink(_p1: *mut c_void, _p2: *mut c_void, _p3: *mut c_void) -> ! {
    println!("Inside of blinky");

    let mut led0 = zephyr::devicetree::aliases::led0::get_instance();

    if !led0.is_ready() {
        println!("LED is not ready");
        loop {
        }
        // return;
    }

    led0.configure(GPIO_OUTPUT_ACTIVE);
    loop {
        led0.toggle_pin();
        zephyr::sys::k_msleep(500);
    }
}

// Hack for usermode.
#[allow(non_camel_case_types)]
type k_thread_entry_t = extern "C" fn(*mut c_void, *mut c_void, *mut c_void) -> !;
extern "C" {
    fn k_thread_user_mode_enter(entry: k_thread_entry_t,
                                p1: *mut c_void,
                                p2: *mut c_void,
                                p3: *mut c_void) -> !;
}
