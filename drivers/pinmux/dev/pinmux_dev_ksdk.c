/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <device.h>
#include <pinmux.h>
#include <pinmux/pinmux_ksdk.h>

struct pinmux_dev_ksdk_config {
	PORT_Type *base;
};

static int pinmux_dev_ksdk_set(struct device *dev, uint32_t pin, uint32_t func)
{
	const struct pinmux_dev_ksdk_config *config = dev->config->config_info;

	return pinmux_ksdk_set(config->base, pin, func);
}

static int pinmux_dev_ksdk_get(struct device *dev, uint32_t pin, uint32_t *func)
{
	const struct pinmux_dev_ksdk_config *config = dev->config->config_info;

	return pinmux_ksdk_get(config->base, pin, func);
}

static int pinmux_dev_ksdk_pullup(struct device *dev, uint32_t pin,
				  uint8_t func)
{
	return -ENOTSUP;
}

static int pinmux_dev_ksdk_input(struct device *dev, uint32_t pin,
				 uint8_t func)
{
	return -ENOTSUP;
}

static const struct pinmux_driver_api pinmux_dev_ksdk_driver_api = {
	.set = pinmux_dev_ksdk_set,
	.get = pinmux_dev_ksdk_get,
	.pullup = pinmux_dev_ksdk_pullup,
	.input = pinmux_dev_ksdk_input,
};

static int pinmux_dev_ksdk_init(struct device *dev)
{
	return 0;
}

#ifdef CONFIG_PINMUX_KSDK_PORTA
static const struct pinmux_dev_ksdk_config pinmux_dev_ksdk_porta_config = {
	.base = PORTA,
};

DEVICE_AND_API_INIT(pinmux_dev_porta, CONFIG_PINMUX_DEV_KSDK_PORTA_NAME,
		    &pinmux_dev_ksdk_init,
		    NULL, &pinmux_dev_ksdk_porta_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &pinmux_dev_ksdk_driver_api);
#endif

#ifdef CONFIG_PINMUX_KSDK_PORTB
static const struct pinmux_dev_ksdk_config pinmux_dev_ksdk_portb_config = {
	.base = PORTB,
};

DEVICE_AND_API_INIT(pinmux_dev_portb, CONFIG_PINMUX_DEV_KSDK_PORTB_NAME,
		    &pinmux_dev_ksdk_init,
		    NULL, &pinmux_dev_ksdk_portb_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &pinmux_dev_ksdk_driver_api);
#endif

#ifdef CONFIG_PINMUX_KSDK_PORTC
static const struct pinmux_dev_ksdk_config pinmux_dev_ksdk_portc_config = {
	.base = PORTC,
};

DEVICE_AND_API_INIT(pinmux_dev_portc, CONFIG_PINMUX_DEV_KSDK_PORTC_NAME,
		    &pinmux_dev_ksdk_init,
		    NULL, &pinmux_dev_ksdk_portc_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &pinmux_dev_ksdk_driver_api);
#endif

#ifdef CONFIG_PINMUX_KSDK_PORTD
static const struct pinmux_dev_ksdk_config pinmux_dev_ksdk_portd_config = {
	.base = PORTD,
};

DEVICE_AND_API_INIT(pinmux_dev_portd, CONFIG_PINMUX_DEV_KSDK_PORTD_NAME,
		    &pinmux_dev_ksdk_init,
		    NULL, &pinmux_dev_ksdk_portd_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &pinmux_dev_ksdk_driver_api);
#endif

#ifdef CONFIG_PINMUX_KSDK_PORTE
static const struct pinmux_dev_ksdk_config pinmux_dev_ksdk_porte_config = {
	.base = PORTE,
};

DEVICE_AND_API_INIT(pinmux_dev_porte, CONFIG_PINMUX_DEV_KSDK_PORTE_NAME,
		    &pinmux_dev_ksdk_init,
		    NULL, &pinmux_dev_ksdk_porte_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &pinmux_dev_ksdk_driver_api);
#endif
