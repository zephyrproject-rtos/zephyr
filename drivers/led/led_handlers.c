/*
 * Copyright (c) 2018 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syscall_handler.h>
#include <led.h>

Z_SYSCALL_HANDLER(led_blink, dev, led, delay_on, delay_off)
{
	Z_OOPS(Z_SYSCALL_DRIVER_LED(dev, blink));
	return _impl_led_blink((struct device *)dev, led, delay_on,
					delay_off);
}

Z_SYSCALL_HANDLER(led_set_brightness, dev, led, value)
{
	Z_OOPS(Z_SYSCALL_DRIVER_LED(dev, set_brightness));
	return _impl_led_set_brightness((struct device *)dev, led, value);
}

Z_SYSCALL_HANDLER(led_set_color, dev, r, g, b)
{
	_SYSCALL_DRIVER_LED(dev, set_color);
	return _impl_led_set_color((struct device *)dev, r, g, b);
}

Z_SYSCALL_HANDLER(led_fade_brightness, dev, led, start, stop, fade_time)
{
	_SYSCALL_DRIVER_LED(dev, fade_brightness);
	return _impl_led_fade_brightness((struct device *)dev, led,
					start, stop, fade_time);
}

Z_SYSCALL_HANDLER(led_on, dev, led)
{
	Z_OOPS(Z_SYSCALL_DRIVER_LED(dev, on));
	return _impl_led_on((struct device *)dev, led);
}

Z_SYSCALL_HANDLER(led_off, dev, led)
{
	Z_OOPS(Z_SYSCALL_DRIVER_LED(dev, off));
	return _impl_led_off((struct device *)dev, led);
}
