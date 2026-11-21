/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 KylinSoft Corporation
 * SPDX-License-Identifier: Apache-2.0
 *
 * RK3588 CRU reset identifiers: SOFTRST_CON register index * 16 + bit. The index counts
 * 32-bit registers from the CRU SOFTRST_CON0, so the PMU1CRU ones, 0x30000 above it,
 * start at index 0xc000.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_RK3588_CRU_RESET_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_RK3588_CRU_RESET_H_

#define SRST_P_SPI0  230
#define SRST_P_SPI1  231
#define SRST_P_SPI2  232
#define SRST_P_SPI3  233
#define SRST_P_SPI4  234
#define SRST_SPI0    235
#define SRST_SPI1    236
#define SRST_SPI2    237
#define SRST_SPI3    238
#define SRST_SPI4    239
#define SRST_H_EMMC  500
#define SRST_A_EMMC  501
#define SRST_C_EMMC  502
#define SRST_B_EMMC  503
#define SRST_T_EMMC  504
#define SRST_A_GMAC0 522
#define SRST_A_GMAC1 523

/* USB 3.0 OTG0 controller and its USB 2.0 PHY */
#define SRST_A_USB3OTG0          676
#define SRST_P_USB2PHY_U3_0_GRF0 1160
#define SRST_OTGPHY_U3_0         786503 /* PMU1CRU SOFTRST_CON04 bit 7 */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RESET_RK3588_CRU_RESET_H_ */
