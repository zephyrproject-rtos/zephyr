/*
 * Copyright (c) 2021, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_PCA9420_I2C_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_PCA9420_I2C_H_

/*
 * Voltage Ranges should be defined without commas, spaces, or brackets. These
 * ranges will be inserted directly into the devicetree as arrays, so that the
 * driver can parse them.
 */
#define PCA9420_SW1_VOLTAGE_RANGE \
	500000 0x00		/* SW1 output voltage 0.500V. */\
	525000 0x01		/* SW1 output voltage 0.525V. */\
	550000 0x02		/* SW1 output voltage 0.550V. */\
	575000 0x03		/* SW1 output voltage 0.575V. */\
	600000 0x04		/* SW1 output voltage 0.600V. */\
	625000 0x05		/* SW1 output voltage 0.625V. */\
	650000 0x06		/* SW1 output voltage 0.650V. */\
	675000 0x07		/* SW1 output voltage 0.675V. */\
	700000 0x08		/* SW1 output voltage 0.700V. */\
	725000 0x09		/* SW1 output voltage 0.725V. */\
	750000 0x0A		/* SW1 output voltage 0.750V. */\
	775000 0x0B		/* SW1 output voltage 0.775V. */\
	800000 0x0C		/* SW1 output voltage 0.800V. */\
	825000 0x0D		/* SW1 output voltage 0.825V. */\
	850000 0x0E		/* SW1 output voltage 0.850V. */\
	875000 0x0F		/* SW1 output voltage 0.875V. */\
	900000 0x10		/* SW1 output voltage 0.900V. */\
	925000 0x11		/* SW1 output voltage 0.925V. */\
	950000 0x12		/* SW1 output voltage 0.950V. */\
	975000 0x13		/* SW1 output voltage 0.975V. */\
	1000000 0x14		/* SW1 output voltage 1.000V. */\
	1025000 0x15		/* SW1 output voltage 1.025V. */\
	1050000 0x16		/* SW1 output voltage 1.050V. */\
	1075000 0x17		/* SW1 output voltage 1.075V. */\
	1100000 0x18		/* SW1 output voltage 1.100V. */\
	1125000 0x19		/* SW1 output voltage 1.125V. */\
	1150000 0x1A		/* SW1 output voltage 1.150V. */\
	1175000 0x1B		/* SW1 output voltage 1.175V. */\
	1200000 0x1C		/* SW1 output voltage 1.200V. */\
	1225000 0x1D		/* SW1 output voltage 1.225V. */\
	1250000 0x1E		/* SW1 output voltage 1.250V. */\
	1275000 0x1F		/* SW1 output voltage 1.275V. */\
	1300000 0x20		/* SW1 output voltage 1.300V. */\
	1325000 0x21		/* SW1 output voltage 1.325V. */\
	1350000 0x22		/* SW1 output voltage 1.350V. */\
	1375000 0x23		/* SW1 output voltage 1.375V. */\
	1400000 0x24		/* SW1 output voltage 1.400V. */\
	1425000 0x25		/* SW1 output voltage 1.425V. */\
	1450000 0x26		/* SW1 output voltage 1.450V. */\
	1475000 0x27		/* SW1 output voltage 1.475V. */\
	1500000 0x28		/* SW1 output voltage 1.500V. */\
	1800000 0x3F		/* SW1 output voltage 1.800V. */\

#define PCA9420_SW2_VOLTAGE_RANGE \
	1500000 0x00		/* SW2 output voltage 1.500V. */\
	1525000 0x01		/* SW2 output voltage 1.525V. */\
	1550000 0x02		/* SW2 output voltage 1.550V. */\
	1575000 0x03		/* SW2 output voltage 1.575V. */\
	1600000 0x04		/* SW2 output voltage 1.600V. */\
	1625000 0x05		/* SW2 output voltage 1.625V. */\
	1650000 0x06		/* SW2 output voltage 1.650V. */\
	1675000 0x07		/* SW2 output voltage 1.675V. */\
	1700000 0x08		/* SW2 output voltage 1.700V. */\
	1725000 0x09		/* SW2 output voltage 1.725V. */\
	1750000 0x0A		/* SW2 output voltage 1.750V. */\
	1775000 0x0B		/* SW2 output voltage 1.775V. */\
	1800000 0x0C		/* SW2 output voltage 1.800V. */\
	1825000 0x0D		/* SW2 output voltage 1.825V. */\
	1850000 0x0E		/* SW2 output voltage 1.850V. */\
	1875000 0x0F		/* SW2 output voltage 1.875V. */\
	1900000 0x10		/* SW2 output voltage 1.900V. */\
	1925000 0x11		/* SW2 output voltage 1.925V. */\
	1950000 0x12		/* SW2 output voltage 1.950V. */\
	1975000 0x13		/* SW2 output voltage 1.975V. */\
	2000000 0x14		/* SW2 output voltage 2.000V. */\
	2025000 0x15		/* SW2 output voltage 2.025V. */\
	2050000 0x16		/* SW2 output voltage 2.050V. */\
	2075000 0x17		/* SW2 output voltage 2.075V. */\
	2100000 0x18		/* SW2 output voltage 2.100V. */\
	2700000 0x20		/* SW2 output voltage 2.700V. */\
	2725000 0x21		/* SW2 output voltage 2.725V. */\
	2750000 0x22		/* SW2 output voltage 2.750V. */\
	2775000 0x23		/* SW2 output voltage 2.775V. */\
	2800000 0x24		/* SW2 output voltage 2.800V. */\
	2825000 0x25		/* SW2 output voltage 2.825V. */\
	2850000 0x26		/* SW2 output voltage 2.850V. */\
	2875000 0x27		/* SW2 output voltage 2.875V. */\
	2900000 0x28		/* SW2 output voltage 2.900V. */\
	2925000 0x29		/* SW2 output voltage 2.925V. */\
	2950000 0x2A		/* SW2 output voltage 2.950V. */\
	2975000 0x2B		/* SW2 output voltage 2.975V. */\
	3000000 0x2C		/* SW2 output voltage 3.000V. */\
	3025000 0x2D		/* SW2 output voltage 3.025V. */\
	3050000 0x2E		/* SW2 output voltage 3.050V. */\
	3075000 0x2F		/* SW2 output voltage 3.075V. */\
	3100000 0x30		/* SW2 output voltage 3.100V. */\
	3125000 0x31		/* SW2 output voltage 3.125V. */\
	3150000 0x32		/* SW2 output voltage 3.150V. */\
	3175000 0x33		/* SW2 output voltage 3.175V. */\
	3200000 0x34		/* SW2 output voltage 3.200V. */\
	3225000 0x35		/* SW2 output voltage 3.225V. */\
	3250000 0x36		/* SW2 output voltage 3.250V. */\
	3275000 0x37		/* SW2 output voltage 3.275V. */\
	3300000 0x38		/* SW2 output voltage 3.300V. */\

#define PCA9420_LDO1_VOLTAGE_RANGE \
	1700000 0x00		/* LDO1 output voltage 1.700V. */\
	1725000 0x10		/* LDO1 output voltage 1.725V. */\
	1750000 0x20		/* LDO1 output voltage 1.750V. */\
	1775000 0x30		/* LDO1 output voltage 1.775V. */\
	1800000 0x40		/* LDO1 output voltage 1.800V. */\
	1825000 0x50		/* LDO1 output voltage 1.825V. */\
	1850000 0x60		/* LDO1 output voltage 1.850V. */\
	1875000 0x70		/* LDO1 output voltage 1.875V. */\
	1900000 0x80		/* LDO1 output voltage 1.900V. */\

#define PCA9420_LDO2_VOLTAGE_RANGE \
	1500000 0x00		/* LDO2 output voltage 1.500V. */\
	1525000 0x01		/* LDO2 output voltage 1.525V. */\
	1550000 0x02		/* LDO2 output voltage 1.550V. */\
	1575000 0x03		/* LDO2 output voltage 1.575V. */\
	1600000 0x04		/* LDO2 output voltage 1.600V. */\
	1625000 0x05		/* LDO2 output voltage 1.625V. */\
	1650000 0x06		/* LDO2 output voltage 1.650V. */\
	1675000 0x07		/* LDO2 output voltage 1.675V. */\
	1700000 0x08		/* LDO2 output voltage 1.700V. */\
	1725000 0x09		/* LDO2 output voltage 1.725V. */\
	1750000 0x0A		/* LDO2 output voltage 1.750V. */\
	1775000 0x0B		/* LDO2 output voltage 1.775V. */\
	1800000 0x0C		/* LDO2 output voltage 1.800V. */\
	1825000 0x0D		/* LDO2 output voltage 1.825V. */\
	1850000 0x0E		/* LDO2 output voltage 1.850V. */\
	1875000 0x0F		/* LDO2 output voltage 1.875V. */\
	1900000 0x10		/* LDO2 output voltage 1.900V. */\
	1925000 0x11		/* LDO2 output voltage 1.925V. */\
	1950000 0x12		/* LDO2 output voltage 1.950V. */\
	1975000 0x13		/* LDO2 output voltage 1.975V. */\
	2000000 0x14		/* LDO2 output voltage 2.000V. */\
	2025000 0x15		/* LDO2 output voltage 2.025V. */\
	2050000 0x16		/* LDO2 output voltage 2.050V. */\
	2075000 0x17		/* LDO2 output voltage 2.075V. */\
	2100000 0x18		/* LDO2 output voltage 2.100V. */\
	2700000 0x20		/* LDO2 output voltage 2.700V. */\
	2725000 0x21		/* LDO2 output voltage 2.725V. */\
	2750000 0x22		/* LDO2 output voltage 2.750V. */\
	2775000 0x23		/* LDO2 output voltage 2.775V. */\
	2800000 0x24		/* LDO2 output voltage 2.800V. */\
	2825000 0x25		/* LDO2 output voltage 2.825V. */\
	2850000 0x26		/* LDO2 output voltage 2.850V. */\
	2875000 0x27		/* LDO2 output voltage 2.875V. */\
	2900000 0x28		/* LDO2 output voltage 2.900V. */\
	2925000 0x29		/* LDO2 output voltage 2.925V. */\
	2950000 0x2A		/* LDO2 output voltage 2.950V. */\
	2975000 0x2B		/* LDO2 output voltage 2.975V. */\
	3000000 0x2C		/* LDO2 output voltage 3.000V. */\
	3025000 0x2D		/* LDO2 output voltage 3.025V. */\
	3050000 0x2E		/* LDO2 output voltage 3.050V. */\
	3075000 0x2F		/* LDO2 output voltage 3.075V. */\
	3100000 0x30		/* LDO2 output voltage 3.100V. */\
	3125000 0x31		/* LDO2 output voltage 3.125V. */\
	3150000 0x32		/* LDO2 output voltage 3.150V. */\
	3175000 0x33		/* LDO2 output voltage 3.175V. */\
	3200000 0x34		/* LDO2 output voltage 3.200V. */\
	3225000 0x35		/* LDO2 output voltage 3.225V. */\
	3250000 0x36		/* LDO2 output voltage 3.250V. */\
	3275000 0x37		/* LDO2 output voltage 3.275V. */\
	3300000 0x38		/* LDO2 output voltage 3.300V. */\

#define PCA9420_CURRENT_LIMIT_LEVELS \
	85000 0x00		/* min: 74mA, typ: 85mA, max: 98mA */\
	225000 0x20		/* min: 222mA, typ: 225mA, max: 293mA */\
	425000 0x40		/* min: 370mA, typ: 425mA, max: 489mA */\
	595000 0x60		/* min: 517mA, typ: 595mA, max: 684mA */\
	765000 0x80		/* min: 665mA, typ: 765mA, max: 880mA */\
	935000 0xA0		/* min: 813mA, typ: 935mA, max: 1075mA */\
	1105000 0xC0		/* min: 961mA, typ: 1105mA, max: 1271mA */\



/** Register memory map. See datasheet for more details. */
/** General purpose registers */
/** @brief Device Information register */
#define PCA9420_DEV_INFO      0x00U
/** @brief Top level interrupt status */
#define PCA9420_TOP_INT       0x01U
/** @brief Sub level interrupt 0 indication */
#define PCA9420_SUB_INT0      0x02U
/*! @brief Sub level interrupt 0 mask */
#define PCA9420_SUB_INT0_MASK 0x03U
/** @brief Sub level interrupt 1 indication */
#define PCA9420_SUB_INT1      0x04U
/** @brief Sub level interrupt 1 mask */
#define PCA9420_SUB_INT1_MASK 0x05U
/** @brief Sub level interrupt 2 indication */
#define PCA9420_SUB_INT2      0x06U
/** @brief Sub level interrupt 2 mask */
#define PCA9420_SUB_INT2_MASK 0x07U
/** @brief Top level system ctrl 0 */
#define PCA9420_TOP_CNTL0     0x09U
/** @brief Top level system ctrl 1 */
#define PCA9420_TOP_CNTL1     0x0AU
/** @brief Top level system ctrl 2 */
#define PCA9420_TOP_CNTL2     0x0BU
/** @brief Top level system ctrl 3 */
#define PCA9420_TOP_CNTL3     0x0CU
/** @brief Top level system ctrl 4 */
#define PCA9420_TOP_CNTL4     0x0DU

/** Battery charger registers */
/** @brief Charger cntl reg 0 */
#define PCA9420_CHG_CNTL0    0x10U
/** @brief Charger cntl reg 1 */
#define PCA9420_CHG_CNTL1    0x11U
/** @brief Charger cntl reg 2 */
#define PCA9420_CHG_CNTL2    0x12U
/** @brief Charger cntl reg 3 */
#define PCA9420_CHG_CNTL3    0x13U
/** @brief Charger cntl reg 4 */
#define PCA9420_CHG_CNTL4    0x14U
/** @brief Charger cntl reg 5 */
#define PCA9420_CHG_CNTL5    0x15U
/** @brief Charger cntl reg 6 */
#define PCA9420_CHG_CNTL6    0x16U
/** @brief Charger cntl reg 7 */
#define PCA9420_CHG_CNTL7    0x17U
/** @brief Charger status reg 0 */
#define PCA9420_CHG_STATUS_0 0x18U
/** @brief Charger status reg 1 */
#define PCA9420_CHG_STATUS_1 0x19U
/** @brief Charger status reg 2 */
#define PCA9420_CHG_STATUS_2 0x1AU
/** @brief Charger status reg 3 */
#define PCA9420_CHG_STATUS_3 0x1BU

/** Regulator status indication registers */
/** @brief Regulator status indication */
#define PCA9420_REG_STATUS           0x20U
/** @brief Active discharge control register */
#define PCA9420_ACT_DISCHARGE_CNTL_1 0x21U
/** @brief Mode configuration for mode 0_0 */
#define PCA9420_MODECFG_0_0          0x22U
/** @brief Mode configuration for mode 0_1 */
#define PCA9420_MODECFG_0_1          0x23U
/** @brief Mode configuration for mode 0_2 */
#define PCA9420_MODECFG_0_2          0x24U
/** @brief Mode configuration for mode 0_3 */
#define PCA9420_MODECFG_0_3          0x25U
/** @brief Mode configuration for mode 1_0 */
#define PCA9420_MODECFG_1_0          0x26U
/** @brief Mode configuration for mode 1_1 */
#define PCA9420_MODECFG_1_1          0x27U
/** @brief Mode configuration for mode 1_2 */
#define PCA9420_MODECFG_1_2          0x28U
/** @brief Mode configuration for mode 1_3 */
#define PCA9420_MODECFG_1_3          0x29U
/** @brief Mode configuration for mode 2_0 */
#define PCA9420_MODECFG_2_0          0x2AU
/** @brief Mode configuration for mode 2_1 */
#define PCA9420_MODECFG_2_1          0x2BU
/** @brief Mode configuration for mode 2_2 */
#define PCA9420_MODECFG_2_2          0x2CU
/** @brief Mode configuration for mode 2_3 */
#define PCA9420_MODECFG_2_3          0x2DU
/** @brief Mode configuration for mode 3_0 */
#define PCA9420_MODECFG_3_0          0x2EU
/** @brief Mode configuration for mode 3_1 */
#define PCA9420_MODECFG_3_1          0x2FU
/** @brief Mode configuration for mode 3_2 */
#define PCA9420_MODECFG_3_2          0x30U
/** @brief Mode configuration for mode 3_3 */
#define PCA9420_MODECFG_3_3          0x31U

/** Define the Register Masks of PCA9420. */
/** @brief Device ID mask */
#define PCA9420_DEV_INFO_DEV_ID_MASK    0xF0U
#define PCA9420_DEV_INFO_DEV_ID_SHIFT   0x04U
/** @brief Device revision mask */
#define PCA9420_DEV_INFO_DEV_REV_MASK    0x0FU
#define PCA9420_DEV_INFO_DEV_REV_SHIFT   0x00U
/** @brief System level interrupt trigger */
#define PCA9420_TOP_INT_SYS_INT_MASK 0x01U
#define PCA9420_TOP_INT_SYS_INT_SHIFT 0x03U
/** @brief charger block interrupt trigger */
#define PCA9420_TOP_INT_CHG_INT_MASK 0x01U
#define PCA9420_TOP_INT_CHG_INT_SHIFT 0x02U
/** @brief buck regulator interrupt trigger */
#define PCA9420_TOP_INT_SW_INT_MASK 0x01U
#define PCA9420_TOP_INT_SW_INT_SHIFT 0x01U
/** @brief ldo block interrupt trigger */
#define PCA9420_TOP_INT_LDO_INT_MASK 0x01U
#define PCA9420_TOP_INT_LDO_INT_SHIFT 0x00U

/** @brief VIN input current limit selection */
#define PCA9420_TOP_CNTL0_VIN_ILIM_SEL_MASK 0xE0U
#define PCA9420_TOP_CNTL0_VIN_ILIM_SEL_SHIFT 0x05U

/** @brief I2C Mode control mask */
#define PCA9420_TOP_CNTL3_MODE_I2C_MASK 0x18U
#define PCA9420_TOP_CNTL3_MODE_I2C_SHIFT 0x03U
/** @brief Mode ship enable mask */
#define PCA9420_MODECFG_0_SHIP_EN_MASK       0x80U
/*
 * @brief Mode control selection mask. When this bit is set, the external
 * PMIC pins MODESEL0 and MODESEL1 can be used to select the active mode
 */
#define PCA9420_MODECFG_0_MODE_CTRL_SEL_MASK 0x40U
/** @brief Mode output voltage mask */
#define PCA9420_MODECFG_0_SW1_OUT_MASK       0x3FU
/*
 * @brief Mode configuration upon falling edge applied to ON pin. If set,
 * the device will switch to mode 0 when a valid falling edge is applied.
 * to the ON pin
 */
#define PCA9420_MODECFG_1_ON_CFG_MASK        0x40U
/** @brief SW2_OUT offset and voltage level mask */
#define PCA9420_MODECFG_1_SW2_OUT_MASK       0x3FU
/** @brief LDO1_OUT voltage level mask */
#define PCA9420_MODECFG_2_LDO1_OUT_MASK      0xF0U
#define PCA9420_MODECFG_2_LDO1_OUT_SHIFT     0x04U
/** @brief SW1 Enable */
#define PCA9420_MODECFG_2_SW1_EN_MASK	     0x08U
#define PCA9420_MODECFG_2_SW1_EN_VAL	     0x08U
/** @brief SW2 Enable */
#define PCA9420_MODECFG_2_SW2_EN_MASK	     0x04U
#define PCA9420_MODECFG_2_SW2_EN_VAL	     0x04U
/** @brief LDO1 Enable */
#define PCA9420_MODECFG_2_LDO1_EN_MASK	     0x02U
#define PCA9420_MODECFG_2_LDO1_EN_VAL	     0x02U
/** @brief LDO2 Enable */
#define PCA9420_MODECFG_2_LDO2_EN_MASK	     0x01U
#define PCA9420_MODECFG_2_LDO2_EN_VAL	     0x01U
/** @brief Watchdog timer setting mask */
#define PCA9420_MODECFG_3_WDIMER_MASK      0xC0U
/** @brief LDO2_OUT offset and voltage level mask */
#define PCA9420_MODECFG_3_LDO2_OUT_MASK      0x3FU



#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_REGULATOR_PCA9420_I2C_H_*/
