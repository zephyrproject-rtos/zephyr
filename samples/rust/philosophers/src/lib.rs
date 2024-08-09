// Copyright (c) 2024 Zühlke Engineering AG
// SPDX-License-Identifier: Apache-2.0

#![no_std]

extern crate alloc;

use alloc::sync::Arc;
use alloc::vec::Vec;
use core::time::Duration;
use zephyr::mutex::Mutex;
use zephyr::{kconfig, printkln};
use zephyr::random::rand32_get;
use zephyr::thread::{K_INHERIT_PERMS, K_USER, Thread};

const PHILOSOPHER_COUNT: usize = 5;
const FORK_COUNT: usize = 4;

fn random_sleep() {
    Thread::sleep(Duration::from_millis(rand32_get(400..800) as u64));
}

fn philosopher(id: usize, fork1: &Mutex, fork2: &Mutex) {
    loop {
        printkln!("Phil {} is THINKING", id);
        random_sleep();

        printkln!("Phil {} is HUNGRY", id);
        fork1.lock(Duration::MAX).unwrap();
        fork2.lock(Duration::MAX).unwrap();

        printkln!("Phil {} is EATING", id);
        random_sleep();

        fork2.unlock().unwrap();
        fork1.unlock().unwrap();
    }
}

#[no_mangle]
extern "C" fn rust_main() {
    printkln!("Hello, world! {}", kconfig::CONFIG_BOARD);

    let mut forks = Vec::new();

    for _ in 0..FORK_COUNT {
        forks.push(Arc::new(Mutex::new().unwrap()));
    }

    let mut philosophers = Vec::new();

    for phil_id in 0..PHILOSOPHER_COUNT {
        let mut fork_id1 = phil_id % FORK_COUNT;
        let mut fork_id2 = (phil_id + 1) % FORK_COUNT;

        if fork_id1 > fork_id2 {
            core::mem::swap(&mut fork_id1, &mut fork_id2);
        }

        let fork1 = forks[fork_id1].clone();
        let fork2 = forks[fork_id2].clone();

        // README: To test user mode, remove K_INHERIT_PERMS -> accessing the mutex created
        // by this thread will fail and lead to a kernel oops.
        let thread = Thread::new(move || {
            philosopher(phil_id, fork1.as_ref(), fork2.as_ref());
        }, 2048, 5, K_USER | K_INHERIT_PERMS, Duration::ZERO).unwrap();

        thread.name_set(alloc::format!("philosopher_{}", phil_id).as_str()).unwrap();

        philosophers.push(thread);
    }

    Thread::user_mode_enter(|| rust_main_user());
}

fn rust_main_user() {
    printkln!("Main thread entered user mode.");
    Thread::sleep(Duration::MAX);
}
