/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file Extra definitions provided by the board to soc.h
 *
 * Background:
 * The POSIC ARCH/SOC/board layering is different than in normal archs
 * The "SOC" does not provide almost any of the typical SOC functionality
 * but that is left for the "board" to define it
 * Device code may rely on the soc.h defining some things (like the interrupts
 * numbers)
 * Therefore this file is included from the inf_clock soc.h to allow a board
 * to define that kind of SOC related snippets
 */

#ifndef _POSIX_NRF52_BOARD_SOC_H
#define _POSIX_NRF52_BOARD_SOC_H

#include <toolchain.h>
#include <sys/util.h>

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/types.h>
#include <stddef.h>
#include "irq.h"
#include "irq_sources.h"
#include <nrfx.h>
#include "cmsis.h"

#define OFFLOAD_SW_IRQ SWI0_EGU0_IRQn

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* _POSIX_NRF52_BOARD_SOC_H */
