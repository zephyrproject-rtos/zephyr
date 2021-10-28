/*
 * Copyright (c) 2021 Microchip Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RV_SMP_DEFS_H_
#define RV_SMP_DEFS_H_

/* Sequence of flag values for wake up procedure */
#define RV_WAKE_INIT 0xAA550101010155AAUL
#define RV_WAKE_WAIT 0xAA550202020255AAUL
#define RV_WAKE_GO   0xAA550303030355AAUL
#define RV_WAKE_DONE 0xAA550404040455AAUL

#endif /* RV_SMP_DEFS_H_ */
