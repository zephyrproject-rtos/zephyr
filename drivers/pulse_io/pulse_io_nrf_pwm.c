/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * Copyright (c) 2026 Dev It Wise
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_pwm_pulse_io

#include <errno.h>

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pulse_io.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include <hal/nrf_gpio.h>
#include <nrfx_pwm.h>
#include <soc.h>

LOG_MODULE_REGISTER(pulse_io_nrf_pwm, CONFIG_PULSE_IO_LOG_LEVEL);

#define NRF_PWM_MAX_PRESCALER 7U
#define NRF_PWM_POLARITY_BIT  BIT(15)
#define NRF_PWM_COMPARE_MAX   BIT_MASK(15)

#define NRF_PWM_CHANNEL_HANDLE ((struct pulse_io_channel *)1)

struct nrf_pwm_pio_config {
	const struct pinctrl_dev_config *pcfg;
	uint16_t *seq;
	size_t max_cells;
	uint32_t clock_hz;
	uint8_t irq_priority;
};

struct nrf_pwm_pio_data {
	nrfx_pwm_t pwm;
	struct k_mutex lock;
	struct k_sem done;
	struct k_spinlock run_lock;
	atomic_t in_use;
	bool running;
	bool canceled;
	bool configured;
	bool idle_high;
	uint32_t resolution_hz;
	uint32_t cell_period_ticks;
	uint32_t base_clock_hz;
	uint16_t top;
};

static uint64_t ticks_to_clock(const struct nrf_pwm_pio_data *data, uint32_t ticks)
{
	uint64_t clocks = (uint64_t)ticks * data->base_clock_hz;

	if (clocks <= UINT32_MAX - data->resolution_hz / 2U) {
		return DIV_ROUND_CLOSEST((uint32_t)clocks, data->resolution_hz);
	}

	return DIV_ROUND_CLOSEST(clocks, data->resolution_hz);
}

static bool channel_valid(const struct device *dev, struct pulse_io_channel *chan)
{
	struct nrf_pwm_pio_data *data = dev->data;

	return (chan == NRF_PWM_CHANNEL_HANDLE) && (atomic_get(&data->in_use) != 0);
}

static void cancel_transmit(struct nrf_pwm_pio_data *data)
{
	k_spinlock_key_t key = k_spin_lock(&data->run_lock);

	if (data->running) {
		data->canceled = true;
		(void)nrfx_pwm_stop(&data->pwm, false);
	}

	k_spin_unlock(&data->run_lock, key);
}

static void nrf_pwm_pio_event_handler(nrfx_pwm_event_type_t event, void *context)
{
	struct nrf_pwm_pio_data *data = context;

	if (event == NRFX_PWM_EVENT_STOPPED) {
		k_sem_give(&data->done);
	}
}

static int nrf_pwm_pio_get_capabilities(const struct device *dev, struct pulse_io_caps *caps)
{
	const struct nrf_pwm_pio_config *config = dev->config;

	*caps = (struct pulse_io_caps){
		.tx_max_streaming = config->max_cells,
		.modes = PULSE_IO_MODE_CELL,
		.min_tick_ns = DIV_ROUND_UP(NSEC_PER_SEC, config->clock_hz),
		.max_tick_ns = DIV_ROUND_UP((uint64_t)NSEC_PER_SEC << NRF_PWM_MAX_PRESCALER,
					    config->clock_hz),
		.tx_channel_mask = BIT(0),
		.cell_duty_bits = 15,
		.num_channels = 1,
		.supports_tx = true,
		.cell_period_per_chunk = true,
	};

	return 0;
}

static int nrf_pwm_pio_channel_get(const struct device *dev, uint8_t channel_idx,
				   struct pulse_io_channel **chan)
{
	struct nrf_pwm_pio_data *data = dev->data;

	if (channel_idx != 0U) {
		return -ENODEV;
	}

	if (!atomic_cas(&data->in_use, 0, 1)) {
		return -EBUSY;
	}

	*chan = NRF_PWM_CHANNEL_HANDLE;

	return 0;
}

static int nrf_pwm_pio_channel_release(const struct device *dev, struct pulse_io_channel *chan)
{
	struct nrf_pwm_pio_data *data = dev->data;

	if (!channel_valid(dev, chan)) {
		return -EINVAL;
	}

	cancel_transmit(data);

	k_mutex_lock(&data->lock, K_FOREVER);
	data->configured = false;
	k_mutex_unlock(&data->lock);

	atomic_set(&data->in_use, 0);

	return 0;
}

static int nrf_pwm_pio_channel_configure(const struct device *dev, struct pulse_io_channel *chan,
					 const struct pulse_io_config *cfg)
{
	const struct nrf_pwm_pio_config *config = dev->config;
	struct nrf_pwm_pio_data *data = dev->data;
	uint32_t psel;
	uint8_t prescaler;
	uint64_t top = 0U;
	int ret = 0;

	if (!channel_valid(dev, chan)) {
		return -EINVAL;
	}

	if ((cfg->mode != PULSE_IO_MODE_CELL) || (cfg->dir != PULSE_IO_DIR_TX) ||
	    cfg->carrier_en) {
		return -ENOTSUP;
	}

	if ((cfg->resolution_hz == 0U) || (cfg->cell_period_ticks == 0U)) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	data->configured = false;
	data->resolution_hz = cfg->resolution_hz;

	for (prescaler = 0U; prescaler <= NRF_PWM_MAX_PRESCALER; prescaler++) {
		data->base_clock_hz = config->clock_hz >> prescaler;
		top = ticks_to_clock(data, cfg->cell_period_ticks);
		if (top <= PWM_COUNTERTOP_COUNTERTOP_Msk) {
			break;
		}
	}

	if ((prescaler > NRF_PWM_MAX_PRESCALER) || (top < 3U)) {
		LOG_ERR("cell period of %u ticks at %u Hz is out of range", cfg->cell_period_ticks,
			cfg->resolution_hz);
		ret = -EINVAL;
		goto out;
	}

	data->top = (uint16_t)top;
	data->cell_period_ticks = cfg->cell_period_ticks;
	data->idle_high = cfg->idle_high;

	nrf_pwm_configure(data->pwm.p_reg, (nrf_pwm_clk_t)prescaler, NRF_PWM_MODE_UP, data->top);

#if NRF_PWM_HAS_IDLEOUT
	nrfy_pwm_channel_idle_set(data->pwm.p_reg, 0, cfg->idle_high);
#endif

	psel = nrf_pwm_pin_get(data->pwm.p_reg, 0);
	if ((psel & PWM_PSEL_OUT_CONNECT_Msk) ==
	    (PWM_PSEL_OUT_CONNECT_Connected << PWM_PSEL_OUT_CONNECT_Pos)) {
		nrf_gpio_pin_write(psel, cfg->idle_high ? 1 : 0);
	}

	data->configured = true;

out:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int nrf_pwm_pio_transmit_sync(const struct device *dev, struct pulse_io_channel *chan,
				     const struct pulse_io_tx_req *req, k_timeout_t timeout)
{
	const struct nrf_pwm_pio_config *config = dev->config;
	struct nrf_pwm_pio_data *data = dev->data;
	nrf_pwm_sequence_t seq = {
		.values.p_common = config->seq,
	};
	k_spinlock_key_t key;
	uint64_t compare;
	bool canceled;
	int ret;

	if (!channel_valid(dev, chan)) {
		return -EINVAL;
	}

	if (req->loop_count != 0U) {
		return -ENOTSUP;
	}

	if (req->count > config->max_cells) {
		return -ENOMEM;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (!data->configured) {
		ret = -EINVAL;
		goto out;
	}

	for (size_t i = 0; i < req->count; i++) {
		if (req->cells[i].duty > data->cell_period_ticks) {
			ret = -EINVAL;
			goto out;
		}
		compare = MIN(ticks_to_clock(data, req->cells[i].duty), data->top);
		config->seq[i] = NRF_PWM_POLARITY_BIT | (uint16_t)compare;
	}

	config->seq[req->count] =
		NRF_PWM_POLARITY_BIT | (data->idle_high ? NRF_PWM_COMPARE_MAX : 0U);
	seq.length = (uint16_t)(req->count + 1U);

	sys_cache_data_flush_range(config->seq, seq.length * sizeof(config->seq[0]));

	k_sem_reset(&data->done);

	key = k_spin_lock(&data->run_lock);
	data->running = true;
	data->canceled = false;
	(void)nrfx_pwm_simple_playback(&data->pwm, &seq, 1, NRFX_PWM_FLAG_STOP);
	k_spin_unlock(&data->run_lock, key);

	ret = k_sem_take(&data->done, timeout);
	if (ret != 0) {
		(void)nrfx_pwm_stop(&data->pwm, true);
		ret = -ETIMEDOUT;
	}

	key = k_spin_lock(&data->run_lock);
	data->running = false;
	canceled = data->canceled;
	k_spin_unlock(&data->run_lock, key);

	if ((ret == 0) && canceled) {
		ret = -ECANCELED;
	}

out:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int nrf_pwm_pio_receive_sync(const struct device *dev, struct pulse_io_channel *chan,
				    const struct pulse_io_rx_req *req, size_t *count,
				    k_timeout_t timeout)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan);
	ARG_UNUSED(req);
	ARG_UNUSED(count);
	ARG_UNUSED(timeout);

	return -ENOTSUP;
}

static int nrf_pwm_pio_stop(const struct device *dev, struct pulse_io_channel *chan)
{
	if (!channel_valid(dev, chan)) {
		return -EINVAL;
	}

	cancel_transmit(dev->data);

	return 0;
}

static DEVICE_API(pulse_io, nrf_pwm_pio_api) = {
	.get_capabilities = nrf_pwm_pio_get_capabilities,
	.channel_get = nrf_pwm_pio_channel_get,
	.channel_release = nrf_pwm_pio_channel_release,
	.channel_configure = nrf_pwm_pio_channel_configure,
	.transmit_sync = nrf_pwm_pio_transmit_sync,
	.receive_sync = nrf_pwm_pio_receive_sync,
	.stop = nrf_pwm_pio_stop,
};

static int nrf_pwm_pio_init(const struct device *dev)
{
	const struct nrf_pwm_pio_config *config = dev->config;
	struct nrf_pwm_pio_data *data = dev->data;
	const nrfx_pwm_config_t pwm_config = {
		.output_pins = {NRF_PWM_PIN_NOT_CONNECTED, NRF_PWM_PIN_NOT_CONNECTED,
				NRF_PWM_PIN_NOT_CONNECTED, NRF_PWM_PIN_NOT_CONNECTED},
		.irq_priority = config->irq_priority,
		.base_clock = (nrf_pwm_clk_t)0,
		.count_mode = NRF_PWM_MODE_UP,
		.top_value = NRF_PWM_COMPARE_MAX,
		.load_mode = NRF_PWM_LOAD_COMMON,
		.step_mode = NRF_PWM_STEP_AUTO,
		.skip_gpio_cfg = true,
		.skip_psel_cfg = true,
	};
	int ret;

	k_mutex_init(&data->lock);
	k_sem_init(&data->done, 0, 1);

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	ret = nrfx_pwm_init(&data->pwm, &pwm_config, nrf_pwm_pio_event_handler, data);
	if (ret < 0) {
		LOG_ERR("failed to initialize %s: %d", dev->name, ret);
		return ret;
	}

	return 0;
}

#define NRF_PWM_PIO_MEMORY_SECTION(inst)                                                           \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, memory_regions),                                   \
		    (__attribute__((__section__(LINKER_DT_NODE_REGION_NAME(                        \
			    DT_INST_PHANDLE(inst, memory_regions)))))),                            \
		    ())

#define NRF_PWM_PIO_DEFINE(inst)                                                                   \
	NRF_DT_CHECK_NODE_HAS_REQUIRED_MEMORY_REGIONS(DT_DRV_INST(inst));                          \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static uint16_t nrf_pwm_pio_seq_##inst[CONFIG_PULSE_IO_NRF_PWM_CELLS + 1]                  \
		NRF_PWM_PIO_MEMORY_SECTION(inst);                                                  \
	static const struct nrf_pwm_pio_config nrf_pwm_pio_config_##inst = {                       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		.seq = nrf_pwm_pio_seq_##inst,                                                     \
		.max_cells = CONFIG_PULSE_IO_NRF_PWM_CELLS,                                        \
		.clock_hz = NRF_PERIPH_GET_FREQUENCY(DT_DRV_INST(inst)),                           \
		.irq_priority = DT_INST_IRQ(inst, priority),                                       \
	};                                                                                         \
	static struct nrf_pwm_pio_data nrf_pwm_pio_data_##inst = {                                 \
		.pwm = NRFX_PWM_INSTANCE(DT_INST_REG_ADDR(inst)),                                  \
	};                                                                                         \
	static int nrf_pwm_pio_init_##inst(const struct device *dev)                               \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),                       \
			    nrfx_pwm_irq_handler, &nrf_pwm_pio_data_##inst.pwm, 0);                \
		return nrf_pwm_pio_init(dev);                                                      \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(inst, nrf_pwm_pio_init_##inst, NULL, &nrf_pwm_pio_data_##inst,       \
			      &nrf_pwm_pio_config_##inst, POST_KERNEL,                             \
			      CONFIG_PULSE_IO_INIT_PRIORITY, &nrf_pwm_pio_api);

DT_INST_FOREACH_STATUS_OKAY(NRF_PWM_PIO_DEFINE)
