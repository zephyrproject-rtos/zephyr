/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __FSL_DEVICE_REGISTERS_H__
#define __FSL_DEVICE_REGISTERS_H__

/*
 * Include the cpu specific register header files.
 *
 * The CPU macro should be declared in the project or makefile.
 */
#if (defined(CPU_LPC55S69JBD100_cm33_core0) || defined(CPU_LPC55S69JET98_cm33_core0))

#define LPC55S69_cm33_core0_SERIES

/* CMSIS-style register definitions */
#include "LPC55S69_cm33_core0.h"
/* CPU specific feature definitions */
#include "LPC55S69_cm33_core0_features.h"

#elif (defined(CPU_LPC55S69JBD100_cm33_core1) || defined(CPU_LPC55S69JET98_cm33_core1))

#define LPC55S69_cm33_core1_SERIES

/* CMSIS-style register definitions */
#include "LPC55S69_cm33_core1.h"
/* CPU specific feature definitions */
#include "LPC55S69_cm33_core1_features.h"

#else
    #error "No valid CPU defined!"
#endif

#endif /* __FSL_DEVICE_REGISTERS_H__ */

/*******************************************************************************
 * EOF
 ******************************************************************************/
