/* Copyright (C) 2023 BeagleBoard.org Foundation
 * Copyright (C) 2023 S Prashanth
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_TI_VIM_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_TI_VIM_H_

#include <zephyr/sys/util_macro.h>

#define IRQ_TYPE_LEVEL  BIT(1)
#define IRQ_TYPE_EDGE   BIT(2)

#define IRQ_DEFAULT_PRIORITY  0xf

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_TI_VIM_H_ */
