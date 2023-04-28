/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SPD_DEV_DESCRIPTION		0
#define SPD_REVISION			1
#define SPD_MEMORY_TYPE			2
#define SPD_MODULE_TYPE			3

#define SPD_MODULE_ORGANIZATION		12
#define SPD_MODULE_MEMORY_BUS_WIDTH	13

/**
 * Memory Type Definitions
 */
#define SPD_TYPE_SDR			4 /* SDR SDRAM memory */
#define SPD_TYPE_DDR			7 /* DDR SDRAM memory */
#define SPD_TYPE_DDR2			8 /* DDR2 SDRAM memory */
#define SPD_TYPE_DDR3			11 /* DDR3 SDRAM memory */
#define SPD_TYPE_DDR4			12 /* DDR4 SDRAM memory */
#define SPD_TYPE_LPDDR3			15 /* LPDDR3 SDRAM memory */
#define SPD_TYPE_LPDDR4			16 /* LPDDR4 SDRAM memory */
#define SPD_TYPE_DDR5			18 /* DDR5 SDRAM memory */

/* SPD4 definitions */
#define SPD4_CRC16_LSB			126
#define SPD4_CRC16_MSB			127

/* SPD5 definitions */
#define SPD5_CRC16_LSB			510
#define SPD5_CRC16_MSB			511

/* Module Manufacturer ID Code First and Second bytes */
#define SPD5_MANUFACTURER_ID_1		512
#define SPD5_MANUFACTURER_ID_2		513
