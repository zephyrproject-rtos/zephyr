/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SPI_NOR_S28HX512T_H__
#define __SPI_NOR_S28HX512T_H__

#define S28HX512T_SPI_NOR_CMD_WR_WRARG 0x71 /* Write Any Register */
#define S28HX512T_SPI_NOR_CMD_RSFDPID  0x5A /* Read SFDP ID */
#define S28HX512T_SPI_NOR_CMD_RREG     0x65 /* Read Any Register */
#define S28HX512T_SPI_NOR_CMD_SE_256KB 0xDC /* Sector Erase 256KB */
#define S28HX512T_SPI_NOR_CMD_ERCHP    0x60 /* Erase Chip */

#define S28HX512T_SPI_NOR_OCMD_WEN      0x0606 /* Octal Write enable */
#define S28HX512T_SPI_NOR_OCMD_RSR      0x0505 /* Octal Read status register */
#define S28HX512T_SPI_NOR_OCMD_WR_REG2  0x7171 /* Octal Write config register 2 */
#define S28HX512T_SPI_NOR_OCMD_RDID     0x9F9F /* Octal Read JEDEC ID */
#define S28HX512T_SPI_NOR_OCMD_RSFDPID  0x5A5A /* Octal Read SFDP ID */
#define S28HX512T_SPI_NOR_OCMD_RREG     0x6565 /* Octal Read Any Register */
#define S28HX512T_SPI_NOR_OCMD_PP_4B    0x1212 /* Octal Page Program 4 Byte Address */
#define S28HX512T_SPI_NOR_OCMD_READ     0xEEEE /* Octal Read data */
#define S28HX512T_SPI_NOR_OCMD_RST_EN   0x6666 /* Octal Reset Enable */
#define S28HX512T_SPI_NOR_OCMD_RST_MEM  0x9999 /* Reset Memory */
#define S28HX512T_SPI_NOR_OCMD_SE_4KB   0x2121 /* Octal Sector Erase 4Kb address */
#define S28HX512T_SPI_NOR_OCMD_SE_256KB 0xDCDC /* Octal Sector Erase 256Kb address */
#define S28HX512T_SPI_NOR_OCMD_ERCHP    0x6060 /* Octal Erase Chip */

#define S28HX512T_SPI_NOR_DUMMY_WR              0U
#define S28HX512T_SPI_NOR_DUMMY_WR_OCTAL        0U
#define S28HX512T_SPI_NOR_DUMMY_RD_STATUS       0U
#define S28HX512T_SPI_NOR_DUMMY_RD_STATUS_OCTAL 4U
#define S28HX512T_SPI_NOR_DUMMY_RD_REG          1U
#define S28HX512T_SPI_NOR_DUMMY_RD_REG_OCTAL    4U
#define S28HX512T_SPI_NOR_DUMMY_RD_MEM          3U
#define S28HX512T_SPI_NOR_DUMMY_RD_MEM_OCTAL    10U
#define S28HX512T_SPI_NOR_DUMMY_RD_SFDP         8U
#define S28HX512T_SPI_NOR_DUMMY_RD_SFDP_OCTAL   8U

#define S28HX512T_SPI_NOR_CFR1V_ADDR 0x00800002
#define S28HX512T_SPI_NOR_CFR2V_ADDR 0x00800003
#define S28HX512T_SPI_NOR_CFR3V_ADDR 0x00800004
#define S28HX512T_SPI_NOR_CFR4V_ADDR 0x00800005
#define S28HX512T_SPI_NOR_CFR5V_ADDR 0x00800006

#endif /*__SPI_NOR_S28HX512T_H__*/
