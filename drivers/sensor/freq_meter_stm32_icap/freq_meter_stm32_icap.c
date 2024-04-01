/*
 * Copyright (c) 2024 Freedom Veiculos Eletricos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_icap_freq_meter

#include <stm32_ll_bus.h>
#include <stm32_ll_tim.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_dma.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>

#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/dma/dma_stm32.h>
#include <zephyr/drivers/dma.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(freq_meter_stm32_icap, CONFIG_SENSOR_LOG_LEVEL);

#define TIMER_MAX_CH 4U

/* clang-format off */
#define NUM_CH(timx)					    \
	(IS_TIM_CCX_INSTANCE(timx, TIM_CHANNEL_4) ? 4U :    \
	 (IS_TIM_CCX_INSTANCE(timx, TIM_CHANNEL_3) ? 3U :   \
	  (IS_TIM_CCX_INSTANCE(timx, TIM_CHANNEL_2) ? 2U :  \
	   (IS_TIM_CCX_INSTANCE(timx, TIM_CHANNEL_1) ? 1U : \
	    0))))

#if !defined(CONFIG_SOC_SERIES_STM32F4X) &&                 \
	!defined(CONFIG_SOC_SERIES_STM32G4X) &&             \
	!defined(CONFIG_SOC_SERIES_STM32MP1X)
static uint32_t(*const get_timer_capture[TIMER_MAX_CH])(const TIM_TypeDef *) = {
	LL_TIM_IC_GetCaptureCH1, LL_TIM_IC_GetCaptureCH2,
	LL_TIM_IC_GetCaptureCH3, LL_TIM_IC_GetCaptureCH4,
};
#else
static uint32_t(*const get_timer_capture[TIMER_MAX_CH])(TIM_TypeDef *) = {
	LL_TIM_IC_GetCaptureCH1, LL_TIM_IC_GetCaptureCH2,
	LL_TIM_IC_GetCaptureCH3, LL_TIM_IC_GetCaptureCH4,
};
#endif

#if defined(CONFIG_SOC_SERIES_STM32F7X)
static uint32_t dma_stm32_slot_to_channel(uint32_t slot)
{
	static const uint32_t channel_nr[] = {
		LL_DMA_CHANNEL_0,
		LL_DMA_CHANNEL_1,
		LL_DMA_CHANNEL_2,
		LL_DMA_CHANNEL_3,
		LL_DMA_CHANNEL_4,
		LL_DMA_CHANNEL_5,
		LL_DMA_CHANNEL_6,
		LL_DMA_CHANNEL_7,
	};

	__ASSERT_NO_MSG(slot < ARRAY_SIZE(channel_nr));

	return channel_nr[slot];
}
#endif
/* clang-format on */

struct icap_dma_stream {
	const struct device *dma_dev;
	DMA_TypeDef *dma;
	uint32_t dma_channel;
	struct dma_config dma_cfg;
	struct dma_block_config blk_cfg;
	int fifo_threshold;
	bool enabled;
};

struct freq_meter_stm32_icap_data {
	struct icap_dma_stream dma_cc;
	uint32_t samples[2];
	uint32_t frequency;
	uint16_t index;

	const struct reset_dt_spec reset;
};

struct freq_meter_stm32_icap_config {
	TIM_TypeDef *timer;
	struct freq_meter_stm32_icap_data *data;
	uint32_t prescaler;
	uint32_t max_top_value;
	uint8_t channels;
	struct stm32_pclken pclken;
	void (*irq_config_func)(const struct device *dev);
	uint32_t irqn;
	const struct pinctrl_dev_config *pcfg;
};

static int freq_meter_stm32_icap_calc_freq(const struct device *dev)
{
	const struct freq_meter_stm32_icap_config *cfg = dev->config;
	struct freq_meter_stm32_icap_data *data        = dev->data;

	uint32_t uwDiffCapture;
	uint32_t TIMxCLK;
	uint32_t PSC;
	uint32_t ICxPSC;
	uint32_t ICxPolarity;

	if (data->samples[1] > data->samples[0]) {
		uwDiffCapture = (data->samples[1] - data->samples[0]);
	} else if (data->samples[1] < data->samples[0]) {
		uwDiffCapture = ((cfg->max_top_value - data->samples[0]) + data->samples[1]) + 1;
		if (!IS_TIM_32B_COUNTER_INSTANCE(cfg->timer)) {
			uwDiffCapture = cfg->max_top_value - uwDiffCapture;
		}
	} else {
		LOG_DBG("No imput frequency or limit reached.");
		data->frequency = 0;

		return -EAGAIN;
	}

	TIMxCLK = SystemCoreClock;
	PSC     = LL_TIM_GetPrescaler(cfg->timer);
	ICxPSC  = __LL_TIM_GET_ICPSC_RATIO(LL_TIM_IC_GetPrescaler(cfg->timer, LL_TIM_CHANNEL_CH1));

	if (LL_TIM_IC_GetPolarity(cfg->timer, LL_TIM_CHANNEL_CH1) == LL_TIM_IC_POLARITY_BOTHEDGE) {
		ICxPolarity = 2;
	} else {
		ICxPolarity = 1;
	}

	LOG_DBG("A: %d, B: %d", data->samples[0], data->samples[1]);
	LOG_DBG("TIMxCLK: %d, ICxPSC: %d, uwDiffCapture: %d, PSC: %d, ICxPolarity: %d", TIMxCLK,
		ICxPSC, uwDiffCapture, PSC, ICxPolarity);

	data->frequency = (TIMxCLK * ICxPSC) / (uwDiffCapture * (PSC + 1) * ICxPolarity);

	return 0;
}

static int freq_meter_stm32_icap_sample_fetch(const struct device *dev,
					      enum sensor_channel chan)
{
	struct freq_meter_stm32_icap_data *data = dev->data;

	if (chan != SENSOR_CHAN_ALL) {
		LOG_ERR("Unsupported sensor channel");
		return -ENOTSUP;
	}

	freq_meter_stm32_icap_calc_freq(dev);

	data->index      = 0;
	data->samples[0] = 0;
	data->samples[1] = 0;

	return 0;
}

static int freq_meter_stm32_icap_channel_get(const struct device *dev,
					     enum sensor_channel chan,
					     struct sensor_value *val)
{
	struct freq_meter_stm32_icap_data *data = dev->data;

	val->val2 = 0;

	if (chan == SENSOR_CHAN_FREQUENCY) {
		val->val1 = data->frequency;
	} else if (chan == SENSOR_CHAN_RPM) {
		val->val1 = data->frequency * 60;
	} else {
		val->val1 = 0;
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api freq_meter_stm32_icap_driver_api = {
	.sample_fetch = freq_meter_stm32_icap_sample_fetch,
	.channel_get  = freq_meter_stm32_icap_channel_get,
};

static inline int freq_meter_stm32_icap_dma_setup(const struct device *dev)
{
	const struct freq_meter_stm32_icap_config *cfg = dev->config;
	struct freq_meter_stm32_icap_data *data        = dev->data;
	struct icap_dma_stream *dma                    = &data->dma_cc;

#if 0
	struct dma_block_config *blk_cfg = &dma->blk_cfg;
	int ret = 0;

	memset(blk_cfg, 0, sizeof(struct dma_block_config));
	blk_cfg->block_size = 8;
	blk_cfg->dest_address = (uint32_t)&data->samples[0];
	blk_cfg->dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	blk_cfg->dest_reload_en = 1;
	blk_cfg->source_address = (uint32_t)&cfg->timer->CCR1;
	blk_cfg->source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	blk_cfg->source_reload_en = 1;
	blk_cfg->fifo_mode_control = dma->fifo_threshold;

	dma->dma_cfg.head_block = blk_cfg;
	dma->dma_cfg.user_data = NULL;

	ret = dma_config(dma->dma_dev, dma->dma_channel, &dma->dma_cfg);
	if (ret != 0) {
		LOG_ERR("UART ERR: RX DMA config failed!");
		return -EINVAL;
	}
/*
	ret = dma_reload(dma->dma_dev, dma->dma_channel,
			 (uint32_t)&cfg->timer->CCR1,
			 (uint32_t)&data->samples[0], 8);
	if (ret != 0) {
		LOG_ERR("UART ERR: RX DMA config failed!");
		return -EINVAL;
	}
*/
#else
	LL_DMA_ConfigTransfer(dma->dma, dma->dma_channel, LL_DMA_DIRECTION_PERIPH_TO_MEMORY
							| LL_DMA_PRIORITY_HIGH
							| LL_DMA_MODE_CIRCULAR
							| LL_DMA_MEMORY_INCREMENT
							| LL_DMA_PDATAALIGN_WORD
							| LL_DMA_MDATAALIGN_WORD);
# if defined(CONFIG_SOC_SERIES_STM32F7X)
	LL_DMA_SetChannelSelection(dma->dma, dma->dma_channel,
				   dma_stm32_slot_to_channel(dma->dma_cfg.dma_slot));
# endif
	LL_DMA_ConfigAddresses(dma->dma, dma->dma_channel,
			       (uint32_t) &cfg->timer->CCR1,
			       (uint32_t) &data->samples[0],
			       LL_DMA_GetDataTransferDirection(dma->dma, dma->dma_channel));
	LL_DMA_SetDataLength(dma->dma, dma->dma_channel, 2);
#endif
	if (dma_start(dma->dma_dev, dma->dma_channel)) {
		LOG_ERR("UART ERR: RX DMA start failed!");
		return -EFAULT;
	}

	return 0;
}

static void freq_meter_stm32_icap_signal_handler(const struct device *dev, uint32_t id)
{
	const struct freq_meter_stm32_icap_config *cfg = dev->config;
	struct freq_meter_stm32_icap_data *data        = dev->data;

	if (data->index == 0) {
		data->samples[0] = get_timer_capture[id](cfg->timer);
		data->index      = 1;

		return;
	}

	data->index      = 0;

	data->samples[1] = get_timer_capture[id](cfg->timer);
}

#define TIM_IRQ_HANDLE_CC(timx, cc)						\
	do {									\
		bool hw_irq = LL_TIM_IsActiveFlag_CC##cc(cfg->timer)		\
			   && LL_TIM_IsEnabledIT_CC##cc(cfg->timer);		\
		if (hw_irq) {							\
			if (hw_irq) {						\
				LL_TIM_ClearFlag_CC##cc(cfg->timer);		\
			}							\
			freq_meter_stm32_icap_signal_handler(dev, cc - 1U);	\
		}								\
	} while (0)

static void freq_meter_stm32_icap_irq_handler(const struct device *dev)
{
	const struct freq_meter_stm32_icap_config *cfg = dev->config;

	TIM_IRQ_HANDLE_CC(cfg->timer, 1);
}

static int counter_stm32_get_tim_clk(const struct stm32_pclken *pclken,
				     uint32_t *tim_clk)
{
	int r;
	const struct device *clk;
	uint32_t bus_clk, apb_psc;

	clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (!device_is_ready(clk)) {
		return -ENODEV;
	}

	r = clock_control_get_rate(clk, (clock_control_subsys_t) pclken, &bus_clk);
	if (r < 0) {
		return r;
	}

#if defined(CONFIG_SOC_SERIES_STM32H7X)
	if (pclken->bus == STM32_CLOCK_BUS_APB1) {
		apb_psc = STM32_D2PPRE1;
	} else {
		apb_psc = STM32_D2PPRE2;
	}
#else
	if (pclken->bus == STM32_CLOCK_BUS_APB1) {
# if defined(CONFIG_SOC_SERIES_STM32MP1X)
		apb_psc = (uint32_t) (READ_BIT(RCC->APB1DIVR, RCC_APB1DIVR_APB1DIV));
# else
		apb_psc = STM32_APB1_PRESCALER;
# endif
	}
# if !defined(CONFIG_SOC_SERIES_STM32F0X) && !defined(CONFIG_SOC_SERIES_STM32G0X)
	else {
#  if defined(CONFIG_SOC_SERIES_STM32MP1X)
		apb_psc = (uint32_t) (READ_BIT(RCC->APB2DIVR, RCC_APB2DIVR_APB2DIV));
#  else
		apb_psc = STM32_APB2_PRESCALER;
#  endif
	}
# endif
#endif

#if defined(RCC_DCKCFGR_TIMPRE) || defined(RCC_DCKCFGR1_TIMPRE) || defined(RCC_CFGR_TIMPRE)
	/*
	 * There are certain series (some F4, F7 and H7) that have the TIMPRE
	 * bit to control the clock frequency of all the timers connected to
	 * APB1 and APB2 domains.
	 *
	 * Up to a certain threshold value of APB{1,2} prescaler, timer clock
	 * equals to HCLK. This threshold value depends on TIMPRE setting
	 * (2 if TIMPRE=0, 4 if TIMPRE=1). Above threshold, timer clock is set
	 * to a multiple of the APB domain clock PCLK{1,2} (2 if TIMPRE=0, 4 if
	 * TIMPRE=1).
	 */

	if (LL_RCC_GetTIMPrescaler() == LL_RCC_TIM_PRESCALER_TWICE) {
		/* TIMPRE = 0 */
		if (apb_psc <= 2u) {
			LL_RCC_ClocksTypeDef clocks;

			LL_RCC_GetSystemClocksFreq(&clocks);
			*tim_clk = clocks.HCLK_Frequency;
		} else {
			*tim_clk = bus_clk * 2u;
		}
	} else {
		/* TIMPRE = 1 */
		if (apb_psc <= 4u) {
			LL_RCC_ClocksTypeDef clocks;

			LL_RCC_GetSystemClocksFreq(&clocks);
			*tim_clk = clocks.HCLK_Frequency;
		} else {
			*tim_clk = bus_clk * 4u;
		}
	}
#else
	/*
	 * If the APB prescaler equals 1, the timer clock frequencies
	 * are set to the same frequency as that of the APB domain.
	 * Otherwise, they are set to twice (×2) the frequency of the
	 * APB domain.
	 */
	if (apb_psc == 1u) {
		*tim_clk = bus_clk;
	} else {
		*tim_clk = bus_clk * 2u;
	}
#endif

	return 0;
}

static int freq_meter_stm32_icap_init(const struct device *dev)
{
	const struct freq_meter_stm32_icap_config *cfg = dev->config;
	struct freq_meter_stm32_icap_data *data        = dev->data;
	uint32_t tim_clk;
	int r;

	r = clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
			     (clock_control_subsys_t) &cfg->pclken);
	if (r < 0) {
		LOG_ERR("Could not initialize clock (%d)", r);
		return r;
	}
	r = counter_stm32_get_tim_clk(&cfg->pclken, &tim_clk);
	if (r < 0) {
		LOG_ERR("Could not obtain timer clock (%d)", r);
		return r;
	}

	if (!device_is_ready(data->reset.dev)) {
		LOG_ERR("reset controller not ready");
		return -ENODEV;
	}

	r = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (r < 0) {
		LOG_ERR("ICAP pinctrl setup failed (%d)", r);
		return r;
	}

	(void) reset_line_toggle_dt(&data->reset);

	cfg->irq_config_func(dev);

	LL_TIM_IC_SetActiveInput(cfg->timer, LL_TIM_CHANNEL_CH1, LL_TIM_ACTIVEINPUT_DIRECTTI);
	LL_TIM_IC_SetFilter(cfg->timer, LL_TIM_CHANNEL_CH1, LL_TIM_IC_FILTER_FDIV1);
	LL_TIM_IC_SetPrescaler(cfg->timer, LL_TIM_CHANNEL_CH1, LL_TIM_ICPSC_DIV1);
	LL_TIM_IC_SetPolarity(cfg->timer, LL_TIM_CHANNEL_CH1, LL_TIM_IC_POLARITY_RISING);

	if (data->dma_cc.enabled) {
		freq_meter_stm32_icap_dma_setup(dev);
		LL_TIM_EnableDMAReq_CC1(cfg->timer);
	} else {
		LL_TIM_EnableIT_CC1(cfg->timer);
	}

	LL_TIM_CC_EnableChannel(cfg->timer, LL_TIM_CHANNEL_CH1);
	LL_TIM_SetPrescaler(cfg->timer, cfg->prescaler);
	LL_TIM_EnableCounter(cfg->timer);

	return 0;
}

#define TIMER(idx)    DT_INST_PARENT(idx)
#define TIM(idx)      ((TIM_TypeDef *) DT_REG_ADDR(TIMER(idx)))
#define DMA(idx, dir) ((DMA_TypeDef *) DT_REG_ADDR(STM32_DMA_CTLR(idx, dir)))

#define FREQ_METER_STM32_ICAP_DMA_INIT(idx, dir, src_dev, dest_dev)				   \
	.dma_dev = DEVICE_DT_GET(STM32_DMA_CTLR(idx, dir)),					   \
	.dma = DMA(idx, dir),									   \
	.dma_channel = DT_INST_DMAS_CELL_BY_NAME(idx, dir, channel),				   \
	.dma_cfg = {										   \
		.dma_slot = STM32_DMA_SLOT(idx, dir, slot),					   \
		.channel_direction = STM32_DMA_CONFIG_DIRECTION(				   \
			STM32_DMA_CHANNEL_CONFIG(idx, dir)),					   \
		.channel_priority = STM32_DMA_CONFIG_PRIORITY(					   \
			STM32_DMA_CHANNEL_CONFIG(idx, dir)),					   \
		.source_data_size = STM32_DMA_CONFIG_##src_dev##_DATA_SIZE(			   \
			STM32_DMA_CHANNEL_CONFIG(idx, dir)),					   \
		.dest_data_size = STM32_DMA_CONFIG_##dest_dev##_DATA_SIZE(			   \
			STM32_DMA_CHANNEL_CONFIG(idx, dir)),					   \
		.source_burst_length = 4, /* SINGLE transfer */					   \
		.dest_burst_length = 4,								   \
		.block_count = 2,								   \
		.cyclic = 1,									   \
		.dma_callback = NULL,								   \
		.user_data = NULL,								   \
	},											   \
	.blk_cfg = { 0 },									   \
	.fifo_threshold = STM32_DMA_FEATURES_FIFO_THRESHOLD(					   \
		STM32_DMA_FEATURES(idx, dir)),							   \
	.enabled = true,

#define FREQ_METER_STM32_ICAP_DMA_CHANNEL(idx, dir, src, dest)					   \
	.dma_##dir = {COND_CODE_1(DT_INST_DMAS_HAS_NAME(idx, dir),				   \
				  (FREQ_METER_STM32_ICAP_DMA_INIT(idx, dir, src, dest)),	   \
				  (NULL))}

#define FREQ_METER_STM32_ICAP_DEVICE_INIT(idx)							   \
	PINCTRL_DT_INST_DEFINE(idx);								   \
	static struct freq_meter_stm32_icap_data freq_meter_stm32_icap_##idx##_data = {		   \
	    FREQ_METER_STM32_ICAP_DMA_CHANNEL(idx, cc, PERIPHERAL, MEMORY),			   \
	    .index     = 0,									   \
	    .frequency = 0,									   \
	    .samples   = {0},									   \
	    .reset     = RESET_DT_SPEC_GET(TIMER(idx)),						   \
	};											   \
												   \
	static void freq_meter_stm32_icap_##idx##_irq_config(const struct device *dev)		   \
	{											   \
		IRQ_CONNECT(DT_IRQ_BY_NAME(TIMER(idx), cc, irq),				   \
			    DT_IRQ_BY_NAME(TIMER(idx), cc, priority),				   \
			    freq_meter_stm32_icap_irq_handler, DEVICE_DT_INST_GET(idx), 0);	   \
		irq_enable(DT_IRQ_BY_NAME(TIMER(idx), cc, irq));				   \
	}											   \
												   \
	static const struct freq_meter_stm32_icap_config freq_meter_stm32_icap_##idx##_config = {  \
	    .timer         = TIM(idx),								   \
	    .data          = &freq_meter_stm32_icap_##idx##_data,				   \
	    .prescaler     = DT_PROP(TIMER(idx), st_prescaler),					   \
	    .max_top_value = IS_TIM_32B_COUNTER_INSTANCE(TIM(idx)) ? 0xFFFFFFFF : 0x0000FFFF,	   \
	    .channels      = NUM_CH(TIM(idx)),							   \
	    .pclken										   \
	    = {.bus = DT_CLOCKS_CELL(TIMER(idx), bus), .enr = DT_CLOCKS_CELL(TIMER(idx), bits)},   \
	    .irq_config_func = freq_meter_stm32_icap_##idx##_irq_config,			   \
	    .irqn            = DT_IRQN(TIMER(idx)),						   \
	    .pcfg            = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),				   \
	};											   \
												   \
	SENSOR_DEVICE_DT_INST_DEFINE(								   \
		idx, freq_meter_stm32_icap_init, NULL,						   \
		&freq_meter_stm32_icap_##idx##_data,						   \
		&freq_meter_stm32_icap_##idx##_config,						   \
		POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,					   \
		&freq_meter_stm32_icap_driver_api);

DT_INST_FOREACH_STATUS_OKAY(FREQ_METER_STM32_ICAP_DEVICE_INIT)
