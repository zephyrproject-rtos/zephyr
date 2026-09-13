/*
 * Copyright (c) 2026 ABOV Semiconductor Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for A31C15x processor
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <cmsis_core.h>

#include "abov_config.h"

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 *
 * Deliberately calls PRV_CHIPSET_Init() (WDT disable, SCU register-write
 * enable, flash wait-state) instead of the vendor's SystemInit() from
 * system_a31xxxx.c: that function also relocates VTOR to a vendor
 * __VECTOR_TABLE that doesn't exist in a Zephyr build, since Zephyr owns the
 * vector table and VTOR itself.
 *
 * PRV_PORT_Init() must run here too: it enables the per-port GPIO
 * peripheral/bus-clock gates (SCU->PER1, SCU->PCER1) and unlocks PCU
 * register writes (PORTEN). Without it, every PCU port (including the ones
 * pinctrl configures for USART pin muxing) is unpowered/unclocked, so reads
 * and writes to its MR1/MR2/CR registers are meaningless.
 */
void soc_early_init_hook(void)
{
	PRV_CHIPSET_Init();
	PRV_PORT_Init();
}
