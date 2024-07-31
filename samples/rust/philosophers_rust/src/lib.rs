// Copyright (c) 2024 Linaro LTD
// SPDX-License-Identifier: Apache-2.0

#![no_std]

use core::ptr::addr_of_mut;

use zephyr::{
    printkln,
    kernel_stack_define,
    kernel_thread_define,
};
use zephyr_sys::{
    k_timeout_t,
    k_mutex_init,
    k_mutex_lock,
    k_mutex_unlock,
    k_msleep,
    k_mutex,
};

#[no_mangle]
extern "C" fn rust_main() {
    printkln!("Hello world from Rust on {}",
              zephyr::kconfig::CONFIG_BOARD);

    // Slightly awkward
    unsafe {
        for i in 0..FORKS.len() {
            k_mutex_init(addr_of_mut!(FORKS[i]));
        }
    }

    // PHIL_THREAD.simple_spawn(PHIL_STACK.token(), phil_thread);
    PHIL_THREAD.spawn(PHIL_STACK.token(), move || {
        phil_thread(1);
    });

    // Steal the fork long enough to make this interesting.
    unsafe {
        printkln!("About to sleep in main");
        k_msleep(1_000);
        printkln!("Done with sleep in main");
    }
    loop {
        unsafe {
            printkln!("stealing fork");
            k_mutex_lock(addr_of_mut!(FORKS[0]), FOREVER);
            printkln!("fork stolen");
            // k_busy_wait(1_000_000);
            k_msleep(1_000);
            printkln!("Main: giving up fork");
            k_mutex_unlock(addr_of_mut!(FORKS[0]));
            k_msleep(1);
        }
    }
}

const FOREVER: k_timeout_t = k_timeout_t { ticks: !0 };

fn phil_thread(n: usize) {
    printkln!("Child {} started", n);
    // Print out periodically.
    loop {
        unsafe {
            k_mutex_lock(addr_of_mut!(FORKS[0]), FOREVER);
            printkln!("Child eat");
            k_mutex_unlock(addr_of_mut!(FORKS[0]));
            k_msleep(500);
        }
    }
}

#[used]
#[link_section = concat!(".", "_k_mutex", ".static.", "FORKS_")]
static mut FORKS: [k_mutex; 6] = unsafe { core::mem::zeroed() };

kernel_stack_define!(PHIL_STACK, 4096);
kernel_thread_define!(PHIL_THREAD);
