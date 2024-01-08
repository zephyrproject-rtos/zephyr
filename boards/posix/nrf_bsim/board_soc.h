/*
 * Copyright (c) 2017 Oticon A/S
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

#ifndef BOARDS_POSIX_NRF_BSIM_BOARD_SOC_H
#define BOARDS_POSIX_NRF_BSIM_BOARD_SOC_H

#include <zephyr/toolchain.h>
#include <zephyr/sys/util.h>

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/irq.h>
#include <nrfx.h>
#include "cmsis.h"

#if defined(CONFIG_BOARD_NRF52_BSIM)
#define OFFLOAD_SW_IRQ SWI0_EGU0_IRQn
#elif defined(CONFIG_BOARD_NRF5340BSIM_NRF5340_CPUAPP)
#define OFFLOAD_SW_IRQ EGU0_IRQn
#elif defined(CONFIG_BOARD_NRF5340BSIM_NRF5340_CPUNET)
#define OFFLOAD_SW_IRQ SWI0_IRQn
#endif

#define FLASH_PAGE_ERASE_MAX_TIME_US	89700UL
#define FLASH_PAGE_MAX_CNT		256UL

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* BOARDS_POSIX_NRF_BSIM_BOARD_SOC_H */
