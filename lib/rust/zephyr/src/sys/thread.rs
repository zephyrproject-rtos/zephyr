//! Zephyr low level threads
//!
//! This is a fairly low level (but still safe) interface to Zephyr threads.  This is intended to
//! work the same way as threads are typically done on Zephyr systems, where the threads and their
//! stacks are statically allocated, a code is called to initialize them.
//!
//! In addition, there are some convenience operations available that require allocation to be
//! available.

use zephyr_sys::{k_thread, k_thread_create, k_timeout_t, z_thread_stack_element, ZR_STACK_ALIGN, ZR_STACK_RESERVED};

use core::{ffi::c_void, ptr::{addr_of, null_mut}};

use crate::{align::AlignAs, object::StaticKernelObject};

/// Adjust the stack size for alignment.  Note that, unlike the C code, we don't include the
/// reservation in this, as it has its own fields in the struct.
pub const fn stack_len(size: usize) -> usize {
    size.next_multiple_of(ZR_STACK_ALIGN)
}

/// A Zephyr stack declaration.  It isn't meant to be used directly, as it needs additional
/// decoration about linker sections and such.  Unlike the C declaration, the reservation is a
/// separate field.  As long as the SIZE is properly aligned, this should work without padding
/// between the fields.
pub struct ThreadStack<const SIZE: usize> {
    pub align: AlignAs<ZR_STACK_ALIGN>,
    pub data: [z_thread_stack_element; SIZE],
    pub extra: [z_thread_stack_element; ZR_STACK_RESERVED],
}

impl<const SIZE: usize> ThreadStack<SIZE> {
    /// Get the size of this stack.  This is the size, minus any reservation.  This is called `size`
    /// to avoid any confusion with `len` which might return the actual size of the stack.
    pub fn size(&self) -> usize {
        self.data.len()
    }

    /// Return the stack base needed as the argument to various zephyr calls.
    pub fn base(&self) -> *mut z_thread_stack_element {
        addr_of!(self.data[0]) as *mut _
    }

    /// Return the token information for this stack, which is a base and size.
    pub fn token(&self) -> StackToken {
        StackToken { base: self.base(), size: self.size() }
    }
}

/// Declare a variable, of a given name, representing the stack for a thread.
#[macro_export]
macro_rules! kernel_stack_define {
    ($name:ident, $size:expr) => {
        #[link_section = concat!(".noinit.", stringify!($name), ".", file!(), line!())]
        static $name: $crate::sys::thread::ThreadStack<{$crate::sys::thread::stack_len($size)}>
            = unsafe { ::core::mem::zeroed() };
    };
}

pub struct Thread {
    pub raw: *mut k_thread,
}
/*
pub struct Thread {
    pub raw: crate::raw::k_thread,
}
*/

unsafe impl Sync for StaticKernelObject<k_thread> { }

/// Declare a global static representing a thread variable.
#[macro_export]
macro_rules! kernel_thread_define {
    ($name:ident) => {
        // Since the static object has an atomic that we assume is initialized, let the compiler put
        // this in the data section it finds appropriate (probably .bss if it is initialized to zero).
        // This only matters when the objects are being checked.
        #[cfg_attr(not(CONFIG_RUST_CHECK_KOBJ_INIT),
                   link_section = concat!(".noinit.", stringify!($name), ".", file!(), line!()))]
        static $name: $crate::object::StaticKernelObject<$crate::raw::k_thread> =
            $crate::object::StaticKernelObject::new();
        // static $name: $crate::sys::thread::Thread = unsafe { ::core::mem::zeroed() };
    };
}

/// For now, this "token" represents the somewhat internal information about thread.
/// What we really want is to make sure that stacks and threads go together.
pub struct StackToken {
    base: *mut z_thread_stack_element,
    size: usize,
}

// This isn't really safe at all, as these can be initialized.  It is unclear how, if even if it is
// possible to implement safe static threads and other data structures in Zephyr.

// The thread itself assumes we've already initialized, so this method is on the wrapper.
impl StaticKernelObject<k_thread> {
    /// Spawn this thread to the given external function.  This is a simplified version that doesn't
    /// take any arguments.  The child runs immediately.
    pub fn simple_spawn(&self, stack: StackToken, child: fn() -> ()) {
        self.init_help(|raw| {
            unsafe {
                k_thread_create(
                    raw,
                    stack.base,
                    stack.size,
                    Some(simple_child),
                    child as *mut c_void,
                    null_mut(),
                    null_mut(),
                    5,
                    0,
                    FOREVER);
            }
        })
    }
}

const FOREVER: k_timeout_t = k_timeout_t { ticks: !0 };

unsafe extern "C" fn simple_child(
    arg: *mut c_void,
    _p2: *mut c_void,
    _p3: *mut c_void,
) {
    let child: fn() -> () = core::mem::transmute(arg);
    (child)();
}
