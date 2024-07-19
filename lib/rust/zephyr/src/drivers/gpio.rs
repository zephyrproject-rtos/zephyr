use bitmask_enum::bitmask;
use crate::errno::{Errno, parse_result};

extern "C" {
    static led: zephyr_sys::gpio_dt_spec;
}

#[bitmask(u32)]
pub enum Flags {
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

pub struct Pin {
    gpio_dt_spec: zephyr_sys::gpio_dt_spec,
}

impl Pin {
    pub fn get_led() -> Pin {
        unsafe {
            return Pin {
                gpio_dt_spec: led.clone(),
            }
        }
    }

    pub fn configure(&self, extra_flags: Flags) -> Result<(), Errno>{
        unsafe {
            parse_result(zephyr_sys::gpio_pin_configure_dt(&self.gpio_dt_spec, extra_flags.bits()))
        }
    }

    pub fn toggle(&self) -> Result<(), Errno> {
        unsafe {
            parse_result(zephyr_sys::gpio_pin_toggle_dt(&self.gpio_dt_spec))
        }
    }
}
