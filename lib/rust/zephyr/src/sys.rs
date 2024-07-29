//! Simple sys bindings for Zephyr.

pub mod gpio {
    //! Higher level bindings to the GPIO interface.

    use crate::raw;
    use raw::{device, gpio_dt_spec};

    use zephyr_sys::gpio_flags_t;
    // Re-export the gpio pin defines.
    pub use zephyr_sys::{
        GPIO_OUTPUT_ACTIVE,
    };

    /// Safe wrapper around the GPIO device itself.  This represents the entire GPIO controller,
    /// rather than an individual pin.
    pub struct Gpio {
        /// The underlying device.
        pub(crate) device: *const device,
    }

    impl Gpio {
        pub fn is_ready(&self) -> bool {
            unsafe {
                raw::device_is_ready(self.device)
            }
        }
    }

    /// Safe Wrapper around a single pin.
    #[allow(dead_code)]
    pub struct GpioPin {
        /// This just wraps the spec from the DT:
        pub(crate) pin: gpio_dt_spec,
    }

    impl GpioPin {
        pub fn is_ready(&self) -> bool {
            self.get_gpio().is_ready()
        }

        pub fn configure(&mut self, extra_flags: gpio_flags_t) {
            // TODO: Error?
            unsafe {
                raw::gpio_pin_configure(self.pin.port,
                                        self.pin.pin,
                                        self.pin.dt_flags as gpio_flags_t | extra_flags);
            }
        }

        pub fn toggle_pin(&mut self) {
            // TODO: Error?
            unsafe {
                raw::gpio_pin_toggle_dt(&self.pin);
            }
        }

        /// Get the underlying Gpio device.
        pub fn get_gpio(&self) -> Gpio {
            Gpio {
                device: self.pin.port,
            }
        }
    }
}

/// Zephyr implementation of critical sections.
pub mod critical {
    use core::{ffi::c_int, ptr::addr_of_mut};

    use critical_section::RawRestoreState;
    use zephyr_sys::{k_spinlock, k_spin_lock, k_spin_unlock, k_spinlock_key_t};

    struct ZephyrCriticalSection;
    critical_section::set_impl!(ZephyrCriticalSection);

    // The critical section shares a single spinlock.
    static mut LOCK: k_spinlock = unsafe { core::mem::zeroed() };

    unsafe impl critical_section::Impl for ZephyrCriticalSection {
        unsafe fn acquire() -> RawRestoreState {
            let res = k_spin_lock(addr_of_mut!(LOCK));
            res.key as RawRestoreState
        }

        unsafe fn release(token: RawRestoreState) {
            k_spin_unlock(addr_of_mut!(LOCK),
                          k_spinlock_key_t {
                              key: token as c_int,
                          });
        }
    }
}
