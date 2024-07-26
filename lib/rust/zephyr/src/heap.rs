// Copyright (c) 2024 Zühlke Engineering AG
// SPDX-License-Identifier: Apache-2.0

use core::alloc::{GlobalAlloc, Layout};
use core::ffi::c_void;

extern "C" {
    static mut rust_heap: zephyr_sys::k_heap;
}

struct ZephyrAllocator {}

#[global_allocator]
static ALLOCATOR: ZephyrAllocator = ZephyrAllocator {};

unsafe impl GlobalAlloc for ZephyrAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        #![allow(static_mut_refs)]
        zephyr_sys::k_heap_aligned_alloc(
            &mut rust_heap, layout.align(),
            layout.size(),
            zephyr_sys::k_timeout_t { ticks: 0 },
        ) as *mut u8
    }

    unsafe fn dealloc(&self, ptr: *mut u8, _layout: Layout) {
        #![allow(static_mut_refs)]
        zephyr_sys::k_heap_free(&mut rust_heap, ptr as *mut c_void)
    }
}
