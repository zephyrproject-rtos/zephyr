/*
 * Copyright (c) 2026 Cody Gu <gujiaqi@iscas.ac.cn>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HPM_SDK_ZEPHYR_BOARD_COMPAT_H_
#define HPM_SDK_ZEPHYR_BOARD_COMPAT_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Some unmodified HPM SDK components include board.h only for this delay
 * helper. Keep the vendor component source unchanged and provide the small
 * board contract it expects from the Zephyr integration.
 */
void board_delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* HPM_SDK_ZEPHYR_BOARD_COMPAT_H_ */
