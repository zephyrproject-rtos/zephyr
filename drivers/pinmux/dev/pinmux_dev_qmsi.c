/* pinmux_dev_qmsi.c -  QMSI pinmux dev driver */

/*
 * Copyright (c) 2016 Intel Corporation
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
#include <nanokernel.h>
#include <device.h>
#include <init.h>
#include <pinmux.h>
#include <sys_io.h>
#include "pinmux/pinmux.h"

#include "qm_pinmux.h"

static int pinmux_dev_set(struct device *dev, uint32_t pin,
			       uint32_t func)
{
	ARG_UNUSED(dev);

	return qm_pmux_select(pin, func) == QM_RC_OK ? 0 : -EIO;
}

static int pinmux_dev_get(struct device *dev, uint32_t pin,
			       uint32_t *func)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(func);

	return -ENODEV;
}

static int pinmux_dev_pullup(struct device *dev, uint32_t pin,
				  uint8_t func)
{
	ARG_UNUSED(dev);

	return qm_pmux_pullup_en(pin, func) == QM_RC_OK ? 0 : -EIO;
}

static int pinmux_dev_input(struct device *dev, uint32_t pin,
				 uint8_t func)
{
	ARG_UNUSED(dev);

	return qm_pmux_input_en(pin, func) == QM_RC_OK ? 0 : -EIO;
}

static struct pinmux_driver_api api_funcs = {
	.set = pinmux_dev_set,
	.get = pinmux_dev_get,
	.pullup = pinmux_dev_pullup,
	.input = pinmux_dev_input
};

static int pinmux_dev_initialize(struct device *port)
{
	return 0;
}

DEVICE_AND_API_INIT(pmux_dev, CONFIG_PINMUX_DEV_NAME,
		    &pinmux_dev_initialize,
		    NULL, NULL,
		    SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &api_funcs);
