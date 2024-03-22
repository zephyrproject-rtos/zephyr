/*
 * Copyright (c) 2017 Oticon A/S
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file Extra definitions provided by the board to soc.h
 *
 * Background:
 * The POSIX ARCH/SOC/board layering is different than in normal archs
 * The "SOC" does not provide almost any of the typical SOC functionality
 * but that is left for the "board" to define it
 * Device code may rely on the soc.h defining some things (like the interrupts
 * numbers)
 * Therefore this file is included from the inf_clock soc.h to allow a board
 * to define that kind of SOC related snippets
 */

#ifndef BOARDS_POSIX_NATIVE_SIM_BOARD_SOC_H
#define BOARDS_POSIX_NATIVE_SIM_BOARD_SOC_H

#include "nsi_cpu0_interrupts.h"

#endif /* BOARDS_POSIX_NATIVE_SIM_BOARD_SOC_H */
