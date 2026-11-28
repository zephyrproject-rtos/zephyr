/*
 * Copyright (c) 2025 Pedro Ramos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FAKE_METER_H_
#define FAKE_METER_H_

#include <zephyr/drivers/energy_meter.h>

/* A device-specific quantity of the fake meter: a raw status word on the total line */
#define FAKE_METER_STATUS (ENERGY_METER_QUANTITY_PRIV_START + 0)

#define FAKE_METER_STATUS_VALUE 0x1234

extern const struct device *const fake_meter;
extern const struct device *const fake_meter_minimal;
extern const struct device *const fake_meter_faulty;

/* Value the fake meter reports for a quantity on a line */
int64_t fake_meter_value(uint8_t quantity, uint8_t line);

/* Add energy to every energy quantity of the fake meter */
void fake_meter_add_energy(int64_t uwh);

/* Raise an event through the callback set on the fake meter */
void fake_meter_raise(uint8_t type, uint8_t index);

#endif /* FAKE_METER_H_ */
