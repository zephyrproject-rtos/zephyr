/*
 * Copyright (c) Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_XILINX_ZYNQ7000_COMMON_PINCTRL_SOC_H_
#define ZEPHYR_SOC_ARM_XILINX_ZYNQ7000_COMMON_PINCTRL_SOC_H_

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* MIO_PIN_xx SLCR register fields (from Xilinx UG585 v1.13, B.28 SLCR) */
#define MIO_PIN_DISABLE_RCVR_MASK BIT(13)
#define MIO_PIN_DISABLE_RCVR(val) FIELD_PREP(MIO_PIN_DISABLE_RCVR_MASK, val)

#define MIO_PIN_PULLUP_MASK       BIT(12)
#define MIO_PIN_PULLUP(val)       FIELD_PREP(MIO_PIN_PULLUP_MASK, val)

#define MIO_PIN_IO_TYPE_MASK      GENMASK(11, 9)
#define MIO_PIN_IO_TYPE(val)      FIELD_PREP(MIO_PIN_IO_TYPE_MASK, val)

#define MIO_PIN_SPEED_MASK        BIT(8)
#define MIO_PIN_SPEED(val)        FIELD_PREP(MIO_PIN_SPEED_MASK, val)

#define MIO_PIN_L3_SEL_MASK       GENMASK(7, 5)
#define MIO_PIN_L3_SEL(val)       FIELD_PREP(MIO_PIN_L3_SEL_MASK, val)

#define MIO_PIN_L2_SEL_MASK       GENMASK(4, 3)
#define MIO_PIN_L2_SEL(val)       FIELD_PREP(MIO_PIN_L2_SEL_MASK, val)

#define MIO_PIN_L1_SEL_MASK       BIT(2)
#define MIO_PIN_L1_SEL(val)       FIELD_PREP(MIO_PIN_L1_SEL_MASK, val)

#define MIO_PIN_L0_SEL_MASK       BIT(1)
#define MIO_PIN_L0_SEL(val)       FIELD_PREP(MIO_PIN_L0_SEL_MASK, val)

#define MIO_PIN_TRI_ENABLE_MASK   BIT(0)
#define MIO_PIN_TRI_ENABLE(val)   FIELD_PREP(MIO_PIN_TRI_ENABLE_MASK, val)

/* MIO_PIN_xx SLCR register L3..L0 multiplexing fields combined */
#define MIO_PIN_LX_SEL_MASK \
	(MIO_PIN_L3_SEL_MASK | MIO_PIN_L2_SEL_MASK | MIO_PIN_L1_SEL_MASK | MIO_PIN_L0_SEL_MASK)
#define MIO_PIN_LX_SEL(l3, l2, l1, l0) \
	(MIO_PIN_L3_SEL(l3) | MIO_PIN_L2_SEL(l2) | MIO_PIN_L1_SEL(l1) | MIO_PIN_L0_SEL(l0))

/* MIO pin function multiplexing (from Xilinx UG585 v1.13, B.28 SLCR) */
#define MIO_PIN_FUNCTION_ETHERNET0       MIO_PIN_LX_SEL(0x0, 0x0, 0x0, 0x1)
#define MIO_PIN_FUNCTION_ETHERNET1       MIO_PIN_LX_SEL(0x0, 0x0, 0x0, 0x1)
#define MIO_PIN_FUNCTION_MDIO0           MIO_PIN_LX_SEL(0x4, 0x0, 0x0, 0x0)
#define MIO_PIN_FUNCTION_MDIO1           MIO_PIN_LX_SEL(0x5, 0x0, 0x0, 0x0)
#define MIO_PIN_FUNCTION_QSPI0           MIO_PIN_LX_SEL(0x0, 0x0, 0x0, 0x1)
#define MIO_PIN_FUNCTION_QSPI1           MIO_PIN_LX_SEL(0x0, 0x0, 0x0, 0x1)
#define MIO_PIN_FUNCTION_QSPI_FBCLK      MIO_PIN_LX_SEL(0x0, 0x0, 0x0, 0x1)
#define MIO_PIN_FUNCTION_QSPI_CS1        MIO_PIN_LX_SEL(0x0, 0x0, 0x0, 0x1)
#define MIO_PIN_FUNCTION_SPI0            MIO_PIN_LX_SEL(0x5, 0x0, 0x0, 0x0)
#define MIO_PIN_FUNCTION_SPI0_SS         MIO_PIN_LX_SEL(0x5, 0x0, 0x0, 0x0)
#define MIO_PIN_FUNCTION_SPI1            MIO_PIN_LX_SEL(0x5, 0x0, 0x0, 0x0)
#define MIO_PIN_FUNCTION_SPI1_SS         MIO_PIN_LX_SEL(0x5, 0x0, 0x0, 0x0)
#define MIO_PIN_FUNCTION_SDIO0           MIO_PIN_LX_SEL(0x4, 0x0, 0x0, 0x0)
#define MIO_PIN_FUNCTION_SDIO0_PC        MIO_PIN_LX_SEL(0x0, 0x3, 0x0, 0x0)
#define MIO_PIN_FUNCTION_SDIO1           MIO_PIN_LX_SEL(0x4, 0x0, 0x0, 0x0)
#define MIO_PIN_FUNCTION_SDIO1_PC        MIO_PIN_LX_SEL(0x0, 0x3, 0x0, 0x0)
#define MIO_PIN_FUNCTION_SMC0_NOR        MIO_PIN_LX_SEL(0x0, 0x1, 0x0, 0x0)
#define MIO_PIN_FUNCTION_SMC0_NOR_CS1    MIO_PIN_LX_SEL(0x0, 0x2, 0x0, 0x0)
#define MIO_PIN_FUNCTION_SMC0_NOR_ADDR25 MIO_PIN_LX_SEL(0x0, 0x1, 0x0, 0x0)
#define MIO_PIN_FUNCTION_SMC0_NAND       MIO_PIN_LX_SEL(0x0, 0x2, 0x0, 0x0)
#define MIO_PIN_FUNCTION_CAN0            MIO_PIN_LX_SEL(0x1, 0x0, 0x0, 0x0)
#define MIO_PIN_FUNCTION_CAN1            MIO_PIN_LX_SEL(0x1, 0x0, 0x0, 0x0)
#define MIO_PIN_FUNCTION_UART0           MIO_PIN_LX_SEL(0x7, 0x0, 0x0, 0x0)
#define MIO_PIN_FUNCTION_UART1           MIO_PIN_LX_SEL(0x7, 0x0, 0x0, 0x0)
#define MIO_PIN_FUNCTION_I2C0            MIO_PIN_LX_SEL(0x2, 0x0, 0x0, 0x0)
#define MIO_PIN_FUNCTION_I2C1            MIO_PIN_LX_SEL(0x2, 0x0, 0x0, 0x0)
#define MIO_PIN_FUNCTION_TTC0            MIO_PIN_LX_SEL(0x6, 0x0, 0x0, 0x0)
#define MIO_PIN_FUNCTION_TTC1            MIO_PIN_LX_SEL(0x6, 0x0, 0x0, 0x0)
#define MIO_PIN_FUNCTION_SWDT0           MIO_PIN_LX_SEL(0x3, 0x0, 0x0, 0x0)
#define MIO_PIN_FUNCTION_GPIO0           MIO_PIN_LX_SEL(0x0, 0x0, 0x0, 0x0)
#define MIO_PIN_FUNCTION_USB0            MIO_PIN_LX_SEL(0x0, 0x0, 0x1, 0x0)
#define MIO_PIN_FUNCTION_USB1            MIO_PIN_LX_SEL(0x0, 0x0, 0x1, 0x0)

/* MIO SDIO CD/WP pin selection (from Xilinx UG585 v1.13, B.28 SLCR) */
#define MIO_PIN_SPECIAL_FUNCTION_SDIO0_CD 1
#define MIO_PIN_SPECIAL_OFFSET_SDIO0_CD   0x0130
#define MIO_PIN_SPECIAL_MASK_SDIO0_CD     GENMASK(21, 16)
#define MIO_PIN_SPECIAL_SHIFT_SDIO0_CD    16

#define MIO_PIN_SPECIAL_FUNCTION_SDIO0_WP 1
#define MIO_PIN_SPECIAL_OFFSET_SDIO0_WP   0x0130
#define MIO_PIN_SPECIAL_MASK_SDIO0_WP     GENMASK(5, 0)
#define MIO_PIN_SPECIAL_SHIFT_SDIO0_WP    0

#define MIO_PIN_SPECIAL_FUNCTION_SDIO1_CD 1
#define MIO_PIN_SPECIAL_OFFSET_SDIO1_CD   0x0134
#define MIO_PIN_SPECIAL_MASK_SDIO1_CD     GENMASK(21, 16)
#define MIO_PIN_SPECIAL_SHIFT_SDIO1_CD    16

#define MIO_PIN_SPECIAL_FUNCTION_SDIO1_WP 1
#define MIO_PIN_SPECIAL_OFFSET_SDIO1_WP   0x0134
#define MIO_PIN_SPECIAL_MASK_SDIO1_WP     GENMASK(5, 0)
#define MIO_PIN_SPECIAL_SHIFT_SDIO1_WP    0

/* MIO pin numbers */
#define MIO0   0
#define MIO1   1
#define MIO2   2
#define MIO3   3
#define MIO4   4
#define MIO5   5
#define MIO6   6
#define MIO7   7
#define MIO8   8
#define MIO9   9
#define MIO10 10
#define MIO11 11
#define MIO12 12
#define MIO13 13
#define MIO14 14
#define MIO15 15
#define MIO16 16
#define MIO17 17
#define MIO18 18
#define MIO19 19
#define MIO20 20
#define MIO21 21
#define MIO22 22
#define MIO23 23
#define MIO24 24
#define MIO25 25
#define MIO26 26
#define MIO27 27
#define MIO28 28
#define MIO29 29
#define MIO30 30
#define MIO31 31
#define MIO32 32
#define MIO33 33
#define MIO34 34
#define MIO35 35
#define MIO36 36
#define MIO37 37
#define MIO38 38
#define MIO39 39
#define MIO40 40
#define MIO41 41
#define MIO42 42
#define MIO43 43
#define MIO44 44
#define MIO45 45
#define MIO46 46
#define MIO47 47
#define MIO48 48
#define MIO49 49
#define MIO50 50
#define MIO51 51
#define MIO52 52
#define MIO53 53

/* MIO pin groups (from Xilinx UG585 v1.13, table 2-4 "MIO-at-a-Glance") */
#define MIO_GROUP_ETHERNET0_0_GRP_PINS     16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27
#define MIO_GROUP_ETHERNET1_0_GRP_PINS     28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39
#define MIO_GROUP_MDIO0_0_GRP_PINS         52, 53
#define MIO_GROUP_MDIO1_0_GRP_PINS         52, 53
#define MIO_GROUP_QSPI0_0_GRP_PINS          1,  2,  3,  4,  5,  6
#define MIO_GROUP_QSPI1_0_GRP_PINS          9, 10, 11, 12, 13
#define MIO_GROUP_QSPI_FBCLK_PINS           8
#define MIO_GROUP_QSPI_CS1_GRP_PINS         0
#define MIO_GROUP_SPI0_0_GRP_PINS          16, 17, 21
#define MIO_GROUP_SPI0_0_SS0_PINS          18
#define MIO_GROUP_SPI0_0_SS1_PINS          19
#define MIO_GROUP_SPI0_0_SS2_PINS          20
#define MIO_GROUP_SPI0_1_GRP_PINS          28, 29, 33
#define MIO_GROUP_SPI0_1_SS0_PINS          30
#define MIO_GROUP_SPI0_1_SS1_PINS          31
#define MIO_GROUP_SPI0_1_SS2_PINS          32
#define MIO_GROUP_SPI0_2_GRP_PINS          40, 41, 45
#define MIO_GROUP_SPI0_2_SS0_PINS          42
#define MIO_GROUP_SPI0_2_SS1_PINS          43
#define MIO_GROUP_SPI0_2_SS2_PINS          44
#define MIO_GROUP_SPI1_0_GRP_PINS          10, 11, 12
#define MIO_GROUP_SPI1_0_SS0_PINS          13
#define MIO_GROUP_SPI1_0_SS1_PINS          14
#define MIO_GROUP_SPI1_0_SS2_PINS          15
#define MIO_GROUP_SPI1_1_GRP_PINS          22, 23, 24
#define MIO_GROUP_SPI1_1_SS0_PINS          25
#define MIO_GROUP_SPI1_1_SS1_PINS          26
#define MIO_GROUP_SPI1_1_SS2_PINS          27
#define MIO_GROUP_SPI1_2_GRP_PINS          34, 35, 36
#define MIO_GROUP_SPI1_2_SS0_PINS          37
#define MIO_GROUP_SPI1_2_SS1_PINS          38
#define MIO_GROUP_SPI1_2_SS2_PINS          39
#define MIO_GROUP_SPI1_3_GRP_PINS          46, 47, 48
#define MIO_GROUP_SPI1_3_SS0_PINS          49
#define MIO_GROUP_SPI1_3_SS1_PINS          50
#define MIO_GROUP_SPI1_3_SS2_PINS          51
#define MIO_GROUP_SDIO0_0_GRP_PINS         16, 17, 18, 19, 20, 21
#define MIO_GROUP_SDIO0_1_GRP_PINS         28, 29, 30, 31, 32, 33
#define MIO_GROUP_SDIO0_2_GRP_PINS         40, 41, 42, 43, 44, 45
#define MIO_GROUP_SDIO1_0_GRP_PINS         10, 11, 12, 13, 14, 15
#define MIO_GROUP_SDIO1_1_GRP_PINS         22, 23, 24, 25, 26, 27
#define MIO_GROUP_SDIO1_2_GRP_PINS         34, 35, 36, 37, 38, 39
#define MIO_GROUP_SDIO1_3_GRP_PINS         46, 47, 48, 49, 50, 51
#define MIO_GROUP_SDIO0_EMIO_WP_PINS       54
#define MIO_GROUP_SDIO0_EMIO_CD_PINS       55
#define MIO_GROUP_SDIO1_EMIO_WP_PINS       56
#define MIO_GROUP_SDIO1_EMIO_CD_PINS       57
#define MIO_GROUP_SMC0_NOR_PINS             0,  3,  4,  5,  6,  7,  8,  9, 10, 11, 13, 15, \
					   16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, \
					   28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39
#define MIO_GROUP_SMC0_NOR_CS1_GRP_PINS     1
#define MIO_GROUP_SMC0_NOR_ADDR25_GRP_PINS  1
#define MIO_GROUP_SMC0_NAND_PINS            0,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, \
					   12, 13, 14, 16, 17, 18, 19, 20, 21, 22, 23
#define MIO_GROUP_CAN0_0_GRP_PINS          10, 11
#define MIO_GROUP_CAN0_1_GRP_PINS          14, 15
#define MIO_GROUP_CAN0_2_GRP_PINS          18, 19
#define MIO_GROUP_CAN0_3_GRP_PINS          22, 23
#define MIO_GROUP_CAN0_4_GRP_PINS          26, 27
#define MIO_GROUP_CAN0_5_GRP_PINS          30, 31
#define MIO_GROUP_CAN0_6_GRP_PINS          34, 35
#define MIO_GROUP_CAN0_7_GRP_PINS          38, 39
#define MIO_GROUP_CAN0_8_GRP_PINS          42, 43
#define MIO_GROUP_CAN0_9_GRP_PINS          46, 47
#define MIO_GROUP_CAN0_10_GRP_PINS         50, 51
#define MIO_GROUP_CAN1_0_GRP_PINS           8,  9
#define MIO_GROUP_CAN1_1_GRP_PINS          12, 13
#define MIO_GROUP_CAN1_2_GRP_PINS          16, 17
#define MIO_GROUP_CAN1_3_GRP_PINS          20, 21
#define MIO_GROUP_CAN1_4_GRP_PINS          24, 25
#define MIO_GROUP_CAN1_5_GRP_PINS          28, 29
#define MIO_GROUP_CAN1_6_GRP_PINS          32, 33
#define MIO_GROUP_CAN1_7_GRP_PINS          36, 37
#define MIO_GROUP_CAN1_8_GRP_PINS          40, 41
#define MIO_GROUP_CAN1_9_GRP_PINS          44, 45
#define MIO_GROUP_CAN1_10_GRP_PINS         48, 49
#define MIO_GROUP_CAN1_11_GRP_PINS         52, 53
#define MIO_GROUP_UART0_0_GRP_PINS         10, 11
#define MIO_GROUP_UART0_1_GRP_PINS         14, 15
#define MIO_GROUP_UART0_2_GRP_PINS         18, 19
#define MIO_GROUP_UART0_3_GRP_PINS         22, 23
#define MIO_GROUP_UART0_4_GRP_PINS         26, 27
#define MIO_GROUP_UART0_5_GRP_PINS         30, 31
#define MIO_GROUP_UART0_6_GRP_PINS         34, 35
#define MIO_GROUP_UART0_7_GRP_PINS         38, 39
#define MIO_GROUP_UART0_8_GRP_PINS         42, 43
#define MIO_GROUP_UART0_9_GRP_PINS         46, 47
#define MIO_GROUP_UART0_10_GRP_PINS        50, 51
#define MIO_GROUP_UART1_0_GRP_PINS          8,  9
#define MIO_GROUP_UART1_1_GRP_PINS         12, 13
#define MIO_GROUP_UART1_2_GRP_PINS         16, 17
#define MIO_GROUP_UART1_3_GRP_PINS         20, 21
#define MIO_GROUP_UART1_4_GRP_PINS         24, 25
#define MIO_GROUP_UART1_5_GRP_PINS         28, 29
#define MIO_GROUP_UART1_6_GRP_PINS         32, 33
#define MIO_GROUP_UART1_7_GRP_PINS         36, 37
#define MIO_GROUP_UART1_8_GRP_PINS         40, 41
#define MIO_GROUP_UART1_9_GRP_PINS         44, 45
#define MIO_GROUP_UART1_10_GRP_PINS        48, 49
#define MIO_GROUP_UART1_11_GRP_PINS        52, 53
#define MIO_GROUP_I2C0_0_GRP_PINS          10, 11
#define MIO_GROUP_I2C0_1_GRP_PINS          14, 15
#define MIO_GROUP_I2C0_2_GRP_PINS          18, 19
#define MIO_GROUP_I2C0_3_GRP_PINS          22, 23
#define MIO_GROUP_I2C0_4_GRP_PINS          26, 27
#define MIO_GROUP_I2C0_5_GRP_PINS          30, 31
#define MIO_GROUP_I2C0_6_GRP_PINS          34, 35
#define MIO_GROUP_I2C0_7_GRP_PINS          38, 39
#define MIO_GROUP_I2C0_8_GRP_PINS          42, 43
#define MIO_GROUP_I2C0_9_GRP_PINS          46, 47
#define MIO_GROUP_I2C0_10_GRP_PINS         50, 51
#define MIO_GROUP_I2C1_0_GRP_PINS          12, 13
#define MIO_GROUP_I2C1_1_GRP_PINS          16, 17
#define MIO_GROUP_I2C1_2_GRP_PINS          20, 21
#define MIO_GROUP_I2C1_3_GRP_PINS          24, 25
#define MIO_GROUP_I2C1_4_GRP_PINS          28, 29
#define MIO_GROUP_I2C1_5_GRP_PINS          32, 33
#define MIO_GROUP_I2C1_6_GRP_PINS          36, 37
#define MIO_GROUP_I2C1_7_GRP_PINS          40, 41
#define MIO_GROUP_I2C1_8_GRP_PINS          44, 45
#define MIO_GROUP_I2C1_9_GRP_PINS          48, 49
#define MIO_GROUP_I2C1_10_GRP_PINS         52, 53
#define MIO_GROUP_TTC0_0_GRP_PINS          18, 19
#define MIO_GROUP_TTC0_1_GRP_PINS          30, 31
#define MIO_GROUP_TTC0_2_GRP_PINS          42, 43
#define MIO_GROUP_TTC1_0_GRP_PINS          16, 17
#define MIO_GROUP_TTC1_1_GRP_PINS          28, 29
#define MIO_GROUP_TTC1_2_GRP_PINS          40, 41
#define MIO_GROUP_SWDT0_0_GRP_PINS         14, 15
#define MIO_GROUP_SWDT0_1_GRP_PINS         26, 27
#define MIO_GROUP_SWDT0_2_GRP_PINS         38, 39
#define MIO_GROUP_SWDT0_3_GRP_PINS         50, 51
#define MIO_GROUP_SWDT0_4_GRP_PINS         52, 53
#define MIO_GROUP_GPIO0_0_GRP_PINS          0
#define MIO_GROUP_GPIO0_1_GRP_PINS          1
#define MIO_GROUP_GPIO0_2_GRP_PINS          2
#define MIO_GROUP_GPIO0_3_GRP_PINS          3
#define MIO_GROUP_GPIO0_4_GRP_PINS          4
#define MIO_GROUP_GPIO0_5_GRP_PINS          5
#define MIO_GROUP_GPIO0_6_GRP_PINS          6
#define MIO_GROUP_GPIO0_7_GRP_PINS          7
#define MIO_GROUP_GPIO0_8_GRP_PINS          8
#define MIO_GROUP_GPIO0_9_GRP_PINS          9
#define MIO_GROUP_GPIO0_10_GRP_PINS        10
#define MIO_GROUP_GPIO0_11_GRP_PINS        11
#define MIO_GROUP_GPIO0_12_GRP_PINS        12
#define MIO_GROUP_GPIO0_13_GRP_PINS        13
#define MIO_GROUP_GPIO0_14_GRP_PINS        14
#define MIO_GROUP_GPIO0_15_GRP_PINS        15
#define MIO_GROUP_GPIO0_16_GRP_PINS        16
#define MIO_GROUP_GPIO0_17_GRP_PINS        17
#define MIO_GROUP_GPIO0_18_GRP_PINS        18
#define MIO_GROUP_GPIO0_19_GRP_PINS        19
#define MIO_GROUP_GPIO0_20_GRP_PINS        20
#define MIO_GROUP_GPIO0_21_GRP_PINS        21
#define MIO_GROUP_GPIO0_22_GRP_PINS        22
#define MIO_GROUP_GPIO0_23_GRP_PINS        23
#define MIO_GROUP_GPIO0_24_GRP_PINS        24
#define MIO_GROUP_GPIO0_25_GRP_PINS        25
#define MIO_GROUP_GPIO0_26_GRP_PINS        26
#define MIO_GROUP_GPIO0_27_GRP_PINS        27
#define MIO_GROUP_GPIO0_28_GRP_PINS        28
#define MIO_GROUP_GPIO0_29_GRP_PINS        29
#define MIO_GROUP_GPIO0_30_GRP_PINS        30
#define MIO_GROUP_GPIO0_31_GRP_PINS        31
#define MIO_GROUP_GPIO0_32_GRP_PINS        32
#define MIO_GROUP_GPIO0_33_GRP_PINS        33
#define MIO_GROUP_GPIO0_34_GRP_PINS        34
#define MIO_GROUP_GPIO0_35_GRP_PINS        35
#define MIO_GROUP_GPIO0_36_GRP_PINS        36
#define MIO_GROUP_GPIO0_37_GRP_PINS        37
#define MIO_GROUP_GPIO0_38_GRP_PINS        38
#define MIO_GROUP_GPIO0_39_GRP_PINS        39
#define MIO_GROUP_GPIO0_40_GRP_PINS        40
#define MIO_GROUP_GPIO0_41_GRP_PINS        41
#define MIO_GROUP_GPIO0_42_GRP_PINS        42
#define MIO_GROUP_GPIO0_43_GRP_PINS        43
#define MIO_GROUP_GPIO0_44_GRP_PINS        44
#define MIO_GROUP_GPIO0_45_GRP_PINS        45
#define MIO_GROUP_GPIO0_46_GRP_PINS        46
#define MIO_GROUP_GPIO0_47_GRP_PINS        47
#define MIO_GROUP_GPIO0_48_GRP_PINS        48
#define MIO_GROUP_GPIO0_49_GRP_PINS        49
#define MIO_GROUP_GPIO0_50_GRP_PINS        50
#define MIO_GROUP_GPIO0_51_GRP_PINS        51
#define MIO_GROUP_GPIO0_52_GRP_PINS        52
#define MIO_GROUP_GPIO0_53_GRP_PINS        53
#define MIO_GROUP_USB0_0_GRP_PINS          28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39
#define MIO_GROUP_USB1_0_GRP_PINS          40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51

typedef struct {
	uint32_t mask;
	uint32_t val;
	uint16_t offset;
} pinctrl_soc_pin_t;

/* Iterate over each pinctrl-n phandle child */
#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)			\
	{DT_FOREACH_CHILD(DT_PHANDLE(node_id, prop),			\
		Z_PINCTRL_STATE_PIN_CHILD_INIT)};

/*
 * If child has groups property:
 *   - Iterate over each pin in group and populate pinctrl_soc_pin_t
 * If child has pins property:
 *   - Iterate over each pin in pins and populate pinctrl_soc_pin_t
 */
#define Z_PINCTRL_STATE_PIN_CHILD_INIT(node_id)				\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, groups),			\
		(DT_FOREACH_PROP_ELEM(node_id, groups, Z_PINCTRL_STATE_PIN_CHILD_GROUP_INIT)), \
		())							\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, pins),			\
		(DT_FOREACH_PROP_ELEM(node_id, pins, Z_PINCTRL_STATE_PIN_CHILD_PIN_INIT)), \
		())

/* Process each pin in group using MIO_GROUP_x_PINS macros defined above */
#define Z_PINCTRL_STATE_PIN_CHILD_GROUP_INIT(node_id, prop, idx)	\
	FOR_EACH_FIXED_ARG(Z_PINCTRL_STATE_PIN_CHILD_GROUP_PIN_INIT, (), node_id, \
		UTIL_CAT(UTIL_CAT(MIO_GROUP_,				\
				  DT_STRING_UPPER_TOKEN_BY_IDX(node_id, prop, idx)), _PINS))

/* Reverse order of arguments to adapt between FOR_EACH_FIXED_ARG() and DT_FOREACH_PROP_ELEM() */
#define Z_PINCTRL_STATE_PIN_CHILD_GROUP_PIN_INIT(pin, node_id)		\
	Z_PINCTRL_STATE_PIN_INIT(node_id, pin)

/* Process pin using MIOx macros defines above */
#define Z_PINCTRL_STATE_PIN_CHILD_PIN_INIT(node_id, prop, idx)		\
	Z_PINCTRL_STATE_PIN_INIT(node_id, DT_STRING_UPPER_TOKEN_BY_IDX(node_id, prop, idx))

/* Process pin functions and special functions (CD, WP) */
#define Z_PINCTRL_STATE_PIN_INIT(node_id, pin)				\
	COND_CODE_1(Z_PINCTRL_STATE_PIN_IS_SPECIAL_FUNCTION(node_id),	\
		(Z_PINCTRL_STATE_PIN_SPECIAL_INIT(node_id, pin)),	\
		(Z_PINCTRL_STATE_PIN_FUNCTION_INIT(node_id, pin)))

/* Determine if pin has special function */
#define Z_PINCTRL_STATE_PIN_IS_SPECIAL_FUNCTION(node_id)		\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, function),		\
		(UTIL_CAT(MIO_PIN_SPECIAL_FUNCTION_, DT_STRING_UPPER_TOKEN(node_id, function))), \
		(0))

/* Populate pinctrl_soc_pin_t for each special function pin */
#define Z_PINCTRL_STATE_PIN_SPECIAL_INIT(node_id, pin)			\
	{								\
		.mask = UTIL_CAT(MIO_PIN_SPECIAL_MASK_,			\
				DT_STRING_UPPER_TOKEN(node_id, function)), \
		.val = pin << UTIL_CAT(MIO_PIN_SPECIAL_SHIFT_,		\
				DT_STRING_UPPER_TOKEN(node_id, function)), \
		.offset = UTIL_CAT(MIO_PIN_SPECIAL_OFFSET_,		\
				DT_STRING_UPPER_TOKEN(node_id, function)), \
	},

/* Populate pinctrl_soc_pin_t for each pin */
#define Z_PINCTRL_STATE_PIN_FUNCTION_INIT(node_id, pin)		\
	{							\
		.mask = Z_PINCTRL_STATE_PIN_MASK(node_id),	\
		.val = Z_PINCTRL_STATE_PIN_VAL(node_id),	\
		.offset = pin * sizeof(uint32_t),		\
	},

#define Z_PINCTRL_STATE_PIN_MASK(node_id) \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, function), (MIO_PIN_LX_SEL_MASK), (0U)) | \
	COND_CODE_1(DT_PROP(node_id, bias_disable),			\
		(MIO_PIN_PULLUP_MASK | MIO_PIN_TRI_ENABLE_MASK), (0U)) | \
	COND_CODE_1(DT_PROP(node_id, low_power_disable), (MIO_PIN_DISABLE_RCVR_MASK), (0U)) | \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, power_source), (MIO_PIN_IO_TYPE_MASK), (0U)) | \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, slew_rate), (MIO_PIN_SPEED_MASK), (0U))

#define Z_PINCTRL_STATE_PIN_VAL(node_id) \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, function),		\
		(UTIL_CAT(MIO_PIN_FUNCTION_, DT_STRING_UPPER_TOKEN(node_id, function))), (0U)) | \
	MIO_PIN_TRI_ENABLE(DT_PROP(node_id, bias_high_impedance)) | \
	MIO_PIN_PULLUP(DT_PROP(node_id, bias_pull_up)) | \
	MIO_PIN_DISABLE_RCVR(DT_PROP(node_id, low_power_enable)) | \
	MIO_PIN_IO_TYPE(DT_PROP_OR(node_id, power_source, (0U))) | \
	MIO_PIN_SPEED(DT_PROP_OR(node_id, slew_rate, (0U)))

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_ARM_XILINX_ZYNQ7000_COMMON_PINCTRL_SOC_H_ */
