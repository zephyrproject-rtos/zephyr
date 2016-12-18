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
#include <fsl_common.h>
#include <fsl_clock.h>

struct pinmux_ksdk_config {
	clock_ip_name_t clock_ip_name;
	PORT_Type *base;
};

static int pinmux_ksdk_set(struct device *dev, uint32_t pin, uint32_t func)
{
	const struct pinmux_ksdk_config *config = dev->config->config_info;
	PORT_Type *base = config->base;

	base->PCR[pin] = func;

	return 0;
}

static int pinmux_ksdk_get(struct device *dev, uint32_t pin, uint32_t *func)
{
	const struct pinmux_ksdk_config *config = dev->config->config_info;
	PORT_Type *base = config->base;

	*func = base->PCR[pin];

	return 0;
}

static int pinmux_ksdk_pullup(struct device *dev, uint32_t pin, uint8_t func)
{
	return -ENOTSUP;
}

static int pinmux_ksdk_input(struct device *dev, uint32_t pin, uint8_t func)
{
	return -ENOTSUP;
}

static int pinmux_ksdk_init(struct device *dev)
{
	const struct pinmux_ksdk_config *config = dev->config->config_info;

	CLOCK_EnableClock(config->clock_ip_name);

	return 0;
}

static const struct pinmux_driver_api pinmux_ksdk_driver_api = {
	.set = pinmux_ksdk_set,
	.get = pinmux_ksdk_get,
	.pullup = pinmux_ksdk_pullup,
	.input = pinmux_ksdk_input,
};

#ifdef CONFIG_PINMUX_KSDK_PORTA
static const struct pinmux_ksdk_config pinmux_ksdk_porta_config = {
	.base = PORTA,
	.clock_ip_name = kCLOCK_PortA,
};

DEVICE_AND_API_INIT(pinmux_porta, CONFIG_PINMUX_KSDK_PORTA_NAME,
		    &pinmux_ksdk_init,
		    NULL, &pinmux_ksdk_porta_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &pinmux_ksdk_driver_api);
#endif

#ifdef CONFIG_PINMUX_KSDK_PORTB
static const struct pinmux_ksdk_config pinmux_ksdk_portb_config = {
	.base = PORTB,
	.clock_ip_name = kCLOCK_PortB,
};

DEVICE_AND_API_INIT(pinmux_portb, CONFIG_PINMUX_KSDK_PORTB_NAME,
		    &pinmux_ksdk_init,
		    NULL, &pinmux_ksdk_portb_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &pinmux_ksdk_driver_api);
#endif

#ifdef CONFIG_PINMUX_KSDK_PORTC
static const struct pinmux_ksdk_config pinmux_ksdk_portc_config = {
	.base = PORTC,
	.clock_ip_name = kCLOCK_PortC,
};

DEVICE_AND_API_INIT(pinmux_portc, CONFIG_PINMUX_KSDK_PORTC_NAME,
		    &pinmux_ksdk_init,
		    NULL, &pinmux_ksdk_portc_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &pinmux_ksdk_driver_api);
#endif

#ifdef CONFIG_PINMUX_KSDK_PORTD
static const struct pinmux_ksdk_config pinmux_ksdk_portd_config = {
	.base = PORTD,
	.clock_ip_name = kCLOCK_PortD,
};

DEVICE_AND_API_INIT(pinmux_portd, CONFIG_PINMUX_KSDK_PORTD_NAME,
		    &pinmux_ksdk_init,
		    NULL, &pinmux_ksdk_portd_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &pinmux_ksdk_driver_api);
#endif

#ifdef CONFIG_PINMUX_KSDK_PORTE
static const struct pinmux_ksdk_config pinmux_ksdk_porte_config = {
	.base = PORTE,
	.clock_ip_name = kCLOCK_PortE,
};

DEVICE_AND_API_INIT(pinmux_porte, CONFIG_PINMUX_KSDK_PORTE_NAME,
		    &pinmux_ksdk_init,
		    NULL, &pinmux_ksdk_porte_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &pinmux_ksdk_driver_api);
#endif
