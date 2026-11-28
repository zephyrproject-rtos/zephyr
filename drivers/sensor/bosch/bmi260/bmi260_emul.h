/*
 * Copyright (c) 2026 Shreyas Acharya
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BOSCH_BMI260_BMI260_EMUL_H_
#define ZEPHYR_DRIVERS_SENSOR_BOSCH_BMI260_BMI260_EMUL_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/drivers/emul.h>

/*
 * Places a value in the register file of the emulated device, so that a test
 * can hand the driver a reading and check what it makes of it.
 */
void bmi260_emul_set_reg(const struct emul *target, uint8_t reg, const uint8_t *val, size_t len);

/* Reads back what the driver left in the register file. */
void bmi260_emul_get_reg(const struct emul *target, uint8_t reg, uint8_t *val, size_t len);

/* How many soft reset commands the device has been given. */
uint32_t bmi260_emul_get_soft_reset_count(const struct emul *target);

/*
 * How many times the chip identifier has been read since the last soft reset.
 * On SPI a throwaway read is what moves the device off I2C, so this is how a
 * test sees whether the driver re-latched the interface after resetting it.
 */
uint32_t bmi260_emul_get_chip_id_reads_since_reset(const struct emul *target);

/*
 * How many times a register has been written. Lets a test tell a register the
 * driver deliberately set apart from one that merely holds its reset value.
 */
uint16_t bmi260_emul_get_write_count(const struct emul *target, uint8_t reg);

/* How many bytes have reached the configuration write port since the reset. */
uint32_t bmi260_emul_get_init_data_byte_count(const struct emul *target);

/* The word address of the most recent configuration chunk. */
uint16_t bmi260_emul_get_last_init_addr(const struct emul *target);

/*
 * Withholds the report that the configuration took, so a test can see what the
 * driver does when the device never comes up.
 */
void bmi260_emul_set_fail_config_load(const struct emul *target, bool fail);

/*
 * Fails the given write, counted in register writes from now, so a test can
 * interrupt a transfer partway through. The writes before it succeed, and so
 * does everything after: one failure is enough to see an abort, and leaving
 * the bus broken would only obscure what the driver did next.
 */
void bmi260_emul_fail_write_after(const struct emul *target, uint32_t writes);

#endif /* ZEPHYR_DRIVERS_SENSOR_BOSCH_BMI260_BMI260_EMUL_H_ */
