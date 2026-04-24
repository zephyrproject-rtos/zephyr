/*
 * Copyright (c) 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr clock_monitor back-end for NXP CMU_FM (Frequency Meter).
 *
 * Wraps the MCUX SDK fsl_cmu_fm HAL. Supports METER mode only. Uses its
 * own ISR wired via IRQ_CONNECT (not CMU_FM_RegisterCallBack, whose
 * callback does not carry a context pointer) so it can reach the Zephyr
 * struct device from the ISR.
 */

#define DT_DRV_COMPAT nxp_cmu_fm

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_monitor.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <fsl_cmu_fm.h>

LOG_MODULE_REGISTER(clock_monitor_nxp_cmu_fm,
		    CONFIG_CLOCK_MONITOR_LOG_LEVEL);

struct nxp_cmu_fm_config {
	struct clock_monitor_common_config common; /* MUST be first */
	CMU_FM_Type *base;
	const struct device    *ref_clk_dev;
	clock_control_subsys_t  ref_clk_subsys;
	const struct device    *mon_clk_dev;
	clock_control_subsys_t  mon_clk_subsys;
	void (*irq_config_func)(const struct device *dev);
};

enum nxp_cmu_fm_state {
	NXP_CMU_FM_STATE_IDLE = 0,
	NXP_CMU_FM_STATE_CONFIGURED,
	NXP_CMU_FM_STATE_RUNNING,
};

struct nxp_cmu_fm_data {
	struct k_spinlock lock;
	enum nxp_cmu_fm_state state;
	struct clock_monitor_config cfg;
	uint32_t ref_hz;            /* cached from clock_control at configure */
	uint32_t ref_cnt;
	uint32_t latched_events;
	struct k_sem measure_sem;
	uint32_t measure_result_hz;
	int measure_status;
};

static uint32_t compute_ref_cnt(uint32_t window_ns, uint32_t ref_hz)
{
	uint64_t cnt = (uint64_t)window_ns * (uint64_t)ref_hz / 1000000000ULL;

	if (cnt < 1U) {
		cnt = 1U;
	}
	if (cnt > CMU_FM_RCCR_REF_CNT_MASK) {
		cnt = CMU_FM_RCCR_REF_CNT_MASK;
	}
	return (uint32_t)cnt;
}

static int nxp_cmu_fm_configure(const struct device *dev,
				const struct clock_monitor_config *cfg)
{
	const struct nxp_cmu_fm_config *config = dev->config;
	struct nxp_cmu_fm_data *data = dev->data;
	k_spinlock_key_t key;

	if (cfg->mode != CLOCK_MONITOR_MODE_METER &&
	    cfg->mode != CLOCK_MONITOR_MODE_DISABLED) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	if (data->state == NXP_CMU_FM_STATE_RUNNING) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	if (cfg->mode == CLOCK_MONITOR_MODE_DISABLED) {
		CMU_FM_Deinit(config->base);
		data->cfg = *cfg;
		data->latched_events = 0U;
		data->state = NXP_CMU_FM_STATE_IDLE;
		k_spin_unlock(&data->lock, key);
		return 0;
	}
	k_spin_unlock(&data->lock, key);

	uint32_t ref_hz = 0U;
	int cc_ret = clock_control_get_rate(config->ref_clk_dev,
					    config->ref_clk_subsys, &ref_hz);

	if (cc_ret != 0 || ref_hz == 0U) {
		return -EIO;
	}

	uint32_t ref_cnt = compute_ref_cnt(cfg->meter.window_ns, ref_hz);

	cmu_fm_config_t hal_cfg;

	CMU_FM_GetDefaultConfig(&hal_cfg);
	hal_cfg.refClockCount = ref_cnt;
	hal_cfg.enableInterrupt = (config->irq_config_func != NULL);

	status_t st = CMU_FM_Init(config->base, &hal_cfg);

	key = k_spin_lock(&data->lock);
	if (st != kStatus_Success) {
		data->state = NXP_CMU_FM_STATE_IDLE;
		k_spin_unlock(&data->lock, key);
		return -EIO;
	}
	data->cfg = *cfg;
	data->latched_events = 0U;
	data->ref_hz = ref_hz;
	data->ref_cnt = ref_cnt;
	data->state = NXP_CMU_FM_STATE_CONFIGURED;
	k_spin_unlock(&data->lock, key);
	return 0;
}

static int nxp_cmu_fm_check(const struct device *dev)
{
	const struct nxp_cmu_fm_config *config = dev->config;
	struct nxp_cmu_fm_data *data = dev->data;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (data->state != NXP_CMU_FM_STATE_CONFIGURED ||
	    data->cfg.mode != CLOCK_MONITOR_MODE_METER) {
		k_spin_unlock(&data->lock, key);
		return -EINVAL;
	}

	data->state = NXP_CMU_FM_STATE_RUNNING;
	CMU_FM_StartFreqMetering(config->base);
	k_spin_unlock(&data->lock, key);
	return 0;
}

static int nxp_cmu_fm_stop(const struct device *dev)
{
	const struct nxp_cmu_fm_config *config = dev->config;
	struct nxp_cmu_fm_data *data = dev->data;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (data->state == NXP_CMU_FM_STATE_RUNNING) {
		CMU_FM_StopFreqMetering(config->base);
		data->state = NXP_CMU_FM_STATE_CONFIGURED;
	}
	k_spin_unlock(&data->lock, key);
	return 0;
}

static int nxp_cmu_fm_measure(const struct device *dev, uint32_t *rate_hz,
			      k_timeout_t timeout)
{
	const struct nxp_cmu_fm_config *config = dev->config;
	struct nxp_cmu_fm_data *data = dev->data;

	if (config->irq_config_func == NULL) {
		return -ENOSYS;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (data->state != NXP_CMU_FM_STATE_CONFIGURED ||
	    data->cfg.mode != CLOCK_MONITOR_MODE_METER) {
		k_spin_unlock(&data->lock, key);
		return -EINVAL;
	}

	k_sem_reset(&data->measure_sem);
	data->measure_status = -EAGAIN;
	data->state = NXP_CMU_FM_STATE_RUNNING;
	CMU_FM_StartFreqMetering(config->base);
	k_spin_unlock(&data->lock, key);

	if (k_sem_take(&data->measure_sem, timeout) != 0) {
		key = k_spin_lock(&data->lock);
		if (data->state == NXP_CMU_FM_STATE_RUNNING) {
			CMU_FM_StopFreqMetering(config->base);
			data->state = NXP_CMU_FM_STATE_CONFIGURED;
		}
		k_spin_unlock(&data->lock, key);
		return -EAGAIN;
	}

	key = k_spin_lock(&data->lock);
	int ret = data->measure_status;

	if (ret == 0) {
		*rate_hz = data->measure_result_hz;
	}
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int nxp_cmu_fm_get_events(const struct device *dev, uint32_t *events)
{
	struct nxp_cmu_fm_data *data = dev->data;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	*events = data->latched_events;
	data->latched_events = 0U;
	k_spin_unlock(&data->lock, key);
	return 0;
}

static void nxp_cmu_fm_isr(const struct device *dev)
{
	const struct nxp_cmu_fm_config *config = dev->config;
	struct nxp_cmu_fm_data *data = dev->data;
	uint32_t flags = CMU_FM_GetStatusFlags(config->base);
	uint32_t evts = 0U;
	struct clock_monitor_event_data cb_data = {0};
	clock_monitor_callback_t cb = NULL;
	void *user_data = NULL;
	bool wake_measure = false;

	CMU_FM_ClearStatusFlags(config->base, flags);

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if ((flags & (uint32_t)kCMU_FM_MeterComplete) != 0U) {
		uint32_t met_cnt = CMU_FM_GetMeteredClkCnt(config->base);

		data->measure_result_hz = CMU_FM_CalcMeteredClkFreq(
			met_cnt, data->ref_cnt, data->ref_hz);
		data->measure_status = 0;
		evts |= CLOCK_MONITOR_EVT_MEASURE_DONE;
		cb_data.measured_hz = data->measure_result_hz;
		wake_measure = true;
		data->state = NXP_CMU_FM_STATE_CONFIGURED;
	}

	if ((flags & (uint32_t)kCMU_FM_MeterTimeout) != 0U) {
		data->measure_status = -EIO;
		evts |= CLOCK_MONITOR_EVT_CLOCK_LOST;
		data->state = NXP_CMU_FM_STATE_CONFIGURED;
		wake_measure = true;
	}

	data->latched_events |= evts;
	cb_data.events = evts & data->cfg.meter.event_mask;
	cb = data->cfg.callback;
	user_data = data->cfg.user_data;

	k_spin_unlock(&data->lock, key);

	if (wake_measure) {
		k_sem_give(&data->measure_sem);
	}

	if (cb != NULL && cb_data.events != 0U) {
		cb(dev, &cb_data, user_data);
	}
}

static int nxp_cmu_fm_init(const struct device *dev)
{
	const struct nxp_cmu_fm_config *config = dev->config;
	struct nxp_cmu_fm_data *data = dev->data;

	k_sem_init(&data->measure_sem, 0, 1);
	data->state = NXP_CMU_FM_STATE_IDLE;

	if (config->irq_config_func != NULL) {
		config->irq_config_func(dev);
	} else {
		LOG_INF("%s: instantiated without NVIC IRQ (polling-only)",
			dev->name);
	}
	return 0;
}

static DEVICE_API(clock_monitor, nxp_cmu_fm_api) = {
	.configure  = nxp_cmu_fm_configure,
	.check      = nxp_cmu_fm_check,
	.stop       = nxp_cmu_fm_stop,
	.measure    = nxp_cmu_fm_measure,
	.get_events = nxp_cmu_fm_get_events,
};

/* ---------- Instantiation ---------- */

#define NXP_CMU_FM_IRQ_WIRE(inst)                                              \
	static void nxp_cmu_fm_irq_cfg_##inst(const struct device *dev)        \
	{                                                                      \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),   \
			    nxp_cmu_fm_isr, DEVICE_DT_INST_GET(inst), 0);      \
		irq_enable(DT_INST_IRQN(inst));                                \
	}

#define NXP_CMU_FM_IRQ_PTR(inst)                                               \
	COND_CODE_1(DT_INST_IRQ_HAS_IDX(inst, 0),                              \
		    (nxp_cmu_fm_irq_cfg_##inst), (NULL))

#define NXP_CMU_FM_DEVICE_INIT(inst)                                           \
	COND_CODE_1(DT_INST_IRQ_HAS_IDX(inst, 0),                              \
		    (NXP_CMU_FM_IRQ_WIRE(inst)), ())                           \
	static const struct clock_monitor_capabilities                         \
		nxp_cmu_fm_caps_##inst = {                                     \
		.supported_modes  = BIT(CLOCK_MONITOR_MODE_METER),             \
		.supported_events = CLOCK_MONITOR_EVT_MEASURE_DONE |           \
				    CLOCK_MONITOR_EVT_CLOCK_LOST,              \
	};                                                                     \
	static struct nxp_cmu_fm_data nxp_cmu_fm_data_##inst;                  \
	static const struct nxp_cmu_fm_config nxp_cmu_fm_cfg_##inst = {        \
		.common = {.caps = &nxp_cmu_fm_caps_##inst},                   \
		.base = (CMU_FM_Type *)DT_INST_REG_ADDR(inst),                 \
		.ref_clk_dev = DEVICE_DT_GET(                                  \
			DT_INST_CLOCKS_CTLR_BY_NAME(inst, reference)),         \
		.ref_clk_subsys = (clock_control_subsys_t)(uintptr_t)          \
			DT_INST_CLOCKS_CELL_BY_NAME(inst, reference, name),    \
		.mon_clk_dev = DEVICE_DT_GET(                                  \
			DT_INST_CLOCKS_CTLR_BY_NAME(inst, monitored)),         \
		.mon_clk_subsys = (clock_control_subsys_t)(uintptr_t)          \
			DT_INST_CLOCKS_CELL_BY_NAME(inst, monitored, name),    \
		.irq_config_func = NXP_CMU_FM_IRQ_PTR(inst),                   \
	};                                                                     \
	DEVICE_DT_INST_DEFINE(inst, nxp_cmu_fm_init, NULL,                     \
			      &nxp_cmu_fm_data_##inst,                         \
			      &nxp_cmu_fm_cfg_##inst,                          \
			      POST_KERNEL,                                     \
			      CONFIG_CLOCK_MONITOR_INIT_PRIORITY,              \
			      &nxp_cmu_fm_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_CMU_FM_DEVICE_INIT)
