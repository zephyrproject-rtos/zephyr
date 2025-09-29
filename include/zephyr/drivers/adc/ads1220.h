/*
 * Copyright (c) 2025 Baumer Group
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ADC_ADS1220_H_
#define ZEPHYR_INCLUDE_DRIVERS_ADC_ADS1220_H_

/**
 * @file
 * @brief Texas Instruments ADS1220 ADC driver API
 *
 * This file contains the API for the ADS1220 24-bit, 4-channel, low-power,
 * Delta-Sigma ADC with integrated PGA, VREF, SPI interface, and two IDACs.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ADS1220 ADC driver API
 * @defgroup ads1220_interface ADS1220 ADC driver API
 * @ingroup adc_interface
 * @{
 */

/** ADC resolution in bits */
#define ADS1220_RESOLUTION 24u
/** Number of input channels */
#define ADS1220_CHANNELS   4u
/** Device ID register value */
#define ADS1220_CHANNEL_ID 0x00u

/** Mask for MUX field in configuration register 0 */
#define ADS1220_REG0_MUX_MSK         0xF0u
/** MUX: AINP=AIN0, AINN=AIN1 */
#define ADS1220_REG0_MUX_AIN0_AIN1   (0x0u << 0x04)
/** MUX: AINP=AIN0, AINN=AIN2 */
#define ADS1220_REG0_MUX_AIN0_AIN2   (0x1u << 0x04)
/** MUX: AINP=AIN0, AINN=AIN3 */
#define ADS1220_REG0_MUX_AIN0_AIN3   (0x2u << 0x04)
/** MUX: AINP=AIN1, AINN=AIN2 */
#define ADS1220_REG0_MUX_AIN1_AIN2   (0x3u << 0x04)
/** MUX: AINP=AIN1, AINN=AIN3 */
#define ADS1220_REG0_MUX_AIN1_AIN3   (0x4u << 0x04)
/** MUX: AINP=AIN2, AINN=AIN3 */
#define ADS1220_REG0_MUX_AIN2_AIN3   (0x5u << 0x04)
/** MUX: AINP=AIN1, AINN=AIN0 */
#define ADS1220_REG0_MUX_AIN1_AIN0   (0x6u << 0x04)
/** MUX: AINP=AIN3, AINN=AIN2 */
#define ADS1220_REG0_MUX_AIN3_AIN2   (0x7u << 0x04)
/** MUX: AINP=AIN0, AINN=AVSS */
#define ADS1220_REG0_MUX_AIN0_AVSS   (0x8u << 0x04)
/** MUX: AINP=AIN1, AINN=AVSS */
#define ADS1220_REG0_MUX_AIN1_AVSS   (0x9u << 0x04)
/** MUX: AINP=AIN2, AINN=AVSS */
#define ADS1220_REG0_MUX_AIN2_AVSS   (0xAu << 0x04)
/** MUX: AINP=AIN3, AINN=AVSS */
#define ADS1220_REG0_MUX_AIN3_AVSS   (0xBu << 0x04)
/** MUX: (V_REFPx - V_REFNx) / 4 monitor */
#define ADS1220_REG0_MUX_V_REF       (0xCu << 0x04)
/** MUX: (AVDD - AVSS) / 4 monitor */
#define ADS1220_REG0_MUX_AVDD_AVSS_1 (0xDu << 0x04)
/** MUX: AINP and AINN shorted to (AVDD + AVSS) / 2 */
#define ADS1220_REG0_MUX_AVDD_AVSS_2 (0xEu << 0x04)
/** MUX: Reserved */
#define ADS1220_REG0_MUX_RESERVED    (0xFu << 0x04)

/** Mask for GAIN field in configuration register 0 */
#define ADS1220_REG0_GAIN_MSK 0x0Eu
/** PGA gain = 1 */
#define ADS1220_REG0_GAIN_1   (0x0u << 0x01)
/** PGA gain = 2 */
#define ADS1220_REG0_GAIN_2   (0x1u << 0x01)
/** PGA gain = 4 */
#define ADS1220_REG0_GAIN_4   (0x2u << 0x01)
/** PGA gain = 8 */
#define ADS1220_REG0_GAIN_8   (0x3u << 0x01)
/** PGA gain = 16 */
#define ADS1220_REG0_GAIN_16  (0x4u << 0x01)
/** PGA gain = 32 */
#define ADS1220_REG0_GAIN_32  (0x5u << 0x01)
/** PGA gain = 64 */
#define ADS1220_REG0_GAIN_64  (0x6u << 0x01)
/** PGA gain = 128 */
#define ADS1220_REG0_GAIN_128 (0x7u << 0x01)

/** PGA bypass bit in configuration register 0 */
#define ADS1220_REG0_PGA_BYPASS BIT(0)

/** Mask for DR field in configuration register 1 */
#define ADS1220_REG1_DR_MSK 0xE0u
/** Bit position of DR field in configuration register 1 */
#define ADS1220_REG1_DR_POS 0x5u

/** Mask for MODE field in configuration register 1 */
#define ADS1220_REG1_MODE_MSK        0x18u
/** Bit position of MODE field in configuration register 1 */
#define ADS1220_REG1_MODE_POS        0x3u
/** Operating mode: normal */
#define ADS1220_REG1_MODE_NORMAL     (0x0u << 0x03)
/** Operating mode: duty-cycle */
#define ADS1220_REG1_MODE_DUTY_CYCLE (0x1u << 0x03)
/** Operating mode: turbo */
#define ADS1220_REG1_MODE_TURBO      (0x2u << 0x03)

/** Conversion mode bit in configuration register 1 */
#define ADS1220_REG1_CM  BIT(2)
/** Temperature sensor mode bit in configuration register 1 */
#define ADS1220_REG1_TS  BIT(1)
/** Burn-out current sources bit in configuration register 1 */
#define ADS1220_REG1_BCS BIT(0)

/** Mask for VREF field in configuration register 2 */
#define ADS1220_REG2_VREF_MSK    0xC0u
/** VREF: internal 2.048V reference */
#define ADS1220_REG2_VREF_INT    (0x0u << 0x06)
/** VREF: external reference 0 (REFP0/REFN0) */
#define ADS1220_REG2_VREF_EXT_0  (0x1u << 0x06)
/** VREF: external reference 1 (REFP1/REFN1) */
#define ADS1220_REG2_VREF_EXT_1  (0x2u << 0x06)
/** VREF: analog supply (AVDD - AVSS) */
#define ADS1220_REG2_VREF_SUPPLY (0x3u << 0x06)

/** Mask for 50/60Hz rejection filter in configuration register 2 */
#define ADS1220_REG2_50_60HZ_MSK  0x30u
/** 50/60Hz filter: no rejection */
#define ADS1220_REG2_50_60HZ_NONE (0x0u << 0x04)
/** 50/60Hz filter: reject 50Hz only */
#define ADS1220_REG2_50_60HZ_50HZ (0x1u << 0x04)
/** 50/60Hz filter: reject 60Hz only */
#define ADS1220_REG2_50_60HZ_60HZ (0x2u << 0x04)
/** 50/60Hz filter: reject both 50Hz and 60Hz */
#define ADS1220_REG2_50_60HZ_BOTH (0x3u << 0x04)

/** Low-side power switch bit in configuration register 2 */
#define ADS1220_REG2_PSW BIT(3)

/** Mask for IDAC current field in configuration register 2 */
#define ADS1220_REG2_IDAC_MSK    0x07u
/** IDAC current: off */
#define ADS1220_REG2_IDAC_OFF    (0x0u)
/** IDAC current: 10 uA */
#define ADS1220_REG2_IDAC_10UA   (0x1u)
/** IDAC current: 50 uA */
#define ADS1220_REG2_IDAC_50UA   (0x2u)
/** IDAC current: 100 uA */
#define ADS1220_REG2_IDAC_100UA  (0x3u)
/** IDAC current: 250 uA */
#define ADS1220_REG2_IDAC_250UA  (0x4u)
/** IDAC current: 500 uA */
#define ADS1220_REG2_IDAC_500UA  (0x5u)
/** IDAC current: 1000 uA */
#define ADS1220_REG2_IDAC_1000UA (0x6u)
/** IDAC current: 1500 uA */
#define ADS1220_REG2_IDAC_1500UA (0x7u)

/** Mask for I1MUX field in configuration register 3 */
#define ADS1220_REG3_I1MUX_MSK        0xE0u
/** Bit position of I1MUX field in configuration register 3 */
#define ADS1220_REG3_I1MUX_POS        0x5u
/** I1MUX: IDAC1 disabled */
#define ADS1220_REG3_I1MUX_DISABLED   (0x0u << 0x05)
/** I1MUX: IDAC1 connected to AIN0/REFP1 */
#define ADS1220_REG3_I1MUX_AIN0_REFP1 (0x1u << 0x05)
/** I1MUX: IDAC1 connected to AIN1 */
#define ADS1220_REG3_I1MUX_AIN1       (0x2u << 0x05)
/** I1MUX: IDAC1 connected to AIN2 */
#define ADS1220_REG3_I1MUX_AIN2       (0x3u << 0x05)
/** I1MUX: IDAC1 connected to AIN3/REFN1 */
#define ADS1220_REG3_I1MUX_AIN3_REFN1 (0x4u << 0x05)
/** I1MUX: IDAC1 connected to REFP0 */
#define ADS1220_REG3_I1MUX_REFP       (0x5u << 0x05)
/** I1MUX: IDAC1 connected to REFN0 */
#define ADS1220_REG3_I1MUX_REFN       (0x6u << 0x05)

/** Mask for I2MUX field in configuration register 3 */
#define ADS1220_REG3_I2MUX_MSK        0x1Cu
/** Bit position of I2MUX field in configuration register 3 */
#define ADS1220_REG3_I2MUX_POS        0x2u
/** I2MUX: IDAC2 disabled */
#define ADS1220_REG3_I2MUX_DISABLED   (0x0u << 0x02)
/** I2MUX: IDAC2 connected to AIN0/REFP1 */
#define ADS1220_REG3_I2MUX_AIN0_REFP1 (0x1u << 0x02)
/** I2MUX: IDAC2 connected to AIN1 */
#define ADS1220_REG3_I2MUX_AIN1       (0x2u << 0x02)
/** I2MUX: IDAC2 connected to AIN2 */
#define ADS1220_REG3_I2MUX_AIN2       (0x3u << 0x02)
/** I2MUX: IDAC2 connected to AIN3/REFN1 */
#define ADS1220_REG3_I2MUX_AIN3_REFN1 (0x4u << 0x02)
/** I2MUX: IDAC2 connected to REFP0 */
#define ADS1220_REG3_I2MUX_REFP       (0x5u << 0x02)
/** I2MUX: IDAC2 connected to REFN0 */
#define ADS1220_REG3_I2MUX_REFN       (0x6u << 0x02)

/** DRDY mode bit in configuration register 3 */
#define ADS1220_REG3_DRDYM    BIT(1)
/** Reserved bit in configuration register 3 */
#define ADS1220_REG3_RESERVED BIT(0)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_ESF_DRIVERS_ADC_ADS1220_H */
