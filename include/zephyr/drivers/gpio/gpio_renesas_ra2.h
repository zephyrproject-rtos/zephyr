/*
 * Copyright (c) 2023-2024 MUNIC SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GPIO_RA2_H_
#define ZEPHYR_DRIVERS_GPIO_RA2_H_

int gpio_ra_activate_wakeup(const struct device *port, gpio_pin_t pin,
		irq_ra_sense_t sense, nmi_irq_ra_division_t div, int filtered);
int gpio_ra_deactivate_wakeup(const struct device *port, gpio_pin_t pin);

static inline int gpio_ra_activate_wakeup_dt(const struct gpio_dt_spec *spec,
		irq_ra_sense_t sense, nmi_irq_ra_division_t div, int filtered)
{
	return gpio_ra_activate_wakeup(spec->port, spec->pin, sense, div,
			filtered);
}

static inline int gpio_ra_deactivate_wakeup_dt(const struct gpio_dt_spec *spec)
{
	return gpio_ra_deactivate_wakeup(spec->port, spec->pin);
}

#endif
