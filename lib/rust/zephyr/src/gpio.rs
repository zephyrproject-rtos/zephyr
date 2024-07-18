use zephyr_sys::{gpio_dt_spec, gpio_pin_toggle_dt};

extern "C" {
    static led: gpio_dt_spec;
}

extern "C" {
    fn sample_sleep();
}

pub fn toggle() {
    unsafe {
        gpio_pin_toggle_dt(&led);
    }
}

pub fn sleep() {
    unsafe {
        sample_sleep();
    }
}

