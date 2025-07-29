/*
 * Copyright (c) 2021 KT-Elektronik, Klaucke und Partner GmbH
 * Copyright (c) 2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Option-Setting Memory for the RX. This region of memory (located in flash)
 * determines the state of the MCU after reset and can not be changed on runtime
 *
 * All registers are set to 0xffffffff by default, which are "safe" settings.
 * Please refer to the Renesas RX Group User's Manual before changing any of
 * the values as some changes can be permanent or lock access to the device.
 *
 * Address range: 0xFFFFFF80 to 0xFFFFFF8F (16 Bytes)
 */

#define __OFS_MDE __attribute__((section(".ofs_mde")))
#define __OFS0 __attribute__((section(".ofs0")))
#define __OFS1 __attribute__((section(".ofs1")))

/* Endian Select Register (MDE)
 *
 * b2 to b0: endian select between (0 0 0) for big endian and (1 1 1) for little
 * endian. Set this according to __BYTE_ORDER__ (cf. include\toolchain\gcc.h)
 *
 * b6-b4 (Bank Mode Select) indicate whether the flash is operated in
 * Dual mode (0 0 0) or Linear mode (1 1 1).
 *
 * all other bits are reserved and have to be set to 1
 */
const unsigned long __OFS_MDE __MDEreg = 0xffffffffU; /* little */

/* Option Function Select Register 0 (OFS0) */
const unsigned long __OFS0 __OFS0reg = 0xffffffffU;

/* Option Function Select Register 1 (OFS1) */
const unsigned long __OFS1 __OFS1reg = 0xffffffffU;
