/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC_DEFS_H
#define _MEC_DEFS_H

#ifndef BIT
#define BIT(n) (1ul << (n))
#endif

#ifndef SHLU32
#define SHLU32(v, n) ((unsigned long)(v) << (n))
#endif

#ifndef BIT_CLR
#define BIT_CLR(v, bpos) (v) &= ~BIT(bpos)
#endif

#endif /* #ifndef _MEC_DEFS_H */
