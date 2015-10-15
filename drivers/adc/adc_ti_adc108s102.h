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

/* 8 chans maximum + 1 dummy, 16 bits per-chans -> 18 bytes */
#define ADC108S102_CMD_BUFFER_SIZE		9
/* 17 maximum dummy bytes + 16 result bytes maximum + 4 timestamp bytes */
#define ADC108S102_SAMPLING_BUFFER_SIZE		13
#define ADC108S102_SAMPLING_STACK_SIZE		128
#define ADC108S102_CHANNELS			8
#define ADC108S102_CHANNELS_SIZE		\
			(ADC108S102_CHANNELS * sizeof(uint32_t))
#define ADC108S102_CHANNEL_CMD(_channel_)	\
			sys_cpu_to_be16((_channel_ << 8) << 3)

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
	char __stack sampling_stack[ADC108S102_SAMPLING_STACK_SIZE];
	uint16_t cmd_buffer[ADC108S102_CMD_BUFFER_SIZE];
	uint8_t cmd_buf_len;
	uint16_t sampling_buffer[ADC108S102_SAMPLING_BUFFER_SIZE];
	uint8_t sampling_buf_len;
	struct device *spi;
	struct ti_adc108s102_chan chans[ADC108S102_CHANNELS];
	struct adc_seq_table *seq_table;
	adc_callback_t cb;
};

#endif /* __ADC108S102_PRIV_H__ */
