/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Option setting header file for Renesas RX26T
 */

#include <zephyr/devicetree.h>

/*
 * Option-Setting Memory for the RX. This region of memory (located in flash)
 * determines the state of the MCU after reset and can not be changed on runtime
 *
 * All registers are set to 0xffffffff by default, which are "safe" settings.
 * Please refer to the Renesas RX Group User's Manual before changing any of
 * the values as some changes can be permanent or lock access to the device.
 *
 */

/* Endian Select Register (MDE) at  0x00120064
 *
 * b2 to b0: endian select between (0 0 0) for big endian and (1 1 1) for little
 * endian.
 * b6 to b4: bank mode select between (0 0 0) for dual mode and (1 1 1) for linear mode
 * all other bits are reserved and have to be set to 1
 */
#define SOC_RX_MDE (0xFFFFFFFFUL) /* little endian */

/* Option Function Select Register 0 (OFS0)
 * This section sets the IWDT (Independent Watchdog Timer) behavior immediately after reset
 * by programming the OFS0 register. When enabled, IWDT starts counting automatically
 * starts after a reset.
 */
#define SOC_RX_OFS0 (0xFFFFFFFFUL)

/* Option Function Select Register 1 (OFS1) (Voltage detection and HOCO)
 */
#define SOC_RX_OFS1 (0xFFFFFFFFUL)
