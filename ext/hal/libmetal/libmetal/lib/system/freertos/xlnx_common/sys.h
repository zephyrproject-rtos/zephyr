/*
 * Copyright (c) 2015, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	freertos/xlnx_common/sys.h
 * @brief	freertos Xilinx common system primitives for libmetal.
 */

#ifndef __METAL_FREERTOS_SYS__H__
#error "Include metal/sys.h instead of metal/freertos/@PROJECT_MACHINE@/sys.h"
#endif

#ifndef __METAL_FREERTOS_XLNX_COMMON_SYS__H__
#define __METAL_FREERTOS_XLNX_COMMON_SYS__H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief	metal_xlnx_irq_isr
 *
 * Xilinx interrupt ISR can be registered to the Xilinx embeddedsw
 * IRQ controller driver.
 *
 * @param[in] arg input argument, interrupt vector id.
 */
void metal_xlnx_irq_isr(void *arg);

/**
 * @brief	metal_xlnx_irq_int
 *
 * Xilinx interrupt controller initialization. It will initialize
 * the metal Xilinx IRQ controller data structure.
 *
 * @return 0 for success, or negative value for failure
 */
int metal_xlnx_irq_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __METAL_FREERTOS_XLNX_COMMON_SYS__H__ */
