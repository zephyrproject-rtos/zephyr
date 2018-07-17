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
#include <uart.h>
#include <device.h>
#include <init.h>

#ifdef CONFIG_X86_MMU
/* loapic */
MMU_BOOT_REGION(CONFIG_LOAPIC_BASE_ADDRESS, 4 * 1024, MMU_ENTRY_WRITE);

/* ioapic */
MMU_BOOT_REGION(CONFIG_IOAPIC_BASE_ADDRESS, 1024 * 1024, MMU_ENTRY_WRITE);

#ifdef CONFIG_HPET_TIMER
MMU_BOOT_REGION(CONFIG_HPET_TIMER_BASE_ADDRESS, KB(4), MMU_ENTRY_WRITE);
#endif  /* CONFIG_HPET_TIMER */

/* for UARTs */
#ifdef CONFIG_UART_NS16550

#ifdef CONFIG_UART_NS16550_PORT_0
MMU_BOOT_REGION(CONFIG_UART_NS16550_PORT_0_BASE_ADDR, 0x1000,
		(MMU_ENTRY_READ | MMU_ENTRY_WRITE));
#endif

#ifdef CONFIG_UART_NS16550_PORT_1
MMU_BOOT_REGION(CONFIG_UART_NS16550_PORT_1_BASE_ADDR, 0x1000,
		(MMU_ENTRY_READ | MMU_ENTRY_WRITE));
#endif

#endif  /* CONFIG_UART_NS16550 */

#endif  /* CONFIG_X86_MMU */
