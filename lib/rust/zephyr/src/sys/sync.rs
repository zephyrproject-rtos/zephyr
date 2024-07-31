//! Lightweight wrappers around Zephyr's synchronization mechanisms.

use zephyr_sys::{
    k_mutex, k_mutex_init, k_mutex_lock, k_mutex_unlock, k_timeout_t
};

use crate::object::{KobjInit, StaticKernelObject};

#[derive(Clone)]
pub struct Mutex {
    pub item: *mut k_mutex,
}

unsafe impl Sync for StaticKernelObject<k_mutex> { }

impl KobjInit<k_mutex, Mutex> for StaticKernelObject<k_mutex> {
    fn wrap(ptr: *mut k_mutex) -> Mutex {
        Mutex { item: ptr }
    }
}

impl Mutex {
    pub fn lock(&self, timeout: k_timeout_t) {
        unsafe { k_mutex_lock(self.item, timeout); }
    }

    pub fn unlock(&self) {
        unsafe { k_mutex_unlock(self.item); }
    }
}

#[macro_export]
macro_rules! sys_mutex_define {
    ($name:ident) => {
        #[link_section = concat!("._k_mutex.static.", stringify!($name), ".", file!(), line!())]
        static $name: $crate::object::StaticKernelObject<$crate::raw::k_mutex> =
            $crate::object::StaticKernelObject::new();
    };
}

impl StaticKernelObject<k_mutex> {
    pub fn init(&self) {
        self.init_help(|raw| {
            unsafe {
                k_mutex_init(raw);
            }
        })
    }
}
