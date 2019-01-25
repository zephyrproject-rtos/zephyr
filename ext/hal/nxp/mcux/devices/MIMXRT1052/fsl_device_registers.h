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
#if (defined(CPU_MIMXRT1052CVJ5B) || defined(CPU_MIMXRT1052CVL5B) || defined(CPU_MIMXRT1052DVJ6B) || \
    defined(CPU_MIMXRT1052DVL6B))

#define MIMXRT1052_SERIES

/* CMSIS-style register definitions */
#include "MIMXRT1052.h"
/* CPU specific feature definitions */
#include "MIMXRT1052_features.h"

#else
    #error "No valid CPU defined!"
#endif

#endif /* __FSL_DEVICE_REGISTERS_H__ */

/*******************************************************************************
 * EOF
 ******************************************************************************/
