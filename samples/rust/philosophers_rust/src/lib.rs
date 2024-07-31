// Copyright (c) 2024 Linaro LTD
// SPDX-License-Identifier: Apache-2.0

#![no_std]

use zephyr::{
    printkln,
    kernel_stack_define,
    kernel_thread_define,
    sys_mutex_define,
};
use zephyr::object::KobjInit;
use zephyr_sys::{
    k_timeout_t,
    k_msleep,
};

#[no_mangle]
extern "C" fn rust_main() {
    printkln!("Hello world from Rust on {}",
              zephyr::kconfig::CONFIG_BOARD);

    // Slightly awkward
    /*
    unsafe {
        for i in 0..FORKS.len() {
            k_mutex_init(addr_of_mut!(FORKS[i]));
        }
    }
    */
    FORK1.init();
    let fork = FORK1.get();

    printkln!("Pre fork");
    // PHIL_THREAD.simple_spawn(PHIL_STACK.token(), phil_thread);
    PHIL_THREAD.spawn(PHIL_STACK.token(), move || {
        phil_thread(1);
    });
    printkln!("Post fork");

    // Steal the fork long enough to make this interesting.
    unsafe {
        printkln!("About to sleep in main");
        k_msleep(1_000);
        printkln!("Done with sleep in main");
    }
    loop {
        printkln!("stealing fork");
        fork.lock(FOREVER);
        printkln!("fork stolen");
            // k_busy_wait(1_000_000);
        unsafe { k_msleep(1_000) };
        printkln!("Main: giving up fork");

        fork.unlock();
        unsafe { k_msleep(1) };
    }
}

const FOREVER: k_timeout_t = k_timeout_t { ticks: !0 };

fn phil_thread(n: usize) {
    printkln!("Child {} started", n);
    let fork = FORK1.get();
    // Print out periodically.
    loop {
        fork.lock(FOREVER);
        printkln!("Child eat");
        fork.unlock();
        unsafe { k_msleep(500) };
    }
}

/*
#[used]
#[link_section = concat!(".", "_k_mutex", ".static.", "FORKS_")]
static mut FORKS: [k_mutex; 6] = unsafe { core::mem::zeroed() };
*/
sys_mutex_define!(FORK1);

kernel_stack_define!(PHIL_STACK, 4096);
kernel_thread_define!(PHIL_THREAD);
