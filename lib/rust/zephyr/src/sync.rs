//! Higher level synchronization primitives.
//!
//! These are modeled after the synchonization primitives in `std::sync` in as much as that makes
//! sense.

use core::{cell::UnsafeCell, marker::PhantomData, ops::{Deref, DerefMut}};

use crate::sys::{sync as sys, K_FOREVER};

/// Until poisoning is implemented, mutexes never return an error, and we just get back the guard.
pub type LockResult<Guard> = Result<Guard, ()>;

/// A mutual exclusion primitive useful for protecting shared data.
///
/// At this point, we don't implement poisoning, which will be done after panic is supported.
pub struct Mutex<T: ?Sized> {
    inner: sys::Mutex,
    // poison: ...
    data: UnsafeCell<T>,
}

// At least if correctly done, the Mutex provides for Send and Sync as long as the inner data
// supports Send.
unsafe impl<T: ?Sized + Send> Send for Mutex<T> {}
unsafe impl<T: ?Sized + Send> Sync for Mutex<T> {}

/// An RAII implementation of a "scoped lock" of a mutex.  When this structure is dropped (faslls
/// out of scope), the lock will be unlocked.
///
/// The data protected by the mutex can be accessed through this guard via its [`Deref`] and
/// [`DerefMut`] implementations.
///
/// This structure is created by the [`lock`] and [`try_lock`] methods on [`Mutex`].
pub struct MutexGuard<'a, T: ?Sized + 'a> {
    lock: &'a Mutex<T>,
    // until <https://github.com/rust-lang/rust/issues/68318> is implemented, we have to mark unsend
    // explicitly.  This can be done by holding Phantom data with an unsafe cell in it.
    _nosend: PhantomData<UnsafeCell<()>>,
}

// Make sure the guard doesn't get sent.
// Negative trait bounds are unstable, see marker above.
// impl<T: ?Sized> !Send for MutexGuard<'_, T> {}
unsafe impl<T: ?Sized + Sync> Sync for MutexGuard<'_, T> {}

impl<T> Mutex<T> {
    /// Construct a new wrapped Mutex, using the given underlying sys mutex.  This is different that
    /// `std::sync::Mutex` in that in Zephyr, objects are frequently allocated statically, and the
    /// sys Mutex will be taken by this structure.  It is safe to share the underlying Mutex between
    /// different items, but without careful use, it is easy to deadlock, so it is not recommended.
    pub const fn new(t: T, raw_mutex: sys::Mutex) -> Mutex<T> {
        Mutex { inner: raw_mutex, data: UnsafeCell::new(t) }
    }
}

impl<T: ?Sized> Mutex<T> {
    /// Acquires a mutex, blocking the current thread until it is able to do so.
    /// ...
    pub fn lock(&self) -> LockResult<MutexGuard<'_, T>> {
        self.inner.lock(K_FOREVER);
        unsafe {
            MutexGuard::new(self)
        }
    }
}

impl<'mutex, T: ?Sized> MutexGuard<'mutex, T> {
    unsafe fn new(lock: &'mutex Mutex<T>) -> LockResult<MutexGuard<'mutex, T>> {
        // poison todo
        Ok(MutexGuard { lock, _nosend: PhantomData })
    }
}

impl<T: ?Sized> Deref for MutexGuard<'_, T> {
    type Target = T;

    fn deref(&self) -> &T {
        unsafe {
            &*self.lock.data.get()
        }
    }
}

impl<T: ?Sized> DerefMut for MutexGuard<'_, T> {
    fn deref_mut(&mut self) -> &mut T {
        unsafe { &mut *self.lock.data.get() }
    }
}

impl<T: ?Sized> Drop for MutexGuard<'_, T> {
    #[inline]
    fn drop(&mut self) {
        self.lock.inner.unlock();
    }
}
