//! A very simplistic logger that allocates and uses printk.
//!
//! This is just a stop gap so that users of Rust can have some simple messages.  This provides a
//! `println` macro that works similar to the one in std.
//!
//! This module allocates at least twice as much space as the final message, for the duration of the
//! print call.
//!
//! An unfortunate caveat of this crate is that using it requires clients to have `extern crate
//! alloc;` in each module that uses it.

extern crate alloc;

use core::ffi::c_char;
use alloc::ffi::CString;

pub use alloc::format;

// Printk, with a fixed C-string formatting argument, and one we've built here.
extern "C" {
    fn printk(fmt: *const i8, message: *const c_char);
}

pub fn log_message(message: &str) {
    let raw = CString::new(message).expect("CString::new failed");
    unsafe {
        printk(c"%s\n".as_ptr(), raw.as_ptr());
    }
}

// We assume the log message is accessible at $crate::log::log_message.
#[macro_export]
macro_rules! println {
    ($($arg:tt)+) => {
        {
            let message = $crate::log::format!($($arg)+);
            $crate::log::log_message(&message);
        }
    };
}
