/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Telink Semiconductor (Shanghai) Co., Ltd.
 */

#include <zephyr/logging/log.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pwm.h>
#include <ipc/ipc_based_driver.h>

#define LOG_LEVEL CONFIG_PWM_LOG_LEVEL
LOG_MODULE_REGISTER(pwm_telink);

/* Driver dts compatibility: telink,w91_pwm */
#define DT_DRV_COMPAT telink_w91_pwm

#define FREQ_DIVIDER 40u

enum {
	IPC_DISPATCHER_TIMER_CONFIG = IPC_DISPATCHER_PWM,
	IPC_DISPATCHER_TIMER0_GET_SPEED,
	IPC_DISPATCHER_TIMER1_GET_SPEED,
};


struct pwm_w91_config {
	const pinctrl_soc_pin_t *pins;
	uint8_t channels;
	uint8_t instance_id;            /* instance id */
};

struct timer_get_speed_resp {
	int err;
	uint32_t value;
};

struct pwm_w91_data {
	uint8_t out_pin_ch_connected;
	uint32_t timer0_clock_frequency;
	uint32_t timer1_clock_frequency;
	struct ipc_based_driver ipc;    /* ipc driver part */
};

struct pwm_w91_ipc_config {
	uint32_t ch;
	uint32_t param;
};

/* APIs implementation: pin configure */
static size_t pack_pwm_w91_ipc_configure(uint8_t inst, void *unpack_data, uint8_t *pack_data)
{
	struct pwm_w91_ipc_config *p_pwm_config_req = unpack_data;
	size_t pack_data_len = sizeof(uint32_t) +
			sizeof(p_pwm_config_req->ch) + sizeof(p_pwm_config_req->param);

	if (pack_data != NULL) {
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_TIMER_CONFIG, inst);

		IPC_DISPATCHER_PACK_FIELD(pack_data, id);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_pwm_config_req->ch);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_pwm_config_req->param);
	}

	return pack_data_len;
}

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(pwm_w91_ipc_configure);

static int pwm_w91_ipc_configure(const struct device *dev, uint32_t channel,
				uint32_t high_cycles, uint32_t low_cycles)
{
	int err;
	struct pwm_w91_ipc_config pwm_config_req;

	pwm_config_req.ch = channel;
	pwm_config_req.param = (high_cycles << 16) | (low_cycles & 0xffff);

	struct ipc_based_driver *ipc_data = &((struct pwm_w91_data *)dev->data)->ipc;
	uint8_t inst = ((struct pwm_w91_config *)dev->config)->instance_id;

	IPC_DISPATCHER_HOST_SEND_DATA(ipc_data, inst,
		pwm_w91_ipc_configure, &pwm_config_req, &err,
		CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	return err;
}

IPC_DISPATCHER_PACK_FUNC_WITHOUT_PARAM(timer0_ipc_wrap_get_speed, IPC_DISPATCHER_TIMER0_GET_SPEED);

static void unpack_timer0_ipc_wrap_get_speed(void *unpack_data,
		const uint8_t *pack_data, size_t pack_data_len)
{
	struct timer_get_speed_resp *p_timer_get_speed_resp = unpack_data;
	size_t expect_len = sizeof(uint32_t) +
			sizeof(p_timer_get_speed_resp->err) + sizeof(p_timer_get_speed_resp->value);

	if (expect_len != pack_data_len) {
		p_timer_get_speed_resp->err = -EINVAL;
		return;
	}

	pack_data += sizeof(uint32_t);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_timer_get_speed_resp->err);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_timer_get_speed_resp->value);
}

IPC_DISPATCHER_PACK_FUNC_WITHOUT_PARAM(timer1_ipc_wrap_get_speed, IPC_DISPATCHER_TIMER1_GET_SPEED);

static void unpack_timer1_ipc_wrap_get_speed(void *unpack_data,
		const uint8_t *pack_data, size_t pack_data_len)
{
	struct timer_get_speed_resp *p_timer_get_speed_resp = unpack_data;
	size_t expect_len = sizeof(uint32_t) +
			sizeof(p_timer_get_speed_resp->err) + sizeof(p_timer_get_speed_resp->value);

	if (expect_len != pack_data_len) {
		p_timer_get_speed_resp->err = -EINVAL;
		return;
	}

	pack_data += sizeof(uint32_t);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_timer_get_speed_resp->err);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_timer_get_speed_resp->value);
}

/* API implementation: init */
static int pwm_w91_init(const struct device *dev)
{
	struct pwm_w91_data *data = dev->data;
	struct timer_get_speed_resp timers_get_speed_resp;
	struct ipc_based_driver *ipc_data = &((struct pwm_w91_data *)dev->data)->ipc;
	uint8_t inst = ((struct pwm_w91_config *)dev->config)->instance_id;

	ipc_based_driver_init(ipc_data);

	IPC_DISPATCHER_HOST_SEND_DATA(ipc_data, inst,
		timer0_ipc_wrap_get_speed, NULL, &timers_get_speed_resp,
		CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	data->timer0_clock_frequency = timers_get_speed_resp.value / FREQ_DIVIDER;

	IPC_DISPATCHER_HOST_SEND_DATA(ipc_data, inst,
		timer1_ipc_wrap_get_speed, NULL, &timers_get_speed_resp,
		CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	data->timer1_clock_frequency = timers_get_speed_resp.value / FREQ_DIVIDER;

	return 0;
}

/* API implementation: set_cycles */
static int pwm_w91_set_cycles(const struct device *dev, uint32_t channel,
			      uint32_t period_cycles, uint32_t pulse_cycles,
			      pwm_flags_t flags)
{
	struct pwm_w91_data *data = dev->data;
	const struct pwm_w91_config *config = dev->config;

	/* check pwm channel */
	if (channel >= config->channels) {
		return -EINVAL;
	}
	uint32_t high_cycles, low_cycles;
	/* set polarity */
	if (flags & PWM_POLARITY_INVERTED) {
		high_cycles = period_cycles - pulse_cycles;
		low_cycles = pulse_cycles;
	} else {
		high_cycles = pulse_cycles;
		low_cycles = period_cycles - pulse_cycles;
	}

	/* check size of pulse and period (2 bytes) */
	if (((low_cycles * FREQ_DIVIDER) > UINT16_MAX)
		|| ((high_cycles * FREQ_DIVIDER) > UINT16_MAX)) {
		return -EINVAL;
	}

	/* connect output */
	if (!(data->out_pin_ch_connected & BIT(channel)) &&
		config->pins[channel] != UINT32_MAX) {
		const struct pinctrl_state pinctrl_state = {
			.pins = &config->pins[channel],
			.pin_cnt = 1,
			.id = PINCTRL_STATE_DEFAULT,
		};
		const struct pinctrl_dev_config pinctrl = {
			.states = &pinctrl_state,
			.state_cnt = 1,
		};

		if (!pinctrl_apply_state(&pinctrl, PINCTRL_STATE_DEFAULT)) {
			data->out_pin_ch_connected |= BIT(channel);
		} else {
			return -EIO;
		}
	}
	pwm_w91_ipc_configure(dev, channel, high_cycles, low_cycles);

	return 0;
}

/* API implementation: get_cycles_per_sec */


static int pwm_w91_get_cycles_per_sec(const struct device *dev,
				      uint32_t channel, uint64_t *cycles)
{
	const struct pwm_w91_config *config = dev->config;
	struct pwm_w91_data *data = dev->data;

	/* check pwm channel */
	if (channel >= config->channels) {
		return -EINVAL;
	}

	if (channel < 4) {
		*cycles = data->timer0_clock_frequency;
	} else {
		*cycles = data->timer1_clock_frequency;
	}

	return 0;
}

/* PWM driver APIs structure */
static const struct pwm_driver_api pwm_w91_driver_api = {
	.set_cycles = pwm_w91_set_cycles,
	.get_cycles_per_sec = pwm_w91_get_cycles_per_sec,
};

/* PWM driver registration */
#define PWM_w91_INIT(n)                                                 \
                                                                        \
	static const pinctrl_soc_pin_t pwm_w91_pins##n[] = {                \
		COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n), pinctrl_ch0),      \
		(Z_PINCTRL_STATE_PIN_INIT(DT_DRV_INST(n), pinctrl_ch0, 0)),     \
		(UINT32_MAX,))                                                  \
		COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n), pinctrl_ch1),      \
		(Z_PINCTRL_STATE_PIN_INIT(DT_DRV_INST(n), pinctrl_ch1, 0)),     \
		(UINT32_MAX,))                                                  \
		COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n), pinctrl_ch2),      \
		(Z_PINCTRL_STATE_PIN_INIT(DT_DRV_INST(n), pinctrl_ch2, 0)),     \
		(UINT32_MAX,))                                                  \
		COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n), pinctrl_ch3),      \
		(Z_PINCTRL_STATE_PIN_INIT(DT_DRV_INST(n), pinctrl_ch3, 0)),     \
		(UINT32_MAX,))                                                  \
		COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n), pinctrl_ch4),      \
		(Z_PINCTRL_STATE_PIN_INIT(DT_DRV_INST(n), pinctrl_ch4, 0)),     \
		(UINT32_MAX,))                                                  \
	};                                                                  \
                                                                        \
	static const struct pwm_w91_config config##n = {                    \
		.pins = pwm_w91_pins##n,                                        \
		.channels = DT_INST_PROP(n, channels),                          \
	};                                                                  \
                                                                        \
	struct pwm_w91_data data##n;                                        \
                                                                        \
	DEVICE_DT_INST_DEFINE(n, pwm_w91_init,                              \
		NULL, &data##n, &config##n,                                     \
		POST_KERNEL, CONFIG_TELINK_W91_IPC_DRIVERS_INIT_PRIORITY,       \
		&pwm_w91_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_w91_INIT);
