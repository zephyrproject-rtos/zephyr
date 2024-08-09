//! A Rust global allocator that uses the stdlib allocator in Zephyr

// This entire module is only use if CONFIG_RUST_ALLOC is enabled.
extern crate alloc;

use core::alloc::{GlobalAlloc, Layout};

use alloc::alloc::handle_alloc_error;

/// Define size_t, as it isn't defined within the FFI.
#[allow(non_camel_case_types)]
type c_size_t = usize;

extern "C" {
    fn malloc(size: c_size_t) -> *mut u8;
    fn free(ptr: *mut u8);
}

pub struct ZephyrAllocator;

unsafe impl GlobalAlloc for ZephyrAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        let size = layout.size();
        let align = layout.align();

        // The C allocation library assumes an alignment of 8.  For now, just panic if this cannot
        // be satistifed.
        if align > 8 {
            handle_alloc_error(layout);
        }

        malloc(size)
    }

    unsafe fn dealloc(&self, ptr: *mut u8, _layout: Layout) {
        free(ptr)
    }
}

#[global_allocator]
static ZEPHYR_ALLOCATOR: ZephyrAllocator = ZephyrAllocator;
