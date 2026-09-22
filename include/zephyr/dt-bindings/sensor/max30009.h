/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the Analog Devices MAX30009 BioZ AFE.
 * @ingroup max30009_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_MAX30009_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_MAX30009_H_

/**
 * @defgroup max30009_interface MAX30009
 * @ingroup sensor_interface_ext_adi
 * @brief Analog Devices MAX30009 Bio-Impedance AFE
 * @{
 */

/**
 * @name BioZ digital high-pass filter cutoff
 *
 * Values for the `bioz-dhpf` devicetree property.
 * @{
 */
/** DHPF bypass (filter disabled) */
#define MAX30009_DT_DHPF_BYPASS     0
/** DHPF cutoff 0.00025 x SR_BIOZ */
#define MAX30009_DT_DHPF_0_00025_SR 1
/** DHPF cutoff 0.002 x SR_BIOZ */
#define MAX30009_DT_DHPF_0_002_SR   2
/** @} */

/**
 * @name BioZ digital low-pass filter cutoff
 *
 * Values for the `bioz-dlpf` devicetree property.
 * @{
 */
/** DLPF bypass (filter disabled) */
#define MAX30009_DT_DLPF_BYPASS   0
/** DLPF cutoff 0.005 x SR_BIOZ */
#define MAX30009_DT_DLPF_0_005_SR 1
/** DLPF cutoff 0.02 x SR_BIOZ */
#define MAX30009_DT_DLPF_0_02_SR  2
/** DLPF cutoff 0.08 x SR_BIOZ */
#define MAX30009_DT_DLPF_0_08_SR  3
/** DLPF cutoff 0.25 x SR_BIOZ */
#define MAX30009_DT_DLPF_0_25_SR  4
/** @} */

/**
 * @name BioZ threshold comparator channel
 *
 * Values for the `bioz-cmp` devicetree property.
 * @{
 */
/** Compare the I channel */
#define MAX30009_DT_CMP_I 0
/** Compare the Q channel */
#define MAX30009_DT_CMP_Q 1
/** Compare the magnitude (Z) */
#define MAX30009_DT_CMP_Z 2
/** @} */

/**
 * @name BioZ transmit drive mode
 *
 * Values for the `bioz-drv-mode` devicetree property.
 * @{
 */
/** Current drive */
#define MAX30009_DT_DRV_MODE_CURRENT 0
/** Voltage drive */
#define MAX30009_DT_DRV_MODE_VOLTAGE 1
/** H-bridge drive */
#define MAX30009_DT_DRV_MODE_HBRIDGE 2
/** Standby */
#define MAX30009_DT_DRV_MODE_STANDBY 3
/** @} */

/**
 * @name BioZ internal current-range resistor
 *
 * Values for the `bioz-idrv-rge` devicetree property.
 * @{
 */
/** 552.5 kOhm */
#define MAX30009_DT_IDRV_RGE_552_5K  0
/** 110.5 kOhm */
#define MAX30009_DT_IDRV_RGE_110_5K  1
/** 5.525 kOhm */
#define MAX30009_DT_IDRV_RGE_5_525K  2
/** 276.25 Ohm */
#define MAX30009_DT_IDRV_RGE_276_25R 3
/** @} */

/**
 * @name BioZ voltage-drive magnitude
 *
 * Values for the `bioz-vdrv-mag` devicetree property.
 * @{
 */
/** 50 mVpk */
#define MAX30009_DT_VDRV_MAG_50MV  0
/** 100 mVpk */
#define MAX30009_DT_VDRV_MAG_100MV 1
/** 250 mVpk */
#define MAX30009_DT_VDRV_MAG_250MV 2
/** 500 mVpk */
#define MAX30009_DT_VDRV_MAG_500MV 3
/** @} */

/**
 * @name BioZ analog high-pass filter (AHPF) selection
 *
 * Values for the `bioz-ahpf` devicetree property.
 * @{
 */
/** 100 Hz */
#define MAX30009_DT_AHPF_100HZ      0
/** 200 Hz */
#define MAX30009_DT_AHPF_200HZ      1
/** 500 Hz */
#define MAX30009_DT_AHPF_500HZ      2
/** 1 kHz */
#define MAX30009_DT_AHPF_1KHZ       3
/** 2 kHz */
#define MAX30009_DT_AHPF_2KHZ       4
/** 5 kHz */
#define MAX30009_DT_AHPF_5KHZ       5
/** 10 kHz */
#define MAX30009_DT_AHPF_10KHZ      6
/** AHPF bypassed (resistor opened, internal capacitors shorted) */
#define MAX30009_DT_AHPF_BYPASS     7
/** 42.4 MOhm, internal capacitors shorted */
#define MAX30009_DT_AHPF_42_4M      8
/** 21.2 MOhm, internal capacitors shorted */
#define MAX30009_DT_AHPF_21_2M      9
/** 8.4 MOhm, internal capacitors shorted */
#define MAX30009_DT_AHPF_8_4M       10
/** 4.2 MOhm, internal capacitors shorted */
#define MAX30009_DT_AHPF_4_2M       11
/** 2.2 MOhm, internal capacitors shorted */
#define MAX30009_DT_AHPF_2_2M       12
/** 848 kOhm, internal capacitors shorted */
#define MAX30009_DT_AHPF_848K       13
/** 848 kOhm, internal capacitors shorted */
#define MAX30009_DT_AHPF_848K_ALT   14
/** AHPF bypassed (resistor opened, internal capacitors shorted) */
#define MAX30009_DT_AHPF_BYPASS_ALT 15
/** @} */

/**
 * @name BioZ instrumentation amplifier power mode
 *
 * Values for the `bioz-ina-mode` devicetree property.
 * @{
 */
/** High-power (low-noise) mode */
#define MAX30009_DT_INA_MODE_HIGH_POWER 0
/** Low-power mode */
#define MAX30009_DT_INA_MODE_LOW_POWER  1
/** @} */

/**
 * @name BioZ receive-channel gain
 *
 * Values for the `bioz-gain` devicetree property.
 * @{
 */
/** 1x gain */
#define MAX30009_DT_GAIN_1X  0
/** 2x gain */
#define MAX30009_DT_GAIN_2X  1
/** 5x gain */
#define MAX30009_DT_GAIN_5X  2
/** 10x gain */
#define MAX30009_DT_GAIN_10X 3
/** @} */

/**
 * @name BioZ DC-restore switch
 *
 * Values for the `bioz-dc-restore` devicetree property.
 * @{
 */
/** DC_RESTORE switch open */
#define MAX30009_DT_DC_RESTORE_OPEN   0
/** DC_RESTORE switch closed */
#define MAX30009_DT_DC_RESTORE_CLOSED 1
/** @} */

/**
 * @name BioZ amplifier range
 *
 * Values for the `bioz-amp-rge` devicetree property.
 * @{
 */
/** Low */
#define MAX30009_DT_AMP_RGE_LOW         0
/** Medium-low */
#define MAX30009_DT_AMP_RGE_MEDIUM_LOW  1
/** Medium-high */
#define MAX30009_DT_AMP_RGE_MEDIUM_HIGH 2
/** High */
#define MAX30009_DT_AMP_RGE_HIGH        3
/** @} */

/**
 * @name BioZ amplifier bandwidth
 *
 * Values for the `bioz-amp-bw` devicetree property.
 * @{
 */
/** Low bandwidth */
#define MAX30009_DT_AMP_BW_LOW         0
/** Medium-low bandwidth */
#define MAX30009_DT_AMP_BW_MEDIUM_LOW  1
/** Medium-high bandwidth */
#define MAX30009_DT_AMP_BW_MEDIUM_HIGH 2
/** High bandwidth */
#define MAX30009_DT_AMP_BW_HIGH        3
/** @} */

/**
 * @name BioZ Q-channel demodulator clock phase
 *
 * Values for the `bioz-q-clk-phase` devicetree property.
 * @{
 */
/** Normal operation (Q demodulator clock in quadrature phase) */
#define MAX30009_DT_Q_CLK_PHASE_NORMAL 0
/** I phase (Q demodulator clock in phase with stimulus) */
#define MAX30009_DT_Q_CLK_PHASE_I      1
/** @} */

/**
 * @name BioZ I-channel demodulator clock phase
 *
 * Values for the `bioz-i-clk-phase` devicetree property.
 * @{
 */
/** Normal operation (I demodulator clock in phase with stimulus) */
#define MAX30009_DT_I_CLK_PHASE_NORMAL 0
/** Q phase (I demodulator clock in quadrature phase) */
#define MAX30009_DT_I_CLK_PHASE_Q      1
/** @} */

/**
 * @name BioZ mux internal-load resistor
 *
 * Values for the `bmux-rsel` devicetree property.
 * @{
 */
/** 5100 Ohm */
#define MAX30009_DT_BMUX_RSEL_5100R 0
/** 900 Ohm */
#define MAX30009_DT_BMUX_RSEL_900R  1
/** 600 Ohm */
#define MAX30009_DT_BMUX_RSEL_600R  2
/** 280 Ohm */
#define MAX30009_DT_BMUX_RSEL_280R  3
/** @} */

/**
 * @name BioZ GSR-load resistor
 *
 * Values for the `bmux-gsr-rsel` devicetree property.
 * @{
 */
/** 25.7 kOhm */
#define MAX30009_DT_BMUX_GSR_RSEL_25_7K 0
/** 101 kOhm */
#define MAX30009_DT_BMUX_GSR_RSEL_101K  1
/** 505 kOhm */
#define MAX30009_DT_BMUX_GSR_RSEL_505K  2
/** 1000 kOhm */
#define MAX30009_DT_BMUX_GSR_RSEL_1000K 3
/** @} */

/**
 * @name BioZ positive-input (BIP) electrode assignment
 *
 * Values for the `bip-assign` devicetree property.
 * @{
 */
/** EL1 */
#define MAX30009_DT_BIP_ASSIGN_EL1  0
/** EL2A */
#define MAX30009_DT_BIP_ASSIGN_EL2A 1
/** EL2B */
#define MAX30009_DT_BIP_ASSIGN_EL2B 2
/** @} */

/**
 * @name BioZ negative-input (BIN) electrode assignment
 *
 * Values for the `bin-assign` devicetree property.
 * @{
 */
/** EL4 */
#define MAX30009_DT_BIN_ASSIGN_EL4  0
/** EL3A */
#define MAX30009_DT_BIN_ASSIGN_EL3A 1
/** EL3B */
#define MAX30009_DT_BIN_ASSIGN_EL3B 2
/** @} */

/**
 * @name BioZ positive-drive (DRVP) electrode assignment
 *
 * Values for the `drvp-assign` devicetree property.
 * @{
 */
/** EL1 (low resistance) */
#define MAX30009_DT_DRVP_ASSIGN_EL1  0
/** EL2A */
#define MAX30009_DT_DRVP_ASSIGN_EL2A 1
/** EL2B */
#define MAX30009_DT_DRVP_ASSIGN_EL2B 2
/** @} */

/**
 * @name BioZ negative-drive (DRVN) electrode assignment
 *
 * Values for the `drvn-assign` devicetree property.
 * @{
 */
/** EL4 (low resistance) */
#define MAX30009_DT_DRVN_ASSIGN_EL4  0
/** EL3A */
#define MAX30009_DT_DRVN_ASSIGN_EL3A 1
/** EL3B */
#define MAX30009_DT_DRVN_ASSIGN_EL3B 2
/** @} */

/**
 * @name DC lead-off current polarity
 *
 * Values for the `loff-ipol` devicetree property.
 * @{
 */
/** Non-inverted (BIP sources, BIN sinks) */
#define MAX30009_DT_LOFF_IPOL_NON_INVERTED 0
/** Inverted (BIP sinks, BIN sources) */
#define MAX30009_DT_LOFF_IPOL_INVERTED     1
/** @} */

/**
 * @name DC lead-off current magnitude
 *
 * Values for the `loff-imag` devicetree property.
 * @{
 */
/** 0 nA (sources disabled) */
#define MAX30009_DT_LOFF_IMAG_0NA   0
/** 5 nA */
#define MAX30009_DT_LOFF_IMAG_5NA   1
/** 10 nA */
#define MAX30009_DT_LOFF_IMAG_10NA  2
/** 20 nA */
#define MAX30009_DT_LOFF_IMAG_20NA  3
/** 50 nA */
#define MAX30009_DT_LOFF_IMAG_50NA  4
/** 100 nA */
#define MAX30009_DT_LOFF_IMAG_100NA 5
/** @} */

/**
 * @name Lead-bias resistor value
 *
 * Values for the `rbias-value` devicetree property.
 * @{
 */
/** 50 MOhm */
#define MAX30009_DT_RBIAS_50M  0
/** 100 MOhm */
#define MAX30009_DT_RBIAS_100M 1
/** 200 MOhm */
#define MAX30009_DT_RBIAS_200M 2
/** @} */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_MAX30009_H_ */
