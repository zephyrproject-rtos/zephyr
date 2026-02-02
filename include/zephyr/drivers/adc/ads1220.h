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

/* ADS1220 Device Information */
#define ADS1220_RESOLUTION 24u   /* ADC resolution in bits */
#define ADS1220_CHANNELS   4u    /* Number of input channels */
#define ADS1220_CHANNEL_ID 0x00u /* Device ID register value */

/* Configuration Register 0  */
/* Input multiplexer settings */
#define ADS1220_REG0_MUX_MSK         0b11110000u                    /* MUX field mask */
#define ADS1220_REG0_MUX_POS         0x4u                           /* MUX position */
#define ADS1220_REG0_MUX_AIN0_AIN1   (0x0u << ADS1220_REG0_MUX_POS) /* AINP=AIN0, AINN=AIN1 */
#define ADS1220_REG0_MUX_AIN0_AIN2   (0x1u << ADS1220_REG0_MUX_POS) /* AINP=AIN0, AINN=AIN2 */
#define ADS1220_REG0_MUX_AIN0_AIN3   (0x2u << ADS1220_REG0_MUX_POS) /* AINP=AIN0, AINN=AIN3 */
#define ADS1220_REG0_MUX_AIN1_AIN2   (0x3u << ADS1220_REG0_MUX_POS) /* AINP=AIN1, AINN=AIN2 */
#define ADS1220_REG0_MUX_AIN1_AIN3   (0x4u << ADS1220_REG0_MUX_POS) /* AINP=AIN1, AINN=AIN3 */
#define ADS1220_REG0_MUX_AIN2_AIN3   (0x5u << ADS1220_REG0_MUX_POS) /* AINP=AIN2, AINN=AIN3 */
#define ADS1220_REG0_MUX_AIN1_AIN0   (0x6u << ADS1220_REG0_MUX_POS) /* AINP=AIN1, AINN=AIN0 */
#define ADS1220_REG0_MUX_AIN3_AIN2   (0x7u << ADS1220_REG0_MUX_POS) /* AINP=AIN3, AINN=AIN2 */
#define ADS1220_REG0_MUX_AIN0_AVSS   (0x8u << ADS1220_REG0_MUX_POS) /* AINP=AIN0, AINN=AVSS */
#define ADS1220_REG0_MUX_AIN1_AVSS   (0x9u << ADS1220_REG0_MUX_POS) /* AINP=AIN1, AINN=AVSS */
#define ADS1220_REG0_MUX_AIN2_AVSS   (0xAu << ADS1220_REG0_MUX_POS) /* AINP = AIN2, AINN = AVSS */
#define ADS1220_REG0_MUX_AIN3_AVSS   (0xBu << ADS1220_REG0_MUX_POS) /* AINP = AIN3, AINN = AVSS */
#define ADS1220_REG0_MUX_V_REF       (0xCu << ADS1220_REG0_MUX_POS) /* (V_REFPx–V_REFNx) / 4 */
#define ADS1220_REG0_MUX_AVDD_AVSS_1 (0xDu << ADS1220_REG0_MUX_POS) /* (AVDD–AVSS) / 4 */
#define ADS1220_REG0_MUX_AVDD_AVSS_2 (0xEu << ADS1220_REG0_MUX_POS) /* AINP/AINN->(AVDD+AVSS)/2 */
#define ADS1220_REG0_MUX_RESERVED    (0xFu << ADS1220_REG0_MUX_POS) /* Reserved */

/* PGA gain settings */
#define ADS1220_REG0_GAIN_MSK 0b00001110u                     /* Gain field mask */
#define ADS1220_REG0_GAIN_POS 0x1u                            /* Gain position */
#define ADS1220_REG0_GAIN_1   (0x0u << ADS1220_REG0_GAIN_POS) /* Gain = 1 */
#define ADS1220_REG0_GAIN_2   (0x1u << ADS1220_REG0_GAIN_POS) /* Gain = 2 */
#define ADS1220_REG0_GAIN_4   (0x2u << ADS1220_REG0_GAIN_POS) /* Gain = 4 */
#define ADS1220_REG0_GAIN_8   (0x3u << ADS1220_REG0_GAIN_POS) /* Gain = 8 */
#define ADS1220_REG0_GAIN_16  (0x4u << ADS1220_REG0_GAIN_POS) /* Gain = 16 */
#define ADS1220_REG0_GAIN_32  (0x5u << ADS1220_REG0_GAIN_POS) /* Gain = 32 */
#define ADS1220_REG0_GAIN_64  (0x6u << ADS1220_REG0_GAIN_POS) /* Gain = 64 */
#define ADS1220_REG0_GAIN_128 (0x7u << ADS1220_REG0_GAIN_POS) /* Gain = 128 */

#define ADS1220_REG0_PGA_BYPASS BIT(0) /* PGA bypass bit */

/* Configuration Register 1 */
/* Data rate settings */
#define ADS1220_REG1_DR_MSK 0b11100000u /* DR field mask */
#define ADS1220_REG1_DR_POS 0x5u        /* DR position */

/* Operating modes */
#define ADS1220_REG1_MODE_MSK        0b00011000u                     /* MODE field mask */
#define ADS1220_REG1_MODE_POS        0x3u                            /* MODE position */
#define ADS1220_REG1_MODE_NORMAL     (0x0u << ADS1220_REG1_MODE_POS) /* Normal mode */
#define ADS1220_REG1_MODE_DUTY_CYCLE (0x1u << ADS1220_REG1_MODE_POS) /* Duty-cycle mode */
#define ADS1220_REG1_MODE_TURBO      (0x2u << ADS1220_REG1_MODE_POS) /* Turbo mode */

#define ADS1220_REG1_CM  BIT(2) /* Conversion mode */
#define ADS1220_REG1_TS  BIT(1) /* Temperature sensor mode */
#define ADS1220_REG1_BCS BIT(0) /* Burn-out current sources */

/* Configuration Register 2 */
/* Voltage reference settings */
#define ADS1220_REG2_VREF_MSK    0b11000000u                     /* VREF field mask */
#define ADS1220_REG2_VREF_POS    0x6u                            /* VREF position */
#define ADS1220_REG2_VREF_INT    (0x0u << ADS1220_REG2_VREF_POS) /* Internal 2.048V reference */
#define ADS1220_REG2_VREF_EXT_0  (0x1u << ADS1220_REG2_VREF_POS) /* External reference 0 */
#define ADS1220_REG2_VREF_EXT_1  (0x2u << ADS1220_REG2_VREF_POS) /* External reference 1 */
#define ADS1220_REG2_VREF_SUPPLY (0x3u << ADS1220_REG2_VREF_POS) /* Supply reference */

/* 50/60Hz rejection filter settings */
#define ADS1220_REG2_50_60HZ_MSK  0b00110000u                        /* 50/60Hz filter mask */
#define ADS1220_REG2_50_60HZ_POS  0x4u                               /* 50/60Hz filter position */
#define ADS1220_REG2_50_60HZ_NONE (0x0u << ADS1220_REG2_50_60HZ_POS) /* No rejection */
#define ADS1220_REG2_50_60HZ_50HZ (0x1u << ADS1220_REG2_50_60HZ_POS) /* 50Hz rejection */
#define ADS1220_REG2_50_60HZ_60HZ (0x2u << ADS1220_REG2_50_60HZ_POS) /* 60Hz rejection */
#define ADS1220_REG2_50_60HZ_BOTH (0x3u << ADS1220_REG2_50_60HZ_POS) /* 50Hz/60Hz rejection */

#define ADS1220_REG2_PSW BIT(3) /* Low-side power switch */

/* IDAC current settings */
#define ADS1220_REG2_IDAC_MSK    0b00000111u                     /* IDAC field mask */
#define ADS1220_REG2_IDAC_POS    0x0u                            /* IDAC position */
#define ADS1220_REG2_IDAC_OFF    (0x0u << ADS1220_REG2_IDAC_POS) /* IDAC off */
#define ADS1220_REG2_IDAC_10UA   (0x1u << ADS1220_REG2_IDAC_POS) /* 10 µA */
#define ADS1220_REG2_IDAC_50UA   (0x2u << ADS1220_REG2_IDAC_POS) /* 50 µA */
#define ADS1220_REG2_IDAC_100UA  (0x3u << ADS1220_REG2_IDAC_POS) /* 100 µA */
#define ADS1220_REG2_IDAC_250UA  (0x4u << ADS1220_REG2_IDAC_POS) /* 250 µA */
#define ADS1220_REG2_IDAC_500UA  (0x5u << ADS1220_REG2_IDAC_POS) /* 500 µA */
#define ADS1220_REG2_IDAC_1000UA (0x6u << ADS1220_REG2_IDAC_POS) /* 1000 µA */
#define ADS1220_REG2_IDAC_1500UA (0x7u << ADS1220_REG2_IDAC_POS) /* 1500 µA */

/* Configuration Register 3  */
/* IDAC1 routing options */
#define ADS1220_REG3_I1MUX_MSK        0b11100000u                      /* I1MUX field mask */
#define ADS1220_REG3_I1MUX_POS        0x5u                             /* I1MUX position */
#define ADS1220_REG3_I1MUX_DISABLED   (0x0u << ADS1220_REG3_I1MUX_POS) /* IDAC1 disabled */
#define ADS1220_REG3_I1MUX_AIN0_REFP1 (0x1u << ADS1220_REG3_I1MUX_POS) /* connected to AIN0 */
#define ADS1220_REG3_I1MUX_AIN1       (0x2u << ADS1220_REG3_I1MUX_POS) /* connected to AIN1 */
#define ADS1220_REG3_I1MUX_AIN2       (0x3u << ADS1220_REG3_I1MUX_POS) /* connected to AIN2 */
#define ADS1220_REG3_I1MUX_AIN3_REFN1 (0x4u << ADS1220_REG3_I1MUX_POS) /* connected to AIN3 */
#define ADS1220_REG3_I1MUX_REFP       (0x5u << ADS1220_REG3_I1MUX_POS) /* connected to REFP */
#define ADS1220_REG3_I1MUX_REFN       (0x6u << ADS1220_REG3_I1MUX_POS) /* connected to REFN */

/* IDAC2 routing options */
#define ADS1220_REG3_I2MUX_MSK        0b00011100u                      /* I2MUX field mask */
#define ADS1220_REG3_I2MUX_POS        0x2u                             /* I2MUX position */
#define ADS1220_REG3_I2MUX_DISABLED   (0x0u << ADS1220_REG3_I2MUX_POS) /* IDAC2 disabled */
#define ADS1220_REG3_I2MUX_AIN0_REFP1 (0x1u << ADS1220_REG3_I2MUX_POS) /* connected to AIN0 */
#define ADS1220_REG3_I2MUX_AIN1       (0x2u << ADS1220_REG3_I2MUX_POS) /* connected to AIN1 */
#define ADS1220_REG3_I2MUX_AIN2       (0x3u << ADS1220_REG3_I2MUX_POS) /* connected to AIN2 */
#define ADS1220_REG3_I2MUX_AIN3_REFN1 (0x4u << ADS1220_REG3_I2MUX_POS) /* connected to AIN3 */
#define ADS1220_REG3_I2MUX_REFP       (0x5u << ADS1220_REG3_I2MUX_POS) /* connected to REFP */
#define ADS1220_REG3_I2MUX_REFN       (0x6u << ADS1220_REG3_I2MUX_POS) /* connected to REFN */

#define ADS1220_REG3_DRDYM    BIT(1) /* DRDY mode */
#define ADS1220_REG3_RESERVED BIT(0) /* Reserved bit */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_ESF_DRIVERS_ADC_ADS1220_H */
