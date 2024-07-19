pub fn msleep(ms: i32) -> i32 {
    unsafe { zephyr_sys::k_msleep(ms) }
}