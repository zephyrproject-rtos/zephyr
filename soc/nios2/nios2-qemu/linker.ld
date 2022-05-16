/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Linker script for the Nios II/e CPU with timer and 16550 UART
 */

#include <layout.h>
#include <zephyr/devicetree.h>

#define _RAM_ADDR	DT_REG_ADDR(DT_CHOSEN(zephyr_sram))
#define _RAM_SIZE	DT_REG_SIZE(DT_CHOSEN(zephyr_sram))

#define _ROM_ADDR	DT_REG_ADDR(DT_CHOSEN(zephyr_flash))
#define _ROM_SIZE	DT_REG_SIZE(DT_CHOSEN(zephyr_flash))

#include <zephyr/arch/nios2/linker.ld>
