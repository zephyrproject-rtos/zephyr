/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NPCX_SOC_GPIO_H_
#define _NUVOTON_NPCX_SOC_GPIO_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Pin number for each GPIO device */
#define NPCX_GPIO_PORT_PIN_NUM 8U

/**
 * @brief Get GPIO device instance by port index
 *
 * @param port GPIO device index
 *
 * @retval Pointer to structure device
 * @retval NULL Invalid parameter of GPIO port index
 */
const struct device *npcx_get_gpio_dev(int port);

/**
 * @brief Enable the connection between io pads and GPIO instance
 *
 * @param dev Pointer to device structure for the gpio driver instance.
 * @param pin Pin number.
 */
void npcx_gpio_enable_io_pads(const struct device *dev, int pin);

/**
 * @brief Disable the connection between io pads and GPIO instance
 *
 * @param dev Pointer to device structure for the gpio driver instance.
 * @param pin Pin number.
 */
void npcx_gpio_disable_io_pads(const struct device *dev, int pin);

#ifdef __cplusplus
}
#endif

#endif /* _NUVOTON_NPCX_SOC_GPIO_H_ */
