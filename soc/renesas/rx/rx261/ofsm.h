/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Option setting header file for Renesas RX261
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
 * Address range: 0xFFFFFF80 to 0xFFFFFF8F
 */

/* Endian Select Register (MDE) at 0xFFFFFF80
 *
 * b2 to b0: endian select between (0 0 0) for big endian and (1 1 1) for little
 * endian. Set this according to __BYTE_ORDER__ (cf. include\toolchain\gcc.h)
 *
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

#define RX_HOCO_FREQ DT_PROP(DT_NODELABEL(hoco), clock_frequency)

#if (RX_HOCO_FREQ == 24000000)
#define SOC_OFS1_HOCO 0b10
#elif (RX_HOCO_FREQ == 32000000)
#define SOC_OFS1_HOCO 0b11
#elif (RX_HOCO_FREQ == 48000000)
#define SOC_OFS1_HOCO 0b01
#elif (RX_HOCO_FREQ == 64000000)
#define SOC_OFS1_HOCO 0b00
#else
#error "Unsupported HOCO clock frequency (must be 24/32/48/64 MHz)"
#endif

#define SOC_RX_OFS1 ((SOC_OFS1_HOCO << 12) | 0xFFFFCFFFUL)
