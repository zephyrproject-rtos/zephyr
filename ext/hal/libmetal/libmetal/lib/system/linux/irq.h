/*
 * Copyright (c) 2016, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	linux/irq.h
 * @brief	Linux libmetal irq definitions.
 */

#ifndef __METAL_IRQ__H__
#error "Include metal/irq.h instead of metal/linux/irq.h"
#endif

#ifndef __METAL_LINUX_IRQ__H__
#ifdef METAL_INTERNAL

#include <metal/device.h>

/**
 * @brief	metal_linux_register_dev
 *
 * Metal Linux internal function to register metal device to a IRQ
 * which is generated from the device.
 *
 * @param[in]	dev pointer to metal device
 * @param[in]	irq interrupt id
 */
void metal_linux_irq_register_dev(struct metal_device *dev, int irq);

#endif /* METAL_INTERNAL */
#define __METAL_LINUX_IRQ__H__

#endif /* __METAL_LINUX_IRQ__H__ */
