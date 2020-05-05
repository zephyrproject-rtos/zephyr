/*
 * Copyright (c) 2018, Intel Corporation
 * Copyright (c) 2011-2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for the Apollo Lake SoC
 *
 * This module provides routines to initialize and support soc-level hardware
 * for the Apollo Lake SoC.
 */

#include <kernel.h>
#include "soc.h"
#include <drivers/uart.h>
#include <device.h>
#include <init.h>

#ifdef CONFIG_X86_MMU
/* loapic */
MMU_BOOT_REGION(CONFIG_LOAPIC_BASE_ADDRESS, 4 * 1024, MMU_ENTRY_WRITE);

/* ioapic */
MMU_BOOT_REGION(DT_REG_ADDR(DT_INST(0, intel_ioapic)), 1024 * 1024, MMU_ENTRY_WRITE);

#ifdef CONFIG_HPET_TIMER
MMU_BOOT_REGION(DT_REG_ADDR(DT_INST(0, intel_hpet)), KB(4), MMU_ENTRY_WRITE);
#endif  /* CONFIG_HPET_TIMER */

/* for UARTs */
#if DT_HAS_NODE_STATUS_OKAY(DT_INST(0, ns16550))
MMU_BOOT_REGION(DT_REG_ADDR(DT_INST(0, ns16550)), 0x1000,
		(MMU_ENTRY_READ | MMU_ENTRY_WRITE));
#endif

#if DT_HAS_NODE_STATUS_OKAY(DT_INST(1, ns16550))
MMU_BOOT_REGION(DT_REG_ADDR(DT_INST(1, ns16550)), 0x1000,
		(MMU_ENTRY_READ | MMU_ENTRY_WRITE));
#endif

#if DT_HAS_NODE_STATUS_OKAY(DT_INST(2, ns16550))
MMU_BOOT_REGION(DT_REG_ADDR(DT_INST(2, ns16550)), 0x1000,
		(MMU_ENTRY_READ | MMU_ENTRY_WRITE));
#endif

#if DT_HAS_NODE_STATUS_OKAY(DT_INST(3, ns16550))
MMU_BOOT_REGION(DT_REG_ADDR(DT_INST(3, ns16550)), 0x1000,
		(MMU_ENTRY_READ | MMU_ENTRY_WRITE));
#endif

/* for I2C controllers */
#ifdef CONFIG_I2C

#if DT_HAS_NODE_STATUS_OKAY(DT_INST(0, snps_designware_i2c))
MMU_BOOT_REGION(DT_REG_ADDR(DT_INST(0, snps_designware_i2c)), 0x1000,
		(MMU_ENTRY_READ | MMU_ENTRY_WRITE));
#endif

#if DT_HAS_NODE_STATUS_OKAY(DT_INST(1, snps_designware_i2c))
MMU_BOOT_REGION(DT_REG_ADDR(DT_INST(1, snps_designware_i2c)), 0x1000,
		(MMU_ENTRY_READ | MMU_ENTRY_WRITE));
#endif

#if DT_HAS_NODE_STATUS_OKAY(DT_INST(2, snps_designware_i2c))
MMU_BOOT_REGION(DT_REG_ADDR(DT_INST(2, snps_designware_i2c)), 0x1000,
		(MMU_ENTRY_READ | MMU_ENTRY_WRITE));
#endif

#if DT_HAS_NODE_STATUS_OKAY(DT_INST(3, snps_designware_i2c))
MMU_BOOT_REGION(DT_REG_ADDR(DT_INST(3, snps_designware_i2c)), 0x1000,
		(MMU_ENTRY_READ | MMU_ENTRY_WRITE));
#endif

#if DT_HAS_NODE_STATUS_OKAY(DT_INST(4, snps_designware_i2c))
MMU_BOOT_REGION(DT_REG_ADDR(DT_INST(4, snps_designware_i2c)), 0x1000,
		(MMU_ENTRY_READ | MMU_ENTRY_WRITE));
#endif

#if DT_HAS_NODE_STATUS_OKAY(DT_INST(5, snps_designware_i2c))
MMU_BOOT_REGION(DT_REG_ADDR(DT_INST(5, snps_designware_i2c)), 0x1000,
		(MMU_ENTRY_READ | MMU_ENTRY_WRITE));
#endif

#if DT_HAS_NODE_STATUS_OKAY(DT_INST(6, snps_designware_i2c))
MMU_BOOT_REGION(DT_REG_ADDR(DT_INST(6, snps_designware_i2c)), 0x1000,
		(MMU_ENTRY_READ | MMU_ENTRY_WRITE));
#endif

#if DT_HAS_NODE_STATUS_OKAY(DT_INST(7, snps_designware_i2c))
MMU_BOOT_REGION(DT_REG_ADDR(DT_INST(7, snps_designware_i2c)), 0x1000,
		(MMU_ENTRY_READ | MMU_ENTRY_WRITE));
#endif

#endif  /* CONFIG_I2C */

/* for GPIO controller */
#ifdef CONFIG_GPIO_INTEL_APL
MMU_BOOT_REGION(DT_REG_ADDR(DT_NODELABEL(gpio_n_000)),
		DT_REG_SIZE(DT_NODELABEL(gpio_n_000)),
		(MMU_ENTRY_READ | MMU_ENTRY_WRITE));
MMU_BOOT_REGION(DT_REG_ADDR(DT_NODELABEL(gpio_nw_000)),
		DT_REG_SIZE(DT_NODELABEL(gpio_nw_000)),
		(MMU_ENTRY_READ | MMU_ENTRY_WRITE));
MMU_BOOT_REGION(DT_REG_ADDR(DT_NODELABEL(gpio_w_000)),
		DT_REG_SIZE(DT_NODELABEL(gpio_w_000)),
		(MMU_ENTRY_READ | MMU_ENTRY_WRITE));
MMU_BOOT_REGION(DT_REG_ADDR(DT_NODELABEL(gpio_sw_000)),
		DT_REG_SIZE(DT_NODELABEL(gpio_sw_000)),
		(MMU_ENTRY_READ | MMU_ENTRY_WRITE));
#endif

#endif  /* CONFIG_X86_MMU */
