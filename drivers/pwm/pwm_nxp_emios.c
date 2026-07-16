/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT	nxp_emios_pwm

#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_PWM_CAPTURE
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>
#endif

LOG_MODULE_REGISTER(nxp_emios_pwm, CONFIG_PWM_LOG_LEVEL);

/* Counter bus mode enumeration definitions */
#define EMIOS_CNT_BUS_MODE_UP		(0U)
#define EMIOS_CNT_BUS_MODE_UP_DOWN	(1U)

/* PWM mode enumeration definitions */
#define EMIOS_PWM_MODE_OPWFMB			(0U)
#define EMIOS_PWM_MODE_OPWMB			(1U)
#define EMIOS_PWM_MODE_OPWMCB_TRAIL_EDGE	(2U)
#define EMIOS_PWM_MODE_OPWMCB_LEAD_EDGE		(3U)
#define EMIOS_PWM_MODE_SAIC			(4U)

/* Counter bus type enumeration definitions */
#define EMIOS_BUS_TYPE_A	(0U)
#define EMIOS_BUS_TYPE_B	(1U)
#define EMIOS_BUS_TYPE_C	(2U)
#define EMIOS_BUS_TYPE_D	(3U)
#define EMIOS_BUS_TYPE_F	(4U)
#define EMIOS_BUS_TYPE_INTERNAL	(5U)

/*
 * Global counter bus A driven by UC23
 * Local counter buses B, C, D driven by UC16, UC8, UC0
 * Second global counter bus F driven by UC22
 */
#define EMIOS_CNT_BUS_A		(23U)
#define EMIOS_CNT_BUS_BCD	((16U) | (8U))
#define EMIOS_CNT_BUS_F		(22U)

#define EMIOS_CNT_BUS_A_BSL	(0U)
#define EMIOS_CNT_BUS_BCD_BSL	(1U)
#define EMIOS_CNT_BUS_F_BSL	(2U)
#define EMIOS_CNT_BUS_INT_BSL	(3U)

#define EMIOS_MODE_MCB_UP		(0x50U)
#define EMIOS_MODE_MCB_UPDOWN		(0x54U)
#define EMIOS_MODE_OPWFMB		(0x58U)
#define EMIOS_MODE_OPWMCB_TRAIL_EDGE	(0x5CU)
#define EMIOS_MODE_OPWMCB_LEAD_EDGE	(0x5DU)
#define EMIOS_MODE_OPWMB		(0x60U)
#define EMIOS_MODE_SAIC			(0x42U)

#define RISING_FALLING_EDGE_MASK	(0x80000000U)

/* Counter bus unified channel configuration */
struct pwm_nxp_emios_cnt_bus_uc_cfg {
	uint8_t channel;
	uint8_t prescaler;
	uint8_t mode;
};

/* PWM or input capture unified channel configuration */
struct pwm_nxp_emios_pwm_uc_cfg {
	uint8_t channel;
	uint8_t prescaler;
	uint16_t dead_time;
	uint16_t phase_shift;
	uint8_t input_filter;
	uint8_t mode;
	uint8_t bus_type;
};

struct pwm_nxp_emios_config {
	EMIOS_Type *base;
	uint32_t global_prescaler;

	struct pwm_nxp_emios_cnt_bus_uc_cfg *cnt_bus_uc_cfg;
	struct pwm_nxp_emios_pwm_uc_cfg *pwm_uc_cfg;

	uint8_t num_cnt_bus_uc;
	uint8_t num_pwm_uc;

	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct pinctrl_dev_config *pincfg;

#ifdef CONFIG_PWM_CAPTURE
	void (*irq_config_func)(const struct device *dev);
#endif
};

struct pwm_nxp_emios_capture_data {
	pwm_capture_callback_handler_t callback;
	void *user_data;
	/*
	 * capture_value[0]: first active edge timestamp
	 * capture_value[1]: opposite edge timestamp (pulse end / both-mode intermediate)
	 */
	uint32_t capture_value[2];
	bool capture_period;
	bool capture_pulse;
	bool continuous;
	bool enabled; /* true when capture is active (between enable and disable) */
	uint8_t state; /* 0=idle, 1=after first active edge, 2=after opposite edge */
	uint8_t channel;
};

struct pwm_nxp_emios_channel_data {
	uint32_t ch_period;
	uint32_t polarity;

#ifdef CONFIG_PWM_CAPTURE
	struct pwm_nxp_emios_capture_data capture;
#endif
};

struct pwm_nxp_emios_data {
	uint32_t emios_clk;

	/* Mapping from hardware channel number to pwm_uc_cfg array index */
	uint8_t ch_to_cfg_idx[EMIOS_CH_UC_UC_COUNT];

	/* Mapping from counter bus channel number to cnt_bus_uc_cfg array index */
	uint8_t cnt_bus_ch_to_cfg_idx[EMIOS_CH_UC_UC_COUNT];

	struct pwm_nxp_emios_channel_data ch_data[EMIOS_CH_UC_UC_COUNT];
};

struct pwm_nxp_emios_cnt_bus_info {
	uint32_t bus_id;
	uint32_t bus_field;
};

#ifdef CONFIG_PWM_CAPTURE
/*
 * Compute elapsed ticks between two SAIC capture timestamps.
 *
 * SAIC uses the channel's internal 16-bit counter ([0, 0xFFFF]).
 * Both end and start are held in uint32_t but their values are in
 * [0, 0xFFFF]. Masking the 32-bit unsigned subtraction to 16 bits
 * handles wrap-around naturally without a conditional branch:
 *   e.g. end=5, start=0xFFFE -> (5 - 0xFFFE) & 0xFFFF = 7
 *
 * Maximum measurable range: [0, 0xFFFF] ticks
 *
 * This implementation correctly handles a single counter wrap-around
 * (when end < start), but CANNOT reliably measure signals with periods
 * exceeding the counter modulus (65535 ticks).
 *
 * Fundamental limitation - Why software overflow counting is not feasible:
 *
 * The eMIOS hardware does NOT generate interrupts on counter overflow.
 * The S[OVFL] status bit is set when the internal counter wraps from
 * 0xFFFF to 0x0000, but this flag does NOT trigger an interrupt.
 *
 * When multiple channels share a counter bus (external timebase), the
 * limitation is even more severe: the overflow event would be at the
 * timebase level with no channel context, making it impossible to attribute
 * the overflow to a specific capture session.
 *
 * Workarounds for signals with period > 0xFFFF is to increase the prescaler
 * to reduce counter frequency (sacrifices resolution)
 */
static inline uint32_t pwm_nxp_emios_elapsed(uint32_t end, uint32_t start)
{
	return (end - start) & 0xFFFFU;
}
#endif

static void pwm_nxp_emios_get_cnt_bus_info(uint32_t channel, uint32_t bus_type,
					   struct pwm_nxp_emios_cnt_bus_info *info)
{
	switch (bus_type) {
	case EMIOS_BUS_TYPE_A:
		info->bus_id = EMIOS_CNT_BUS_A;
		info->bus_field = EMIOS_CNT_BUS_A_BSL;
		break;
	case EMIOS_BUS_TYPE_B:
	case EMIOS_BUS_TYPE_C:
	case EMIOS_BUS_TYPE_D:
		info->bus_id = channel & EMIOS_CNT_BUS_BCD;
		info->bus_field = EMIOS_CNT_BUS_BCD_BSL;
		break;
	case EMIOS_BUS_TYPE_F:
		info->bus_id = EMIOS_CNT_BUS_F;
		info->bus_field = EMIOS_CNT_BUS_F_BSL;
		break;
	default:
		LOG_ERR("Unsupported bus type for channel %u", channel);
		info->bus_id = 0U;
		info->bus_field = 0U;
		return;
	}
}

static int pwm_nxp_emios_set_cycles_opwfmb(const struct device *dev, uint32_t channel,
					   uint32_t period_cycles, uint32_t pulse_cycles,
					   pwm_flags_t flags)
{
	const struct pwm_nxp_emios_config *config = dev->config;
	struct pwm_nxp_emios_data *data = dev->data;
	struct pwm_nxp_emios_channel_data *ch_data = &data->ch_data[channel];
	uint8_t cfg_idx = data->ch_to_cfg_idx[channel];
	uint32_t reg;
	uint32_t polarity = flags & PWM_POLARITY_MASK;

	if (config->pwm_uc_cfg[cfg_idx].bus_type != EMIOS_BUS_TYPE_INTERNAL) {
		LOG_ERR("OPWFMB mode requires internal counter bus");
		return -ENOTSUP;
	}

	if (ch_data->ch_period == 0U || ch_data->polarity != polarity) {
		/*
		 * First time configuration or polarity change, set period, pulse and polarity
		 * To change the UC to OPWFMB mode, first change to GPIO input mode
		 */
		reg = config->base->UC[channel].C;
		reg &= ~(EMIOS_C_MODE_MASK | EMIOS_C_EDPOL_MASK);
		reg |= EMIOS_C_EDSEL_MASK;
		config->base->UC[channel].C = reg;

		config->base->UC[channel].A = pulse_cycles;
		config->base->UC[channel].B = period_cycles;
		config->base->UC[channel].C |= EMIOS_C_EDPOL((polarity == PWM_POLARITY_NORMAL) ?
							      0U : 1U);
		config->base->UC[channel].CNT = 1U;

		reg = config->base->UC[channel].C;
		reg &= ~(EMIOS_C_FORCMA_MASK | EMIOS_C_FORCMB_MASK | EMIOS_C_EDSEL_MASK |
			 EMIOS_C_FREN_MASK | EMIOS_C_ODIS_MASK | EMIOS_C_ODISSL_MASK |
			 EMIOS_C_FEN_MASK | EMIOS_C_DMA_MASK);

		/* UC works in OPWFMB mode requires internal counter bus */
		reg = (reg & ~EMIOS_C_BSL_MASK) | EMIOS_C_BSL(EMIOS_CNT_BUS_INT_BSL);

		reg = (reg & ~EMIOS_C_MODE_MASK) | EMIOS_C_MODE(EMIOS_MODE_OPWFMB);

		config->base->UC[channel].C = reg;

		config->base->UC[channel].C |= EMIOS_C_UCPREN_MASK;
	} else {
		/* Period changed, update both period and pulse */
		config->base->UC[channel].A = pulse_cycles;
		config->base->UC[channel].B = period_cycles;
	}

	ch_data->polarity = polarity;
	ch_data->ch_period = period_cycles;
	return 0;
}

static int pwm_nxp_emios_set_cycles_opwmb(const struct device *dev, uint32_t channel,
					  uint32_t period_cycles, uint32_t pulse_cycles,
					  pwm_flags_t flags)
{
	const struct pwm_nxp_emios_config *config = dev->config;
	struct pwm_nxp_emios_data *data = dev->data;
	struct pwm_nxp_emios_channel_data *ch_data = &data->ch_data[channel];
	struct pwm_nxp_emios_channel_data *cnt_ch_data = NULL;
	struct pwm_nxp_emios_cnt_bus_info bus_info;
	uint8_t cfg_idx = data->ch_to_cfg_idx[channel];
	uint8_t cnt_bus_cfg_idx;
	uint32_t trailing_edge;
	uint32_t reg;
	uint32_t polarity = flags & PWM_POLARITY_MASK;

	/* Get counter bus information */
	pwm_nxp_emios_get_cnt_bus_info(channel, config->pwm_uc_cfg[cfg_idx].bus_type, &bus_info);

	/* Get counter bus configuration index and check mode */
	cnt_bus_cfg_idx = data->cnt_bus_ch_to_cfg_idx[bus_info.bus_id];
	if (cnt_bus_cfg_idx == 0xFFU) {
		LOG_ERR("Counter bus channel %u is not configured", bus_info.bus_id);
		return -EINVAL;
	}

	if (config->cnt_bus_uc_cfg[cnt_bus_cfg_idx].mode != EMIOS_CNT_BUS_MODE_UP) {
		LOG_ERR("OPWMB mode requires counter bus in MCB UP Count mode");
		return -ENOTSUP;
	}

	cnt_ch_data = &data->ch_data[bus_info.bus_id];

	if (ch_data->ch_period == 0U || ch_data->polarity != polarity) {
		/* First time configuration or polarity change */
		/* Update counter bus period (A register) */
		config->base->UC[bus_info.bus_id].A = period_cycles;

		/* To change UC to OPWMB mode, first change to GPIO input mode */
		reg = config->base->UC[channel].C;
		reg &= ~(EMIOS_C_MODE_MASK | EMIOS_C_EDPOL_MASK);
		reg |= EMIOS_C_EDSEL_MASK;
		config->base->UC[channel].C = reg;

		/* Calculate trailing edge position: phaseShift + pulse_cycles */
		trailing_edge = config->pwm_uc_cfg[cfg_idx].phase_shift + pulse_cycles;

		/* Set A (leading edge) and B (trailing edge) registers */
		config->base->UC[channel].A = config->pwm_uc_cfg[cfg_idx].phase_shift;
		config->base->UC[channel].B = trailing_edge;

		/* Set output polarity */
		config->base->UC[channel].C |= EMIOS_C_EDPOL((polarity == PWM_POLARITY_NORMAL) ?
							      1U : 0U);

		reg = config->base->UC[channel].C;
		reg &= ~(EMIOS_C_FORCMA_MASK | EMIOS_C_FORCMB_MASK | EMIOS_C_EDSEL_MASK |
			 EMIOS_C_FREN_MASK | EMIOS_C_ODIS_MASK | EMIOS_C_ODISSL_MASK |
			 EMIOS_C_FEN_MASK | EMIOS_C_DMA_MASK);

		/* Select counter bus */
		reg = (reg & ~EMIOS_C_BSL_MASK) | EMIOS_C_BSL(bus_info.bus_field);

		/* Set OPWMB mode (0x60) */
		reg = (reg & ~EMIOS_C_MODE_MASK) | EMIOS_C_MODE(EMIOS_MODE_OPWMB);

		config->base->UC[channel].C = reg;

		/* Enable UC prescaler and global prescaler */
		config->base->UC[channel].C |= EMIOS_C_UCPREN_MASK;
		config->base->UC[bus_info.bus_id].C |= EMIOS_C_UCPREN_MASK;

		ch_data->ch_period = period_cycles;
		cnt_ch_data->ch_period = period_cycles;
	} else if (cnt_ch_data->ch_period != period_cycles) {
		/* Update counter bus period if changed */
		config->base->UC[bus_info.bus_id].A = period_cycles;

		/* Update PWM channel trailing edge */
		trailing_edge = config->pwm_uc_cfg[cfg_idx].phase_shift + pulse_cycles;
		config->base->UC[channel].B = trailing_edge;

		ch_data->ch_period = period_cycles;
		cnt_ch_data->ch_period = period_cycles;
	} else {
		/* Update duty cycle: update B register (trailing edge) */
		trailing_edge = config->pwm_uc_cfg[cfg_idx].phase_shift + pulse_cycles;
		config->base->UC[channel].B = trailing_edge;
	}

	ch_data->polarity = polarity;
	return 0;
}

static int pwm_nxp_emios_set_cycles_opwmcb(const struct device *dev, uint32_t channel,
					   uint32_t period_cycles, uint32_t pulse_cycles,
					   pwm_flags_t flags)
{
	const struct pwm_nxp_emios_config *config = dev->config;
	struct pwm_nxp_emios_data *data = dev->data;
	struct pwm_nxp_emios_channel_data *ch_data = &data->ch_data[channel];
	struct pwm_nxp_emios_channel_data *cnt_ch_data = NULL;
	struct pwm_nxp_emios_cnt_bus_info bus_info;
	uint8_t cfg_idx = data->ch_to_cfg_idx[channel];
	uint8_t cnt_bus_cfg_idx;
	uint32_t period_cycles_field;
	uint32_t pulse_cycles_field;
	uint32_t reg;
	uint32_t polarity = flags & PWM_POLARITY_MASK;

	/* Get counter bus information */
	pwm_nxp_emios_get_cnt_bus_info(channel, config->pwm_uc_cfg[cfg_idx].bus_type, &bus_info);

	/* Get counter bus configuration index and check mode */
	cnt_bus_cfg_idx = data->cnt_bus_ch_to_cfg_idx[bus_info.bus_id];
	if (cnt_bus_cfg_idx == 0xFFU) {
		LOG_ERR("Counter bus channel %u is not configured", bus_info.bus_id);
		return -EINVAL;
	}

	if (config->cnt_bus_uc_cfg[cnt_bus_cfg_idx].mode != EMIOS_CNT_BUS_MODE_UP_DOWN) {
		LOG_ERR("OPWMCB mode requires counter bus in MCB UPDOWN Count mode");
		return -ENOTSUP;
	}

	cnt_ch_data = &data->ch_data[bus_info.bus_id];

	period_cycles_field = period_cycles / 2U + 1U;

	if (period_cycles == pulse_cycles) {
		pulse_cycles_field = 1U;
	} else if (pulse_cycles == 0U || pulse_cycles == 1U) {
		pulse_cycles_field = period_cycles_field + 1U;
	} else {
		pulse_cycles_field = pulse_cycles >> 1U;
		if (period_cycles_field <= pulse_cycles_field) {
			LOG_ERR("Invalid pulse_cycles %u for period_cycles %u",
					pulse_cycles, period_cycles);
			return -EINVAL;
		}
		pulse_cycles_field = period_cycles_field - pulse_cycles_field;
	}

	if (ch_data->ch_period == 0U || ch_data->polarity != polarity) {
		/* First time configuration or polarity change */
		/* Update counter bus period (A register) */
		config->base->UC[bus_info.bus_id].A = period_cycles_field;

		/* To change UC to OPWMCB mode, first change to GPIO input mode */
		reg = config->base->UC[channel].C;
		reg &= ~(EMIOS_C_MODE_MASK | EMIOS_C_EDPOL_MASK);
		reg |= EMIOS_C_EDSEL_MASK;
		config->base->UC[channel].C = reg;

		/* Set A (duty cycle) and B (dead time) registers */
		config->base->UC[channel].A = pulse_cycles_field;
		config->base->UC[channel].B = config->pwm_uc_cfg[cfg_idx].dead_time;

		/* Set output polarity */
		config->base->UC[channel].C |= EMIOS_C_EDPOL((polarity == PWM_POLARITY_NORMAL) ?
							      1U : 0U);

		reg = config->base->UC[channel].C;
		reg &= ~(EMIOS_C_FORCMA_MASK | EMIOS_C_FORCMB_MASK | EMIOS_C_EDSEL_MASK |
			 EMIOS_C_FREN_MASK | EMIOS_C_ODIS_MASK | EMIOS_C_ODISSL_MASK |
			 EMIOS_C_FEN_MASK | EMIOS_C_DMA_MASK);

		/* Select counter bus */
		reg = (reg & ~EMIOS_C_BSL_MASK) | EMIOS_C_BSL(bus_info.bus_field);

		/* Set OPWMCB mode (0x5C or 0x5D) */
		if (config->pwm_uc_cfg[cfg_idx].mode == EMIOS_PWM_MODE_OPWMCB_LEAD_EDGE) {
			reg = (reg & ~EMIOS_C_MODE_MASK) |
				EMIOS_C_MODE(EMIOS_MODE_OPWMCB_LEAD_EDGE);
		} else {
			reg = (reg & ~EMIOS_C_MODE_MASK) |
				EMIOS_C_MODE(EMIOS_MODE_OPWMCB_TRAIL_EDGE);
		}

		config->base->UC[channel].C = reg;

		/* Enable UC prescaler and global prescaler */
		config->base->UC[channel].C |= EMIOS_C_UCPREN_MASK;
		config->base->UC[bus_info.bus_id].C |= EMIOS_C_UCPREN_MASK;

		ch_data->ch_period = period_cycles;
		cnt_ch_data->ch_period = period_cycles;
	} else if (cnt_ch_data->ch_period != period_cycles) {
		/* Update counter bus period if changed */
		config->base->UC[bus_info.bus_id].A = period_cycles_field;

		/* Update duty cycle */
		config->base->UC[channel].A = pulse_cycles_field;

		ch_data->ch_period = period_cycles;
		cnt_ch_data->ch_period = period_cycles;
	} else {
		/* Update duty cycle: update A register */
		config->base->UC[channel].A = pulse_cycles_field;
	}

	ch_data->polarity = polarity;
	return 0;
}

static int pwm_nxp_emios_set_cycles(const struct device *dev, uint32_t channel,
				    uint32_t period_cycles, uint32_t pulse_cycles,
				    pwm_flags_t flags)
{
	const struct pwm_nxp_emios_config *config = dev->config;
	struct pwm_nxp_emios_data *data = dev->data;
	uint8_t cfg_idx;
	uint8_t mode;
	int err = 0;

	if (period_cycles == 0U) {
		LOG_ERR("Period cycles cannot be zero for channel %u", channel);
		return -EINVAL;
	}

	if (period_cycles > 0xFFFFU || pulse_cycles > 0xFFFFU) {
		LOG_ERR("Period or pulse cycles exceed 16-bit limit for channel %u", channel);
		return -EINVAL;
	}

	if (channel >= EMIOS_CH_UC_UC_COUNT) {
		LOG_ERR("Invalid channel %u", channel);
		return -EINVAL;
	}

	/* Get configuration array index from hardware channel number */
	cfg_idx = data->ch_to_cfg_idx[channel];
	if (cfg_idx == 0xFFU) {
		LOG_ERR("Channel %u is not configured as PWM", channel);
		return -EINVAL;
	}

	mode = config->pwm_uc_cfg[cfg_idx].mode;

	switch (mode) {
	case EMIOS_PWM_MODE_OPWFMB:
		err = pwm_nxp_emios_set_cycles_opwfmb(dev, channel, period_cycles, pulse_cycles,
						      flags);
		break;
	case EMIOS_PWM_MODE_OPWMB:
		err = pwm_nxp_emios_set_cycles_opwmb(dev, channel, period_cycles, pulse_cycles,
						     flags);
		break;
	case EMIOS_PWM_MODE_OPWMCB_TRAIL_EDGE:
	case EMIOS_PWM_MODE_OPWMCB_LEAD_EDGE:
		err = pwm_nxp_emios_set_cycles_opwmcb(dev, channel, period_cycles, pulse_cycles,
						      flags);
		break;
	default:
		LOG_ERR("Unsupported PWM mode %u for channel %u", mode, channel);
		err = -ENOTSUP;
		break;
	}

	return err;
}

static int pwm_nxp_emios_get_cycles_per_sec(const struct device *dev,
					    uint32_t channel, uint64_t *cycles)
{
	const struct pwm_nxp_emios_config *config = dev->config;
	struct pwm_nxp_emios_data *data = dev->data;
	struct pwm_nxp_emios_cnt_bus_info bus_info;
	uint8_t cfg_idx;
	uint8_t cnt_bus_cfg_idx;
	uint8_t mode;
	uint64_t pwm_clk;

	if (channel >= EMIOS_CH_UC_UC_COUNT) {
		LOG_ERR("Invalid channel %u", channel);
		return -EINVAL;
	}

	/* Get configuration array index from hardware channel number */
	cfg_idx = data->ch_to_cfg_idx[channel];
	if (cfg_idx == 0xFFU) {
		LOG_ERR("Channel %u is not configured as PWM", channel);
		return -EINVAL;
	}

	mode = config->pwm_uc_cfg[cfg_idx].mode;

	/* Calculate PWM clock based on mode */
	if (mode == EMIOS_PWM_MODE_OPWFMB || mode == EMIOS_PWM_MODE_SAIC) {
		/*
		 * OPWFMB/SAIC uses internal counter
		 * clock = emios_clk / global_prescaler / pwm_prescaler
		 */
		pwm_clk = data->emios_clk / config->global_prescaler;
		pwm_clk /= config->pwm_uc_cfg[cfg_idx].prescaler;
	} else {
		/* OPWMB/OPWMCB use external counter bus */
		/* Get counter bus information */
		pwm_nxp_emios_get_cnt_bus_info(channel, config->pwm_uc_cfg[cfg_idx].bus_type,
					       &bus_info);

		/* Get counter bus configuration index */
		cnt_bus_cfg_idx = data->cnt_bus_ch_to_cfg_idx[bus_info.bus_id];
		if (cnt_bus_cfg_idx == 0xFFU) {
			LOG_ERR("Counter bus channel %u is not configured", bus_info.bus_id);
			return -EINVAL;
		}

		/* Clock = emios_clk / global_prescaler / cnt_bus_prescaler */
		pwm_clk = data->emios_clk / config->global_prescaler;
		pwm_clk /= config->cnt_bus_uc_cfg[cnt_bus_cfg_idx].prescaler;
	}

	*cycles = pwm_clk;

	return 0;
}

#ifdef CONFIG_PWM_CAPTURE
static int pwm_nxp_emios_configure_capture(const struct device *dev,
					   uint32_t channel, pwm_flags_t flags,
					   pwm_capture_callback_handler_t cb,
					   void *user_data)
{
	const struct pwm_nxp_emios_config *config = dev->config;
	struct pwm_nxp_emios_data *data = dev->data;
	struct pwm_nxp_emios_capture_data *capture;
	uint8_t cfg_idx;
	uint8_t mode;
	uint32_t reg;
	bool capture_period = (flags & PWM_CAPTURE_TYPE_PERIOD) != 0U;
	bool capture_pulse = (flags & PWM_CAPTURE_TYPE_PULSE) != 0U;
	bool continuous = (flags & PWM_CAPTURE_MODE_CONTINUOUS) != 0U;
	uint32_t polarity = flags & PWM_POLARITY_MASK;

	if (channel >= EMIOS_CH_UC_UC_COUNT) {
		LOG_ERR("Invalid channel %u", channel);
		return -EINVAL;
	}

	capture = &data->ch_data[channel].capture;

	/* Get configuration array index from hardware channel number */
	cfg_idx = data->ch_to_cfg_idx[channel];
	if (cfg_idx == 0xFFU) {
		LOG_ERR("Channel %u is not configured", channel);
		return -EINVAL;
	}

	/* SAIC mode uses internal counter with range [0, 0xFFFF]. */
	if (config->pwm_uc_cfg[cfg_idx].bus_type != EMIOS_BUS_TYPE_INTERNAL) {
		LOG_ERR("SAIC mode requires internal counter bus");
		return -ENOTSUP;
	}

	mode = config->pwm_uc_cfg[cfg_idx].mode;

	/* Only SAIC mode supports input capture */
	if (config->pwm_uc_cfg[cfg_idx].mode != EMIOS_PWM_MODE_SAIC) {
		LOG_ERR("Channel %u is not configured for input capture", channel);
		return -ENOTSUP;
	}

	if (capture->enabled) {
		LOG_ERR("Channel %u capture is busy", channel);
		return -EBUSY;
	}

	if (!(flags & PWM_CAPTURE_TYPE_MASK)) {
		LOG_ERR("Must specify at least one capture type");
		return -EINVAL;
	}

	/*
	 * To change the UC to SAIC mode, first change to GPIO input mode
	 * and disable interrupt
	 */
	reg = config->base->UC[channel].C;
	reg &= ~(EMIOS_C_MODE_MASK | EMIOS_C_EDPOL_MASK | EMIOS_C_FEN_MASK);
	reg |= EMIOS_C_EDSEL_MASK;
	config->base->UC[channel].C = reg;

	/* Clear any pending flags */
	config->base->UC[channel].S = (EMIOS_S_FLAG_MASK | EMIOS_S_OVR_MASK);

	reg = config->base->UC[channel].C;

	reg &= ~(EMIOS_C_FORCMA_MASK | EMIOS_C_FORCMB_MASK);

	/*
	 * Always use both-edge triggering. The state machine uses A[31]
	 * (RISING_FALLING_EDGE_MASK) to identify edge direction at each
	 * interrupt, so EDSEL must be 1 regardless of capture type.
	 */
	reg |= EMIOS_C_EDSEL_MASK;

	/* EDPOL selects the initial output level; edge type is read from A[31] */
	reg = (reg & ~EMIOS_C_EDPOL_MASK) |
	      EMIOS_C_EDPOL((polarity == PWM_POLARITY_NORMAL) ? 1U : 0U);

	/* Select counter bus */
	reg = (reg & ~EMIOS_C_BSL_MASK) | EMIOS_C_BSL(EMIOS_CNT_BUS_INT_BSL);

	/* Set SAIC mode */
	reg = (reg & ~EMIOS_C_MODE_MASK) | EMIOS_C_MODE(EMIOS_MODE_SAIC);

	/* Use prescaled clock as input filter */
	reg &= ~EMIOS_C_FCK_MASK;

	/* Disable freeze mode */
	reg &= ~EMIOS_C_FREN_MASK;

	/* Disable DMA request */
	reg &= ~EMIOS_C_DMA_MASK;

	config->base->UC[channel].C = reg;

	/* Save capture configuration */
	capture->callback = cb;
	capture->user_data = user_data;
	capture->capture_value[0] = 0U;
	capture->capture_value[1] = 0U;
	capture->capture_period = capture_period;
	capture->capture_pulse = capture_pulse;
	capture->continuous = continuous;
	capture->enabled = false;
	capture->state = 0U;
	capture->channel = channel;

	LOG_DBG("Configured channel %u for capture: period=%d pulse=%d continuous=%d",
		channel, capture_period, capture_pulse, continuous);

	return 0;
}

static int pwm_nxp_emios_enable_capture(const struct device *dev, uint32_t channel)
{
	const struct pwm_nxp_emios_config *config = dev->config;
	struct pwm_nxp_emios_data *data = dev->data;
	uint8_t cfg_idx;

	if (channel >= EMIOS_CH_UC_UC_COUNT) {
		LOG_ERR("Invalid channel %u", channel);
		return -EINVAL;
	}

	/* Get configuration array index from hardware channel number */
	cfg_idx = data->ch_to_cfg_idx[channel];
	if (cfg_idx == 0xFFU) {
		LOG_ERR("Channel %u is not configured", channel);
		return -EINVAL;
	}

	/* Only SAIC mode supports input capture */
	if (config->pwm_uc_cfg[cfg_idx].mode != EMIOS_PWM_MODE_SAIC) {
		LOG_ERR("Channel %u is not configured for input capture", channel);
		return -ENOTSUP;
	}

	if (data->ch_data[channel].capture.enabled) {
		LOG_ERR("Channel %u capture is already enabled", channel);
		return -EBUSY;
	}

	data->ch_data[channel].capture.enabled = true;

	/* Enable interrupt for capture channel */
	config->base->UC[channel].C |= EMIOS_C_FEN_MASK;

	/* Enable input capture channel prescaler */
	config->base->UC[channel].C |= EMIOS_C_UCPREN_MASK;

	return 0;
}

static int pwm_nxp_emios_disable_capture(const struct device *dev, uint32_t channel)
{
	const struct pwm_nxp_emios_config *config = dev->config;
	struct pwm_nxp_emios_data *data = dev->data;
	uint8_t cfg_idx;

	if (channel >= EMIOS_CH_UC_UC_COUNT) {
		LOG_ERR("Invalid channel %u", channel);
		return -EINVAL;
	}

	/* Get configuration array index from hardware channel number */
	cfg_idx = data->ch_to_cfg_idx[channel];
	if (cfg_idx == 0xFFU) {
		LOG_ERR("Channel %u is not configured", channel);
		return -EINVAL;
	}

	/* Only SAIC mode supports input capture */
	if (config->pwm_uc_cfg[cfg_idx].mode != EMIOS_PWM_MODE_SAIC) {
		LOG_ERR("Channel %u is not configured for input capture", channel);
		return -ENOTSUP;
	}

	data->ch_data[channel].capture.enabled = false;

	/* Disable interrupt for capture channel */
	config->base->UC[channel].C &= ~EMIOS_C_FEN_MASK;

	/* Clear any pending flags */
	config->base->UC[channel].S = (EMIOS_S_FLAG_MASK | EMIOS_S_OVR_MASK);

	/* Disable input capture channel prescaler */
	config->base->UC[channel].C &= ~EMIOS_C_UCPREN_MASK;

	return 0;
}

static void pwm_nxp_emios_isr(const struct device *dev)
{
	const struct pwm_nxp_emios_config *config = dev->config;
	struct pwm_nxp_emios_data *data = dev->data;
	struct pwm_nxp_emios_capture_data *capture;
	uint32_t gflag;
	uint32_t channel;
	uint32_t a_reg;
	uint32_t cap_val;
	uint32_t period_cycles;
	uint32_t pulse_cycles;
	bool is_active;
	uint8_t cfg_idx;
	uint8_t cnt_bus_cfg_idx;
	int lsb;

	/*
	 * Read GFLAG register: each bit n is a mirror of UC[n].S[FLAG].
	 * Use it to efficiently determine which channels need service.
	 */
	gflag = config->base->GFLAG & BIT_MASK(EMIOS_CH_UC_UC_COUNT);

	while (gflag != 0U) {
		lsb = find_lsb_set(gflag);
		channel = (uint32_t)(lsb - 1);
		gflag &= ~BIT(channel);

		/*
		 * Counter bus channel (used by OPWMB/OPWMCB modes): clear
		 * FLAG and skip. These channels generate periodic interrupts
		 * at the end of each MCB period; they are not capture events
		 * and require no further processing here.
		 */
		cnt_bus_cfg_idx = data->cnt_bus_ch_to_cfg_idx[channel];
		if (cnt_bus_cfg_idx != 0xFFU) {
			config->base->UC[channel].S |= EMIOS_S_FLAG_MASK;
			continue;
		}

		/* Input capture channel: must be configured as SAIC */
		cfg_idx = data->ch_to_cfg_idx[channel];
		if (cfg_idx == 0xFFU ||
		    config->pwm_uc_cfg[cfg_idx].mode != EMIOS_PWM_MODE_SAIC) {
			config->base->UC[channel].S |= EMIOS_S_FLAG_MASK;
			continue;
		}

		capture = &data->ch_data[channel].capture;

		if (capture->callback == NULL) {
			config->base->UC[channel].S |= EMIOS_S_FLAG_MASK;
			continue;
		}

		/*
		 * SAIC mode: the A register holds the counter value latched
		 * on the triggering edge.
		 *
		 * Read A before clearing FLAG to ensure the latched value is
		 * consumed before the hardware can overwrite it on the next edge.
		 */
		a_reg = config->base->UC[channel].A;
		cap_val = a_reg & EMIOS_A_A_MASK;
		/*
		 * Bit 31 of A reflects the active (reference) edge regardless
		 * of polarity: 1 = active edge occurred (rising when EDPOL=1,
		 * falling when EDPOL=0).
		 */
		is_active = (a_reg & RISING_FALLING_EDGE_MASK) != 0U;

		/* Overrun: a new edge arrived before the previous capture was consumed */
		if (config->base->UC[channel].S & EMIOS_S_OVR_MASK) {
			config->base->UC[channel].S |= (EMIOS_S_FLAG_MASK | EMIOS_S_OVR_MASK);
			capture->state = 0U;
			if (!capture->continuous) {
				capture->enabled = false;
				config->base->UC[channel].C &= ~EMIOS_C_FEN_MASK;
			}
			capture->callback(dev, channel, 0U, 0U, -ERANGE, capture->user_data);
			continue;
		}

		config->base->UC[channel].S |= EMIOS_S_FLAG_MASK;

		switch (capture->state) {
		case 0U:
			/*
			 * Idle: wait for the first active (reference) edge to
			 * start the measurement window.
			 */
			if (is_active) {
				capture->capture_value[0] = cap_val;
				capture->state = 1U;
			}
			break;

		case 1U:
			if (capture->capture_period && !capture->capture_pulse) {
				/*
				 * Period-only: measure from one active edge to
				 * the next active edge.
				 */
				if (is_active) {
					period_cycles = pwm_nxp_emios_elapsed(
						cap_val,
						capture->capture_value[0]);
					if (capture->continuous) {
						capture->capture_value[0] = cap_val;
					} else {
						capture->state = 0U;
						capture->enabled = false;
						config->base->UC[channel].C &=
							~EMIOS_C_FEN_MASK;
					}
					capture->callback(dev, channel,
							  period_cycles, 0U, 0,
							  capture->user_data);
				}
			} else if (!capture->capture_period && capture->capture_pulse) {
				/*
				 * Pulse-only: measure from the active edge to
				 * the opposite edge.
				 */
				if (!is_active) {
					pulse_cycles = pwm_nxp_emios_elapsed(
						cap_val,
						capture->capture_value[0]);
					capture->state = 0U;
					if (!capture->continuous) {
						capture->enabled = false;
						config->base->UC[channel].C &=
							~EMIOS_C_FEN_MASK;
					}
					capture->callback(dev, channel, 0U,
							  pulse_cycles, 0,
							  capture->user_data);
				}
			} else {
				/*
				 * Both period and pulse: record the opposite-edge
				 * timestamp, then advance to state 2 to wait for
				 * the period-closing active edge.
				 */
				if (!is_active) {
					capture->capture_value[1] = cap_val;
					capture->state = 2U;
				}
			}
			break;

		case 2U:
			/*
			 * Both period and pulse: the second active edge completes
			 * the measurement.
			 *   period = second_active_edge - first_active_edge
			 *   pulse  = opposite_edge      - first_active_edge
			 */
			if (is_active) {
				period_cycles = pwm_nxp_emios_elapsed(
					cap_val,
					capture->capture_value[0]);
				pulse_cycles = pwm_nxp_emios_elapsed(
					capture->capture_value[1],
					capture->capture_value[0]);
				if (capture->continuous) {
					capture->capture_value[0] = cap_val;
					capture->state = 1U;
				} else {
					capture->state = 0U;
					capture->enabled = false;
					config->base->UC[channel].C &=
						~EMIOS_C_FEN_MASK;
				}
				capture->callback(dev, channel,
						  period_cycles, pulse_cycles,
						  0, capture->user_data);
			}
			break;

		default:
			capture->state = 0U;
			break;
		}
	}
}
#endif

static int pwm_nxp_emios_init(const struct device *dev)
{
	const struct pwm_nxp_emios_config *config = dev->config;
	struct pwm_nxp_emios_data *data = dev->data;
	uint32_t reg;
	uint8_t channel;
	int err = 0;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("Clock device not ready");
		return -ENODEV;
	}

	err = clock_control_on(config->clock_dev, config->clock_subsys);
	if (err) {
		LOG_ERR("Failed to enable clock: %u", err);
		return err;
	}

	err = clock_control_get_rate(config->clock_dev, config->clock_subsys, &data->emios_clk);
	if (err) {
		LOG_ERR("Failed to get clock rate for eMIOS: %u", err);
		return -EINVAL;
	}
	LOG_DBG("eMIOS clock rate: %u Hz", data->emios_clk);

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	/* Configure global prescaler */
	reg = config->base->MCR;
	reg = (reg & ~EMIOS_MCR_GPRE_MASK) | EMIOS_MCR_GPRE(config->global_prescaler - 1U);
	reg |= EMIOS_MCR_GTBE_MASK;
	reg &= ~(EMIOS_MCR_GPREN_MASK | EMIOS_MCR_MDIS_MASK);
	config->base->MCR = reg;

	/* Initialize channel-to-config-index mapping */
	for (uint8_t i = 0; i < EMIOS_CH_UC_UC_COUNT; i++) {
		data->ch_to_cfg_idx[i] = 0xFFU; /* Invalid index */
		data->cnt_bus_ch_to_cfg_idx[i] = 0xFFU; /* Invalid index */
	}

	/* Configure counter bus channels */
	for (uint8_t i = 0; i < config->num_cnt_bus_uc; i++) {
		channel = config->cnt_bus_uc_cfg[i].channel;

		if (channel >= EMIOS_CH_UC_UC_COUNT) {
			LOG_ERR("Invalid counter bus channel %u", channel);
			return -EINVAL;
		}

		/* Build mapping from counter bus channel to config index */
		data->cnt_bus_ch_to_cfg_idx[channel] = i;

		/* To change the UC to MCB mode, first change to GPIO input mode */
		reg = config->base->UC[channel].C;
		reg &= ~(EMIOS_C_MODE_MASK | EMIOS_C_EDPOL_MASK | EMIOS_C_UCPREN_MASK |
			 EMIOS_C_FREN_MASK);
		reg |= EMIOS_C_EDSEL_MASK;
		config->base->UC[channel].C = reg;

		/* Configure prescaler in C2 register */
		reg = config->base->UC[channel].C2;
		reg = (reg & ~EMIOS_C2_UCEXTPRE_MASK) |
		      EMIOS_C2_UCEXTPRE(config->cnt_bus_uc_cfg[i].prescaler - 1U);
		/* Use prescaled clock as default */
		reg &= ~(EMIOS_C2_UCPRECLK_MASK);
		config->base->UC[channel].C2 = reg;

		/* Initialize counter to 1 for MCB mode */
		config->base->UC[channel].CNT = 1U;

		/* Configure counter bus mode */
		reg = config->base->UC[channel].C;
		/* Set BSL to 3 (internal counter) */
		reg |= EMIOS_C_BSL_MASK;
		reg &= ~(EMIOS_C_EDSEL_MASK);

		/* Set mode based on configuration: MCB_UP or MCB_UPDOWN */
		if (config->cnt_bus_uc_cfg[i].mode == EMIOS_CNT_BUS_MODE_UP) {
			reg = (reg & ~EMIOS_C_MODE_MASK) | EMIOS_C_MODE(EMIOS_MODE_MCB_UP);
		} else {
			reg = (reg & ~EMIOS_C_MODE_MASK) | EMIOS_C_MODE(EMIOS_MODE_MCB_UPDOWN);
		}

		config->base->UC[channel].C = reg;

		LOG_DBG("Counter bus[%u]: ch=%u configured", i, channel);
	}

	/* Check if at least one PWM channel is configured */
	if (config->num_pwm_uc == 0U) {
		LOG_ERR("No PWM channels configured");
		return -EINVAL;
	}

	/* Configure PWM channels */
	for (uint8_t i = 0; i < config->num_pwm_uc; i++) {
		channel = config->pwm_uc_cfg[i].channel;

		if (channel >= EMIOS_CH_UC_UC_COUNT) {
			LOG_ERR("Invalid PWM channel %u", channel);
			return -EINVAL;
		}

		/* Build mapping from channel number to config index */
		data->ch_to_cfg_idx[channel] = i;

		/* Configure prescaler in C2 register */
		reg = config->base->UC[channel].C2;
		reg = (reg & ~EMIOS_C2_UCEXTPRE_MASK) |
		      EMIOS_C2_UCEXTPRE(config->pwm_uc_cfg[i].prescaler - 1U);
		/* Use prescaled clock as default */
		reg &= ~(EMIOS_C2_UCPRECLK_MASK);
		config->base->UC[channel].C2 = reg;

		/* Configure input filter in C register and disable UC prescaler */
		reg = config->base->UC[channel].C;
		reg = (reg & ~EMIOS_C_IF_MASK) |
		      EMIOS_C_IF(config->pwm_uc_cfg[i].input_filter / 2U);
		/* Use prescaled clock for filter */
		reg = (reg & ~EMIOS_C_FCK_MASK) | EMIOS_C_FCK(0U);
		reg &= ~EMIOS_C_UCPREN_MASK;
		config->base->UC[channel].C = reg;

		LOG_DBG("PWM[%u]: ch=%u configured", i, channel);
	}

	config->base->MCR |= EMIOS_MCR_GPREN_MASK;

#ifdef CONFIG_PWM_CAPTURE
	config->irq_config_func(dev);
#endif /* CONFIG_PWM_CAPTURE */

	return err;
}

static DEVICE_API(pwm, pwm_nxp_emios_driver_api) = {
	.set_cycles = pwm_nxp_emios_set_cycles,
	.get_cycles_per_sec = pwm_nxp_emios_get_cycles_per_sec,
#ifdef CONFIG_PWM_CAPTURE
	.configure_capture = pwm_nxp_emios_configure_capture,
	.enable_capture = pwm_nxp_emios_enable_capture,
	.disable_capture = pwm_nxp_emios_disable_capture,
#endif /* CONFIG_PWM_CAPTURE */
};


/* Generate counter bus configuration for one child node */
#define EMIOS_CNT_BUS_CFG_INIT(id)						\
	{									\
		.channel = DT_PROP(id, channel),				\
		.prescaler = DT_PROP_OR(id, prescaler, 1),			\
		.mode = DT_ENUM_IDX(id, mode),					\
	},

/* Get the counter_bus node from parent emios node */
#define EMIOS_COUNTER_BUS_NODE(id)						\
	DT_CHILD(DT_INST_PARENT(id), counter_bus)

/* Check if counter_bus node exists and has status okay children */
#define EMIOS_HAS_COUNTER_BUS(id)						\
	DT_NODE_EXISTS(EMIOS_COUNTER_BUS_NODE(id))

/* Initialize all counter bus configurations */
#define EMIOS_CNT_BUS_CFG_ARRAY(id)						\
	COND_CODE_1(EMIOS_HAS_COUNTER_BUS(id),					\
		    (DT_FOREACH_CHILD_STATUS_OKAY(EMIOS_COUNTER_BUS_NODE(id),	\
					      EMIOS_CNT_BUS_CFG_INIT)),		\
		    ())

/* Define counter bus array for the instance */
#define EMIOS_CNT_BUS_ARRAY_DEFINE(id)						\
	COND_CODE_1(EMIOS_HAS_COUNTER_BUS(id),					\
		   (static const struct pwm_nxp_emios_cnt_bus_uc_cfg		\
			pwm_nxp_emios_cnt_bus_uc_cfg_##id[] = {			\
				EMIOS_CNT_BUS_CFG_ARRAY(id)			\
			};), ())

/* Initialize cnt_bus_uc_cfg and num_cnt_bus_uc config fields */
#define EMIOS_CNT_BUS_CONFIG_INIT(id)						\
	COND_CODE_1(EMIOS_HAS_COUNTER_BUS(id),					\
		   (.cnt_bus_uc_cfg = (struct pwm_nxp_emios_cnt_bus_uc_cfg *)	\
		    pwm_nxp_emios_cnt_bus_uc_cfg_##id,				\
		    .num_cnt_bus_uc = ARRAY_SIZE(pwm_nxp_emios_cnt_bus_uc_cfg_##id),),	\
		   (.cnt_bus_uc_cfg = NULL,					\
		    .num_cnt_bus_uc = 0,))

/* Generate PWM channel configuration for one child node */
#define EMIOS_PWM_UC_CFG_INIT(id)						\
	{									\
		.channel = DT_PROP(id, channel),				\
		.prescaler = DT_PROP_OR(id, prescaler, 1),			\
		.dead_time = DT_PROP_OR(id, dead_time, 0),			\
		.phase_shift = DT_PROP_OR(id, phase_shift, 0),			\
		.input_filter = DT_PROP_OR(id, input_filter, 0),		\
		.mode = DT_ENUM_IDX(id, mode),					\
		.bus_type = DT_ENUM_IDX(id, counter_bus_type),			\
	},

/* Check if PWM node has status okay children */
#define EMIOS_HAS_PWM_CHANNELS(id)						\
	DT_NODE_HAS_STATUS_OKAY(DT_DRV_INST(id))

/* Initialize all PWM channel configurations */
#define EMIOS_PWM_UC_CFG_ARRAY(id)						\
	DT_FOREACH_CHILD_STATUS_OKAY(DT_DRV_INST(id), EMIOS_PWM_UC_CFG_INIT)

/* Define PWM channel array for the instance */
#define EMIOS_PWM_UC_ARRAY_DEFINE(id)						\
	COND_CODE_1(EMIOS_HAS_PWM_CHANNELS(id),					\
		   (static const struct pwm_nxp_emios_pwm_uc_cfg		\
			pwm_nxp_emios_pwm_uc_cfg_##id[] = {			\
				EMIOS_PWM_UC_CFG_ARRAY(id)			\
			};), ())

/* Initialize pwm_uc_cfg and num_pwm_uc config fields */
#define EMIOS_PWM_UC_CONFIG_INIT(id)						\
	COND_CODE_1(EMIOS_HAS_PWM_CHANNELS(id),					\
		   (.pwm_uc_cfg = (struct pwm_nxp_emios_pwm_uc_cfg *)		\
		    pwm_nxp_emios_pwm_uc_cfg_##id,				\
		    .num_pwm_uc = ARRAY_SIZE(pwm_nxp_emios_pwm_uc_cfg_##id),),	\
		   (.pwm_uc_cfg = NULL,						\
		    .num_pwm_uc = 0,))

#ifdef CONFIG_PWM_CAPTURE
#define EMIOS_IRQ_BY_IDX(node_id, prop, idx, cell)				\
	DT_IRQ_BY_NAME(node_id, DT_STRING_TOKEN_BY_IDX(node_id, prop, idx), cell)

#define EMIOS_IRQ_ENABLE_CODE(node_id, prop, idx)				\
	irq_enable(EMIOS_IRQ_BY_IDX(node_id, prop, idx, irq));

#define EMIOS_IRQ_CONFIG_CODE(node_id, prop, idx)				\
	do {									\
		IRQ_CONNECT(EMIOS_IRQ_BY_IDX(node_id, prop, idx, irq),		\
			    EMIOS_IRQ_BY_IDX(node_id, prop, idx, priority),	\
			    pwm_nxp_emios_isr,					\
			    DEVICE_DT_GET(DT_CHILD(node_id, pwm)), 0);		\
		EMIOS_IRQ_ENABLE_CODE(node_id, prop, idx);			\
	} while (false);

#define EMIOS_IRQ_CONFIG_FUNC(id)						\
	static void pwm_nxp_emios_config_func_##id(const struct device *dev)	\
	{									\
		DT_FOREACH_PROP_ELEM(DT_INST_PARENT(id), interrupt_names, EMIOS_IRQ_CONFIG_CODE); \
	}
#else
#define EMIOS_IRQ_CONFIG_FUNC(id)
#endif /* CONFIG_PWM_CAPTURE */

#define PWM_NXP_EMIOS_DEVICE_INIT(id)						\
	PINCTRL_DT_INST_DEFINE(id);						\
	EMIOS_CNT_BUS_ARRAY_DEFINE(id)						\
	EMIOS_PWM_UC_ARRAY_DEFINE(id)						\
	EMIOS_IRQ_CONFIG_FUNC(id)						\
	static const struct pwm_nxp_emios_config pwm_nxp_emios_config_##id = {	\
		.base = (EMIOS_Type *)DT_REG_ADDR(DT_INST_PARENT(id)),		\
		.global_prescaler = DT_PROP_OR(DT_INST_PARENT(id), global_prescaler, 1),\
		EMIOS_CNT_BUS_CONFIG_INIT(id)					\
		EMIOS_PWM_UC_CONFIG_INIT(id)					\
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_INST_PARENT(id))),	\
		.clock_subsys = (clock_control_subsys_t)			\
			DT_CLOCKS_CELL(DT_INST_PARENT(id), name),		\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(id),			\
		IF_ENABLED(CONFIG_PWM_CAPTURE,					\
			   (.irq_config_func = pwm_nxp_emios_config_func_##id,))\
	};									\
	static struct pwm_nxp_emios_data pwm_nxp_emios_data_##id;		\
	DEVICE_DT_INST_DEFINE(id,						\
			&pwm_nxp_emios_init,					\
			NULL,							\
			&pwm_nxp_emios_data_##id,				\
			&pwm_nxp_emios_config_##id,				\
			POST_KERNEL,						\
			CONFIG_PWM_INIT_PRIORITY,				\
			&pwm_nxp_emios_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_NXP_EMIOS_DEVICE_INIT)
