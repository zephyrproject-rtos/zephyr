/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>

#include <Emios_Mcl_Ip.h>
#include <Emios_Pwm_Ip.h>

#ifdef CONFIG_PWM_CAPTURE
#include <Emios_Icu_Ip.h>
#endif

#define LOG_MODULE_NAME nxp_s32_emios_pwm
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_PWM_LOG_LEVEL);

#define DT_DRV_COMPAT	nxp_s32_emios_pwm

#if !defined(EMIOS_PWM_IP_NUM_OF_CHANNELS_USED)
#define EMIOS_PWM_IP_NUM_OF_CHANNELS_USED EMIOS_PWM_IP_NUM_OF_CHANNELS_USED_U8
#endif

/*
 * Need to fill to this array at runtime, cannot do at build time like
 * the HAL over configuration tool due to limitation of the integration
 */
#if EMIOS_PWM_IP_USED
extern uint8 eMios_Pwm_Ip_IndexInChState[EMIOS_PWM_IP_INSTANCE_COUNT][EMIOS_PWM_IP_CHANNEL_COUNT];
#define EMIOS_PWM_MASTER_CHANNEL(channel, bus)					\
	((bus == EMIOS_PWM_IP_BUS_A) ? 23 :					\
	((bus == EMIOS_PWM_IP_BUS_F) ? 22 :					\
	((bus == EMIOS_PWM_IP_BUS_BCDE) ? ((channel >> 3) * 8) : channel)))
#endif

#ifdef CONFIG_PWM_CAPTURE
extern uint8 eMios_Icu_Ip_IndexInChState[EMIOS_ICU_IP_INSTANCE_COUNT][EMIOS_ICU_IP_NUM_OF_CHANNELS];

#define EMIOS_ICU_MASTER_CHANNEL(channel, bus)					\
	((bus == EMIOS_ICU_BUS_A) ? 23 :					\
	((bus == EMIOS_ICU_BUS_F) ? 22 :					\
	((bus == EMIOS_ICU_BUS_DIVERSE) ? ((channel >> 3) * 8) : channel)))

/* We need maximum three edges for measure both period and cycle */
#define MAX_NUM_EDGE 3
#endif

struct pwm_nxp_s32_channel_data {
	bool inverted;
	uint8_t master_channel;

#if EMIOS_PWM_IP_USED
	uint32_t curr_period;
#endif

#ifdef CONFIG_PWM_CAPTURE
	bool continuous;
	bool pulse_capture;
	bool period_capture;
	void *user_data;
	pwm_capture_callback_handler_t callback;
	eMios_Icu_ValueType edge_buff[MAX_NUM_EDGE];
#endif
};

struct pwm_nxp_s32_data {
	uint32_t emios_clk;
#if EMIOS_PWM_IP_USED
	uint8_t start_pwm_ch;
#endif
	struct pwm_nxp_s32_channel_data ch_data[eMIOS_CH_UC_UC_COUNT];
};

#if EMIOS_PWM_IP_USED
struct pwm_nxp_s32_pulse_info {
	uint8_t pwm_pulse_channels;
	Emios_Pwm_Ip_ChannelConfigType *pwm_info;
};
#endif

struct pwm_nxp_s32_config {
	eMIOS_Type *base;
	uint8_t instance;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct pinctrl_dev_config *pincfg;

#if EMIOS_PWM_IP_USED
	struct pwm_nxp_s32_pulse_info *pulse_info;
#endif

#ifdef CONFIG_PWM_CAPTURE
	eMios_Icu_Ip_ConfigType * icu_cfg;
#endif
};

#if EMIOS_PWM_IP_USED
#ifdef EMIOS_PWM_IP_MODE_OPWFMB_USED
static int pwm_nxp_s32_set_cycles_opwfmb(const struct device *dev, uint32_t channel,
					uint32_t period_cycles, uint32_t pulse_cycles,
					pwm_flags_t flags)
{
	const struct pwm_nxp_s32_config *config = dev->config;
	struct pwm_nxp_s32_data *data = dev->data;
	struct pwm_nxp_s32_channel_data *ch_data = &data->ch_data[channel];

	Emios_Pwm_Ip_PolarityType polarity;

	unsigned int key;

	if ((ch_data->inverted != flags) || (!ch_data->curr_period)) {
		/* If PWM flag is changed or this is the first time PWM channel is configured */
		polarity = (flags & PWM_POLARITY_MASK) ? EMIOS_PWM_IP_ACTIVE_LOW :
							 EMIOS_PWM_IP_ACTIVE_HIGH;

		config->base->CH.UC[channel].C &= ~(eMIOS_C_MODE_MASK |
						    eMIOS_C_EDPOL_MASK);
		config->base->CH.UC[channel].A = pulse_cycles;
		config->base->CH.UC[channel].B = period_cycles;

		/*
		 * When entering OPWFMB mode, Output = Cn[EDPOL]. Unless 100% pulse cycle is
		 * expected, Cn[EDPOL] is set to complement value (i.e 0 if active high and
		 * 1 if active low).
		 */
		if (pulse_cycles) {
			config->base->CH.UC[channel].C |= eMIOS_C_EDPOL(polarity);
		} else {
			config->base->CH.UC[channel].C |= eMIOS_C_EDPOL(!polarity);
		}

		key = irq_lock();
		config->base->CH.UC[channel].C |= eMIOS_C_MODE(EMIOS_PWM_IP_MODE_OPWFMB_FLAG);

		if (pulse_cycles) {
			/* Restore expected value for Cn[EDPOL] */
			config->base->CH.UC[channel].C = (config->base->CH.UC[channel].C &
							~(eMIOS_C_EDPOL_MASK)) |
							eMIOS_C_EDPOL(!polarity);
		}
		irq_unlock(key);

		ch_data->curr_period = period_cycles;
		ch_data->inverted = flags;
	} else {
		key = irq_lock();
		config->base->CH.UC[channel].A = pulse_cycles;
		config->base->CH.UC[channel].B = period_cycles;
		irq_unlock(key);
	}

	return 0;
}
#endif

#if defined(EMIOS_PWM_IP_MODE_OPWMCB_USED)
static int pwm_nxp_s32_set_cycles_opwmcb(const struct device *dev, uint32_t channel,
					uint32_t period_cycles, uint32_t pulse_cycles,
					Emios_Pwm_Ip_ChannelConfigType *pwm_info,
					pwm_flags_t flags)
{
	const struct pwm_nxp_s32_config *config = dev->config;
	struct pwm_nxp_s32_data *data = dev->data;
	struct pwm_nxp_s32_channel_data *ch_data = &data->ch_data[channel];
	struct pwm_nxp_s32_channel_data *master_ch_data = &data->ch_data[ch_data->master_channel];

	unsigned int key;
	Emios_Pwm_Ip_PolarityType polarity;

	/* Convert to written value into eMIOS register */
	if (pulse_cycles == 0) {
		pulse_cycles = EMIOS_PWM_IP_MAX_CNT_VAL;
	} else if (pulse_cycles == period_cycles) {
		pulse_cycles = 1;
	} else {
		pulse_cycles = period_cycles - (pulse_cycles >> 1);
	}

	if ((ch_data->inverted != flags) || (!ch_data->curr_period)) {
		/* If PWM flag is changed or this is the first time PWM channel is configured */
		polarity = (flags & PWM_POLARITY_MASK) ? EMIOS_PWM_IP_ACTIVE_LOW :
							 EMIOS_PWM_IP_ACTIVE_HIGH;

		if (master_ch_data->curr_period != period_cycles) {
			/*
			 * Move timebase channel to GPIO mode --> configure period --> MCB mode
			 * Period can be shared between multiple PWM channels, only configure
			 * when needed.
			 */
			config->base->CH.UC[ch_data->master_channel].C &= ~eMIOS_C_MODE_MASK;
			config->base->CH.UC[ch_data->master_channel].A = period_cycles;
		}

		config->base->CH.UC[channel].C &= ~(eMIOS_C_MODE_MASK  |
						    eMIOS_C_EDPOL_MASK |
						    eMIOS_C_BSL_MASK);

		/*
		 * When entering OPWMCB mode, Output = !Cn[EDPOL]. If 100% pulse cycle is expected
		 * Cn[EDPOL] is set to complement value (i.e 0 if active high and 1 if active low).
		 */
		if (pulse_cycles == 1) {
			/* 100% pulse cycle */
			config->base->CH.UC[channel].C |= eMIOS_C_EDPOL(!polarity);
		} else {
			config->base->CH.UC[channel].C |= eMIOS_C_EDPOL(polarity);
		}

		config->base->CH.UC[channel].A = pulse_cycles;
		config->base->CH.UC[channel].B = pwm_info->DeadTime;

		key = irq_lock();
		config->base->CH.UC[channel].C |= (eMIOS_C_MODE(pwm_info->Mode) |
						   eMIOS_C_BSL(pwm_info->Timebase));

		if (pulse_cycles == 1) {
			config->base->CH.UC[channel].C = (config->base->CH.UC[channel].C &
							 ~eMIOS_C_EDPOL_MASK) |
							 eMIOS_C_EDPOL(polarity);
		}
		irq_unlock(key);

		if (master_ch_data->curr_period != period_cycles) {
			config->base->CH.UC[ch_data->master_channel].C |=
							eMIOS_C_MODE(EMIOS_IP_MCB_UP_DOWN_COUNTER);
			master_ch_data->curr_period = period_cycles;
		}

		ch_data->inverted = flags;
		ch_data->curr_period = period_cycles;
	} else if (master_ch_data->curr_period != period_cycles) {
		key = irq_lock();
		config->base->CH.UC[ch_data->master_channel].A = period_cycles;
		config->base->CH.UC[channel].A = pulse_cycles;
		irq_unlock(key);

		master_ch_data->curr_period = period_cycles;
	} else {
		config->base->CH.UC[channel].A = pulse_cycles;
	}

	return 0;
}
#endif

#if defined(EMIOS_PWM_IP_MODE_OPWMB_USED)
static int pwm_nxp_s32_set_cycles_opwmb(const struct device *dev, uint32_t channel,
					uint32_t period_cycles, uint32_t pulse_cycles,
					Emios_Pwm_Ip_ChannelConfigType *pwm_info,
					pwm_flags_t flags)
{
	const struct pwm_nxp_s32_config *config = dev->config;
	struct pwm_nxp_s32_data *data = dev->data;
	struct pwm_nxp_s32_channel_data *ch_data = &data->ch_data[channel];
	struct pwm_nxp_s32_channel_data *master_ch_data = &data->ch_data[ch_data->master_channel];

	Emios_Pwm_Ip_PolarityType polarity;

	unsigned int key;

	if ((ch_data->inverted != flags) || (!ch_data->curr_period)) {
		/* If PWM flag is changed or this is the first time PWM channel is configured */
		polarity = (flags & PWM_POLARITY_MASK) ? EMIOS_PWM_IP_ACTIVE_LOW :
							 EMIOS_PWM_IP_ACTIVE_HIGH;

		if (master_ch_data->curr_period != period_cycles) {
			/*
			 * Move timebase channel to GPIO mode --> configure period --> MCB mode
			 * Period can be shared between multiple PWM channels, only configure
			 * when needed.
			 */
			config->base->CH.UC[ch_data->master_channel].C &= ~eMIOS_C_MODE_MASK;
			config->base->CH.UC[ch_data->master_channel].A = period_cycles;
		}

		config->base->CH.UC[channel].C &= ~(eMIOS_C_MODE_MASK |
						    eMIOS_C_EDPOL_MASK | eMIOS_C_BSL_MASK);

		config->base->CH.UC[channel].A = pwm_info->PhaseShift;
		config->base->CH.UC[channel].B = pulse_cycles;

		/*
		 * When entering OPWMB mode, Output = Cn[EDPOL]. Unless 100% pulse cycle is
		 * expected, Cn[EDPOL] is set to complement value (i.e 0 if active high and
		 * 1 if active low).
		 */
		if (pulse_cycles == period_cycles) {
			config->base->CH.UC[channel].C |= eMIOS_C_EDPOL(polarity);
		} else {
			config->base->CH.UC[channel].C |= eMIOS_C_EDPOL(!polarity);
		}

		key = irq_lock();
		config->base->CH.UC[channel].C |= eMIOS_C_MODE(pwm_info->Mode) |
						  eMIOS_C_BSL(pwm_info->Timebase);

		if (pulse_cycles != period_cycles) {
			config->base->CH.UC[channel].C = (config->base->CH.UC[channel].C &
							~(eMIOS_C_EDPOL_MASK)) |
							eMIOS_C_EDPOL(polarity);
		}

		if (!pwm_info->PhaseShift) {
			/*
			 * If Phase Shift == 0, Force Match A to ensure PWM pulse can be
			 * generated immediately. Otherwise, it will need to wait until
			 * next period boundary.
			 */
			config->base->CH.UC[channel].C |= eMIOS_C_FORCMA(1);
		}
		irq_unlock(key);

		if (master_ch_data->curr_period != period_cycles) {
			config->base->CH.UC[ch_data->master_channel].C |=
							eMIOS_C_MODE(EMIOS_IP_MCB_UP_COUNTER);
			master_ch_data->curr_period = period_cycles;
		}

		ch_data->inverted = flags;
		ch_data->curr_period = period_cycles;
	} else if (master_ch_data->curr_period != period_cycles) {
		key = irq_lock();
		config->base->CH.UC[ch_data->master_channel].A = period_cycles;
		config->base->CH.UC[channel].B = pulse_cycles;
		irq_unlock(key);

		master_ch_data->curr_period = period_cycles;
	} else {
		config->base->CH.UC[channel].B = pulse_cycles;
	}

	return 0;
}
#endif

static int pwm_nxp_s32_set_cycles(const struct device *dev, uint32_t channel,
				  uint32_t period_cycles, uint32_t pulse_cycles,
				  pwm_flags_t flags)
{
	const struct pwm_nxp_s32_config *config = dev->config;
	struct pwm_nxp_s32_data *data = dev->data;

	Emios_Pwm_Ip_ChannelConfigType *pwm_info;
	uint8_t logic_ch;

	if (channel >= EMIOS_PWM_IP_CHANNEL_COUNT) {
		LOG_ERR("invalid channel %d", channel);
		return -EINVAL;
	}

	if (eMios_Pwm_Ip_IndexInChState[config->instance][channel] >=
		EMIOS_PWM_IP_NUM_OF_CHANNELS_USED) {
		LOG_ERR("Channel %d is not configured for PWM", channel);
		return -EINVAL;
	}

	logic_ch = eMios_Pwm_Ip_IndexInChState[config->instance][channel] - data->start_pwm_ch;
	pwm_info = &config->pulse_info->pwm_info[logic_ch];

	switch (pwm_info->Mode) {
#ifdef EMIOS_PWM_IP_MODE_OPWFMB_USED
	case EMIOS_PWM_IP_MODE_OPWFMB_FLAG:

		if ((period_cycles > EMIOS_PWM_IP_MAX_CNT_VAL) ||
			(period_cycles <= EMIOS_PWM_IP_MIN_CNT_VAL)) {
			LOG_ERR("Period cycles is out of range");
			return -EINVAL;
		}

		return pwm_nxp_s32_set_cycles_opwfmb(dev, channel, period_cycles,
						     pulse_cycles, flags);
#endif

#ifdef EMIOS_PWM_IP_MODE_OPWMCB_USED
	case EMIOS_PWM_IP_MODE_OPWMCB_TRAIL_EDGE_FLAG:
	case EMIOS_PWM_IP_MODE_OPWMCB_LEAD_EDGE_FLAG:

		period_cycles = (period_cycles + 2) / 2;

		if ((period_cycles > EMIOS_PWM_IP_MAX_CNT_VAL) ||
			(period_cycles <= EMIOS_PWM_IP_MIN_CNT_VAL)) {
			LOG_ERR("Period cycles is out of range");
			return -EINVAL;
		}

		return pwm_nxp_s32_set_cycles_opwmcb(dev, channel, period_cycles,
						     pulse_cycles, pwm_info, flags);
#endif

#if defined(EMIOS_PWM_IP_MODE_OPWMB_USED)
	case EMIOS_PWM_IP_MODE_OPWMB_FLAG:

		if ((period_cycles > EMIOS_PWM_IP_MAX_CNT_VAL) ||
			(period_cycles <= EMIOS_PWM_IP_MIN_CNT_VAL)) {
			LOG_ERR("Period cycles is out of range");
			return -EINVAL;
		}

		pulse_cycles += pwm_info->PhaseShift;

		if (pulse_cycles > period_cycles) {
			LOG_ERR("Pulse cycles is out of range");
			return -EINVAL;
		}

		return pwm_nxp_s32_set_cycles_opwmb(dev, channel, period_cycles,
						    pulse_cycles, pwm_info, flags);
#endif

	default:
		/* Never reach here */
		return 0;
	}
}
#endif

#ifdef CONFIG_PWM_CAPTURE
static ALWAYS_INLINE eMios_Icu_ValueType pwm_nxp_s32_capture_calc(eMios_Icu_ValueType first_cnt,
								  eMios_Icu_ValueType second_cnt)
{
	if (first_cnt < second_cnt) {
		return second_cnt - first_cnt;
	}

	/* Counter top value is always 0xFFFF */
	return EMIOS_ICU_IP_COUNTER_MASK - first_cnt + second_cnt;
}

static ALWAYS_INLINE eMios_Icu_ValueType pwm_nxp_s32_pulse_calc(bool inverted,
								eMios_Icu_ValueType *edge_buff,
								eMios_Icu_Ip_LevelType input_state)
{
	eMios_Icu_ValueType first_cnt, second_cnt;

	if (input_state ^ inverted) {
		 /* 3 edges captured is raise, fall, raise */
		first_cnt = edge_buff[0];
		second_cnt = edge_buff[1];
	} else {
		 /* 3 edges captured is fall, raise, fall */
		first_cnt = edge_buff[1];
		second_cnt = edge_buff[2];
	}

	return pwm_nxp_s32_capture_calc(first_cnt, second_cnt);
}

static int pwm_nxp_s32_capture_configure(const struct device *dev,
					uint32_t channel,
					pwm_flags_t flags,
					pwm_capture_callback_handler_t cb,
					void *user_data)
{
	const struct pwm_nxp_s32_config *config = dev->config;
	struct pwm_nxp_s32_data *data = dev->data;

	if (channel >= EMIOS_ICU_IP_NUM_OF_CHANNELS) {
		LOG_ERR("Invalid channel %d", channel);
		return -EINVAL;
	}

	if (!flags) {
		LOG_ERR("Invalid PWM capture flag");
		return -EINVAL;
	}

	if (eMios_Icu_Ip_IndexInChState[config->instance][channel] >=
		EMIOS_ICU_IP_NUM_OF_CHANNELS_USED) {
		LOG_ERR("Channel %d is not configured for PWM", channel);
		return -EINVAL;
	}

	/* If interrupt is enabled --> channel is on-going */
	if (config->base->CH.UC[channel].C & eMIOS_C_FEN_MASK) {
		LOG_ERR("Channel %d is busy", channel);
		return -EBUSY;
	}

	data->ch_data[channel].continuous = (flags & PWM_CAPTURE_MODE_MASK);
	data->ch_data[channel].inverted = (flags & PWM_POLARITY_MASK);
	data->ch_data[channel].pulse_capture = (flags & PWM_CAPTURE_TYPE_PULSE);
	data->ch_data[channel].period_capture = (flags & PWM_CAPTURE_TYPE_PERIOD);
	data->ch_data[channel].callback = cb;
	data->ch_data[channel].user_data = user_data;

	return 0;
}

static int pwm_nxp_s32_capture_enable(const struct device *dev, uint32_t channel)
{
	const struct pwm_nxp_s32_config *config = dev->config;
	struct pwm_nxp_s32_data *data = dev->data;

	eMios_Icu_Ip_EdgeType edge;
	uint8_t num_edge;

	if (channel >= EMIOS_ICU_IP_NUM_OF_CHANNELS) {
		LOG_ERR("Invalid channel %d", channel);
		return -EINVAL;
	}

	if (eMios_Icu_Ip_IndexInChState[config->instance][channel] >=
		EMIOS_ICU_IP_NUM_OF_CHANNELS_USED) {
		LOG_ERR("Channel %d is not configured for PWM", channel);
		return -EINVAL;
	}

	if (!data->ch_data[channel].callback) {
		LOG_ERR("Callback is not configured");
		return -EINVAL;
	}

	/* If interrupt is enabled --> channel is on-going */
	if (config->base->CH.UC[channel].C & eMIOS_C_FEN_MASK) {
		LOG_ERR("Channel %d is busy", channel);
		return -EBUSY;
	}

	/* If just measure period, we just need 2 edges */
	if (data->ch_data[channel].period_capture && !data->ch_data[channel].pulse_capture) {
		num_edge = 2U;
		edge = EMIOS_ICU_RISING_EDGE;
	} else {
		num_edge = 3U;
		edge = EMIOS_ICU_BOTH_EDGES;
	}

	Emios_Icu_Ip_SetActivation(config->instance, channel, edge);

	Emios_Icu_Ip_EnableNotification(config->instance, channel);

	Emios_Icu_Ip_StartTimestamp(config->instance, channel,
				    data->ch_data[channel].edge_buff,
				    MAX_NUM_EDGE, num_edge);

	return 0;
}

static int pwm_nxp_s32_capture_disable(const struct device *dev, uint32_t channel)
{
	const struct pwm_nxp_s32_config *config = dev->config;

	if (channel >= EMIOS_ICU_IP_NUM_OF_CHANNELS) {
		LOG_ERR("Invalid channel %d", channel);
		return -EINVAL;
	}

	if (eMios_Icu_Ip_IndexInChState[config->instance][channel] >=
		EMIOS_ICU_IP_NUM_OF_CHANNELS_USED) {
		LOG_ERR("Channel %d is not configured for PWM", channel);
		return -EINVAL;
	}

	Emios_Icu_Ip_StopTimestamp(config->instance, channel);

	return 0;
}
#endif

static int pwm_nxp_s32_get_cycles_per_sec(const struct device *dev,
					  uint32_t channel, uint64_t *cycles)
{
	const struct pwm_nxp_s32_config *config = dev->config;
	struct pwm_nxp_s32_data *data = dev->data;

	uint8_t internal_prescaler, global_prescaler, master_channel;

	master_channel = data->ch_data[channel].master_channel;
	internal_prescaler = (config->base->CH.UC[master_channel].C2 & eMIOS_C2_UCEXTPRE_MASK) >>
			      eMIOS_C2_UCEXTPRE_SHIFT;

	/* Clock source for internal prescaler is from either eMIOS or eMIOS / global prescaler */
	if (config->base->CH.UC[master_channel].C2 & eMIOS_C2_UCPRECLK_MASK) {
		*cycles = data->emios_clk / (internal_prescaler + 1);
	} else {
		global_prescaler = (config->base->MCR & eMIOS_MCR_GPRE_MASK) >>
				    eMIOS_MCR_GPRE_SHIFT;
		*cycles = data->emios_clk / ((internal_prescaler + 1) * (global_prescaler + 1));
	}

	return 0;
}

#if EMIOS_PWM_IP_USED
static int pwm_nxp_s32_pulse_gen_init(const struct device *dev)
{
	const struct pwm_nxp_s32_config *config = dev->config;
	struct pwm_nxp_s32_data *data = dev->data;

	struct pwm_nxp_s32_channel_data *ch_data;
	Emios_Pwm_Ip_ChannelConfigType pwm_info;

	uint8_t ch_id;
	static uint8_t logic_ch;

	data->start_pwm_ch = logic_ch;

	for (ch_id = 0; ch_id < config->pulse_info->pwm_pulse_channels; ch_id++) {
		memcpy(&pwm_info, &config->pulse_info->pwm_info[ch_id],
			sizeof(Emios_Pwm_Ip_ChannelConfigType));

		/*
		 * Let eMIOS channel is in GPIO mode, the actual PWM mode will be
		 * configured at the first time pwm_set* is called.
		 */
		pwm_info.Mode = EMIOS_PWM_IP_MODE_GPO;
		eMios_Pwm_Ip_IndexInChState[config->instance][pwm_info.ChannelId] = logic_ch++;
		Emios_Pwm_Ip_InitChannel(config->instance, &pwm_info);

		ch_data = &data->ch_data[pwm_info.ChannelId];
		ch_data->master_channel = EMIOS_PWM_MASTER_CHANNEL(pwm_info.ChannelId,
								   pwm_info.Timebase);
	}

	return 0;
}
#endif

#ifdef CONFIG_PWM_CAPTURE
static int pwm_nxp_s32_pulse_capture_init(const struct device *dev)
{
	const struct pwm_nxp_s32_config *config = dev->config;
	struct pwm_nxp_s32_data *data = dev->data;

	struct pwm_nxp_s32_channel_data *ch_data;
	const eMios_Icu_Ip_ChannelConfigType *icu_info;

	uint8_t ch_id;
	static uint8_t logic_ch;

	for (ch_id = 0; ch_id < config->icu_cfg->nNumChannels; ch_id++) {
		icu_info = &(*config->icu_cfg->pChannelsConfig)[ch_id];
		ch_data = &data->ch_data[icu_info->hwChannel];

		eMios_Icu_Ip_IndexInChState[config->instance][icu_info->hwChannel] = logic_ch++;
		ch_data->master_channel = EMIOS_ICU_MASTER_CHANNEL(icu_info->hwChannel,
								   icu_info->CntBus);
	}

	if (Emios_Icu_Ip_Init(config->instance, config->icu_cfg)) {
		return -EINVAL;
	}

	return 0;
}

static void pwm_nxp_s32_capture_callback(const struct device *dev, uint32_t channel)
{
	const struct pwm_nxp_s32_config *config = dev->config;
	struct pwm_nxp_s32_data *data = dev->data;

	uint32_t period = 0, pulse = 0;

	if (data->ch_data[channel].period_capture && !data->ch_data[channel].pulse_capture) {
		period = pwm_nxp_s32_capture_calc(data->ch_data[channel].edge_buff[0],
						 data->ch_data[channel].edge_buff[1]);
	} else {
		if (data->ch_data[channel].pulse_capture) {
			pulse = pwm_nxp_s32_pulse_calc(data->ch_data[channel].inverted,
						       data->ch_data[channel].edge_buff,
						       Emios_Icu_Ip_GetInputLevel(config->instance,
										  channel));
		}

		if (data->ch_data[channel].period_capture) {
			period = pwm_nxp_s32_capture_calc(data->ch_data[channel].edge_buff[0],
							  data->ch_data[channel].edge_buff[2]);
		}
	}

	if (!data->ch_data[channel].continuous) {
		Emios_Icu_Ip_StopTimestamp(config->instance, channel);
	}

	data->ch_data[channel].callback(dev, channel, period, pulse, 0,
					data->ch_data[channel].user_data);
}
#endif

static int pwm_nxp_s32_init(const struct device *dev)
{
	const struct pwm_nxp_s32_config *config = dev->config;
	struct pwm_nxp_s32_data *data = dev->data;

	int err = 0;

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
					&data->emios_clk)) {
		return -EINVAL;
	}

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

#if EMIOS_PWM_IP_USED
	err = pwm_nxp_s32_pulse_gen_init(dev);
	if (err < 0) {
		return err;
	}
#endif

#ifdef CONFIG_PWM_CAPTURE
	err = pwm_nxp_s32_pulse_capture_init(dev);
	if (err < 0) {
		return err;
	}
#endif

	return err;
}

static DEVICE_API(pwm, pwm_nxp_s32_driver_api) = {
	.set_cycles = pwm_nxp_s32_set_cycles,
	.get_cycles_per_sec = pwm_nxp_s32_get_cycles_per_sec,
#ifdef CONFIG_PWM_CAPTURE
	.configure_capture = pwm_nxp_s32_capture_configure,
	.enable_capture = pwm_nxp_s32_capture_enable,
	.disable_capture = pwm_nxp_s32_capture_disable,
#endif
};

#define EMIOS_PWM_IS_MODE_OPWFMB(node_id)							\
	DT_ENUM_HAS_VALUE(node_id, pwm_mode, OPWFMB)

#define EMIOS_PWM_IS_MODE_OPWMCB(node_id)							\
	UTIL_OR(DT_ENUM_HAS_VALUE(node_id, pwm_mode, OPWMCB_TRAIL_EDGE),			\
		DT_ENUM_HAS_VALUE(node_id, pwm_mode, OPWMCB_LEAD_EDGE))				\

#define EMIOS_PWM_IS_MODE_OPWMB(node_id)							\
	DT_ENUM_HAS_VALUE(node_id, pwm_mode, OPWMB)

#define EMIOS_PWM_IS_MODE_SAIC(node_id)								\
	DT_ENUM_HAS_VALUE(node_id, pwm_mode, SAIC)

#define EMIOS_PWM_IS_CAPTURE_MODE(node_id)							\
	EMIOS_PWM_IS_MODE_SAIC(node_id)

#define EMIOS_PWM_LOG(node_id, msg)								\
	DT_NODE_PATH(node_id) ": " DT_PROP(node_id, pwm_mode) ": " msg				\

#define EMIOS_PWM_VERIFY_MASTER_BUS(node_id)							\
	BUILD_ASSERT(BIT(DT_PROP(node_id, channel)) &						\
		     DT_PROP_BY_PHANDLE(node_id, master_bus, channel_mask),			\
		     EMIOS_PWM_LOG(node_id, "invalid master bus"));

#define EMIOS_PWM_PULSE_GEN_COMMON_VERIFY(node_id)						\
	BUILD_ASSERT(DT_NODE_HAS_PROP(node_id, input_filter),					\
		     EMIOS_PWM_LOG(node_id, "input-filter is not used"));

#define EMIOS_PWM_VERIFY_MODE_OPWFMB(node_id)							\
	EMIOS_PWM_PULSE_GEN_COMMON_VERIFY(node_id)						\
	BUILD_ASSERT(!DT_NODE_HAS_PROP(node_id, master_bus),					\
		     EMIOS_PWM_LOG(node_id, "master-bus must not be configured"));		\
	BUILD_ASSERT(DT_PROP(node_id, dead_time) == 0,						\
		     EMIOS_PWM_LOG(node_id, "dead-time is not used"));				\
	BUILD_ASSERT(DT_PROP(node_id, phase_shift) == 0,					\
		     EMIOS_PWM_LOG(node_id, "phase-shift is not used"));

#define EMIOS_PWM_VERIFY_MODE_OPWMCB(node_id)							\
	EMIOS_PWM_PULSE_GEN_COMMON_VERIFY(node_id)						\
	BUILD_ASSERT(DT_ENUM_HAS_VALUE(DT_PHANDLE(node_id, master_bus),	mode,			\
		     MCB_UP_DOWN_COUNTER),							\
		     EMIOS_PWM_LOG(node_id, "master-bus must be configured in MCB up-down"));	\
	BUILD_ASSERT(DT_PROP(node_id, phase_shift) == 0,					\
		     EMIOS_PWM_LOG(node_id, "phase-shift is not used"));			\
	BUILD_ASSERT(!DT_NODE_HAS_PROP(node_id, prescaler),					\
		     EMIOS_PWM_LOG(node_id, "prescaler is not used,"				\
		     " driver takes the value from master bus"));				\
	BUILD_ASSERT(DT_ENUM_HAS_VALUE(node_id, prescaler_src, PRESCALED_CLOCK),		\
		     EMIOS_PWM_LOG(node_id, "prescaler-src is not used,"			\
		     " always use prescalered source"));					\

#define EMIOS_PWM_VERIFY_MODE_OPWMB(node_id)							\
	EMIOS_PWM_PULSE_GEN_COMMON_VERIFY(node_id)						\
	BUILD_ASSERT(DT_ENUM_HAS_VALUE(DT_PHANDLE(node_id, master_bus),	mode, MCB_UP_COUNTER),	\
		     EMIOS_PWM_LOG(node_id, "master-bus must be configured in MCB up"));	\
	BUILD_ASSERT(DT_PROP(node_id, dead_time) == 0,						\
		     EMIOS_PWM_LOG(node_id, "dead-time is not used"));				\
	BUILD_ASSERT(!DT_NODE_HAS_PROP(node_id, prescaler),					\
		     EMIOS_PWM_LOG(node_id, "prescaler is not used"));				\
	BUILD_ASSERT(DT_ENUM_HAS_VALUE(node_id, prescaler_src, PRESCALED_CLOCK),		\
		     EMIOS_PWM_LOG(node_id, "prescaler-src is not used,"			\
		     " always use prescalered source"));					\

#define EMIOS_PWM_VERIFY_MODE_SAIC(node_id)							\
	IF_ENABLED(DT_NODE_HAS_PROP(node_id, master_bus),					\
		  (BUILD_ASSERT(								\
		   DT_ENUM_HAS_VALUE(DT_PHANDLE(node_id, master_bus), mode, MCB_UP_COUNTER),	\
		   EMIOS_PWM_LOG(node_id, "master-bus must be configured in MCB up"));))	\
	IF_ENABLED(UTIL_NOT(DT_NODE_HAS_PROP(node_id, master_bus)),				\
		   (BUILD_ASSERT(								\
		    BIT(DT_PROP(node_id, channel)) & DT_PROP(DT_GPARENT(node_id), internal_cnt),\
		    EMIOS_PWM_LOG(node_id, "master-bus must be chosen,"				\
		    " channel do not have has counter"))));					\
	IF_ENABLED(UTIL_NOT(DT_NODE_HAS_PROP(node_id, master_bus)),				\
		   (BUILD_ASSERT(DT_NODE_HAS_PROP(node_id, prescaler),				\
		    EMIOS_PWM_LOG(node_id, "if use internal counter, prescaler must"		\
		    " be configured"))));							\
	BUILD_ASSERT(DT_ENUM_HAS_VALUE(node_id, prescaler_src, PRESCALED_CLOCK),		\
		     EMIOS_PWM_LOG(node_id, "prescaler-src is not used,"			\
		     " always use prescalered source"));

#define _EMIOS_PWM_VERIFY_CONFIG(node_id)							\
	IF_ENABLED(DT_NODE_HAS_PROP(node_id, master_bus),					\
		  (EMIOS_PWM_VERIFY_MASTER_BUS(node_id)))					\
	IF_ENABLED(EMIOS_PWM_IS_MODE_OPWFMB(node_id),						\
		  (EMIOS_PWM_VERIFY_MODE_OPWFMB(node_id)))					\
	IF_ENABLED(EMIOS_PWM_IS_MODE_OPWMCB(node_id),						\
		  (EMIOS_PWM_VERIFY_MODE_OPWMCB(node_id)))					\
	IF_ENABLED(EMIOS_PWM_IS_MODE_OPWMB(node_id),						\
		  (EMIOS_PWM_VERIFY_MODE_OPWMB(node_id)))					\
	IF_ENABLED(EMIOS_PWM_IS_MODE_SAIC(node_id),						\
		   (EMIOS_PWM_VERIFY_MODE_SAIC(node_id)))

#if EMIOS_PWM_IP_USED
/* Macros used to glue devicetree with RTD's definition */
#define EMIOS_PWM_BUS_A			EMIOS_PWM_IP_BUS_A
#define EMIOS_PWM_BUS_B			EMIOS_PWM_IP_BUS_BCDE
#define EMIOS_PWM_BUS_C			EMIOS_PWM_IP_BUS_BCDE
#define EMIOS_PWM_BUS_D			EMIOS_PWM_IP_BUS_BCDE
#define EMIOS_PWM_BUS_E			EMIOS_PWM_IP_BUS_BCDE
#define EMIOS_PWM_BUS_F			EMIOS_PWM_IP_BUS_F

#define EMIOS_PWM_BUS(mode)		DT_CAT(EMIOS_PWM_, mode)
#define EMIOS_PWM_MODE(mode)		DT_CAT3(EMIOS_PWM_IP_MODE_, mode, _FLAG)
#define EMIOS_PWM_PS_SRC(mode)		DT_CAT(EMIOS_PWM_IP_PS_SRC_, mode)

/* Keep minimal configuration used at driver initialization, no PWM signal is produced */
#define _EMIOS_PWM_PULSE_GEN_CONFIG(node_id)							\
	IF_ENABLED(UTIL_NOT(EMIOS_PWM_IS_CAPTURE_MODE(node_id)),				\
	({											\
		.ChannelId = DT_PROP(node_id, channel),						\
		.Mode      = EMIOS_PWM_MODE(DT_STRING_TOKEN(node_id, pwm_mode)),		\
		.InternalPsSrc = EMIOS_PWM_PS_SRC(DT_STRING_TOKEN(node_id, prescaler_src)),	\
		.InternalPs = DT_PROP_OR(node_id, prescaler,					\
					DT_PROP_BY_PHANDLE(node_id, master_bus, prescaler)) - 1,\
		.Timebase = COND_CODE_1(DT_NODE_HAS_PROP(node_id, master_bus),			\
			    (EMIOS_PWM_BUS(DT_STRING_TOKEN(					\
					  DT_PHANDLE(node_id, master_bus), bus_type))),		\
					  (EMIOS_PWM_IP_BUS_INTERNAL)),				\
		.PhaseShift = DT_PROP(node_id, phase_shift),					\
		.DeadTime   = DT_PROP(node_id, dead_time),					\
		.OutputDisableSource = EMIOS_PWM_IP_OUTPUT_DISABLE_NONE,			\
		.OutputPolarity = EMIOS_PWM_IP_ACTIVE_LOW,					\
		.DebugMode   = DT_PROP(node_id, freeze),					\
		.PeriodCount = 0,								\
		.DutyCycle   = 0,								\
	},))

#define EMIOS_PWM_PULSE_GEN_CONFIG(n)								\
	const Emios_Pwm_Ip_ChannelConfigType emios_pwm_##n##_init[] = {				\
		DT_INST_FOREACH_CHILD_STATUS_OKAY(n, _EMIOS_PWM_PULSE_GEN_CONFIG)		\
	};											\
	const struct pwm_nxp_s32_pulse_info emios_pwm_##n##_info = {				\
		.pwm_pulse_channels = ARRAY_SIZE(emios_pwm_##n##_init),				\
		.pwm_info = (Emios_Pwm_Ip_ChannelConfigType *)emios_pwm_##n##_init,		\
	};

#define EMIOS_PWM_PULSE_GEN_GET_CONFIG(n)							\
	.pulse_info = (struct pwm_nxp_s32_pulse_info *)&emios_pwm_##n##_info,
#else
#define EMIOS_PWM_PULSE_GEN_CONFIG(n)
#define EMIOS_PWM_PULSE_GEN_GET_CONFIG(n)
#endif

#ifdef CONFIG_PWM_CAPTURE
/* Macros used to glue devicetree with RTD's definition */
#define EMIOS_ICU_BUS_A				EMIOS_ICU_BUS_A
#define EMIOS_ICU_BUS_B				EMIOS_ICU_BUS_DIVERSE
#define EMIOS_ICU_BUS_C				EMIOS_ICU_BUS_DIVERSE
#define EMIOS_ICU_BUS_D				EMIOS_ICU_BUS_DIVERSE
#define EMIOS_ICU_BUS_E				EMIOS_ICU_BUS_DIVERSE
#define EMIOS_ICU_BUS_F				EMIOS_ICU_BUS_F

#define DIGITAL_FILTER_0			EMIOS_DIGITAL_FILTER_BYPASSED
#define DIGITAL_FILTER_2			EMIOS_DIGITAL_FILTER_02
#define DIGITAL_FILTER_4			EMIOS_DIGITAL_FILTER_04
#define DIGITAL_FILTER_8			EMIOS_DIGITAL_FILTER_08
#define DIGITAL_FILTER_16			EMIOS_DIGITAL_FILTER_16

#define EMIOS_PWM_CAPTURE_FILTER(filter)	DT_CAT(DIGITAL_FILTER_, filter)
#define EMIOS_PWM_CAPTURE_MODE(mode)		DT_CAT(EMIOS_ICU_, mode)
#define EMIOS_PWM_CAPTURE_BUS(mode)		DT_CAT(EMIOS_ICU_, mode)

#define EMIOS_PWM_CAPTURE_CB(n, ch)								\
	DT_CAT5(pwm_nxp_s32_, n, _channel_, ch, _capture_callback)

#define EMIOS_PWM_CALLBACK_DECLARE(node_id, n)							\
	void EMIOS_PWM_CAPTURE_CB(n, DT_PROP(node_id, channel))(void)				\
	{											\
		pwm_nxp_s32_capture_callback(DEVICE_DT_INST_GET(n), DT_PROP(node_id, channel));	\
	}											\

#define _EMIOS_PWM_PULSE_CAPTURE_CONFIG(node_id, n)						\
	IF_ENABLED(EMIOS_PWM_IS_CAPTURE_MODE(node_id),						\
	({											\
		.hwChannel = DT_PROP(node_id, channel),						\
		.ucMode	   = EMIOS_PWM_CAPTURE_MODE(DT_STRING_TOKEN(node_id, pwm_mode)),	\
		.FreezeEn  = DT_PROP(node_id, freeze),						\
		.Prescaler = COND_CODE_1(DT_NODE_HAS_PROP(node_id, master_bus),			\
					(DT_PROP_BY_PHANDLE(node_id, master_bus, prescaler)),	\
					(DT_PROP(node_id, prescaler))) - 1,			\
		.CntBus    = COND_CODE_1(DT_NODE_HAS_PROP(node_id, master_bus),			\
			     (EMIOS_PWM_CAPTURE_BUS(DT_STRING_TOKEN(				\
						    DT_PHANDLE(node_id, master_bus), bus_type))),\
			      (EMIOS_ICU_BUS_INTERNAL_COUNTER)),				\
		.chMode    = EMIOS_ICU_MODE_TIMESTAMP,						\
		.chSubMode = EMIOS_ICU_MODE_WITHOUT_DMA,					\
		.measurementMode = EMIOS_ICU_NO_MEASUREMENT,					\
		.edgeAlignement  = EMIOS_ICU_BOTH_EDGES,					\
		.Filter   = EMIOS_PWM_CAPTURE_FILTER(DT_PROP(node_id, input_filter)),		\
		.callback = NULL_PTR,								\
		.logicChStateCallback = NULL_PTR,						\
		.callbackParams      = 255U,							\
		.bWithoutInterrupt   = FALSE,							\
		.timestampBufferType = EMIOS_ICU_CIRCULAR_BUFFER,				\
		.eMiosChannelNotification = EMIOS_PWM_CAPTURE_CB(n, DT_PROP(node_id, channel)),	\
		.eMiosOverflowNotification = NULL_PTR,						\
	},))

#define EMIOS_PWM_PULSE_CAPTURE_CONFIG(n)							\
	DT_INST_FOREACH_CHILD_STATUS_OKAY_VARGS(n, EMIOS_PWM_CALLBACK_DECLARE, n)		\
	const eMios_Icu_Ip_ChannelConfigType emios_pwm_##n##_capture_init[] = {			\
		DT_INST_FOREACH_CHILD_STATUS_OKAY_VARGS(n, _EMIOS_PWM_PULSE_CAPTURE_CONFIG, n)	\
	};											\
	const eMios_Icu_Ip_ConfigType emios_pwm_##n##_capture_info = {				\
		.nNumChannels = ARRAY_SIZE(emios_pwm_##n##_capture_init),			\
		.pChannelsConfig = &emios_pwm_##n##_capture_init,				\
	};

#define EMIOS_PWM_PULSE_CAPTURE_GET_CONFIG(n)							\
	.icu_cfg = (eMios_Icu_Ip_ConfigType *)&emios_pwm_##n##_capture_info,
#else
#define EMIOS_PWM_PULSE_CAPTURE_CONFIG(n)
#define EMIOS_PWM_PULSE_CAPTURE_GET_CONFIG(n)
#endif

#define EMIOS_PWM_VERIFY_CONFIG(n)								\
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, _EMIOS_PWM_VERIFY_CONFIG)

#define EMIOS_NXP_S32_INSTANCE_CHECK(idx, node_id)						\
	((DT_REG_ADDR(node_id) == IP_EMIOS_##idx##_BASE) ? idx : 0)

#define EMIOS_NXP_S32_GET_INSTANCE(node_id)							\
	LISTIFY(__DEBRACKET eMIOS_INSTANCE_COUNT, EMIOS_NXP_S32_INSTANCE_CHECK, (|), node_id)

#define PWM_NXP_S32_INIT_DEVICE(n)								\
	PINCTRL_DT_INST_DEFINE(n);								\
	EMIOS_PWM_VERIFY_CONFIG(n)								\
	EMIOS_PWM_PULSE_GEN_CONFIG(n)								\
	EMIOS_PWM_PULSE_CAPTURE_CONFIG(n)							\
	static const struct pwm_nxp_s32_config pwm_nxp_s32_config_##n = {			\
		.base = (eMIOS_Type *)DT_REG_ADDR(DT_INST_PARENT(n)),				\
		.instance = EMIOS_NXP_S32_GET_INSTANCE(DT_INST_PARENT(n)),			\
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_INST_PARENT(n))),			\
		.clock_subsys = (clock_control_subsys_t)DT_CLOCKS_CELL(DT_INST_PARENT(n), name),\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),					\
		EMIOS_PWM_PULSE_GEN_GET_CONFIG(n)						\
		EMIOS_PWM_PULSE_CAPTURE_GET_CONFIG(n)						\
	};											\
	static struct pwm_nxp_s32_data pwm_nxp_s32_data_##n;					\
	DEVICE_DT_INST_DEFINE(n,								\
			&pwm_nxp_s32_init,							\
			NULL,									\
			&pwm_nxp_s32_data_##n,							\
			&pwm_nxp_s32_config_##n,						\
			POST_KERNEL,								\
			CONFIG_PWM_INIT_PRIORITY,						\
			&pwm_nxp_s32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_NXP_S32_INIT_DEVICE)
