/*
 * Copyright (c) 2022 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_IT8XXX2_WUC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_IT8XXX2_WUC_H_

#include <zephyr/dt-bindings/dt-util.h>

/** WUC reserved register of reg property */
#define IT8XXX2_WUC_UNUSED_REG	0

/**
 * @name wakeup controller flags
 * @{
 */
/** WUC rising edge trigger mode */
#define WUC_TYPE_EDGE_RISING	BIT(0)
/** WUC falling edge trigger mode */
#define WUC_TYPE_EDGE_FALLING	BIT(1)
/** WUC both edge trigger mode */
#define WUC_TYPE_EDGE_BOTH	(WUC_TYPE_EDGE_RISING | WUC_TYPE_EDGE_FALLING)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_IT8XXX2_WUC_H_ */
