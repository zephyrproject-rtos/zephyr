use zephyr_sys::{gpio_dt_spec, gpio_pin_toggle_dt, k_msleep};

extern "C" {
    static led: gpio_dt_spec;
}

pub fn toggle() {
    unsafe {
        gpio_pin_toggle_dt(&led);
    }
}

pub fn sleep() {
    unsafe {
        k_msleep(1000);
    }
}

