/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef _STM32WBA_LINK_LAYER_PLAT_LOCAL_H_
#define _STM32WBA_LINK_LAYER_PLAT_LOCAL_H_

/*
 * @brief  Link Layer ISR registration
 * @param  force: force ISR registration even if already done before
 */
void link_layer_register_isr(bool force);

/*
 * @brief  Link Layer related ISR disable
 */
void link_layer_disable_isr(void);

#endif /* _STM32WBA_LINK_LAYER_PLAT_LOCAL_H_ */
