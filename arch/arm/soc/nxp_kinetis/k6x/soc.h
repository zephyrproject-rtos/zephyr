/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Board configuration macros for the fsl_frdm_k64f platform
 *
 * This header file is used to specify and describe board-level aspects for the
 * 'fsl_frdm_k64f' platform.
 */

#ifndef _SOC__H_
#define _SOC__H_

#include <misc/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/* default system clock */

#define SYSCLK_DEFAULT_IOSC_HZ MHZ(120)
#define BUSCLK_DEFAULT_IOSC_HZ (SYSCLK_DEFAULT_IOSC_HZ / \
				CONFIG_K64_BUS_CLOCK_DIVIDER)

/* address bases */

#define PERIPH_ADDR_BASE_WDOG 0x40052000 /* Watchdog Timer module */

/* IRQs */

#define IRQ_SPI0 26
#define IRQ_SPI1 27
#define IRQ_GPIO_PORTA 59
#define IRQ_GPIO_PORTB 60
#define IRQ_GPIO_PORTC 61
#define IRQ_GPIO_PORTD 62
#define IRQ_GPIO_PORTE 63
#define IRQ_ETH_IEEE1588_TMR 82
#define IRQ_ETH_TX 83
#define IRQ_ETH_RX 84
#define IRQ_ETH_ERR_MISC 85

#ifndef _ASMLANGUAGE

#include <fsl_common.h>
#include <device.h>
#include <misc/util.h>
#include <random/rand32.h>

/* For IMG_MANAGER */
#if defined(CONFIG_SOC_FLASH_MCUX)
#define FLASH_DRIVER_NAME	CONFIG_SOC_FLASH_MCUX_DEV_NAME
#endif

#endif /* !_ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _SOC__H_ */
