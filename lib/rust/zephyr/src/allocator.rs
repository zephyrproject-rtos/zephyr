use core::alloc::{GlobalAlloc, Layout};
use zephyr_sys::{k_aligned_alloc, k_free};

struct ZephyrAllocator {}

#[global_allocator]
static ALLOCATOR: ZephyrAllocator = ZephyrAllocator {};

unsafe impl Sync for ZephyrAllocator {}

unsafe impl GlobalAlloc for ZephyrAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        return k_aligned_alloc(layout.align(), layout.size()) as *mut u8;
    }

    unsafe fn dealloc(&self, _ptr: *mut u8, _layout: Layout) {
        return k_free(_ptr as *mut core::ffi::c_void);
    }
}
