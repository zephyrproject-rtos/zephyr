/*
 * Copyright (c) 2026 Testo SE & Co. KGaA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ADC_ADS1X2S14_ADC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ADC_ADS1X2S14_ADC_H_

/*
 * Analog inputs, used for zephyr,input-positive, zephyr,input-negative and
 * zephyr,current-source-pin.
 */
#define ADS1X2S14_AIN0 0
#define ADS1X2S14_AIN1 1
#define ADS1X2S14_AIN2 2
#define ADS1X2S14_AIN3 3
#define ADS1X2S14_AIN4 4
#define ADS1X2S14_AIN5 5
#define ADS1X2S14_AIN6 6
#define ADS1X2S14_AIN7 7

/* Speed modes, selecting the modulator clock frequency fMOD */
#define ADS1X2S14_SPEED_MODE_0 0 /* fMOD = 32 kHz */
#define ADS1X2S14_SPEED_MODE_1 1 /* fMOD = 256 kHz */
#define ADS1X2S14_SPEED_MODE_2 2 /* fMOD = 512 kHz */
#define ADS1X2S14_SPEED_MODE_3 3 /* fMOD = 1.024 MHz */

/*
 * Digital filter settings. The output data rate is fMOD / OSR for the
 * oversampling ratio settings. The 20 SPS and 25 SPS settings provide
 * 50 Hz and 60 Hz line-cycle rejection in every speed mode.
 */
#define ADS1X2S14_OSR_16   0
#define ADS1X2S14_OSR_32   1
#define ADS1X2S14_OSR_128  2
#define ADS1X2S14_OSR_256  3
#define ADS1X2S14_OSR_512  4
#define ADS1X2S14_OSR_1024 5
#define ADS1X2S14_DR_25SPS 6
#define ADS1X2S14_DR_20SPS 7

/*
 * Value for the acquisition time in ADC_ACQ_TIME_TICKS unit, combining a
 * speed mode with a digital filter setting.
 */
#define ADS1X2S14_DATA_RATE(speed_mode, fltr_osr) (((speed_mode) << 3) | (fltr_osr))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ADC_ADS1X2S14_ADC_H_ */
