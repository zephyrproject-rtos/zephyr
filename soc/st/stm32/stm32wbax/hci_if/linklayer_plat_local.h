/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef _STM32WBA_LINK_LAYER_PLAT_LOCAL_H_
#define _STM32WBA_LINK_LAYER_PLAT_LOCAL_H_

#include <stdint.h>

/*
 * @brief  Register and enable Link Layer ISR
 */
void link_layer_register_isr(void);

/**
 * @brief Disable radio IRQs and temporarily mask lower-priority exceptions.
 *
 * @return Previous BASEPRI value, which the caller must restore after teardown.
 */
uint32_t link_layer_disable_isr(void);

#endif /* _STM32WBA_LINK_LAYER_PLAT_LOCAL_H_ */
