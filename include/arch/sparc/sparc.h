/*
 * Copyright (c) 2019-2020 Cobham Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_SPARC_SPARC_H_
#define ZEPHYR_INCLUDE_ARCH_SPARC_SPARC_H_

/*
 * @file
 * @brief Definitions for the SPARC V8 architecture.
 */

/* Processor State Register */
#define PSR_VER_BIT                     24
#define PSR_PIL_BIT                      8

#define PSR_VER                         (0xf << PSR_VER_BIT)
#define PSR_EF                          (1 << 12)
#define PSR_S                           (1 <<  7)
#define PSR_PS                          (1 <<  6)
#define PSR_ET                          (1 <<  5)
#define PSR_PIL                         (0xf << PSR_PIL_BIT)
#define PSR_CWP                         0x1f


/* Trap Base Register */
#define TBR_TT_BIT                       4

#define TBR_TBA                         0xfffff000
#define TBR_TT                          0x00000ff0

/* Trap types in TBR.TT */
#define TT_RESET                        0x00
#define TT_WINDOW_OVERFLOW              0x05
#define TT_WINDOW_UNDERFLOW             0x06
#define TT_DATA_ACCESS_EXCEPTION        0x09

#endif /* ZEPHYR_INCLUDE_ARCH_SPARC_SPARC_H_ */
