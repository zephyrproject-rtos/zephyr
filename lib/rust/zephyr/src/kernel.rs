// Copyright (c) 2024 Zühlke Engineering AG
// SPDX-License-Identifier: Apache-2.0

use core::ffi::c_char;
use alloc::ffi::CString;

#[macro_export]
macro_rules! printk {
    ($msg:expr) => {
        crate::kernel::printk($msg);
    };
    ($fmt:expr, $($arg:tt)*) => {
        crate::kernel::printk(::alloc::format!($fmt, $($arg)*).as_str());
    };
}

pub fn printk(msg: &str) {
    let cstring = CString::new(msg).unwrap();

    unsafe {
        zephyr_sys::printk("%s\0".as_ptr() as *const c_char, cstring.as_ptr() as *const c_char);
    }
}

pub fn msleep(ms: i32) -> i32 {
    unsafe { zephyr_sys::k_msleep(ms) }
}
