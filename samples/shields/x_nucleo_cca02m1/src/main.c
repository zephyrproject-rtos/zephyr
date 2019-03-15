/*
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr.h>
#include <misc/printk.h>

#include <gpio.h>
#include <led.h>

#include <audio/dmic.h>

/* uncomment if you want PCM output in ascii */
/*#define PCM_OUTPUT_IN_ASCII		1  */

#define STEREO	1

#define AUDIO_FREQ		16000
#define CHAN_SIZE		16
#ifdef STEREO
#define PCM_BLK_SIZE_MS		(((AUDIO_FREQ/1000) * sizeof(s16_t)) * 2)
#else
#define PCM_BLK_SIZE_MS		((AUDIO_FREQ/1000) * sizeof(s16_t))
#endif

#define NUM_MS		1000

K_MEM_SLAB_DEFINE(rx_mem_slab, PCM_BLK_SIZE_MS, NUM_MS, 1);

struct pcm_stream_cfg mic_streams = {
	.pcm_rate = AUDIO_FREQ,
	.pcm_width = CHAN_SIZE,
	.block_size = PCM_BLK_SIZE_MS,
	.mem_slab = &rx_mem_slab,
};

struct dmic_cfg cfg = {
	.io = {
		/* requesting a pdm freq around 2MHz */
		.min_pdm_clk_freq = 1800000,
		.max_pdm_clk_freq = 2500000,
	},
	.streams = &mic_streams,
	.channel = {
#ifdef STEREO
		.req_num_chan = 2,
#else
		.req_num_chan = 1,
#endif
	},
};

void *rx_block[NUM_MS];
size_t rx_size = PCM_BLK_SIZE_MS;

#ifdef STEREO
#include <stm32f4xx_ll_tim.h>

/*
 * MIC_CLK_x2:     PB4 (TIM3_CH1) - IN
 * MIC_CLK_NUCLEO: PB5 (TIM3_CH2) - OUT
 */
static int AUDIO_IN_Timer_Init(void)
{
	TIM_TypeDef *tim3 = (TIM_TypeDef *)DT_TIM_STM32_3_BASE_ADDRESS;
	LL_TIM_IC_InitTypeDef sICConfig;
	LL_TIM_InitTypeDef sConfig;
	LL_TIM_OC_InitTypeDef ocConfig;

	/* Enable TIM3 peripheral clock */
	LL_APB1_GRP1_EnableClock(DT_TIM_STM32_3_CLOCK_BITS);

	/* Configure the Input: channel_1 */
	LL_TIM_IC_StructInit(&sICConfig);
	sICConfig.ICPolarity  = LL_TIM_IC_POLARITY_RISING;
	sICConfig.ICActiveInput = LL_TIM_ACTIVEINPUT_DIRECTTI;
	sICConfig.ICPrescaler = LL_TIM_ICPSC_DIV1;
	sICConfig.ICFilter = 0;
	if (LL_TIM_IC_Init(tim3, LL_TIM_CHANNEL_CH1, &sICConfig) == ERROR) {
		return -1;
	}

	/* Configure TIM3 in Gated Slave mode for the external trigger
	 * (Filtered Timer Input 1) */
	LL_TIM_SetSlaveMode(tim3, LL_TIM_CLOCKSOURCE_EXT_MODE1);
	LL_TIM_SetTriggerInput(tim3, LL_TIM_TS_TI1FP1);

	/* Initialize TIM3 counter */
	LL_TIM_StructInit(&sConfig);
	sConfig.Prescaler = 0;
	sConfig.ClockDivision = 0;
	sConfig.Autoreload = 1;
	sConfig.CounterMode = LL_TIM_COUNTERMODE_UP;
	sConfig.RepetitionCounter = 0;
	if (LL_TIM_Init(tim3, &sConfig) == ERROR) {
		return -1;
	}

	LL_TIM_SetTriggerOutput(tim3, LL_TIM_TRGO_RESET);

	/* Initialize TIM3 peripheral in PWM mode */
	LL_TIM_OC_StructInit(&ocConfig);
	ocConfig.OCMode = LL_TIM_OCMODE_PWM1;
	ocConfig.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
	if (LL_TIM_OC_Init(tim3, LL_TIM_CHANNEL_CH2, &ocConfig) == ERROR) {
		return -1;
	}

	LL_TIM_OC_SetCompareCH2(tim3, 1);

	/* Enable TIM3 */
	LL_TIM_CC_EnableChannel(tim3, LL_TIM_CHANNEL_CH1 | LL_TIM_CHANNEL_CH2);
	LL_TIM_EnableCounter(tim3);

	return 0;
}
#endif

void main(void)
{
	int i;
	u32_t ms;

	printk("x_nucleo_cca02m1 test!!\n");

	int ret;

	struct device *mic_dev = device_get_binding(DT_ST_MPXXDTYY_0_LABEL);

	if (!mic_dev) {
		printk("Could not get pointer to %s device\n",
			DT_ST_MPXXDTYY_0_LABEL);
		return;
	}

#ifdef STEREO
	ret = AUDIO_IN_Timer_Init();
	if (ret < 0) {
		printk("timer init error\n");
		return;
	}
#endif

	ret = dmic_configure(mic_dev, &cfg);
	if (ret < 0) {
		printk("microphone configuration error\n");
		return;
	}

	ret = dmic_trigger(mic_dev, DMIC_TRIGGER_START);
	if (ret < 0) {
		printk("microphone start trigger error\n");
		return;
	}

	/* Acquire microphone audio */
	for (ms = 0; ms < NUM_MS; ms++) {
		ret = dmic_read(mic_dev, 0, &rx_block[ms], &rx_size, 2000);
		if (ret < 0) {
			printk("microphone audio read error\n");
			return;
		}
	}

	/* print PCM stream */
#ifdef PCM_OUTPUT_IN_ASCII
	printk("-- start\n");
	int j;

	for (i = 0; i < NUM_MS; i++) {
		u16_t *pcm_out = rx_block[i];

		for (j = 0; j < rx_size/2; j++) {
			printk("0x%04x,\n", pcm_out[j]);
		}
	}
	printk("-- end\n");
#else
	unsigned char pcm_l, pcm_h;
	int j;

	for (i = 0; i < NUM_MS; i++) {
		u16_t *pcm_out = rx_block[i];

		for (j = 0; j < rx_size/2; j++) {
			pcm_l = (char)(pcm_out[j] & 0xFF);
			pcm_h = (char)((pcm_out[j] >> 8) & 0xFF);

			z_impl_k_str_out(&pcm_l, 1);
			z_impl_k_str_out(&pcm_h, 1);
		}
	}
#endif
}
