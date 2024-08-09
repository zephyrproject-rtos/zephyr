use core::time::Duration;
use crate::errno::{check_result, ErrnoResult};
use crate::heap::{object_alloc, object_free};
use crate::timeout::into_timeout;

pub struct Mutex {
    mutex: *mut zephyr_sys::k_mutex,
}

impl Mutex {
    pub fn new() -> ErrnoResult<Self> {
        unsafe {
            let mutex = object_alloc()?;
            zephyr_sys::k_mutex_init(mutex);
            Ok(Mutex { mutex })
        }
    }

    pub fn lock(&self, timeout: Duration) -> ErrnoResult<()> {
        unsafe {
            check_result(zephyr_sys::k_mutex_lock(self.mutex, into_timeout(timeout)))
        }
    }

    pub fn unlock(&self) -> ErrnoResult<()> {
        unsafe {
            check_result(zephyr_sys::k_mutex_unlock(self.mutex))
        }
    }
}

unsafe impl Send for Mutex {}
unsafe impl Sync for Mutex {}

impl Drop for Mutex {
    fn drop(&mut self) {
        unsafe {
            object_free(self.mutex);
        }
    }
}
