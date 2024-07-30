pub fn rand32_get(range: core::ops::Range<u32>) -> u32 {
    let range_size = range.end - range.start;
    let random_value = unsafe { zephyr_sys::sys_rand32_get() };
    range.start + (random_value % range_size)
}