/*
 * Copyright (c) 2025 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __APOLLO5_PINCTRL_H__
#define __APOLLO5_PINCTRL_H__

#define APOLLO5_ALT_FUNC_POS 0
#define APOLLO5_ALT_FUNC_MASK 0xf

#define APOLLO5_PIN_NUM_POS 4
#define APOLLO5_PIN_NUM_MASK 0xff

#define APOLLO5_PINMUX(pin_num, alt_func)       \
        (pin_num << APOLLO5_PIN_NUM_POS |       \
         alt_func << APOLLO5_ALT_FUNC_POS)

#endif /* __APOLLO5_PINCTRL_H__ */
