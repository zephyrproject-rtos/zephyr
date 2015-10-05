/* adc-ti-adc108s102.h - Private  */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
