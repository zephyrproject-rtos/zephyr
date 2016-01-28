/* adc-ti-adc108s102.h - Private  */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __ADC108S102_PRIV_H__
#define __ADC108S102_PRIV_H__

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
			(ADC108S102_CHANNELS * sizeof(uint32_t))
#define ADC108S102_CHANNEL_CMD(_channel_)	\
			sys_cpu_to_be16((_channel_ << 8) << 3)
#define ADC108S102_RESULT_MASK			0xfff /* 12 bits resolution */
#define ADC108S102_RESULT(_res_)		\
			(sys_be16_to_cpu(_res_) & ADC108S102_RESULT_MASK)

struct ti_adc108s102_config {
	const char *spi_port;
	uint32_t spi_config_flags;
	uint32_t spi_freq;
	uint32_t spi_slave;
};

struct ti_adc108s102_chan {
	uint32_t buf_idx;
};

struct ti_adc108s102_data {
	uint16_t cmd_buffer[ADC108S102_CMD_BUFFER_SIZE];
	uint16_t sampling_buffer[ADC108S102_SAMPLING_BUFFER_SIZE];
	struct device *spi;
	struct ti_adc108s102_chan chans[ADC108S102_CHANNELS];
	struct adc_seq_table *seq_table;

	uint8_t cmd_buf_len;
	uint8_t sampling_buf_len;
	uint8_t stride[2];
};

#ifdef __cplusplus
}
#endif

#endif /* __ADC108S102_PRIV_H__ */
