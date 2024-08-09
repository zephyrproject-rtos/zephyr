// Copyright (c) 2024 Zühlke Engineering AG
// SPDX-License-Identifier: Apache-2.0

use alloc::boxed::Box;
use alloc::ffi::CString;
use core::ffi::c_void;
use core::ptr;
use core::time::Duration;
use crate::errno::{check_result, ErrnoResult};
use crate::heap::{object_alloc, thread_stack_alloc};
use crate::timeout::into_timeout;

pub const K_USER: u32 = 1 << 2;
pub const K_INHERIT_PERMS: u32 = 1 << 3;

pub struct Thread {
    pub(crate) tid: zephyr_sys::k_tid_t,
}

impl Thread {
    pub fn current_get() -> Thread {
        unsafe {
            Thread { tid: zephyr_sys::k_current_get() }
        }
    }

    pub fn new<F>(entry_point: F, stack_size: usize, priority: i32, options: u32, delay: Duration) -> ErrnoResult<Self>
    where
        F: FnOnce() + 'static,
        F: Send,
    {
        let boxed_closure = Box::new(Box::new(entry_point) as Box<dyn FnOnce()>);
        let closure_ptr = Box::into_raw(boxed_closure) as *mut c_void;

        unsafe {
            // TODO: Ownership of these is not handled yet
            let thread = object_alloc()?;
            let thread_stack = thread_stack_alloc(stack_size, options)?;

            let tid = zephyr_sys::k_thread_create(
                thread,
                thread_stack,
                stack_size,
                Some(rust_thread_entry),
                closure_ptr,
                ptr::null_mut(),
                ptr::null_mut(),
                priority,
                options,
                into_timeout(delay),
            );

            Ok(Thread { tid })
        }
    }

    pub fn start(&self) {
        unsafe {
            zephyr_sys::k_thread_start(self.tid);
        }
    }

    pub fn name_set(&self, name: &str) -> ErrnoResult<()> {
        unsafe {
            let cname = CString::new(name).unwrap();
            check_result(zephyr_sys::k_thread_name_set(self.tid, cname.as_ptr()))
        }
    }

    pub fn user_mode_enter<F>(entry_point: F) -> !
    where
        F: FnOnce() + 'static,
        F: Send,
    {
        let boxed_closure = Box::new(Box::new(entry_point) as Box<dyn FnOnce()>);
        let closure_ptr = Box::into_raw(boxed_closure) as *mut c_void;

        unsafe {
            zephyr_sys::k_thread_user_mode_enter(
                Some(rust_thread_entry),
                closure_ptr,
                ptr::null_mut(),
                ptr::null_mut()
            )
        }
    }

    pub fn sleep(duration: Duration) -> i32 {
        unsafe {
            zephyr_sys::k_sleep(into_timeout(duration)) as i32
        }
    }
}

extern fn rust_thread_entry(closure_ptr: *mut c_void, _p2: *mut c_void, _p3: *mut c_void) {
    let closure_box: Box<Box<dyn FnOnce()>> = unsafe {
        Box::from_raw(closure_ptr as *mut Box<dyn FnOnce()>)
    };

    closure_box();
}
