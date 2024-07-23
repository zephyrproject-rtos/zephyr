// Copyright (c) 2024 Zühlke Engineering AG
// SPDX-License-Identifier: Apache-2.0

use bitmask_enum::bitmask;
use crate::errno::Errno;

// Maybe this should be somehow wrapped to not expose zephyr_sys stuff to the application.
pub type GpioDtSpec = zephyr_sys::gpio_dt_spec;

#[bitmask(u32)]
pub enum GpioFlags {
    None = 0,
    Input = zephyr_sys::GPIO_INPUT,
    Output = zephyr_sys::GPIO_OUTPUT,
    Disconnected = zephyr_sys::GPIO_DISCONNECTED,
    InitLow = zephyr_sys::GPIO_OUTPUT_INIT_LOW,
    InitHigh = zephyr_sys::GPIO_OUTPUT_INIT_HIGH,
    InitLogical = zephyr_sys::GPIO_OUTPUT_INIT_LOGICAL,
    OutputLow = zephyr_sys::GPIO_OUTPUT | zephyr_sys::GPIO_OUTPUT_INIT_LOW,
    OutputHigh = zephyr_sys::GPIO_OUTPUT | zephyr_sys::GPIO_OUTPUT_INIT_HIGH,
    OutputInactive = zephyr_sys::GPIO_OUTPUT | zephyr_sys::GPIO_OUTPUT_INIT_LOW | zephyr_sys::GPIO_OUTPUT_INIT_LOGICAL,
    OutputActive = zephyr_sys::GPIO_OUTPUT | zephyr_sys::GPIO_OUTPUT_INIT_HIGH | zephyr_sys::GPIO_OUTPUT_INIT_LOGICAL,
}

pub struct GpioPin {
    gpio_dt_spec: GpioDtSpec,
}

impl GpioPin {
    pub fn new(gpio_dt_spec: GpioDtSpec) -> Self {
        GpioPin { gpio_dt_spec }
    }

    pub fn configure(&self, extra_flags: GpioFlags) -> Result<(), Errno> {
        unsafe {
            Errno::from(zephyr_sys::gpio_pin_configure_dt(&self.gpio_dt_spec, extra_flags.bits()))
        }
    }

    pub fn toggle(&self) -> Result<(), Errno> {
        unsafe {
            Errno::from(zephyr_sys::gpio_pin_toggle_dt(&self.gpio_dt_spec))
        }
    }
}

#[macro_export]
macro_rules! dt_gpio_ctrl_by_idx {
    ($node:expr, $prop:ident, $idx:tt) => { $node.$prop.$idx.phandle }
}

#[macro_export]
macro_rules! dt_gpio_pin_by_idx {
    ($node:expr, $prop:ident, $idx:tt) => { $node.$prop.$idx.cell_pin };
}

#[macro_export]
macro_rules! dt_gpio_flags_by_idx {
    ($node:expr, $prop:ident, $idx:tt) => { $node.$prop.$idx.cell_flags };
}

#[macro_export]
macro_rules! gpio_dt_spec_get_by_idx {
    ($node:expr, $prop:ident, $idx:tt) => {
        crate::drivers::gpio::GpioDtSpec {
            port: device_dt_get!(dt_gpio_ctrl_by_idx!($node, $prop, $idx)),
            pin: dt_gpio_pin_by_idx!($node, $prop, $idx) as u8,
            dt_flags: dt_gpio_flags_by_idx!($node, $prop, $idx) as u16,
        }
    }
}

#[macro_export]
macro_rules! gpio_dt_spec_get {
    ($node:expr, $prop:ident) => { gpio_dt_spec_get_by_idx!($node, $prop, 0) }
}
