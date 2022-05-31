/*
 * Copyright (c) 2022 Microchip Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RV_SMP_DEFS_H_
#define RV_SMP_DEFS_H_

/* Sequence of flag values for wake up procedure - natural reg size */
#if CONFIG_64BIT
#define RV_WAKE_INIT 0xAA550101010155AAUL
#define RV_WAKE_WAIT 0xAA550202020255AAUL
#define RV_WAKE_GO   0xAA550303030355AAUL
#define RV_WAKE_DONE 0xAA550404040455AAUL
#else
#define RV_WAKE_INIT 0xAA010155UL
#define RV_WAKE_WAIT 0xAA020255UL
#define RV_WAKE_GO   0xAA030355UL
#define RV_WAKE_DONE 0xAA040455UL
#endif
#endif /* RV_SMP_DEFS_H_ */
