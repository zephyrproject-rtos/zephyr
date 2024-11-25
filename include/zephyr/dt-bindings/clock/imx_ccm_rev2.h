/*
 * Copyright 2021,2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_IMX_CCM_REV2_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_IMX_CCM_REV2_H_

/* Peripheral:
 *   range: 0 - 0xFF, starting from 0
 *
 * Instance:
 *   range: 0 - 0xFF, starting from 0
 */
#define IMX_CCM_PERIPHERAL_MASK        0xFF00UL
#define IMX_CCM_INSTANCE_MASK          0xFFUL

#define IMX_CCM_CORESYS_CLK            0
#define IMX_CCM_PLATFORM_CLK           0x1UL
#define IMX_CCM_BUS_CLK                0x2UL

/* LPUART */
#define IMX_CCM_LPUART_CLK             0x300UL
#define IMX_CCM_LPUART1_CLK            0x300UL
#define IMX_CCM_LPUART0102_CLK         0x300UL
#define IMX_CCM_LPUART2_CLK            0x301UL
#define IMX_CCM_LPUART0304_CLK         0x301UL
#define IMX_CCM_LPUART3_CLK            0x302UL
#define IMX_CCM_LPUART0506_CLK         0x302UL
#define IMX_CCM_LPUART4_CLK            0x303UL
#define IMX_CCM_LPUART0708_CLK         0x303UL
#define IMX_CCM_LPUART5_CLK            0x304UL
#define IMX_CCM_LPUART0910_CLK         0x304UL
#define IMX_CCM_LPUART6_CLK            0x305UL
#define IMX_CCM_LPUART1112_CLK         0x305UL
#define IMX_CCM_LPUART7_CLK            0x306UL
#define IMX_CCM_LPUART8_CLK            0x307UL
#define IMX_CCM_LPUART9_CLK            0x308UL
#define IMX_CCM_LPUART10_CLK           0x309UL
#define IMX_CCM_LPUART11_CLK           0x30aUL
#define IMX_CCM_LPUART12_CLK           0x30bUL

/* LPI2C */
#define IMX_CCM_LPI2C_CLK              0x400UL
#define IMX_CCM_LPI2C0102_CLK          0x400UL
#define IMX_CCM_LPI2C1_CLK             0x400UL
#define IMX_CCM_LPI2C2_CLK             0x401UL
#define IMX_CCM_LPI2C0304_CLK          0x401UL
#define IMX_CCM_LPI2C3_CLK             0x402UL
#define IMX_CCM_LPI2C4_CLK             0x403UL
#define IMX_CCM_LPI2C0506_CLK          0x402UL
#define IMX_CCM_LPI2C5_CLK             0x404UL
#define IMX_CCM_LPI2C6_CLK             0x405UL
#define IMX_CCM_LPI2C0708_CLK          0x403UL
#define IMX_CCM_LPI2C7_CLK             0x406UL
#define IMX_CCM_LPI2C8_CLK             0x407UL

/* LPSPI */
#define IMX_CCM_LPSPI_CLK              0x500UL
#define IMX_CCM_LPSPI1_CLK             0x500UL
#define IMX_CCM_LPSPI2_CLK             0x501UL
#define IMX_CCM_LPSPI3_CLK             0x502UL
#define IMX_CCM_LPSPI4_CLK             0x503UL
#define IMX_CCM_LPSPI5_CLK             0x504UL
#define IMX_CCM_LPSPI6_CLK             0x505UL
#define IMX_CCM_LPSPI7_CLK             0x506UL
#define IMX_CCM_LPSPI8_CLK             0x507UL

/* USDHC */
#define IMX_CCM_USDHC1_CLK             0x600UL
#define IMX_CCM_USDHC2_CLK             0x601UL

/* DMA  */
#define IMX_CCM_EDMA_CLK               0x700UL
#define IMX_CCM_EDMA_LPSR_CLK          0x701UL

/* PWM */
#define IMX_CCM_PWM_CLK                0x800UL

/* CAN */
#define IMX_CCM_CAN_CLK                0x900UL
#define IMX_CCM_CAN1_CLK               0x900UL
#define IMX_CCM_CAN2_CLK               0x901UL
#define IMX_CCM_CAN3_CLK               0x902UL

/* GPT */
#define IMX_CCM_GPT_CLK                0x1000UL
#define IMX_CCM_GPT1_CLK               0x1000UL
#define IMX_CCM_GPT2_CLK               0x1001UL
#define IMX_CCM_GPT3_CLK               0x1002UL
#define IMX_CCM_GPT4_CLK               0x1003UL
#define IMX_CCM_GPT5_CLK               0x1004UL
#define IMX_CCM_GPT6_CLK               0x1005UL

/* SAI */
#define IMX_CCM_SAI1_CLK               0x1100UL
#define IMX_CCM_SAI2_CLK               0x1101UL
#define IMX_CCM_SAI3_CLK               0x1102UL
#define IMX_CCM_SAI4_CLK               0x1103UL

/* ENET */
#define IMX_CCM_ENET_CLK               0x1200UL
#define IMX_CCM_ENET_PLL               0x1201UL
#define IMX_CCM_ENET1G_CLK             0x1202UL
#define IMX_CCM_ENET1G_PLL             0x1203UL

/* FLEXSPI */
#define IMX_CCM_FLEXSPI_CLK            0x1300UL
#define IMX_CCM_FLEXSPI2_CLK           0x1301UL

/* PIT */
#define IMX_CCM_PIT_CLK                0x1400UL
#define IMX_CCM_PIT1_CLK               0x1401UL

/* ADC */
#define IMX_CCM_LPADC1_CLK             0x1500UL
#define IMX_CCM_LPADC2_CLK             0x1501UL

/* TPM */
#define IMX_CCM_TPM_CLK                0x1600UL
#define IMX_CCM_TPM1_CLK               0x1600UL
#define IMX_CCM_TPM2_CLK               0x1601UL
#define IMX_CCM_TPM3_CLK               0x1602UL
#define IMX_CCM_TPM4_CLK               0x1603UL
#define IMX_CCM_TPM5_CLK               0x1604UL
#define IMX_CCM_TPM6_CLK               0x1605UL

/* FLEXIO */
#define IMX_CCM_FLEXIO_CLK             0x1700UL
#define IMX_CCM_FLEXIO1_CLK            0x1700UL
#define IMX_CCM_FLEXIO2_CLK            0x1701UL

/* NETC */
#define IMX_CCM_NETC_CLK 0x1800UL

/* MIPI CSI2RX */
#define IMX_CCM_MIPI_CSI2RX_ROOT_CLK 0x1900UL
#define IMX_CCM_MIPI_CSI2RX_UI_CLK   0x2000UL
#define IMX_CCM_MIPI_CSI2RX_ESC_CLK  0x2100UL

/* I3C */
#define IMX_CCM_I3C_CLK                0x2200UL
#define IMX_CCM_I3C1_CLK               0x2200UL
#define IMX_CCM_I3C2_CLK               0x2201UL

/* QTMR */
#define IMX_CCM_QTMR_CLK               0x6000UL
#define IMX_CCM_QTMR1_CLK              0x6000UL
#define IMX_CCM_QTMR2_CLK              0x6001UL
#define IMX_CCM_QTMR3_CLK              0x6002UL
#define IMX_CCM_QTMR4_CLK              0x6003UL

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_IMX_CCM_REV2_H_ */
