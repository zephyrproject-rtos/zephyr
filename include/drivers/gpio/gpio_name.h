/*
 * Copyright (c) 2022 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Map GPIO name from gpio-line-names to/from GPIO spec.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_NAME_H_
#define ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_NAME_H_

#include <zephyr/types.h>
#include <drivers/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief GPIO name API
 * @defgroup gpio_name GPIO name
 * @ingroup gpio_interface
 * @{
 *
 * Maps GPIO names as defined in the gpio-line-names DTS
 * configuration of GPIO controllers to/from a GPIO controller
 * device and pin number.
 *
 * - Given a string, return the GPIO controller device and pin number
 *   that corresponds to the matching string in gpio-line-names
 *   for that GPIO controller.
 * - Given a GPIO controller device pointer and a pin number,
 *   return the corresponding GPIO name for this pin (as found
 *   in the gpio-line-names device tree for that controller.
 *
 * The GPIO names are typically placed in a DTS overlay thus:
 *
 *  &gpioa {
 *      gpio-line-names =
 *        "turn-on-light",
 *        "",
 *        "check-level",
 *        "user-button";
 *  };
 *
 * To extract and store the GPIO names from the gpio-line-names
 * property on each GPIO controller, the controller's device
 * tree entries are scanned. To do this, the parent device tree
 * node that contains the GPIO controllers must be known.
 * By default, the 'soc' node is assumed to be the parent node,
 * but some boards use a different node, in which case
 * a device tree alias 'gpio-controller-parent' can be used
 * to define the parent node of the GPIO controller.
 *
 * Example devicetree fragment:
 *
 *      aliases {
 *          gpio-controller-parent = &pinctrl;
 *      };
 *
 * If the GPIO shell is enabled, the names can be used to
 * select the GPIOs for conf/get/set etc.
 */

/**
 * @brief Get the name associated with this GPIO port and pin.
 *
 * @param port The GPIO port
 * @param pin  The pin number of the GPIO
 *
 * @return GPIO name corresponding to port and pin.
 * @return NULL No name is associated with this port and pin.
 */
const char *gpio_pin_get_name(const struct device *port, gpio_pin_t pin);

/**
 * @brief Get the GPIO port and pin associated with this name.
 *
 * @param name The name used to match against the gpio-line-names of the port.
 * @param port A pointer where the port will be stored, NULL if none found.
 * @param pin A pointer where the pin will be stored, 0 if none found.
 *
 * @return 0 on success
 * @return -ENOENT if the name is not found.
 */
int gpio_pin_by_name(const char *name, const struct device **port,
		     gpio_pin_t *pin);

/**
 * @brief Get the name associated with a GPIO spec.
 *
 * @param spec Pointer to the GPIO spec.
 *
 * @return Pointer to name associated with GPIO.
 * @return NULL No name was found for this GPIO.
 */
static inline const char *
gpio_pin_get_name_dt(const struct gpio_dt_spec *spec)
{
	return gpio_pin_get_name(spec->port, spec->pin);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_NAME_H_ */
