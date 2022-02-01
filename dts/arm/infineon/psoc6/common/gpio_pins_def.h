/*
 * Copyright (c) 2021, Cypress Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */

/** Gets a pin definition from the provided port and pin numbers */
#define CYHAL_GET_GPIO(port, pin)   ((((port)) << 3U) + ((pin)))

/** Definitions for all of the pins that are bonded out on the PSoC 6. */
#define NC    0xFF

/* PORT 0 */
#define P0_0  CYHAL_GET_GPIO(0, 0)
#define P0_1  CYHAL_GET_GPIO(0, 1)
#define P0_2  CYHAL_GET_GPIO(0, 2)
#define P0_3  CYHAL_GET_GPIO(0, 3)
#define P0_4  CYHAL_GET_GPIO(0, 4)
#define P0_5  CYHAL_GET_GPIO(0, 5)
#define P0_6  CYHAL_GET_GPIO(0, 6)
#define P0_7  CYHAL_GET_GPIO(0, 7)

/* PORT 1 */
#define P1_0  CYHAL_GET_GPIO(1, 0)
#define P1_1  CYHAL_GET_GPIO(1, 1)
#define P1_2  CYHAL_GET_GPIO(1, 2)
#define P1_3  CYHAL_GET_GPIO(1, 3)
#define P1_4  CYHAL_GET_GPIO(1, 4)
#define P1_5  CYHAL_GET_GPIO(1, 5)
#define P1_6  CYHAL_GET_GPIO(1, 6)
#define P1_7  CYHAL_GET_GPIO(1, 7)

/* PORT 2 */
#define P2_0  CYHAL_GET_GPIO(2, 0)
#define P2_1  CYHAL_GET_GPIO(2, 1)
#define P2_2  CYHAL_GET_GPIO(2, 2)
#define P2_3  CYHAL_GET_GPIO(2, 3)
#define P2_4  CYHAL_GET_GPIO(2, 4)
#define P2_5  CYHAL_GET_GPIO(2, 5)
#define P2_6  CYHAL_GET_GPIO(2, 6)
#define P2_7  CYHAL_GET_GPIO(2, 7)

/* PORT 3 */
#define P3_0  CYHAL_GET_GPIO(3, 0)
#define P3_1  CYHAL_GET_GPIO(3, 1)
#define P3_2  CYHAL_GET_GPIO(3, 2)
#define P3_3  CYHAL_GET_GPIO(3, 3)
#define P3_4  CYHAL_GET_GPIO(3, 4)
#define P3_5  CYHAL_GET_GPIO(3, 5)
#define P3_6  CYHAL_GET_GPIO(3, 6)
#define P3_7  CYHAL_GET_GPIO(3, 7)

/* PORT 4 */
#define P4_0  CYHAL_GET_GPIO(4, 0)
#define P4_1  CYHAL_GET_GPIO(4, 1)
#define P4_2  CYHAL_GET_GPIO(4, 2)
#define P4_3  CYHAL_GET_GPIO(4, 3)
#define P4_4  CYHAL_GET_GPIO(4, 4)
#define P4_5  CYHAL_GET_GPIO(4, 5)
#define P4_6  CYHAL_GET_GPIO(4, 6)
#define P4_7  CYHAL_GET_GPIO(4, 7)

/* PORT 5 */
#define P5_0  CYHAL_GET_GPIO(5, 0)
#define P5_1  CYHAL_GET_GPIO(5, 1)
#define P5_2  CYHAL_GET_GPIO(5, 2)
#define P5_3  CYHAL_GET_GPIO(5, 3)
#define P5_4  CYHAL_GET_GPIO(5, 4)
#define P5_5  CYHAL_GET_GPIO(5, 5)
#define P5_6  CYHAL_GET_GPIO(5, 6)
#define P5_7  CYHAL_GET_GPIO(5, 7)

/* PORT 6 */
#define P6_0  CYHAL_GET_GPIO(6, 0)
#define P6_1  CYHAL_GET_GPIO(6, 1)
#define P6_2  CYHAL_GET_GPIO(6, 2)
#define P6_3  CYHAL_GET_GPIO(6, 3)
#define P6_4  CYHAL_GET_GPIO(6, 4)
#define P6_5  CYHAL_GET_GPIO(6, 5)
#define P6_6  CYHAL_GET_GPIO(6, 6)
#define P6_7  CYHAL_GET_GPIO(6, 7)

/* PORT 7 */
#define P7_0  CYHAL_GET_GPIO(7, 0)
#define P7_1  CYHAL_GET_GPIO(7, 1)
#define P7_2  CYHAL_GET_GPIO(7, 2)
#define P7_3  CYHAL_GET_GPIO(7, 3)
#define P7_4  CYHAL_GET_GPIO(7, 4)
#define P7_5  CYHAL_GET_GPIO(7, 5)
#define P7_6  CYHAL_GET_GPIO(7, 6)
#define P7_7  CYHAL_GET_GPIO(7, 7)

/* PORT 8 */
#define P8_0  CYHAL_GET_GPIO(8, 0)
#define P8_1  CYHAL_GET_GPIO(8, 1)
#define P8_2  CYHAL_GET_GPIO(8, 2)
#define P8_3  CYHAL_GET_GPIO(8, 3)
#define P8_4  CYHAL_GET_GPIO(8, 4)
#define P8_5  CYHAL_GET_GPIO(8, 5)
#define P8_6  CYHAL_GET_GPIO(8, 6)
#define P8_7  CYHAL_GET_GPIO(8, 7)

/* PORT 9 */
#define P9_0  CYHAL_GET_GPIO(9, 0)
#define P9_1  CYHAL_GET_GPIO(9, 1)
#define P9_2  CYHAL_GET_GPIO(9, 2)
#define P9_3  CYHAL_GET_GPIO(9, 3)
#define P9_4  CYHAL_GET_GPIO(9, 4)
#define P9_5  CYHAL_GET_GPIO(9, 5)
#define P9_6  CYHAL_GET_GPIO(9, 6)
#define P9_7  CYHAL_GET_GPIO(9, 7)

/* PORT 10 */
#define P10_0  CYHAL_GET_GPIO(10, 0)
#define P10_1  CYHAL_GET_GPIO(10, 1)
#define P10_2  CYHAL_GET_GPIO(10, 2)
#define P10_3  CYHAL_GET_GPIO(10, 3)
#define P10_4  CYHAL_GET_GPIO(10, 4)
#define P10_5  CYHAL_GET_GPIO(10, 5)
#define P10_6  CYHAL_GET_GPIO(10, 6)
#define P10_7  CYHAL_GET_GPIO(10, 7)

/* PORT 11 */
#define P11_0  CYHAL_GET_GPIO(11, 0)
#define P11_1  CYHAL_GET_GPIO(11, 1)
#define P11_2  CYHAL_GET_GPIO(11, 2)
#define P11_3  CYHAL_GET_GPIO(11, 3)
#define P11_4  CYHAL_GET_GPIO(11, 4)
#define P11_5  CYHAL_GET_GPIO(11, 5)
#define P11_6  CYHAL_GET_GPIO(11, 6)
#define P11_7  CYHAL_GET_GPIO(11, 7)

/* PORT 12 */
#define P12_0  CYHAL_GET_GPIO(12, 0)
#define P12_1  CYHAL_GET_GPIO(12, 1)
#define P12_2  CYHAL_GET_GPIO(12, 2)
#define P12_3  CYHAL_GET_GPIO(12, 3)
#define P12_4  CYHAL_GET_GPIO(12, 4)
#define P12_5  CYHAL_GET_GPIO(12, 5)
#define P12_6  CYHAL_GET_GPIO(12, 6)
#define P12_7  CYHAL_GET_GPIO(12, 7)

/* PORT 13 */
#define P13_0  CYHAL_GET_GPIO(13, 0)
#define P13_1  CYHAL_GET_GPIO(13, 1)
#define P13_2  CYHAL_GET_GPIO(13, 2)
#define P13_3  CYHAL_GET_GPIO(13, 3)
#define P13_4  CYHAL_GET_GPIO(13, 4)
#define P13_5  CYHAL_GET_GPIO(13, 5)
#define P13_6  CYHAL_GET_GPIO(13, 6)
#define P13_7  CYHAL_GET_GPIO(13, 7)

/* PORT 14 */
#define P14_0  CYHAL_GET_GPIO(14, 0)
#define P14_1  CYHAL_GET_GPIO(14, 1)
#define P14_2  CYHAL_GET_GPIO(14, 2)
#define P14_3  CYHAL_GET_GPIO(14, 3)
#define P14_4  CYHAL_GET_GPIO(14, 4)
#define P14_5  CYHAL_GET_GPIO(14, 5)
#define P14_6  CYHAL_GET_GPIO(14, 6)
#define P14_7  CYHAL_GET_GPIO(14, 7)