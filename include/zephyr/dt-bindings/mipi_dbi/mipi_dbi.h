/*
 * Copyright 2024,2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_MIPI_DBI_MIPI_DBI_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_MIPI_DBI_MIPI_DBI_H_

/**
 * @brief MIPI-DBI driver APIs
 * @defgroup mipi_dbi_interface MIPI-DBI driver APIs
 * @ingroup io_interfaces
 * @{
 */

/**
 * SPI 3 wire (Type C1). Uses 9 write clocks to send a byte of data.
 * The bit sent on the 9th clock indicates whether the byte is a
 * command or data byte
 *
 *
 *           .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-
 *     SCK  -' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-'
 *
 *          -.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.-
 *     DOUT  |D/C| D7| D6| D5| D4| D3| D2| D1| D0|D/C| D7| D6| D5| D4|...|
 *          -'---'---'---'---'---'---'---'---'---'---'---'---'---'---'---'-
 *           | Word 1                            | Word n
 *
 *          -.								 .-
 *     CS    '-----------------------------------------------------------'
 */
#define MIPI_DBI_MODE_SPI_3WIRE 0x1
/**
 * SPI 4 wire (Type C3). Uses 8 write clocks to send a byte of data.
 * an additional C/D pin will be use to indicate whether the byte is a
 * command or data byte
 *
 *           .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-.
 *     SCK  -' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '---
 *
 *          -.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.-
 *     DOUT  | D7| D6| D5| D4| D3| D2| D1| D0| D7| D6| D5| D4| D3| D2| D1| D0|
 *          -'---'---'---'---'---'---'---'---'---'---'---'---'---'---'---'---'-
 *           | Word 1                        | Word n
 *
 *          -.								     .-
 *     CS    '---------------------------------------------------------------'
 *
 *          -.-------------------------------.-------------------------------.-
 *     CD    |             D/C               |             D/C               |
 *          -'-------------------------------'-------------------------------'-
 */
#define MIPI_DBI_MODE_SPI_4WIRE 0x2
/**
 * Parallel Bus protocol for MIPI DBI Type A based on Motorola 6800 bus.
 *
 *              -.   .--------.	  .------------------------
 *     CS        '---'        '---'
 *
 *              -------------------------------------------
 *     RESX
 *
 *			  .--------------------------------
 *     D/CX     ----------'
 *
 *
 *     R/WX     -------------------------------------------
 *
 *              -------------------------------------------
 *     E
 *
 *               .--------.   .--------------------------.
 *     D[15:0]/ -| COMMAND|---|  DATA                    |
 *     D[8:0]/   '--------'   '--------------------------'
 *     D[7:0]
 *
 * Please refer to the MIPI DBI specification for a detailed cycle diagram.
 */
#define MIPI_DBI_MODE_6800_BUS_16_BIT 0x3
#define MIPI_DBI_MODE_6800_BUS_9_BIT 0x4
#define MIPI_DBI_MODE_6800_BUS_8_BIT 0x5
/**
 * Parallel Bus protocol for MIPI DBI Type B based on Intel 8080 bus.
 *
 *              -.					 .-
 *     CS        '---------------------------------------'
 *
 *              -------------------------------------------
 *     RESX
 *
 *              --.	      .----------------------------
 *     D/CX       '-----------'
 *
 *              ---.   .--------.   .----------------------
 *     WRX         '---'	'---'
 *
 *              -------------------------------------------
 *     RDX
 *
 *                 .--------.   .--------------------------.
 *     D[15:0]/ ---| COMMAND|---|  DATA                    |
 *     D[8:0]/     '--------'   '--------------------------'
 *     D[7:0]
 *
 * Please refer to the MIPI DBI specification for a detailed cycle diagram.
 */
#define MIPI_DBI_MODE_8080_BUS_16_BIT 0x6
#define MIPI_DBI_MODE_8080_BUS_9_BIT 0x7
#define MIPI_DBI_MODE_8080_BUS_8_BIT 0x8

/** Color coding for MIPI DBI Type A or Type B interface. */
/**
 * For 8-bit data bus width, 1 pixel is sent in 1 cycle. For 16-bit data bus width,
 * 2 pixels are sent in 1 cycle.
 */
#define MIPI_DBI_MODE_RGB332 (0x1 << 4U)
/**
 * For 8-bit data bus width, 2 pixels are sent in 3 cycles. For 16-bit data bus width,
 * 1 pixel is sent in 1 cycle, the high 4 bits are not used.
 */
#define MIPI_DBI_MODE_RGB444 (0x2 << 4U)
/**
 * For 8-bit data bus width, 1 pixel is sent in 2 cycles. For 16-bit data bus width,
 * 1 pixel is sent in 1 cycle.
 */
#define MIPI_DBI_MODE_RGB565 (0x3 << 4U)
/**
 * For 8-bit data bus width, MIPI_DBI_MODE_RGB666_1 and MIPI_DBI_MODE_RGB666_2
 * are the same. 1 pixel is sent in 3 cycles, R component first, and the low 2
 * bits are not used.
 * For 9-bit data bus width, MIPI_DBI_MODE_RGB666_1 and MIPI_DBI_MODE_RGB666_2
 * are the same. 1 pixel is sent in 2 cycles.
 * For 16-bit data bus width, MIPI_DBI_MODE_RGB666_1 is option 1,
 * 2 pixels are sent in 3 cycles. The first pixel's R/G/B components are sent in
 * cycle 1 bits 10-15, cycle 1 bits 2-7 and cycle 2 bits 10-15.
 * The second pixel's R/G/B components are sent in cycle 2 bits 2-7, cycle 3 bits
 * 10-15 and cycle 3 bits 2-7.
 * MIPI_DBI_MODE_RGB666_2 is option 2, 1 pixel is sent in 2 cycles. The pixel's
 * R/G/B components are sent in cycle 1 bits 2-7, cycle 2 bits 10-15 and cycle 2
 * bits 2-7.
 */
#define MIPI_DBI_MODE_RGB666_1 (0x4 << 4U)
#define MIPI_DBI_MODE_RGB666_2 (0x5 << 4U)
/**
 * For 8-bit data bus width, MIPI_DBI_MODE_RGB666_1 and MIPI_DBI_MODE_RGB666_2
 * are the same. 1 pixel is sent in 3 cycles, R component first.
 * For 16-bit data bus width, MIPI_DBI_MODE_RGB666_1 is option 1,
 * 2 pixels are sent in 3 cycles. The first pixel's R/G/B components are sent in
 * cycle 1 bits 8-15, cycle 1 bits 0-7 and cycle 2 bits 0-15.
 * The second pixel's R/G/B components are sent in cycle 2 bits 0-7, cycle 3 bits
 * 8-15 and cycle 3 bits 0-7.
 * MIPI_DBI_MODE_RGB666_2 is option 2, 1 pixel is sent in 2 cycles. The pixel's
 * R/G/B components are sent in cycle 1 bits 0-7, cycle 2 bits 8-15 and cycle 2
 * bits 0-7.
 */
#define MIPI_DBI_MODE_RGB888_1 (0x6 << 4U)
#define MIPI_DBI_MODE_RGB888_2 (0x7 << 4U)

/** MIPI DBI tearing enable synchronization is disabled. */
#define MIPI_DBI_TE_NO_EDGE 0x0

/**
 * MIPI DBI tearing enable synchronization on rising edge of TE signal.
 * The controller will only send display write data on a rising edge of TE.
 * This should be used when the controller can send a frame worth of data
 * data to the display panel faster than the display panel can read a frame
 * from its RAM
 *
 *                   .------.                        .------.
 *     TE       -----'      '------------------------'      '-------------
 *              -----.        .----------------------.
 *     CS            '--------'                      '--------------------
 */
#define MIPI_DBI_TE_RISING_EDGE 0x1

/**
 * MIPI DBI tearing enable synchronization on falling edge of TE signal.
 * The controller will only send display write data on a falling edge of TE.
 * This should be used when the controller sends a frame worth of data
 * data to the display panel slower than the display panel can read a frame
 * from its RAM. TE synchronization in this mode will only work if the
 * controller can complete the write before the display panel completes 2
 * read cycles, otherwise the read pointer will "catch up" with the write
 * pointer.
 *
 *                   .------.                        .------.
 *     TE       -----'      '------------------------'      '-------------
 *              ------------.                                       .-----
 *     CS                   '---------------------------------------'
 */
#define MIPI_DBI_TE_FALLING_EDGE 0x2

/**
 * SPI transfer of DBI commands as 8-bit blocks, the default behaviour in
 * SPI 4 wire (Type C3) mode. The clocking diagram corresponds exactly to
 * the illustration of Type C3.
 */
#define MIPI_DBI_SPI_XFR_8BIT 8
/**
 * SPI transfer of DBI commands as 16-bit blocks, a rare and seldom behaviour
 * in SPI 4 wire (Type C3) mode. The corresponding clocking diagram is slightly
 * different to the illustration of Type C3.
 *
 *           .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-.
 *     SCK  -' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '---
 *
 *          -.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.-
 *     DOUT  |D15|D14|D13|D12|D11|D10| D9| D8| D7| D6| D5| D4| D3| D2| D1| D0|
 *          -'---'---'---'---'---'---'---'---'---'---'---'---'---'---'---'---'-
 *           | Word 1             (stuffing) :                        (byte) |
 *
 *          -.								     .-
 *     CS    '---------------------------------------------------------------'
 *
 *          -.---------------------------------------------------------------.-
 *     CD    |                              D/C                              |
 *          -'---------------------------------------------------------------'-
 */
#define MIPI_DBI_SPI_XFR_16BIT 16

/**
 * @}
 */


#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_MIPI_DBI_MIPI_DBI_H_ */
