/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_DEVICES_H_
#define _SOC_DEVICES_H_

#include <soc_memory_map.h>
#include <soc_irq.h>

#if defined(CONFIG_I2C_SBCON)
#define I2C_SBCON_0_BASE_ADDR	I2C_TOUCH_BASE_ADDR
#define I2C_SBCON_1_BASE_ADDR	I2C_AUDIO_CONF_BASE_ADDR
#define I2C_SBCON_2_BASE_ADDR	I2C_SHIELD0_BASE_ADDR
#define I2C_SBCON_3_BASE_ADDR	I2C_SHIELD1_BASE_ADDR
#endif

#ifndef _ASMLANGUAGE

#include "soc_registers.h"

/* System Control Register (SYSCON) */
#define __MPS2_SYSCON ((volatile struct mps2_syscon *)SYSCON_BASE_ADDR)

/* FPGA system control block (FPGAIO) */
#define __MPS2_FPGAIO ((volatile struct mps2_fpgaio *)FPGAIO_BASE_ADDR)

/* Names of GPIO drivers used to provide access to some FPGAIO registers */
#define FPGAIO_LED0_GPIO_NAME		"FPGAIO_LED0"
#define FPGAIO_BUTTON_GPIO_NAME		"FPGAIO_BUTTON"
#define FPGAIO_MISC_GPIO_NAME		"FPGAIO_MISC"

#endif /* !_ASMLANGUAGE */

#endif /* _SOC_DEVICES_H_ */
