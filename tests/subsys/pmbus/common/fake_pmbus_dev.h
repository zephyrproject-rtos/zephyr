/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FAKE_PMBUS_DEV_H_
#define FAKE_PMBUS_DEV_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/atomic.h>

#define FAKE_PMBUS_DEV_PAGES    2
#define FAKE_PMBUS_DEV_BUF_SIZE 8

struct fake_pmbus_dev {
	struct i2c_target_config cfg;
	uint8_t pages;
	uint8_t vout_mode;

	uint8_t page;
	uint8_t cml;
	uint16_t regs[FAKE_PMBUS_DEV_PAGES][256];
	/* Unsupported commands NACKed on the command byte */
	ATOMIC_DEFINE(nack, 256);
	/* Unsupported commands only reported in STATUS_CML */
	ATOMIC_DEFINE(cml_only, 256);
	bool absent;
	bool pec;
	int page_writes;
	int clear_faults;
	uint8_t clear_faults_page;

	uint8_t buf[FAKE_PMBUS_DEV_BUF_SIZE];
	uint8_t len;
	bool reading;
	uint8_t rd[2];
	uint8_t rd_len;
	uint8_t rd_pos;
	uint8_t crc;
};

int fake_pmbus_dev_register(struct fake_pmbus_dev *fake, const struct device *bus, uint16_t addr,
			    uint8_t pages, uint8_t vout_mode);

void fake_pmbus_dev_reset(struct fake_pmbus_dev *fake);

#endif /* FAKE_PMBUS_DEV_H_ */
