/*
 * Copyright (c) 2015 Intel Corporation.
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

#include <drivers/uart.h>

/**
 * @brief UART initialization routine for platform drivers.
 *
 * This calls the config_func specified in the UART device config,
 * if it is specified.
 *
 * @param dev UART device struct
 *
 * @return DEV_OK if successful, failed otherwise.
 */
int uart_platform_init(struct device *dev)
{
	struct uart_device_config_t *dev_cfg =
	    (struct uart_device_config_t *)dev->config->config_info;

	if (dev_cfg->config_func) {
		return dev_cfg->config_func(dev);
	}

	return DEV_OK;
}
