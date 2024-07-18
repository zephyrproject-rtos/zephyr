use core::ffi::c_char;
use alloc::ffi::CString;
use zephyr_sys::printk;

#[macro_export]
macro_rules! println {
    ($msg:expr) => {
        ::zephyr::print::println_helper($msg);
    };
    ($fmt:expr, $($arg:tt)*) => {
        let formatted = ::alloc::format!($fmt, $($arg)*);
        ::zephyr::print::println_helper(formatted.as_str());
    };
}

pub fn println_helper(msg: &str) {
    let cstring = CString::new(msg).unwrap();

    unsafe {
        printk("%s\n\0".as_ptr() as *const c_char, cstring.as_ptr() as *const c_char);
    }
}
