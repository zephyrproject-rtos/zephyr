/*
 * Copyright (c) 2018 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syscall_handler.h>
#include <led.h>

_SYSCALL_HANDLER(led_blink, dev, led, delay_on, delay_off)
{
	_SYSCALL_DRIVER_LED(dev, blink);
	return _impl_led_blink((struct device *)dev, led, delay_on,
					delay_off);
}

_SYSCALL_HANDLER(led_set_brightness, dev, led, value)
{
	_SYSCALL_DRIVER_LED(dev, set_brightness);
	return _impl_led_set_brightness((struct device *)dev, led, value);
}

_SYSCALL_HANDLER(led_on, dev, led)
{
	_SYSCALL_DRIVER_LED(dev, on);
	return _impl_led_on((struct device *)dev, led);
}

_SYSCALL_HANDLER(led_off, dev, led)
{
	_SYSCALL_DRIVER_LED(dev, off);
	return _impl_led_off((struct device *)dev, led);
}
