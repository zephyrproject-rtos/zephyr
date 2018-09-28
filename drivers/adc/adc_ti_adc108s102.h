/* adc-ti-adc108s102.h - Private  */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_ADC_ADC_TI_ADC108S102_H_
#define ZEPHYR_DRIVERS_ADC_ADC_TI_ADC108S102_H_

#include <spi.h>
#include <adc.h>

#include <misc/byteorder.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Internal return code */
#define ADC108S102_DONE				(-1)
/* 8 chans maximum + 1 dummy, 16 bits per-chans -> 18 bytes */
#define ADC108S102_CMD_BUFFER_SIZE		9
/* 1 dummy + 8 results maximum, 16 bits per-chans -> 18 bytes */
#define ADC108S102_SAMPLING_BUFFER_SIZE		9
#define ADC108S102_CHANNELS			8
#define ADC108S102_CHANNELS_SIZE		\
			(ADC108S102_CHANNELS * sizeof(u32_t))
#define ADC108S102_CHANNEL_CMD(_channel_)	\
			sys_cpu_to_be16((_channel_ << 8) << 3)
#define ADC108S102_RESULT_MASK			0xfff /* 12 bits resolution */
#define ADC108S102_RESULT(_res_)		\
			(sys_be16_to_cpu(_res_) & ADC108S102_RESULT_MASK)

struct ti_adc108s102_chan {
	u32_t buf_idx;
};

struct ti_adc108s102_data {
	u16_t cmd_buffer[ADC108S102_CMD_BUFFER_SIZE];
	u16_t sampling_buffer[ADC108S102_SAMPLING_BUFFER_SIZE];
	struct device *spi;
	struct spi_config spi_cfg;
	struct ti_adc108s102_chan chans[ADC108S102_CHANNELS];
	struct adc_seq_table *seq_table;

	u8_t cmd_buf_len;
	u8_t sampling_buf_len;
	u8_t stride[2];
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_ADC_ADC_TI_ADC108S102_H_ */
