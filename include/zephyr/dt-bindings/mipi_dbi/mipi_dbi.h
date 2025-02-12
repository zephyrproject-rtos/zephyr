/*
 * Copyright 2024 NXP
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
 *           .---.   .---.   .---.   .---.   .---.   .---.   .---.   .---.
 *     SCK  -'   '---'   '---'   '---'   '---'   '---'   '---'   '---'   '---
 *
 *          -.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.
 *     DOUT  |D/C| D7| D6| D5| D4| D3| D2| D1| D0|D/C| D7| D6| D5| D4|...|
 *          -'---'---'---'---'---'---'---'---'---'---'---'---'---'---'---'
 *           | Word 1                            | Word n
 *
 *          -.								 .--
 *     CS    '-----------------------------------------------------------'
 */
#define MIPI_DBI_MODE_SPI_3WIRE 0x1
/**
 * SPI 4 wire (Type C3). Uses 8 write clocks to send a byte of data.
 * an additional C/D pin will be use to indicate whether the byte is a
 * command or data byte
 *
 *           .---.   .---.   .---.   .---.   .---.   .---.   .---.   .---.
 *     SCK  -'   '---'   '---'   '---'   '---'   '---'   '---'   '---'   '---
 *
 *          -.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.
 *     DOUT  | D7| D6| D5| D4| D3| D2| D1| D0| D7| D6| D5| D4| D3| D2| D1| D0|
 *          -'---'---'---'---'---'---'---'---'---'---'---'---'---'---'---'---'
 *           | Word 1                        | Word n
 *
 *          -.								     .--
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

/**
 * @}
 */


#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_MIPI_DBI_MIPI_DBI_H_ */
