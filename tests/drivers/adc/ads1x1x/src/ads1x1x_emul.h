/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTS_ADS1X1X_EMUL_H_
#define TESTS_ADS1X1X_EMUL_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/drivers/emul.h>

/** Inputs of the chip. */
#define ADS_EMUL_INPUTS 4

/** How many writes of the configuration register are remembered. */
#define ADS_EMUL_LOG_SIZE 32

/** State of a simulated ADS1x1x chip. */
struct ads1x1x_emul_data {
	/** Registers: conversion, configuration, low threshold, high threshold. */
	uint16_t reg[4];
	/** Voltage on every input, relative to GND, in microvolts. */
	int32_t input_uv[ADS_EMUL_INPUTS];
	/** The chip says "still converting" this many times after a conversion was started. */
	uint8_t busy_polls;
	uint8_t busy_left;
	/** Fail the n-th next transfer (1 is the next one) with fail_error. 0 is off. */
	int fail_in;
	int fail_error;
	/** Every value written to the configuration register, oldest first. */
	uint16_t config_log[ADS_EMUL_LOG_SIZE];
	size_t config_writes;
	/** Conversions that were started. */
	size_t conversions;
	/** I2C transfers the chip has seen. */
	size_t transfers;
	/** A conversion was started and the chip has not yet said that it is done. */
	bool converting;
	/** Writes of the configuration register that came while a conversion was running. */
	size_t writes_while_converting;
};

/** Put the chip in its power-up state, and forget the log and the failures. */
void ads1x1x_emul_reset(const struct emul *target);

#endif /* TESTS_ADS1X1X_EMUL_H_ */
