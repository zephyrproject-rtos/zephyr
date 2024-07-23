//! Simple sys bindings for Zephyr.

use core::ffi::{c_char, c_int, c_void};

// At this point, we only support the non-userspace setup, where the syscalls are just function
// calls.
// #[cfg(CONFIG_USERSPACE)]
// compile_error!("CONFIG_USERSPACE not yet supported in Rust");

// Ultimately, this will be better, but this is enough to get things started.

/// Since we never allocate one of these, and only refer to them by pointer, it isn't necessary to
/// have all of the fields.
#[allow(non_camel_case_types)]
#[repr(C)]
pub struct device {
    #[allow(dead_code)]
    name: *const c_char,
    #[allow(dead_code)]
    config: *const c_void,
    #[allow(dead_code)]
    api: *const c_void,
}

#[allow(non_camel_case_types)]
#[repr(C)]
pub struct gpio_dt_spec {
    /// GPIO device controlling the pin.
    pub port: *const device,
    /// The pin's number on the device.
    pub pin: gpio_pin_t,
    /// The pin's configuration flags as specified in devicetree.
    pub dt_flags: gpio_dt_flags_t,
}

#[allow(non_camel_case_types)]
#[repr(C)]
pub struct gpio_driver_api {
    pin_configure: extern "C" fn(port: *const device, pin: gpio_pin_t, flags: gpio_flags_t) -> c_int,
    #[cfg(CONFIG_GPIO_GET_CONFIG)]
    port_get_config: extern "C" fn(port: *const device, pin: gpio_pin_t, flags: *mut gpio_flags_t) -> c_int,
    port_get_raw: extern "C" fn(port: *const device, value: *mut gpio_port_value_t) -> c_int,
    port_set_masked_raw: extern "C" fn(port: *const device, mask: gpio_port_pins_t,
                                       value: gpio_port_value_t) -> c_int,
    port_set_bits_raw: extern "C" fn(port: *const device, pins: gpio_port_pins_t) -> c_int,
    port_clear_bits_raw: extern "C" fn(port: *const device, pins: gpio_port_pins_t) -> c_int,
    port_toggle_bits: extern "C" fn(port: *const device, pins: gpio_port_pins_t) -> c_int,
}

#[allow(non_camel_case_types)]
pub type gpio_dt_flags_t = u16;

#[allow(non_camel_case_types)]
pub type gpio_port_value_t = u32;

#[allow(non_camel_case_types)]
pub type gpio_port_pins_t = u32;

#[allow(non_camel_case_types)]
pub type gpio_flags_t = u32;

#[allow(non_camel_case_types)]
pub type gpio_pin_t = u8;

// These constants are untyped in C. Since they are bigger than 16-bits, I will assume they are
// actually part of the `gpio_dt_flags_t` type.  For now, we'll just make these constants, but bit
// flags might be more useful for this (note that this is a dependency that would be brought into
// Zephyr code).
pub const GPIO_ACTIVE_LOW: gpio_flags_t = 1 << 1;
pub const GPIO_ACTIVE_HIGH: gpio_flags_t = 0 << 1;
pub const GPIO_INPUT: gpio_flags_t = 1 << 16;
pub const GPIO_OUTPUT: gpio_flags_t = 1 << 17;
pub const GPIO_DISCONNECTED: gpio_flags_t = 0;
pub const GPIO_OUTPUT_INIT_LOW: gpio_flags_t = 1 << 18;
pub const GPIO_OUTPUT_INIT_HIGH: gpio_flags_t = 1 << 19;
pub const GPIO_OUTPUT_INIT_LOGICAL: gpio_flags_t = 1 << 20;

pub const GPIO_OUTPUT_LOW: gpio_flags_t = GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW;
pub const GPIO_OUTPUT_HIGH: gpio_flags_t = GPIO_OUTPUT | GPIO_OUTPUT_INIT_HIGH;
pub const GPIO_OUTPUT_INACTIVE: gpio_flags_t = GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW | GPIO_OUTPUT_INIT_LOGICAL;
pub const GPIO_OUTPUT_ACTIVE: gpio_flags_t = GPIO_OUTPUT | GPIO_OUTPUT_INIT_HIGH | GPIO_OUTPUT_INIT_LOGICAL;

impl gpio_dt_spec {
    pub fn is_ready(&self) -> bool {
        unsafe { z_impl_device_is_ready(self.port) != 0 }
    }
    pub fn configure(&mut self, extra_flags: gpio_flags_t) {
        unsafe {
            z_impl_gpio_pin_configure(self.port, self.pin,
                                      self.dt_flags as gpio_flags_t | extra_flags);
        }
    }
    pub fn toggle_pin(&mut self) {
        unsafe {
            gpio_pin_toggle(self.port, self.pin);
            // TODO error handling.
        }
    }
}

extern "C" {
    fn z_impl_device_is_ready(port: *const device) -> c_bool;
    /*
    fn z_impl_gpio_pin_configure(port: *const device,
                                 pin: gpio_pin_t,
                                 flags: gpio_flags_t) -> c_int;
    */
}

unsafe fn z_impl_gpio_pin_configure(port: *const device,
                             pin: gpio_pin_t,
                             flags: gpio_flags_t) -> c_int
{
    let rport = port.as_ref().unwrap();
    let api = (rport.api as *const gpio_driver_api).as_ref().unwrap();
    // TODO, bring in the asserts, and checks.

    let mut flags = flags;
    if (flags & GPIO_OUTPUT_INIT_LOGICAL != 0)
        && (flags & (GPIO_OUTPUT_INIT_LOW | GPIO_OUTPUT_INIT_HIGH) != 0)
        && (flags & GPIO_ACTIVE_LOW != 0)
    {
        flags ^= GPIO_OUTPUT_INIT_LOW | GPIO_OUTPUT_INIT_HIGH;
    }
    flags &= !GPIO_OUTPUT_INIT_LOGICAL;
    // TODO: Pin check.
    (api.pin_configure)(port, pin, flags)
}

unsafe fn gpio_pin_toggle(port: *const device,
                          pin: gpio_pin_t) -> c_int
{
    // TODO: Pin validity check.
    z_impl_gpio_port_toggle_bits(port, (1 << pin) as gpio_port_pins_t)
}

unsafe fn z_impl_gpio_port_toggle_bits(port: *const device,
                                       pins: gpio_port_pins_t) -> c_int
{
    let rport = port.as_ref().unwrap();
    let api = (rport.api as *const gpio_driver_api).as_ref().unwrap();

    (api.port_toggle_bits)(port, pins)
}

// It isn't completely clear what this type should be, and if this is portable or not.
#[allow(non_camel_case_types)]
type c_bool = u8;

// The time macros are somewhat messy.  This is a simplistic implementation that works fine for
// constants, but might not be as efficient when not constants.
pub const Z_HZ_TICKS: u32 = crate::kconfig::CONFIG_SYS_CLOCK_TICKS_PER_SEC as u32;

#[cfg(CONFIG_TIMEOUT_64BIT)]
#[allow(non_camel_case_types)]
pub type k_ticks_t = i64;

#[cfg(not(CONFIG_TIMEOUT_64BIT))]
#[allow(non_camel_case_types)]
pub type k_ticks_t = u32;

pub const K_TICKS_FOREVER: k_ticks_t = !0;

#[allow(non_camel_case_types)]
#[repr(C)]
pub struct k_timeout_t {
    #[allow(dead_code)]
    pub ticks: k_ticks_t,
}

/// Convert the specified time into ticks.  This is simplified over the optimized implementation in
/// macros.
pub fn z_timeout_ms(ticks: i32) -> k_timeout_t {
    let ticks = ticks as k_ticks_t;
    let to_hz = Z_HZ_TICKS as k_ticks_t;
    let from_hz = 1000;

    let out_ticks = (ticks / from_hz) * to_hz +
     ((ticks % from_hz) * to_hz +
        (to_hz - 1)) / from_hz;
    k_timeout_t { ticks: out_ticks }
}

pub fn k_msleep(ms: i32) -> c_int {
    unsafe { z_impl_k_sleep(z_timeout_ms(ms)) }
}

extern "C" {
    fn z_impl_k_sleep(ms: k_timeout_t) -> c_int;
}
