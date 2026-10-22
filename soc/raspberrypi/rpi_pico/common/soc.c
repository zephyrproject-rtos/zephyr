/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2021 Yonatan Schachter
 * Copyright (c) 2024 Andrew Featherstone
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for Raspberry Pi RP2xxx family of processors
 *
 * This module provides routines to initialize and support board-level hardware
 * for the Raspberry Pi RP2xxx family of processors (RP2040, RP235x).
 */

#include <stdio.h>

#include <zephyr/fatal.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

/*
 * Some pico-sdk drivers call panic on fatal error.
 * This alternative implementation of panic handles the panic
 * through Zephyr.
 */
void __attribute__((noreturn)) panic(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vprintf(fmt, args);
	k_fatal_halt(K_ERR_CPU_EXCEPTION);
}
