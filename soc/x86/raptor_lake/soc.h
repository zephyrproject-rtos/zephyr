/*
 * Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Board configuration macros for the Raptor Lake SoC
 *
 * This header file is used to specify and describe soc-level aspects for
 * the 'Raptor Lake' SoC.
 */

#ifndef __SOC_H_
#define __SOC_H_

#include <zephyr/sys/util.h>

#ifndef _ASMLANGUAGE
#include <zephyr/device.h>
#include <zephyr/random/rand32.h>
#endif

#if DT_ON_BUS(DT_CHOSEN(zephyr_console), pcie)
#include <zephyr/drivers/pcie/pcie.h>
#define X86_SOC_EARLY_SERIAL_PCIDEV DT_REG_ADDR(DT_CHOSEN(zephyr_console))
#else
#define X86_SOC_EARLY_SERIAL_MMIO8_ADDR DT_REG_ADDR(DT_CHOSEN(zephyr_console))
#endif

#endif /* __SOC_H_ */
