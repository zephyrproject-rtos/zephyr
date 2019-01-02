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
#if (defined(CPU_MIMXRT1021CAF4A) || defined(CPU_MIMXRT1021CAG4A) || defined(CPU_MIMXRT1021DAF5A) || \
    defined(CPU_MIMXRT1021DAG5A))

#define MIMXRT1021_SERIES

/* CMSIS-style register definitions */
#include "MIMXRT1021.h"
/* CPU specific feature definitions */
#include "MIMXRT1021_features.h"

#else
    #error "No valid CPU defined!"
#endif

#endif /* __FSL_DEVICE_REGISTERS_H__ */

/*******************************************************************************
 * EOF
 ******************************************************************************/
