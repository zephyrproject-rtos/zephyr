use core::ffi::c_void;
use crate::sys::gpio::GpioPin;
use crate::thread::Thread;

pub trait KernelObject {
    fn raw_ptr(&self) -> *const c_void;
}

impl KernelObject for GpioPin {
    fn raw_ptr(&self) -> *const c_void {
        self.pin.port as *const c_void
    }
}

pub fn object_access_grant(object: &dyn KernelObject, thread: &Thread) {
    unsafe {
        zephyr_sys::k_object_access_grant(object.raw_ptr(), thread.tid);
    }
}