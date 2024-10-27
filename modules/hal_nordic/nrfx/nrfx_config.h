/*
 * Copyright (c) 2019 - 2022, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NRFX_CONFIG_H__
#define NRFX_CONFIG_H__

#include <zephyr/devicetree.h>
#include <zephyr/sys/util_macro.h>

/*
 * These are mappings of Kconfig options enabling nrfx drivers and particular
 * peripheral instances to the corresponding symbols used inside of nrfx.
 * Please note that only subsets of these entries are used for particular SoCs
 * supported by nrfx (see the corresponding nrfx_config_*.h files).
 */

#ifdef CONFIG_NRFX_ADC
#define NRFX_ADC_ENABLED 1
#endif
#ifdef CONFIG_NRFX_ADC_LOG
#define NRFX_ADC_CONFIG_LOG_ENABLED 1
#endif

#ifdef CONFIG_NRFX_CLOCK
#define NRFX_CLOCK_ENABLED 1
#endif
#ifdef CONFIG_NRFX_CLOCK_LOG
#define NRFX_CLOCK_CONFIG_LOG_ENABLED 1
#endif

#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC
#if defined(CONFIG_SOC_SERIES_NRF91X) || defined(CONFIG_SOC_COMPATIBLE_NRF53X)
#define NRFX_CLOCK_CONFIG_LF_SRC 1
#else
#define NRFX_CLOCK_CONFIG_LF_SRC 0
#endif
#endif // CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC

#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_XTAL
#if defined(CONFIG_SOC_SERIES_NRF91X) || defined(CONFIG_SOC_COMPATIBLE_NRF53X)
#define NRFX_CLOCK_CONFIG_LF_SRC 2
#else
#define NRFX_CLOCK_CONFIG_LF_SRC 1
#endif
#endif // CONFIG_CLOCK_CONTROL_NRF_K32SRC_XTAL

#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_SYNTH
#ifdef CONFIG_SOC_COMPATIBLE_NRF53X
#define NRFX_CLOCK_CONFIG_LF_SRC 3
#else
#define NRFX_CLOCK_CONFIG_LF_SRC 2
#endif
#endif // CONFIG_CLOCK_CONTROL_NRF_K32SRC_SYNTH

#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_EXT_LOW_SWING
#define NRFX_CLOCK_CONFIG_LF_SRC 131073
#endif

#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_EXT_FULL_SWING
#define NRFX_CLOCK_CONFIG_LF_SRC 196609
#endif

#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC_CALIBRATION
#define NRFX_CLOCK_CONFIG_LF_CAL_ENABLED 1
#endif

#ifdef CONFIG_NRFX_CLOCK_LFXO_TWO_STAGE_ENABLED
#define NRFX_CLOCK_CONFIG_LFXO_TWO_STAGE_ENABLED 1
#endif

#ifdef CONFIG_NRFX_COMP
#define NRFX_COMP_ENABLED 1
#endif
#ifdef CONFIG_NRFX_COMP_LOG
#define NRFX_COMP_CONFIG_LOG_ENABLED 1
#endif

#ifdef CONFIG_NRFX_DPPI
#define NRFX_DPPI_ENABLED 1
#endif
#ifdef CONFIG_NRFX_DPPI_LOG
#define NRFX_DPPI_CONFIG_LOG_ENABLED 1
#endif

#ifdef CONFIG_NRFX_EGU
#define NRFX_EGU_ENABLED 1
#endif
#ifdef CONFIG_NRFX_EGU_LOG
#define NRFX_EGU_CONFIG_LOG_ENABLED 1
#endif
#ifdef CONFIG_NRFX_EGU0
#define NRFX_EGU0_ENABLED 1
#endif
#ifdef CONFIG_NRFX_EGU1
#define NRFX_EGU1_ENABLED 1
#endif
#ifdef CONFIG_NRFX_EGU2
#define NRFX_EGU2_ENABLED 1
#endif
#ifdef CONFIG_NRFX_EGU3
#define NRFX_EGU3_ENABLED 1
#endif
#ifdef CONFIG_NRFX_EGU4
#define NRFX_EGU4_ENABLED 1
#endif
#ifdef CONFIG_NRFX_EGU5
#define NRFX_EGU5_ENABLED 1
#endif
#ifdef CONFIG_NRFX_EGU10
#define NRFX_EGU10_ENABLED 1
#endif
#ifdef CONFIG_NRFX_EGU20
#define NRFX_EGU20_ENABLED 1
#endif
#ifdef CONFIG_NRFX_EGU020
#define NRFX_EGU020_ENABLED 1
#endif
#ifdef CONFIG_NRFX_EGU130
#define NRFX_EGU130_ENABLED 1
#endif

#ifdef CONFIG_NRFX_GRTC
#define NRFX_GRTC_ENABLED 1
#endif
#ifdef CONFIG_NRFX_GRTC_LOG
#define NRFX_GRTC_CONFIG_LOG_ENABLED 1
#endif

#ifdef CONFIG_NRF_GRTC_TIMER_CLOCK_MANAGEMENT
#define NRF_GRTC_HAS_EXTENDED 1
#endif
#ifdef CONFIG_NRF_GRTC_TIMER_AUTO_KEEP_ALIVE
#define NRFX_GRTC_CONFIG_AUTOEN 1
#endif
#ifdef CONFIG_NRF_GRTC_START_SYSCOUNTER
#define NRFX_GRTC_CONFIG_AUTOSTART 1
#endif

#ifdef CONFIG_NRFX_GPIOTE
#define NRFX_GPIOTE_ENABLED 1
#endif
#ifdef CONFIG_NRFX_GPIOTE0
#define NRFX_GPIOTE0_ENABLED 1
#endif
#ifdef CONFIG_NRFX_GPIOTE1
#define NRFX_GPIOTE1_ENABLED 1
#endif
#ifdef CONFIG_NRFX_GPIOTE20
#define NRFX_GPIOTE20_ENABLED 1
#endif
#ifdef CONFIG_NRFX_GPIOTE30
#define NRFX_GPIOTE30_ENABLED 1
#endif
#ifdef CONFIG_NRFX_GPIOTE130
#define NRFX_GPIOTE130_ENABLED 1
#endif
#ifdef CONFIG_NRFX_GPIOTE131
#define NRFX_GPIOTE131_ENABLED 1
#endif

#ifdef CONFIG_NRFX_GPIOTE_NUM_OF_EVT_HANDLERS
#define NRFX_GPIOTE_CONFIG_NUM_OF_EVT_HANDLERS CONFIG_NRFX_GPIOTE_NUM_OF_EVT_HANDLERS
#endif

#ifdef CONFIG_NRFX_I2S
#define NRFX_I2S_ENABLED 1
#endif
#ifdef CONFIG_NRFX_I2S_LOG
#define NRFX_I2S_CONFIG_LOG_ENABLED 1
#endif
#ifdef CONFIG_NRFX_I2S0
#define NRFX_I2S0_ENABLED 1
#endif
#ifdef CONFIG_NRFX_I2S20
#define NRFX_I2S20_ENABLED 1
#endif

#ifdef CONFIG_NRFX_IPC
#define NRFX_IPC_ENABLED 1
#endif
#ifdef CONFIG_NRFX_IPC_LOG
#define NRFX_IPC_CONFIG_LOG_ENABLED 1
#endif

#ifdef CONFIG_NRFX_LPCOMP
#define NRFX_LPCOMP_ENABLED 1
#endif
#ifdef CONFIG_NRFX_LPCOMP_LOG
#define NRFX_LPCOMP_CONFIG_LOG_ENABLED 1
#endif

#ifdef CONFIG_NRFX_NFCT
#define NRFX_NFCT_ENABLED 1
#endif
#ifdef CONFIG_NRFX_NFCT_LOG
#define NRFX_NFCT_CONFIG_LOG_ENABLED 1
#endif

#ifdef CONFIG_NRFX_NVMC
#define NRFX_NVMC_ENABLED 1
#endif
#ifdef CONFIG_NRFX_NVMC_LOG
#define NRFX_NVMC_CONFIG_LOG_ENABLED 1
#endif

#ifdef CONFIG_NRFX_PDM
#define NRFX_PDM_ENABLED 1
#endif
#ifdef CONFIG_NRFX_PDM_LOG
#define NRFX_PDM_CONFIG_LOG_ENABLED 1
#endif
#ifdef CONFIG_NRFX_PDM0
#define NRFX_PDM0_ENABLED 1
#endif
#ifdef CONFIG_NRFX_PDM20
#define NRFX_PDM20_ENABLED 1
#endif
#ifdef CONFIG_NRFX_PDM21
#define NRFX_PDM21_ENABLED 1
#endif

#ifdef CONFIG_NRFX_POWER
#define NRFX_POWER_ENABLED 1
#endif
#ifdef CONFIG_NRFX_POWER_LOG
#define NRFX_POWER_CONFIG_LOG_ENABLED 1
#endif

#ifdef CONFIG_NRFX_PPI
#define NRFX_PPI_ENABLED 1
#endif
#ifdef CONFIG_NRFX_PPI_LOG
#define NRFX_PPI_CONFIG_LOG_ENABLED 1
#endif

#ifdef CONFIG_NRFX_PRS
#define NRFX_PRS_ENABLED 1
#endif
#ifdef CONFIG_NRFX_PRS_LOG
#define NRFX_PRS_CONFIG_LOG_ENABLED 1
#endif
#ifdef CONFIG_NRFX_PRS_BOX_0
#define NRFX_PRS_BOX_0_ENABLED 1
#endif
#ifdef CONFIG_NRFX_PRS_BOX_1
#define NRFX_PRS_BOX_1_ENABLED 1
#endif
#ifdef CONFIG_NRFX_PRS_BOX_2
#define NRFX_PRS_BOX_2_ENABLED 1
#endif
#ifdef CONFIG_NRFX_PRS_BOX_3
#define NRFX_PRS_BOX_3_ENABLED 1
#endif
#ifdef CONFIG_NRFX_PRS_BOX_4
#define NRFX_PRS_BOX_4_ENABLED 1
#endif

#ifdef CONFIG_NRFX_PWM
#define NRFX_PWM_ENABLED 1
#endif
#ifdef CONFIG_NRFX_PWM_LOG
#define NRFX_PWM_CONFIG_LOG_ENABLED 1
#endif
#ifdef CONFIG_NRFX_PWM0
#define NRFX_PWM0_ENABLED 1
#endif
#ifdef CONFIG_NRFX_PWM1
#define NRFX_PWM1_ENABLED 1
#endif
#ifdef CONFIG_NRFX_PWM2
#define NRFX_PWM2_ENABLED 1
#endif
#ifdef CONFIG_NRFX_PWM3
#define NRFX_PWM3_ENABLED 1
#endif
#ifdef CONFIG_NRFX_PWM20
#define NRFX_PWM20_ENABLED 1
#endif
#ifdef CONFIG_NRFX_PWM21
#define NRFX_PWM21_ENABLED 1
#endif
#ifdef CONFIG_NRFX_PWM22
#define NRFX_PWM22_ENABLED 1
#endif
#ifdef CONFIG_NRFX_PWM120
#define NRFX_PWM120_ENABLED 1
#endif
#ifdef CONFIG_NRFX_PWM130
#define NRFX_PWM130_ENABLED 1
#endif
#ifdef CONFIG_NRFX_PWM131
#define NRFX_PWM131_ENABLED 1
#endif
#ifdef CONFIG_NRFX_PWM132
#define NRFX_PWM132_ENABLED 1
#endif
#ifdef CONFIG_NRFX_PWM133
#define NRFX_PWM133_ENABLED 1
#endif

#ifdef CONFIG_NRFX_QDEC
#define NRFX_QDEC_ENABLED 1
#endif
#ifdef CONFIG_NRFX_QDEC_LOG
#define NRFX_QDEC_CONFIG_LOG_ENABLED 1
#endif
#ifdef CONFIG_NRFX_QDEC0
#define NRFX_QDEC0_ENABLED 1
#endif
#ifdef CONFIG_NRFX_QDEC1
#define NRFX_QDEC1_ENABLED 1
#endif
#ifdef CONFIG_NRFX_QDEC20
#define NRFX_QDEC20_ENABLED 1
#endif
#ifdef CONFIG_NRFX_QDEC21
#define NRFX_QDEC21_ENABLED 1
#endif
#ifdef CONFIG_NRFX_QDEC130
#define NRFX_QDEC130_ENABLED 1
#endif
#ifdef CONFIG_NRFX_QDEC131
#define NRFX_QDEC131_ENABLED 1
#endif

#ifdef CONFIG_NRFX_QSPI
#define NRFX_QSPI_ENABLED 1
#endif
#ifdef CONFIG_NRFX_QSPI_LOG
#define NRFX_QSPI_CONFIG_LOG_ENABLED 1
#endif

#ifdef CONFIG_NRFX_RNG
#define NRFX_RNG_ENABLED 1
#endif
#ifdef CONFIG_NRFX_RNG_LOG
#define NRFX_RNG_CONFIG_LOG_ENABLED 1
#endif

#ifdef CONFIG_NRFX_RRAMC
#define NRFX_RRAMC_ENABLED 1
#endif

#ifdef CONFIG_NRFX_RTC
#define NRFX_RTC_ENABLED 1
#endif
#ifdef CONFIG_NRFX_RTC_LOG
#define NRFX_RTC_CONFIG_LOG_ENABLED 1
#endif
#ifdef CONFIG_NRFX_RTC0
#define NRFX_RTC0_ENABLED 1
#endif
#ifdef CONFIG_NRFX_RTC1
#define NRFX_RTC1_ENABLED 1
#endif
#ifdef CONFIG_NRFX_RTC2
#define NRFX_RTC2_ENABLED 1
#endif
#ifdef CONFIG_NRFX_RTC130
#define NRFX_RTC130_ENABLED 1
#endif
#ifdef CONFIG_NRFX_RTC131
#define NRFX_RTC131_ENABLED 1
#endif

#ifdef CONFIG_NRFX_SAADC
#define NRFX_SAADC_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SAADC_LOG
#define NRFX_SAADC_CONFIG_LOG_ENABLED 1
#endif

#ifdef CONFIG_NRFX_SPI
#define NRFX_SPI_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPI_LOG
#define NRFX_SPI_CONFIG_LOG_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPI0
#define NRFX_SPI0_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPI1
#define NRFX_SPI1_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPI2
#define NRFX_SPI2_ENABLED 1
#endif

#ifdef CONFIG_NRFX_SPIM
#define NRFX_SPIM_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIM_LOG
#define NRFX_SPIM_CONFIG_LOG_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIM0
#define NRFX_SPIM0_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIM1
#define NRFX_SPIM1_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIM2
#define NRFX_SPIM2_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIM3
#define NRFX_SPIM3_ENABLED 1
#ifdef CONFIG_NRF52_ANOMALY_198_WORKAROUND
#define NRFX_SPIM3_NRF52840_ANOMALY_198_WORKAROUND_ENABLED 1
#endif
#endif
#ifdef CONFIG_NRFX_SPIM4
#define NRFX_SPIM4_ENABLED 1
#endif

#define NRFX_SPIM_DT_HAS_RX_DELAY(node) DT_PROP(node, rx_delay_supported) +

#if DT_FOREACH_STATUS_OKAY(nordic_nrf_spim, NRFX_SPIM_DT_HAS_RX_DELAY) 0
#define NRFX_SPIM_EXTENDED_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIM00
#define NRFX_SPIM00_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIM20
#define NRFX_SPIM20_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIM21
#define NRFX_SPIM21_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIM22
#define NRFX_SPIM22_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIM30
#define NRFX_SPIM30_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIM120
#define NRFX_SPIM120_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIM121
#define NRFX_SPIM121_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIM130
#define NRFX_SPIM130_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIM131
#define NRFX_SPIM131_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIM132
#define NRFX_SPIM132_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIM133
#define NRFX_SPIM133_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIM134
#define NRFX_SPIM134_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIM135
#define NRFX_SPIM135_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIM136
#define NRFX_SPIM136_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIM137
#define NRFX_SPIM137_ENABLED 1
#endif

#ifdef CONFIG_NRFX_SPIS
#define NRFX_SPIS_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIS_LOG
#define NRFX_SPIS_CONFIG_LOG_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIS0
#define NRFX_SPIS0_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIS1
#define NRFX_SPIS1_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIS2
#define NRFX_SPIS2_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIS3
#define NRFX_SPIS3_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIS00
#define NRFX_SPIS00_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIS20
#define NRFX_SPIS20_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIS21
#define NRFX_SPIS21_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIS22
#define NRFX_SPIS22_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIS30
#define NRFX_SPIS30_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIS120
#define NRFX_SPIS120_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIS130
#define NRFX_SPIS130_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIS131
#define NRFX_SPIS131_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIS132
#define NRFX_SPIS132_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIS133
#define NRFX_SPIS133_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIS134
#define NRFX_SPIS134_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIS135
#define NRFX_SPIS135_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIS136
#define NRFX_SPIS136_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SPIS137
#define NRFX_SPIS137_ENABLED 1
#endif

#ifdef CONFIG_NRFX_SYSTICK
#define NRFX_SYSTICK_ENABLED 1
#endif
#ifdef CONFIG_NRFX_SYSTICK_LOG
#define NRFX_SYSTICK_CONFIG_LOG_ENABLED 1
#endif

#ifdef CONFIG_NRFX_TBM
#define NRFX_TBM_ENABLED 1
#endif

#ifdef CONFIG_NRFX_TEMP
#define NRFX_TEMP_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TEMP_LOG
#define NRFX_TEMP_CONFIG_LOG_ENABLED 1
#endif

#ifdef CONFIG_NRFX_TIMER
#define NRFX_TIMER_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TIMER_LOG
#define NRFX_TIMER_CONFIG_LOG_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TIMER0
#define NRFX_TIMER0_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TIMER1
#define NRFX_TIMER1_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TIMER2
#define NRFX_TIMER2_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TIMER3
#define NRFX_TIMER3_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TIMER4
#define NRFX_TIMER4_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TIMER00
#define NRFX_TIMER00_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TIMER10
#define NRFX_TIMER10_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TIMER20
#define NRFX_TIMER20_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TIMER21
#define NRFX_TIMER21_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TIMER22
#define NRFX_TIMER22_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TIMER23
#define NRFX_TIMER23_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TIMER24
#define NRFX_TIMER24_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TIMER020
#define NRFX_TIMER020_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TIMER021
#define NRFX_TIMER021_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TIMER022
#define NRFX_TIMER022_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TIMER120
#define NRFX_TIMER120_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TIMER121
#define NRFX_TIMER121_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TIMER130
#define NRFX_TIMER130_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TIMER131
#define NRFX_TIMER131_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TIMER132
#define NRFX_TIMER132_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TIMER133
#define NRFX_TIMER133_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TIMER134
#define NRFX_TIMER134_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TIMER135
#define NRFX_TIMER135_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TIMER136
#define NRFX_TIMER136_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TIMER137
#define NRFX_TIMER137_ENABLED 1
#endif

#ifdef CONFIG_NRFX_TWI
#define NRFX_TWI_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWI_LOG
#define NRFX_TWI_CONFIG_LOG_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWI0
#define NRFX_TWI0_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWI1
#define NRFX_TWI1_ENABLED 1
#endif

#ifdef CONFIG_NRFX_TWIM
#define NRFX_TWIM_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIM_LOG
#define NRFX_TWIM_CONFIG_LOG_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIM0
#define NRFX_TWIM0_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIM1
#define NRFX_TWIM1_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIM2
#define NRFX_TWIM2_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIM3
#define NRFX_TWIM3_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIM20
#define NRFX_TWIM20_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIM21
#define NRFX_TWIM21_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIM22
#define NRFX_TWIM22_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIM30
#define NRFX_TWIM30_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIM120
#define NRFX_TWIM120_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIM130
#define NRFX_TWIM130_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIM131
#define NRFX_TWIM131_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIM132
#define NRFX_TWIM132_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIM133
#define NRFX_TWIM133_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIM134
#define NRFX_TWIM134_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIM135
#define NRFX_TWIM135_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIM136
#define NRFX_TWIM136_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIM137
#define NRFX_TWIM137_ENABLED 1
#endif

#ifdef CONFIG_NRFX_TWIS
#define NRFX_TWIS_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIS_LOG
#define NRFX_TWIS_CONFIG_LOG_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIS0
#define NRFX_TWIS0_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIS1
#define NRFX_TWIS1_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIS2
#define NRFX_TWIS2_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIS3
#define NRFX_TWIS3_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIS20
#define NRFX_TWIS20_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIS21
#define NRFX_TWIS21_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIS22
#define NRFX_TWIS22_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIS30
#define NRFX_TWIS30_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIS130
#define NRFX_TWIS130_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIS131
#define NRFX_TWIS131_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIS132
#define NRFX_TWIS132_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIS133
#define NRFX_TWIS133_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIS134
#define NRFX_TWIS134_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIS135
#define NRFX_TWIS135_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIS136
#define NRFX_TWIS136_ENABLED 1
#endif
#ifdef CONFIG_NRFX_TWIS137
#define NRFX_TWIS137_ENABLED 1
#endif

#ifdef CONFIG_NRFX_UART
#define NRFX_UART_ENABLED 1
#endif
#ifdef CONFIG_NRFX_UART_LOG
#define NRFX_UART_CONFIG_LOG_ENABLED 1
#endif
#ifdef CONFIG_NRFX_UART0
#define NRFX_UART0_ENABLED 1
#endif

#ifdef CONFIG_NRFX_UARTE
#define NRFX_UARTE_ENABLED 1
#endif
#ifdef CONFIG_NRFX_UARTE_LOG
#define NRFX_UARTE_CONFIG_LOG_ENABLED 1
#endif
#ifdef CONFIG_NRFX_UARTE0
#define NRFX_UARTE0_ENABLED 1
#endif
#ifdef CONFIG_NRFX_UARTE1
#define NRFX_UARTE1_ENABLED 1
#endif
#ifdef CONFIG_NRFX_UARTE2
#define NRFX_UARTE2_ENABLED 1
#endif
#ifdef CONFIG_NRFX_UARTE3
#define NRFX_UARTE3_ENABLED 1
#endif
#ifdef CONFIG_NRFX_UARTE00
#define NRFX_UARTE00_ENABLED 1
#endif
#ifdef CONFIG_NRFX_UARTE20
#define NRFX_UARTE20_ENABLED 1
#endif
#ifdef CONFIG_NRFX_UARTE21
#define NRFX_UARTE21_ENABLED 1
#endif
#ifdef CONFIG_NRFX_UARTE22
#define NRFX_UARTE22_ENABLED 1
#endif
#ifdef CONFIG_NRFX_UARTE30
#define NRFX_UARTE30_ENABLED 1
#endif
#ifdef CONFIG_NRFX_UARTE120
#define NRFX_UARTE120_ENABLED 1
#endif
#ifdef CONFIG_NRFX_UARTE130
#define NRFX_UARTE130_ENABLED 1
#endif
#ifdef CONFIG_NRFX_UARTE131
#define NRFX_UARTE131_ENABLED 1
#endif
#ifdef CONFIG_NRFX_UARTE132
#define NRFX_UARTE132_ENABLED 1
#endif
#ifdef CONFIG_NRFX_UARTE133
#define NRFX_UARTE133_ENABLED 1
#endif
#ifdef CONFIG_NRFX_UARTE134
#define NRFX_UARTE134_ENABLED 1
#endif
#ifdef CONFIG_NRFX_UARTE135
#define NRFX_UARTE135_ENABLED 1
#endif
#ifdef CONFIG_NRFX_UARTE136
#define NRFX_UARTE136_ENABLED 1
#endif
#ifdef CONFIG_NRFX_UARTE137
#define NRFX_UARTE137_ENABLED 1
#endif
#ifdef CONFIG_NRFX_UARTE_CONFIG_SKIP_GPIO_CONFIG
#define NRFX_UARTE_CONFIG_SKIP_GPIO_CONFIG 1
#endif
#ifdef CONFIG_NRFX_UARTE_CONFIG_SKIP_PSEL_CONFIG
#define NRFX_UARTE_CONFIG_SKIP_PSEL_CONFIG 1
#endif
#ifdef CONFIG_NRFX_UARTE_CONFIG_TX_LINK
#define NRFX_UARTE_CONFIG_TX_LINK 1
#endif
#ifdef CONFIG_NRFX_UARTE_CONFIG_RX_CACHE_ENABLED
#define NRFX_UARTE_CONFIG_RX_CACHE_ENABLED 1
#endif

#ifdef CONFIG_NRFX_USBREG
#define NRFX_USBREG_ENABLED 1
#endif
#ifdef CONFIG_NRFX_USBREG_LOG
#define NRFX_USBREG_CONFIG_LOG_ENABLED 1
#endif

#ifdef CONFIG_NRFX_WDT
#define NRFX_WDT_ENABLED 1
#endif
#ifdef CONFIG_WDT_NRFX_NO_IRQ
#define NRFX_WDT_CONFIG_NO_IRQ 1
#endif
#ifdef CONFIG_NRFX_WDT_LOG
#define NRFX_WDT_CONFIG_LOG_ENABLED 1
#endif
#ifdef CONFIG_NRFX_WDT0
#define NRFX_WDT0_ENABLED 1
#endif
#ifdef CONFIG_NRFX_WDT1
#define NRFX_WDT1_ENABLED 1
#endif
#ifdef CONFIG_NRFX_WDT30
#define NRFX_WDT30_ENABLED 1
#endif
#ifdef CONFIG_NRFX_WDT31
#define NRFX_WDT31_ENABLED 1
#endif
#ifdef CONFIG_NRFX_WDT010
#define NRFX_WDT010_ENABLED 1
#endif
#ifdef CONFIG_NRFX_WDT011
#define NRFX_WDT011_ENABLED 1
#endif
#ifdef CONFIG_NRFX_WDT130
#define NRFX_WDT130_ENABLED 1
#endif
#ifdef CONFIG_NRFX_WDT131
#define NRFX_WDT131_ENABLED 1
#endif
#ifdef CONFIG_NRFX_WDT132
#define NRFX_WDT132_ENABLED 1
#endif

#ifdef CONFIG_NRF52_ANOMALY_109_WORKAROUND
#define NRFX_SPIM_NRF52_ANOMALY_109_WORKAROUND_ENABLED 1
#define NRFX_SPIS_NRF52_ANOMALY_109_WORKAROUND_ENABLED 1
#define NRFX_TWIM_NRF52_ANOMALY_109_WORKAROUND_ENABLED 1
#define NRFX_PWM_NRF52_ANOMALY_109_WORKAROUND_ENABLED 1
#define NRFX_PWM_NRF52_ANOMALY_109_EGU_INSTANCE \
	CONFIG_NRF52_ANOMALY_109_WORKAROUND_EGU_INSTANCE
#endif

#if defined(CONFIG_SOC_SERIES_BSIM_NRFXX)
#include "nrfx_config_bsim.h"
#endif

/*
 * For chips with TrustZone support, MDK provides CMSIS-Core peripheral
 * accessing symbols in two flavors, with secure and non-secure base address
 * mappings. Their names contain the suffix _S or _NS, respectively.
 * Because nrfx HALs and drivers require these peripheral accessing symbols
 * without any suffixes, the following macro is provided that will translate
 * their names according to the kind of the target that is built.
 */
#if defined(NRF_TRUSTZONE_NONSECURE)
#define NRF_PERIPH(P) P##_NS
#else
#define NRF_PERIPH(P) P##_S
#endif

#define NRFX_CONFIG_BIT_DT(node_id, prop, idx) BIT(DT_PROP_BY_IDX(node_id, prop, idx))
#define NRFX_CONFIG_MASK_DT(node_id, prop) \
	(COND_CODE_1(DT_NODE_HAS_PROP(node_id, prop), \
		(DT_FOREACH_PROP_ELEM_SEP(node_id, prop, NRFX_CONFIG_BIT_DT, (|))), \
		(0)))

/* If the GRTC system timer driver is to be used, prepare definitions required
 * by the nrfx_grtc driver (NRFX_GRTC_CONFIG_ALLOWED_CC_CHANNELS_MASK and
 * NRFX_GRTC_CONFIG_NUM_OF_CC_CHANNELS) based on information from devicetree.
 */
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_grtc)
#define NRFX_GRTC_CONFIG_ALLOWED_CC_CHANNELS_MASK \
	(NRFX_CONFIG_MASK_DT(DT_INST(0, nordic_nrf_grtc), owned_channels) & \
	 ~NRFX_CONFIG_MASK_DT(DT_INST(0, nordic_nrf_grtc), child_owned_channels))
#define NRFX_GRTC_CONFIG_NUM_OF_CC_CHANNELS \
	(DT_PROP_LEN_OR(DT_INST(0, nordic_nrf_grtc), owned_channels, 0) - \
	 DT_PROP_LEN_OR(DT_INST(0, nordic_nrf_grtc), child_owned_channels, 0))
#endif /* DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_grtc) */

/* If global of local DPPIC peripherals are used, provide the following macro
 * definitions required by the interconnect/apb layer:
 * - NRFX_DPPI_PUB_CONFIG_ALLOWED_CHANNELS_MASK_BY_INST_NUM(inst_num)
 * - NRFX_DPPI_SUB_CONFIG_ALLOWED_CHANNELS_MASK_BY_INST_NUM(inst_num)
 * - NRFX_DPPI_PUB_OR_SUB_MASK(inst_num)
 * - NRFX_DPPI_CHANNELS_SINGLE_VAR_NAME_BY_INST_NUM(inst_num)
 * - NRFX_INTERCONNECT_APB_GLOBAL_DPPI_DEFINE
 * - NRFX_INTERCONNECT_APB_LOCAL_DPPI_DEFINE
 * based on information from devicetree.
 */
#if	DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_dppic_global) || \
	DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_dppic_local)
#ifndef NRFX_DPPI_ENABLED
#define NRFX_DPPI_ENABLED 1
#endif
/* Source (publish) channels masks generation. */
#define NRFX_DPPI_PUB_CONFIG_ALLOWED_CHANNELS_MASK_BY_INST_NUM(inst_num) \
	NRFX_CONFIG_MASK_DT(DT_NODELABEL(_CONCAT(dppic, inst_num)), source_channels)

/* Sink (subscribe) channels masks generation. */
#define NRFX_DPPI_SUB_CONFIG_ALLOWED_CHANNELS_MASK_BY_INST_NUM(inst_num) \
	NRFX_CONFIG_MASK_DT(DT_NODELABEL(_CONCAT(dppic, inst_num)), sink_channels)

#define NRFX_DPPI_PUB_OR_SUB_MASK(inst_num) \
	UTIL_OR(DT_NODE_HAS_PROP(DT_NODELABEL(_CONCAT(dppic, inst_num)), source_channels), \
		DT_NODE_HAS_PROP(DT_NODELABEL(_CONCAT(dppic, inst_num)), sink_channels))

/* Variables names generation. */
#define NRFX_CONFIG_DPPI_CHANNELS_ENTRY_NAME(node_id) _CONCAT(_CONCAT(m_, node_id), _channels)
#define NRFX_DPPI_CHANNELS_SINGLE_VAR_NAME_BY_INST_NUM(inst_num) \
	NRFX_CONFIG_DPPI_CHANNELS_ENTRY_NAME(DT_NODELABEL(_CONCAT(dppic, inst_num)))

/* Variables entries generation. */
#define NRFX_CONFIG_DPPI_CHANNELS_ENTRY(node_id) \
	static nrfx_atomic_t NRFX_CONFIG_DPPI_CHANNELS_ENTRY_NAME(node_id) \
		__attribute__((used)) = \
		NRFX_CONFIG_MASK_DT(node_id, source_channels) | \
		NRFX_CONFIG_MASK_DT(node_id, sink_channels);
#define NRFX_INTERCONNECT_APB_GLOBAL_DPPI_DEFINE \
	DT_FOREACH_STATUS_OKAY(nordic_nrf_dppic_global, NRFX_CONFIG_DPPI_CHANNELS_ENTRY)
#define NRFX_INTERCONNECT_APB_LOCAL_DPPI_DEFINE \
	DT_FOREACH_STATUS_OKAY(nordic_nrf_dppic_local, NRFX_CONFIG_DPPI_CHANNELS_ENTRY)
#endif /* DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_dppic_global) || ... */

/* If local or global DPPIC peripherals are used, provide the following macro
 * definitions required by the interconnect/ipct layer:
 * - NRFX_IPCTx_PUB_CONFIG_ALLOWED_CHANNELS_MASK_BY_INST_NUM(inst_num)
 * - NRFX_IPCTx_SUB_CONFIG_ALLOWED_CHANNELS_MASK_BY_INST_NUM(inst_num)
 * - NRFX_IPCT_PUB_OR_SUB_MASK(inst_num)
 * - NRFX_IPCTx_CHANNELS_SINGLE_VAR_NAME_BY_INST_NUM(inst_num)
 * - NRFX_INTERCONNECT_IPCT_GLOBAL_DEFINE
 * - NRFX_INTERCONNECT_IPCT_LOCAL_DEFINE
 * based on information from devicetree.
 */
#if	DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_ipct_global) || \
	DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_ipct_local)
/* Channels masks generation. */
#define NRFX_CONFIG_IPCT_MASK_DT(node_id) \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, owned_channels), \
		(NRFX_CONFIG_MASK_DT(node_id, owned_channels)), \
		(COND_CODE_1(DT_NODE_HAS_COMPAT(node_id, nordic_nrf_ipct_local), \
			(BIT_MASK(DT_PROP(node_id, channels))), (0))))

#if defined(NRF_APPLICATION)
#define NRFX_CONFIG_IPCT_LOCAL_NODE DT_NODELABEL(cpuapp_ipct)
#elif defined(NRF_RADIOCORE)
#define NRFX_CONFIG_IPCT_LOCAL_NODE DT_NODELABEL(cpurad_ipct)
#endif
#define NRFX_CONFIG_IPCT_NODE_BY_INST_NUM(inst_num) \
	COND_CODE_1(IS_EMPTY(inst_num), \
		(NRFX_CONFIG_IPCT_LOCAL_NODE), \
		(DT_NODELABEL(_CONCAT(ipct, inst_num))))

#define NRFX_IPCTx_PUB_CONFIG_ALLOWED_CHANNELS_MASK_BY_INST_NUM(inst_num) \
	NRFX_CONFIG_IPCT_MASK_DT(NRFX_CONFIG_IPCT_NODE_BY_INST_NUM(inst_num))

#define NRFX_IPCTx_SUB_CONFIG_ALLOWED_CHANNELS_MASK_BY_INST_NUM(inst_num) \
	NRFX_CONFIG_IPCT_MASK_DT(NRFX_CONFIG_IPCT_NODE_BY_INST_NUM(inst_num))

#define NRFX_IPCT_PUB_OR_SUB_MASK(inst_num) \
	COND_CODE_1(IS_EMPTY(inst_num), \
		(DT_NODE_HAS_STATUS_OKAY(NRFX_CONFIG_IPCT_LOCAL_NODE)), \
		(DT_NODE_HAS_PROP(DT_NODELABEL(_CONCAT(ipct, inst_num)), owned_channels)))

/* Variables names generation. */
#define NRFX_CONFIG_IPCT_CHANNELS_ENTRY_NAME(node_id) _CONCAT(_CONCAT(m_, node_id), _channels)
#define NRFX_IPCTx_CHANNELS_SINGLE_VAR_NAME_BY_INST_NUM(inst_num) \
	COND_CODE_1(IS_EMPTY(inst_num), \
		(NRFX_CONFIG_IPCT_CHANNELS_ENTRY_NAME(NRFX_CONFIG_IPCT_LOCAL_NODE)), \
		(NRFX_CONFIG_IPCT_CHANNELS_ENTRY_NAME(DT_NODELABEL(_CONCAT(ipct, inst_num)))))

/* Variables entries generation. */
#define NRFX_CONFIG_IPCT_CHANNELS_ENTRY(node_id) \
	static nrfx_atomic_t NRFX_CONFIG_IPCT_CHANNELS_ENTRY_NAME(node_id) \
		__attribute__((used)) = \
		NRFX_CONFIG_IPCT_MASK_DT(node_id);
#define NRFX_INTERCONNECT_IPCT_LOCAL_DEFINE \
	DT_FOREACH_STATUS_OKAY(nordic_nrf_ipct_local, NRFX_CONFIG_IPCT_CHANNELS_ENTRY)
#define NRFX_INTERCONNECT_IPCT_GLOBAL_DEFINE \
	DT_FOREACH_STATUS_OKAY(nordic_nrf_ipct_global, NRFX_CONFIG_IPCT_CHANNELS_ENTRY)
#endif /* DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_ipct_global) || ... */

#include <nrfx_config_common.h>
#if defined(NRF51)
    #include <nrfx_config_nrf51.h>
#elif defined(NRF52805_XXAA)
    #include <nrfx_config_nrf52805.h>
#elif defined(NRF52810_XXAA)
    #include <nrfx_config_nrf52810.h>
#elif defined(NRF52811_XXAA)
    #include <nrfx_config_nrf52811.h>
#elif defined(NRF52820_XXAA)
    #include <nrfx_config_nrf52820.h>
#elif defined(NRF52832_XXAA) || defined (NRF52832_XXAB)
    #include <nrfx_config_nrf52832.h>
#elif defined(NRF52833_XXAA)
    #include <nrfx_config_nrf52833.h>
#elif defined(NRF52840_XXAA)
    #include <nrfx_config_nrf52840.h>
#elif defined(NRF5340_XXAA_APPLICATION)
    #include <nrfx_config_nrf5340_application.h>
#elif defined(NRF5340_XXAA_NETWORK)
    #include <nrfx_config_nrf5340_network.h>
#elif (defined(NRF54H20_XXAA) || defined(NRF54H20_ENGB_XXAA)) && defined(NRF_APPLICATION)
    #include <nrfx_config_nrf54h20_application.h>
#elif (defined(NRF54H20_XXAA) || defined(NRF54H20_ENGB_XXAA)) && defined(NRF_RADIOCORE)
    #include <nrfx_config_nrf54h20_radiocore.h>
#elif (defined(NRF54H20_XXAA) || defined(NRF54H20_ENGB_XXAA)) && defined(NRF_PPR)
    #include <nrfx_config_nrf54h20_ppr.h>
#elif (defined(NRF54H20_XXAA) || defined(NRF54H20_ENGB_XXAA)) && defined(NRF_FLPR)
    #include <nrfx_config_nrf54h20_flpr.h>
#elif defined(NRF54L15_XXAA) && defined(NRF_APPLICATION)
    #include <nrfx_config_nrf54l15_application.h>
#elif defined(NRF54L15_XXAA) && defined(NRF_FLPR)
    #include <nrfx_config_nrf54l15_flpr.h>
#elif (defined(NRF54L20_XXAA) || defined(NRF54L20_ENGA_XXAA)) && defined(NRF_APPLICATION)
    #include <nrfx_config_nrf54l20_enga_application.h>
#elif defined(NRF9120_XXAA) || defined(NRF9160_XXAA)
    #include <nrfx_config_nrf91.h>
#elif defined(NRF9230_ENGB_XXAA) && defined(NRF_APPLICATION)
    #include <nrfx_config_nrf9230_engb_application.h>
#elif defined(NRF9230_ENGB_XXAA) && defined(NRF_RADIOCORE)
    #include <nrfx_config_nrf9230_engb_radiocore.h>
#elif defined(NRF9230_ENGB_XXAA) && defined(NRF_PPR)
    #include <nrfx_config_nrf9230_engb_ppr.h>
#else
    #include <nrfx_config_ext.h>
#endif

#endif // NRFX_CONFIG_H__
