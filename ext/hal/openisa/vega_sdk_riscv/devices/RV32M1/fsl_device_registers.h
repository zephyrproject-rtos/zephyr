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
#if defined(CPU_RV32M1_cm0plus)

#define RV32M1_cm0plus_SERIES

/* CMSIS-style register definitions */
#include "RV32M1_cm0plus.h"
/* CPU specific feature definitions */
#include "RV32M1_cm0plus_features.h"

#elif defined(CPU_RV32M1_cm4)

#define RV32M1_cm4_SERIES

/* CMSIS-style register definitions */
#include "RV32M1_cm4.h"
/* CPU specific feature definitions */
#include "RV32M1_cm4_features.h"

#elif defined(CPU_RV32M1_zero_riscy)

#define RV32M1_zero_riscy_SERIES

/* CMSIS-style register definitions */
#include "RV32M1_zero_riscy.h"
/* CPU specific feature definitions */
#include "RV32M1_zero_riscy_features.h"

#elif defined(CPU_RV32M1_ri5cy)

#define RV32M1_ri5cy_SERIES

/* CMSIS-style register definitions */
#include "RV32M1_ri5cy.h"
/* CPU specific feature definitions */
#include "RV32M1_ri5cy_features.h"

#else
    #error "No valid CPU defined!"
#endif

#endif /* __FSL_DEVICE_REGISTERS_H__ */

/*******************************************************************************
 * EOF
 ******************************************************************************/
