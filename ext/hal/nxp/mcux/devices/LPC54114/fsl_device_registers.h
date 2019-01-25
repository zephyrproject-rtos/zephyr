/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018 NXP
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
#if (defined(CPU_LPC54114J256BD64_cm4) || defined(CPU_LPC54114J256UK49_cm4))

#define LPC54114_cm4_SERIES

/* CMSIS-style register definitions */
#include "LPC54114_cm4.h"
/* CPU specific feature definitions */
#include "LPC54114_cm4_features.h"

#elif (defined(CPU_LPC54114J256BD64_cm0plus) || defined(CPU_LPC54114J256UK49_cm0plus))

#define LPC54114_cm0plus_SERIES

/* CMSIS-style register definitions */
#include "LPC54114_cm0plus.h"
/* CPU specific feature definitions */
#include "LPC54114_cm0plus_features.h"

#else
    #error "No valid CPU defined!"
#endif

#endif /* __FSL_DEVICE_REGISTERS_H__ */

/*******************************************************************************
 * EOF
 ******************************************************************************/
