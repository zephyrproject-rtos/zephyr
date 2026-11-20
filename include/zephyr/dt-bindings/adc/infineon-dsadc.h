/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Infineon Delta-Sigma ADC (DSADC) input selection values.
 *
 * Values for zephyr,input-positive and zephyr,input-negative in the channel
 * nodes of an infineon,dsadc node. They match the PDL pin selection values.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ADC_INFINEON_DSADC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ADC_INFINEON_DSADC_H_

/* Positive input selection (zephyr,input-positive) */

/** High voltage divider 0 output */
#define IFX_DSADC_POS_HVDIV0          0x00
/** High voltage divider 1 output */
#define IFX_DSADC_POS_HVDIV1          0x04
/** External temperature sensor */
#define IFX_DSADC_POS_VTS             0x08
/** Internal temperature sensor */
#define IFX_DSADC_POS_VINT            0x0C
/** Analog ground (VSSA) */
#define IFX_DSADC_POS_VSSA            0x10
/** Digital ground (VSSD) */
#define IFX_DSADC_POS_VSSD            0x11
/** High voltage subsystem ground */
#define IFX_DSADC_POS_VSSL            0x12
/** Voltage reference low */
#define IFX_DSADC_POS_VREFL           0x13
/** External temperature sensor return Kelvin */
#define IFX_DSADC_POS_VTS_RET_K       0x15
/** Internal temperature sensor return */
#define IFX_DSADC_POS_VINT_RET        0x16
/** VSUB */
#define IFX_DSADC_POS_VSUB            0x17
/** VCCD / 2 */
#define IFX_DSADC_POS_VCCD_DIV_2      0x18
/** High voltage input bus VCCD / 2 */
#define IFX_DSADC_POS_VCCHIB_DIV_2    0x19
/** VDDA / 4 */
#define IFX_DSADC_POS_VDDA_DIV_4      0x1A
/** VDDD / 4 */
#define IFX_DSADC_POS_VDDD_DIV_4      0x1B
/** 1.2 V HPBGR reference high */
#define IFX_DSADC_POS_VREFH_1P2_HPBGR 0x1C
/** 0.8 V HPBGR reference high */
#define IFX_DSADC_POS_VREFH_0P8_HPBGR 0x1D
/** 0.7 V HPBGR reference high */
#define IFX_DSADC_POS_VREFH_0P7_HPBGR 0x1E
/** 1.2 V SRSS reference high */
#define IFX_DSADC_POS_VREFH_1P2_SRSS  0x1F
/** GPIO P0.0 */
#define IFX_DSADC_POS_GPIO0           0x20
/** GPIO P0.1 */
#define IFX_DSADC_POS_GPIO1           0x24
/** GPIO P0.2 */
#define IFX_DSADC_POS_GPIO2           0x28
/** GPIO P0.3 */
#define IFX_DSADC_POS_GPIO3           0x2C
/** GPIO P0.4 */
#define IFX_DSADC_POS_GPIO4           0x30
/** GPIO P0.5 */
#define IFX_DSADC_POS_GPIO5           0x34
/** GPIO P0.6 */
#define IFX_DSADC_POS_GPIO6           0x38
/** GPIO P0.7 */
#define IFX_DSADC_POS_GPIO7           0x3C
/** Shunt resistor 0 high side */
#define IFX_DSADC_POS_RSH_0           0x40
/** Shunt resistor 1 high side */
#define IFX_DSADC_POS_RSH_1           0x60
/** Analog mux bus A */
#define IFX_DSADC_POS_AMUXA           0x7A
/** Analog mux bus B */
#define IFX_DSADC_POS_AMUXB           0x7B

/* Negative input selection (zephyr,input-negative) */

/** VSSA */
#define IFX_DSADC_NEG_VSSA     0x00
/** Voltage reference low */
#define IFX_DSADC_NEG_VREFL    0x09
/** SRSS ground (VSS) */
#define IFX_DSADC_NEG_SRSS_VSS 0x0A
/** High voltage divider return */
#define IFX_DSADC_NEG_VDIV_RET 0x0B
/** External temperature sensor return */
#define IFX_DSADC_NEG_VTS_RET  0x0C
/** Internal temperature sensor return */
#define IFX_DSADC_NEG_VINT_RET 0x0D
/** GPIO P0.0 */
#define IFX_DSADC_NEG_GPIO0    0x0E
/** GPIO P0.2 */
#define IFX_DSADC_NEG_GPIO2    0x0F
/** Shunt resistor 0 low side */
#define IFX_DSADC_NEG_RSL_0    0x10
/** Shunt resistor 1 low side */
#define IFX_DSADC_NEG_RSL_1    0x18
/** Analog mux bus A */
#define IFX_DSADC_NEG_AMUXA    0x1A
/** Analog mux bus B */
#define IFX_DSADC_NEG_AMUXB    0x1B

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ADC_INFINEON_DSADC_H_ */
