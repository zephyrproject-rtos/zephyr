/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_DEVICES_H_
#define _SOC_DEVICES_H_

#ifndef _ASMLANGUAGE

#include "soc_registers.h"

/* FPGA system control block (FPGAIO) */
#define FPGAIO_BASE_ADDR	 (0x40028000)
#define __MPS2_FPGAIO ((volatile struct mps2_fpgaio *)FPGAIO_BASE_ADDR)

/* Names of GPIO drivers used to provide access to some FPGAIO registers */
#define FPGAIO_LED0_GPIO_NAME		"FPGAIO_LED0"
#define FPGAIO_BUTTON_GPIO_NAME		"FPGAIO_BUTTON"
#define FPGAIO_MISC_GPIO_NAME		"FPGAIO_MISC"

#endif /* !_ASMLANGUAGE */

#endif /* _SOC_DEVICES_H_ */
