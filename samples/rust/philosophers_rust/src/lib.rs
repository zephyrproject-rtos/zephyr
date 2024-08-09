// Copyright (c) 2024 Linaro LTD
// SPDX-License-Identifier: Apache-2.0

#![no_std]

extern crate alloc;

use alloc::sync::Arc;
use zephyr::{
    printkln,
    kernel_stack_define,
    kernel_thread_define,
    sys_mutex_define,
    sync::Mutex,
};
use zephyr::object::KobjInit;
use zephyr_sys::{
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
    let fork = Arc::new(Mutex::new(1, fork));

    printkln!("Pre fork");
    // PHIL_THREAD.simple_spawn(PHIL_STACK.token(), phil_thread);
    let child_fork = fork.clone();
    PHIL_THREAD.spawn(PHIL_STACK.token(), move || {
        phil_thread(1, child_fork);
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
        {
            let mut lock = fork.lock().unwrap();
            let current = *lock;
            *lock += 1;
            printkln!("fork stolen: {}", current);
            unsafe { k_msleep(1_000) };
            printkln!("Main: giving up fork");
        };
        unsafe { k_msleep(1) };
    }
}

fn phil_thread(n: usize, fork: Arc<Mutex<usize>>) {
    printkln!("Child {} started", n);
    loop {
        {
            let mut lock = fork.lock().unwrap();
            let current = *lock;
            *lock += 1;
            printkln!("Child eat: {}", current);
            unsafe { k_msleep(500) };
        }
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
