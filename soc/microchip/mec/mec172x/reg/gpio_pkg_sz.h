/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC172X_GPIO_SZ_H
#define _MEC172X_GPIO_SZ_H

#include <stdint.h>
#include <stddef.h>

/* MEC172XH-B0-SZ (144-pin) */
#define MCHP_GPIO_PORT_A_BITMAP 0x7FFFFF9Du /* GPIO_0000 - 0036  GIRQ11 */
#define MCHP_GPIO_PORT_B_BITMAP 0x7FFFFFFDu /* GPIO_0040 - 0076  GIRQ10 */
#define MCHP_GPIO_PORT_C_BITMAP 0x07FFFCF7u /* GPIO_0100 - 0136  GIRQ09 */
#define MCHP_GPIO_PORT_D_BITMAP 0x272EFFFFu /* GPIO_0140 - 0176  GIRQ08 */
#define MCHP_GPIO_PORT_E_BITMAP 0x00DE00FFu /* GPIO_0200 - 0236  GIRQ12 */
#define MCHP_GPIO_PORT_F_BITMAP 0x0000397Fu /* GPIO_0240 - 0276  GIRQ26 */

#endif /* #ifndef _MEC172X_GPIO_SZ_H */
