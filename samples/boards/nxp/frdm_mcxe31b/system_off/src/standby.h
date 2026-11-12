/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MCXE_SYSTEM_OFF_STANDBY_H_
#define MCXE_SYSTEM_OFF_STANDBY_H_

#include <stdint.h>

#define STANDBY_MAGIC 0x53544259U
#define CPU1_READY 1U
#define CPU1_STOP 2U
#define CPU1_PARKED 3U
#define CPU1_ERROR 4U

struct standby_data {
	uint32_t magic;
	uint32_t cycles;
	uint32_t check;
	volatile uint32_t request;
	volatile uint32_t state;
};

#endif
