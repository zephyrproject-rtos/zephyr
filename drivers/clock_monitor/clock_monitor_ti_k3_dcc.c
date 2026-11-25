/*
 * Copyright (c) 2026 BeagleBoard.org Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr clock_monitor back-end for the TI K3 Dual Clock Comparator (DCC).
 *
 * MEASURE: one-shot. Counter0 runs for window_ns of the reference clock while
 * Counter1 (seeded to max) counts the target. DONE stops the module; the
 * measured rate is (seed1 - CNT1) * ref_hz / seed0.
 *
 * WINDOW: continuous ratio check. Seeds are derived from expected_hz and
 * tolerance_ppm. ERRFLG is polled (ERR typically feeds an ESM, not the CPU
 * VIM). CNT1/CNT0 at the error distinguish FREQ_HIGH vs FREQ_LOW.
 *
 * Reference / target mux values come from DT source lists; set_source()
 * switches them at runtime and drops the device back to IDLE.
 */

#define DT_DRV_COMPAT ti_k3_dcc

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_monitor.h>
#include <zephyr/dt-bindings/clock-monitor/ti-k3-dcc.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include "clock_monitor_common.h"

LOG_MODULE_REGISTER(clock_monitor_ti_k3_dcc, CONFIG_CLOCK_MONITOR_LOG_LEVEL);

#define DCC_GCTRL           0x00U
#define DCC_CNTSEED0        0x08U
#define DCC_VALIDSEED0      0x0cU
#define DCC_CNTSEED1        0x10U
#define DCC_STATUS          0x14U
#define DCC_CNT0            0x18U
#define DCC_CNT1            0x20U
#define DCC_CLKSRC1         0x24U
#define DCC_CLKSRC0         0x28U
#define DCC_GCTRL2          0x2cU

#define DCC_KEY_DISABLE     0x5U
#define DCC_KEY_ENABLE      0xaU

#define DCC_GCTRL_DCCENA_POS      0U
#define DCC_GCTRL_ERRENA_POS      4U
#define DCC_GCTRL_SINGLESHOT_POS  8U
#define DCC_GCTRL_DONEENA_POS     12U

#define DCC_SINGLESHOT_CNT0_VALID 0xaU /* stop when CNT0 and VALID0 expire */
#define DCC_SINGLESHOT_CONTINUOUS 0x5U

#define DCC_STATUS_ERRFLG   BIT(0)
#define DCC_STATUS_DONEFLG  BIT(1)

#define DCC_CLKSRC_KEY_POS  12U
#define DCC_CLKSRC0_MASK    0xfU
#define DCC_CLKSRC1_MASK    0x1fU

#define DCC_GCTRL2_CONT_ON_ERR_POS 0U

#define DCC_CNT_MASK        0xfffffU
#define DCC_VALID_MASK      0xffffU
#define DCC_VALID_MIN       4U
#define DCC_CNT1_SEED_MAX   DCC_CNT_MASK

struct k3_dcc_source {
	uint32_t cookie;
	const struct device *clk_dev;
	clock_control_subsys_t clk_subsys;
	uint32_t fixed_hz;
	bool has_fixed_hz;
	bool is_default;
};

struct k3_dcc_config {
	mem_addr_t base;
	const struct k3_dcc_source *ref_srcs;
	uint8_t ref_srcs_count;
	const struct k3_dcc_source *tar_srcs;
	uint8_t tar_srcs_count;
	void (*irq_config_func)(const struct device *dev);
};

enum k3_dcc_state {
	K3_DCC_STATE_IDLE = 0,
	K3_DCC_STATE_CONFIGURING,
	K3_DCC_STATE_CONFIGURED,
	K3_DCC_STATE_RUNNING,
};

struct k3_dcc_data {
	struct k_spinlock lock;
	enum k3_dcc_state state;
	struct clock_monitor_config cfg;
	uint8_t cur_ref_idx;
	uint8_t cur_tar_idx;
	uint32_t ref_hz;
	uint32_t tar_hz;     /* 0 if unknown; WINDOW may use cfg.window.expected_hz */
	uint32_t seed0;
	uint32_t valid0;
	uint32_t seed1;
	uint32_t last_rate_hz;
	bool has_rate;
	bool clock_lost;
	struct k_work_delayable poll_work;
	const struct device *dev;
};

static inline uint32_t dcc_read(const struct k3_dcc_config *cfg, uint32_t off)
{
	return sys_read32(cfg->base + off);
}

static inline void dcc_write(const struct k3_dcc_config *cfg, uint32_t off, uint32_t val)
{
	sys_write32(val, cfg->base + off);
}

static inline uint32_t dcc_field(uint32_t key, uint32_t pos)
{
	return (key & 0xfU) << pos;
}

static void dcc_stop_hw(const struct k3_dcc_config *cfg)
{
	dcc_write(cfg, DCC_GCTRL,
		  dcc_field(DCC_KEY_DISABLE, DCC_GCTRL_DCCENA_POS) |
		  dcc_field(DCC_KEY_DISABLE, DCC_GCTRL_ERRENA_POS) |
		  dcc_field(DCC_SINGLESHOT_CONTINUOUS, DCC_GCTRL_SINGLESHOT_POS) |
		  dcc_field(DCC_KEY_DISABLE, DCC_GCTRL_DONEENA_POS));
	dcc_write(cfg, DCC_STATUS, DCC_STATUS_ERRFLG | DCC_STATUS_DONEFLG);
}

static void dcc_apply_sources(const struct k3_dcc_config *cfg,
			      const struct k3_dcc_source *ref,
			      const struct k3_dcc_source *tar)
{
	dcc_write(cfg, DCC_CLKSRC0,
		  ((ref->cookie & DCC_CLKSRC0_MASK)) |
		  (DCC_KEY_ENABLE << DCC_CLKSRC_KEY_POS));
	dcc_write(cfg, DCC_CLKSRC1,
		  ((tar->cookie & DCC_CLKSRC1_MASK)) |
		  (DCC_KEY_ENABLE << DCC_CLKSRC_KEY_POS));
}

static int k3_dcc_src_rate(const struct k3_dcc_source *src, uint32_t *hz)
{
	uint32_t rate = 0U;

	if (src->has_fixed_hz) {
		rate = src->fixed_hz;
	} else {
		if (src->clk_dev == NULL || !device_is_ready(src->clk_dev)) {
			return -EIO;
		}
		if (clock_control_get_rate(src->clk_dev, src->clk_subsys, &rate) != 0) {
			return -EIO;
		}
	}

	if (rate == 0U) {
		return -EIO;
	}

	*hz = rate;
	return 0;
}

static int k3_dcc_find_src(const struct k3_dcc_source *srcs, uint8_t count,
			   uint32_t cookie, uint8_t *out_idx)
{
	for (uint8_t i = 0U; i < count; i++) {
		if (srcs[i].cookie == cookie) {
			*out_idx = i;
			return 0;
		}
	}
	return -EINVAL;
}

static uint8_t k3_dcc_default_idx(const struct k3_dcc_source *srcs, uint8_t count)
{
	for (uint8_t i = 0U; i < count; i++) {
		if (srcs[i].is_default) {
			return i;
		}
	}
	return 0U;
}

static void k3_dcc_deliver(struct k3_dcc_data *data, uint32_t events, uint32_t hz)
{
	clock_monitor_callback_t cb;
	void *user_data;
	struct clock_monitor_event_data evt = {
		.events = events,
		.measured_hz = hz,
	};

	cb = data->cfg.callback;
	user_data = data->cfg.user_data;
	if (cb != NULL) {
		cb(data->dev, &evt, user_data);
	}
}

static void k3_dcc_finish_measure(const struct device *dev)
{
	const struct k3_dcc_config *config = dev->config;
	struct k3_dcc_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t cnt1;
	uint32_t counted;
	uint32_t hz = 0U;
	uint32_t events;
	bool lost = false;

	cnt1 = dcc_read(config, DCC_CNT1) & DCC_CNT_MASK;
	dcc_stop_hw(config);

	if (cnt1 > data->seed1) {
		/* Unexpected wrap / unread seed — treat as lost. */
		lost = true;
	} else {
		counted = data->seed1 - cnt1;
		if (counted == 0U || data->seed0 == 0U) {
			lost = true;
		} else {
			hz = (uint32_t)(((uint64_t)counted * (uint64_t)data->ref_hz) /
					(uint64_t)data->seed0);
		}
	}

	key = k_spin_lock(&data->lock);
	if (data->state != K3_DCC_STATE_RUNNING ||
	    data->cfg.mode != CLOCK_MONITOR_MODE_MEASURE) {
		k_spin_unlock(&data->lock, key);
		return;
	}

	if (lost) {
		data->clock_lost = true;
		data->has_rate = false;
		events = CLOCK_MONITOR_EVT_CLOCK_LOST;
		hz = 0U;
	} else {
		data->last_rate_hz = hz;
		data->has_rate = true;
		data->clock_lost = false;
		events = CLOCK_MONITOR_EVT_MEASURE_DONE;
	}
	data->state = K3_DCC_STATE_CONFIGURED;
	k_spin_unlock(&data->lock, key);

	k3_dcc_deliver(data, events, hz);
}

static void k3_dcc_handle_window_err(const struct device *dev)
{
	const struct k3_dcc_config *config = dev->config;
	struct k3_dcc_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t cnt0;
	uint32_t cnt1;
	uint32_t events;

	cnt0 = dcc_read(config, DCC_CNT0) & DCC_CNT_MASK;
	cnt1 = dcc_read(config, DCC_CNT1) & DCC_CNT_MASK;

	/* CNT1 hit 0 first → target too fast; CNT0 done with CNT1 left → too slow. */
	if (cnt1 == 0U && cnt0 != 0U) {
		events = CLOCK_MONITOR_EVT_FREQ_HIGH;
	} else {
		events = CLOCK_MONITOR_EVT_FREQ_LOW;
	}

	dcc_write(config, DCC_STATUS, DCC_STATUS_ERRFLG | DCC_STATUS_DONEFLG);

	key = k_spin_lock(&data->lock);
	if (data->state != K3_DCC_STATE_RUNNING ||
	    data->cfg.mode != CLOCK_MONITOR_MODE_WINDOW) {
		k_spin_unlock(&data->lock, key);
		return;
	}
	k_spin_unlock(&data->lock, key);

	k3_dcc_deliver(data, events, 0U);
}

static void k3_dcc_isr(const struct device *dev)
{
	const struct k3_dcc_config *config = dev->config;
	struct k3_dcc_data *data = dev->data;
	uint32_t status = dcc_read(config, DCC_STATUS);
	enum clock_monitor_mode mode;

	mode = data->cfg.mode;

	/*
	 * MEASURE with seed1 = MAX intentionally never lets CNT1 expire inside
	 * the reference window, so the module latches ERRFLG (ratio mismatch)
	 * rather than DONEFLG. Either flag ends the one-shot capture.
	 */
	if (mode == CLOCK_MONITOR_MODE_MEASURE &&
	    (status & (DCC_STATUS_DONEFLG | DCC_STATUS_ERRFLG)) != 0U) {
		k3_dcc_finish_measure(dev);
		return;
	}

	if ((status & DCC_STATUS_DONEFLG) != 0U) {
		k3_dcc_finish_measure(dev);
		return;
	}

	if ((status & DCC_STATUS_ERRFLG) != 0U) {
		k3_dcc_handle_window_err(dev);
	}
}

static void k3_dcc_poll_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct k3_dcc_data *data = CONTAINER_OF(dwork, struct k3_dcc_data, poll_work);
	const struct device *dev = data->dev;
	const struct k3_dcc_config *config = dev->config;
	k_spinlock_key_t key;
	uint32_t status;
	enum clock_monitor_mode mode;
	bool running;

	key = k_spin_lock(&data->lock);
	running = (data->state == K3_DCC_STATE_RUNNING);
	mode = data->cfg.mode;
	k_spin_unlock(&data->lock, key);

	if (!running) {
		return;
	}

	status = dcc_read(config, DCC_STATUS);

	/*
	 * MEASURE with seed1 = MAX ends on ERRFLG (ratio mismatch); DONEFLG is
	 * only set when CNT1 expires inside the reference window. ERR is wired
	 * to the ESM on K3, not the CPU VIM, so MEASURE must poll. Busy-wait
	 * inside this one-shot work item so tick resolution cannot miss the
	 * ~window_ns completion.
	 */
	if (mode == CLOCK_MONITOR_MODE_MEASURE) {
		for (uint32_t i = 0U; i < 20000U; i++) {
			status = dcc_read(config, DCC_STATUS);
			if ((status & (DCC_STATUS_DONEFLG | DCC_STATUS_ERRFLG)) != 0U ||
			    (dcc_read(config, DCC_CNT0) & DCC_CNT_MASK) == 0U) {
				k3_dcc_finish_measure(dev);
				return;
			}
			k_busy_wait(10);
		}
		return;
	}

	if ((status & DCC_STATUS_ERRFLG) != 0U) {
		k3_dcc_handle_window_err(dev);
	}

	key = k_spin_lock(&data->lock);
	running = (data->state == K3_DCC_STATE_RUNNING &&
		   data->cfg.mode == CLOCK_MONITOR_MODE_WINDOW);
	k_spin_unlock(&data->lock, key);

	if (running) {
		(void)k_work_reschedule(&data->poll_work, K_MSEC(1));
	}
}

static int k3_dcc_program_seeds(const struct device *dev)
{
	const struct k3_dcc_config *config = dev->config;
	struct k3_dcc_data *data = dev->data;

	dcc_stop_hw(config);
	dcc_apply_sources(config, &config->ref_srcs[data->cur_ref_idx],
			  &config->tar_srcs[data->cur_tar_idx]);
	dcc_write(config, DCC_CNTSEED0, data->seed0 & DCC_CNT_MASK);
	dcc_write(config, DCC_VALIDSEED0, data->valid0 & DCC_VALID_MASK);
	dcc_write(config, DCC_CNTSEED1, data->seed1 & DCC_CNT_MASK);
	return 0;
}

static int k3_dcc_derive_measure(struct k3_dcc_data *data, uint32_t window_ns)
{
	uint32_t seed0;
	int ret;

	ret = clock_monitor_compute_ref_cnt(window_ns, data->ref_hz, DCC_CNT_MASK, &seed0);
	if (ret != 0) {
		return ret;
	}

	data->seed0 = seed0;
	data->valid0 = DCC_VALID_MIN;
	data->seed1 = DCC_CNT1_SEED_MAX;
	return 0;
}

static int k3_dcc_derive_window(struct k3_dcc_data *data, const struct clock_monitor_window_cfg *w)
{
	uint32_t seed0;
	uint32_t expected;
	uint64_t seed1;
	uint64_t valid;
	int ret;

	if (w->window_ns == 0U) {
		return -EINVAL;
	}

	expected = w->expected_hz != 0U ? w->expected_hz : data->tar_hz;
	if (expected == 0U) {
		return -EINVAL;
	}

	ret = clock_monitor_compute_ref_cnt(w->window_ns, data->ref_hz, DCC_CNT_MASK, &seed0);
	if (ret != 0) {
		return ret;
	}

	seed1 = ((uint64_t)seed0 * (uint64_t)expected) / (uint64_t)data->ref_hz;
	if (seed1 == 0U || seed1 > DCC_CNT_MASK) {
		return -ERANGE;
	}

	valid = ((uint64_t)seed0 * (uint64_t)w->tolerance_ppm) / 1000000ULL;
	if (valid < DCC_VALID_MIN) {
		valid = DCC_VALID_MIN;
	}
	if (valid > DCC_VALID_MASK) {
		valid = DCC_VALID_MASK;
	}

	data->seed0 = seed0;
	data->valid0 = (uint32_t)valid;
	data->seed1 = (uint32_t)seed1;
	return 0;
}

static int k3_dcc_configure(const struct device *dev, const struct clock_monitor_config *cfg)
{
	const struct k3_dcc_config *config = dev->config;
	struct k3_dcc_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t ref_hz = 0U;
	uint32_t tar_hz = 0U;
	uint8_t ref_idx;
	uint8_t tar_idx;
	int ret;

	if (cfg == NULL) {
		return -EINVAL;
	}
	if (cfg->mode != CLOCK_MONITOR_MODE_MEASURE &&
	    cfg->mode != CLOCK_MONITOR_MODE_WINDOW) {
		return -ENOTSUP;
	}
	if (cfg->mode == CLOCK_MONITOR_MODE_MEASURE && cfg->measure.window_ns == 0U) {
		return -EINVAL;
	}
	if (cfg->mode == CLOCK_MONITOR_MODE_WINDOW && cfg->window.window_ns == 0U) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	if (data->state == K3_DCC_STATE_RUNNING ||
	    data->state == K3_DCC_STATE_CONFIGURING) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}
	ref_idx = data->cur_ref_idx;
	tar_idx = data->cur_tar_idx;
	data->state = K3_DCC_STATE_CONFIGURING;
	k_spin_unlock(&data->lock, key);

	(void)k_work_cancel_delayable(&data->poll_work);
	dcc_stop_hw(config);

	ret = k3_dcc_src_rate(&config->ref_srcs[ref_idx], &ref_hz);
	if (ret != 0) {
		goto out_idle;
	}
	/* Target rate is optional for MEASURE; required for WINDOW auto-derive. */
	(void)k3_dcc_src_rate(&config->tar_srcs[tar_idx], &tar_hz);

	key = k_spin_lock(&data->lock);
	data->ref_hz = ref_hz;
	data->tar_hz = tar_hz;
	data->cfg = *cfg;
	data->has_rate = false;
	data->clock_lost = false;
	data->last_rate_hz = 0U;

	if (cfg->mode == CLOCK_MONITOR_MODE_MEASURE) {
		ret = k3_dcc_derive_measure(data, cfg->measure.window_ns);
	} else {
		ret = k3_dcc_derive_window(data, &cfg->window);
	}
	if (ret != 0) {
		data->state = K3_DCC_STATE_IDLE;
		k_spin_unlock(&data->lock, key);
		return ret;
	}

	k_spin_unlock(&data->lock, key);

	ret = k3_dcc_program_seeds(dev);
	if (ret != 0) {
		goto out_idle;
	}

	key = k_spin_lock(&data->lock);
	data->state = K3_DCC_STATE_CONFIGURED;
	k_spin_unlock(&data->lock, key);
	return 0;

out_idle:
	key = k_spin_lock(&data->lock);
	data->state = K3_DCC_STATE_IDLE;
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int k3_dcc_start(const struct device *dev)
{
	const struct k3_dcc_config *config = dev->config;
	struct k3_dcc_data *data = dev->data;
	k_spinlock_key_t key;
	enum clock_monitor_mode mode;
	uint32_t gctrl;
	uint32_t gctrl2;

	key = k_spin_lock(&data->lock);
	if (data->state == K3_DCC_STATE_IDLE) {
		k_spin_unlock(&data->lock, key);
		return -EINVAL;
	}
	if (data->state == K3_DCC_STATE_RUNNING ||
	    data->state == K3_DCC_STATE_CONFIGURING) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	/* Re-arm seeds each start (MEASURE reloads; WINDOW restarts cleanly). */
	k_spin_unlock(&data->lock, key);
	(void)k3_dcc_program_seeds(dev);

	if (data->cfg.mode == CLOCK_MONITOR_MODE_MEASURE) {
		/* ERRENA on: MEASURE with max seed1 completes via ERRFLG. */
		gctrl = dcc_field(DCC_KEY_ENABLE, DCC_GCTRL_DCCENA_POS) |
			dcc_field(DCC_KEY_ENABLE, DCC_GCTRL_ERRENA_POS) |
			dcc_field(DCC_SINGLESHOT_CNT0_VALID, DCC_GCTRL_SINGLESHOT_POS) |
			dcc_field(DCC_KEY_ENABLE, DCC_GCTRL_DONEENA_POS);
		gctrl2 = dcc_field(DCC_KEY_DISABLE, DCC_GCTRL2_CONT_ON_ERR_POS);
	} else {
		gctrl = dcc_field(DCC_KEY_ENABLE, DCC_GCTRL_DCCENA_POS) |
			dcc_field(DCC_KEY_ENABLE, DCC_GCTRL_ERRENA_POS) |
			dcc_field(DCC_SINGLESHOT_CONTINUOUS, DCC_GCTRL_SINGLESHOT_POS) |
			dcc_field(DCC_KEY_DISABLE, DCC_GCTRL_DONEENA_POS);
		gctrl2 = dcc_field(DCC_KEY_ENABLE, DCC_GCTRL2_CONT_ON_ERR_POS);
	}

	dcc_write(config, DCC_GCTRL2, gctrl2);
	dcc_write(config, DCC_STATUS, DCC_STATUS_ERRFLG | DCC_STATUS_DONEFLG);
	dcc_write(config, DCC_GCTRL, gctrl);

	key = k_spin_lock(&data->lock);
	data->state = K3_DCC_STATE_RUNNING;
	mode = data->cfg.mode;
	k_spin_unlock(&data->lock, key);

	/* MEASURE: one-shot work busy-waits for ERR/DONE. WINDOW: poll ERRFLG. */
	(void)k_work_reschedule(&data->poll_work,
				(mode == CLOCK_MONITOR_MODE_MEASURE) ? K_NO_WAIT
								     : K_MSEC(1));

	return 0;
}

static int k3_dcc_stop(const struct device *dev)
{
	const struct k3_dcc_config *config = dev->config;
	struct k3_dcc_data *data = dev->data;
	k_spinlock_key_t key;

	(void)k_work_cancel_delayable(&data->poll_work);
	dcc_stop_hw(config);

	key = k_spin_lock(&data->lock);
	if (data->state == K3_DCC_STATE_RUNNING) {
		data->state = K3_DCC_STATE_CONFIGURED;
	}
	k_spin_unlock(&data->lock, key);
	return 0;
}

static int k3_dcc_get_rate(const struct device *dev, uint32_t *rate_hz)
{
	struct k3_dcc_data *data = dev->data;
	k_spinlock_key_t key;
	int ret;

	if (rate_hz == NULL) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	if (data->state == K3_DCC_STATE_RUNNING &&
	    data->cfg.mode == CLOCK_MONITOR_MODE_MEASURE) {
		ret = -EAGAIN;
	} else if (data->clock_lost) {
		ret = -EIO;
	} else if (!data->has_rate) {
		ret = -EAGAIN;
	} else {
		*rate_hz = data->last_rate_hz;
		ret = 0;
	}
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int k3_dcc_set_source(const struct device *dev, uint32_t reference, uint32_t target)
{
	const struct k3_dcc_config *config = dev->config;
	struct k3_dcc_data *data = dev->data;
	k_spinlock_key_t key;
	uint8_t ref_idx;
	uint8_t tar_idx;
	int ret;

	key = k_spin_lock(&data->lock);
	if (data->state == K3_DCC_STATE_RUNNING ||
	    data->state == K3_DCC_STATE_CONFIGURING) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}
	ref_idx = data->cur_ref_idx;
	tar_idx = data->cur_tar_idx;
	data->state = K3_DCC_STATE_CONFIGURING;
	k_spin_unlock(&data->lock, key);

	if (reference != K3_DCC_SOURCE_UNCHANGED) {
		ret = k3_dcc_find_src(config->ref_srcs, config->ref_srcs_count,
				      reference, &ref_idx);
		if (ret != 0) {
			goto out_restore;
		}
	}
	if (target != K3_DCC_SOURCE_UNCHANGED) {
		ret = k3_dcc_find_src(config->tar_srcs, config->tar_srcs_count,
				      target, &tar_idx);
		if (ret != 0) {
			goto out_restore;
		}
	}

	dcc_stop_hw(config);
	dcc_apply_sources(config, &config->ref_srcs[ref_idx], &config->tar_srcs[tar_idx]);

	key = k_spin_lock(&data->lock);
	data->cur_ref_idx = ref_idx;
	data->cur_tar_idx = tar_idx;
	data->state = K3_DCC_STATE_IDLE;
	data->has_rate = false;
	data->clock_lost = false;
	k_spin_unlock(&data->lock, key);
	return 0;

out_restore:
	key = k_spin_lock(&data->lock);
	data->state = K3_DCC_STATE_IDLE;
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int k3_dcc_init(const struct device *dev)
{
	const struct k3_dcc_config *config = dev->config;
	struct k3_dcc_data *data = dev->data;

	data->dev = dev;
	data->cur_ref_idx = k3_dcc_default_idx(config->ref_srcs, config->ref_srcs_count);
	data->cur_tar_idx = k3_dcc_default_idx(config->tar_srcs, config->tar_srcs_count);
	k_work_init_delayable(&data->poll_work, k3_dcc_poll_work_handler);

	dcc_stop_hw(config);
	dcc_apply_sources(config, &config->ref_srcs[data->cur_ref_idx],
			  &config->tar_srcs[data->cur_tar_idx]);

	config->irq_config_func(dev);
	return 0;
}

static DEVICE_API(clock_monitor, k3_dcc_api) = {
	.configure = k3_dcc_configure,
	.start = k3_dcc_start,
	.stop = k3_dcc_stop,
	.get_rate = k3_dcc_get_rate,
	.set_source = k3_dcc_set_source,
};

#define K3_DCC_SOURCE_ELEM(node_id)                                                            \
	{                                                                                      \
		.cookie = DT_PROP(node_id, ti_dcc_clk_src),                                    \
		.clk_dev = COND_CODE_1(DT_NODE_HAS_PROP(node_id, clocks),                      \
			(DEVICE_DT_GET(DT_CLOCKS_CTLR(node_id))), (NULL)),                     \
		.clk_subsys = COND_CODE_1(DT_NODE_HAS_PROP(node_id, clocks),                   \
			((clock_control_subsys_t)(uintptr_t)DT_CLOCKS_CELL(node_id, name)),    \
			(NULL)),                                                               \
		.fixed_hz = DT_PROP_OR(node_id, clock_frequency, 0),                           \
		.has_fixed_hz = DT_NODE_HAS_PROP(node_id, clock_frequency),                    \
		.is_default = DT_PROP(node_id, default_source),                                \
	},

#define K3_DCC_SOURCES(inst, container)                                                        \
	static const struct k3_dcc_source k3_dcc_##inst##_##container[] = {                    \
		DT_FOREACH_CHILD(DT_INST_CHILD(inst, container), K3_DCC_SOURCE_ELEM)           \
	}

#define K3_DCC_IRQ_CONNECT(inst)                                                               \
	static void k3_dcc_##inst##_irq_config(const struct device *dev)                       \
	{                                                                                      \
		ARG_UNUSED(dev);                                                               \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), k3_dcc_isr,       \
			    DEVICE_DT_INST_GET(inst),                                          \
			    COND_CODE_1(DT_INST_IRQ_HAS_CELL(inst, flags),                     \
					(DT_INST_IRQ(inst, flags)), (0)));                     \
		irq_enable(DT_INST_IRQN(inst));                                                \
	}

#define K3_DCC_INIT(inst)                                                                      \
	K3_DCC_SOURCES(inst, reference_sources);                                               \
	K3_DCC_SOURCES(inst, target_sources);                                                  \
	K3_DCC_IRQ_CONNECT(inst);                                                              \
	static struct k3_dcc_data k3_dcc_data_##inst;                                           \
	static const struct k3_dcc_config k3_dcc_config_##inst = {                              \
		.base = DT_INST_REG_ADDR(inst),                                                \
		.ref_srcs = k3_dcc_##inst##_reference_sources,                                 \
		.ref_srcs_count = ARRAY_SIZE(k3_dcc_##inst##_reference_sources),               \
		.tar_srcs = k3_dcc_##inst##_target_sources,                                    \
		.tar_srcs_count = ARRAY_SIZE(k3_dcc_##inst##_target_sources),                  \
		.irq_config_func = k3_dcc_##inst##_irq_config,                                 \
	};                                                                                     \
	DEVICE_DT_INST_DEFINE(inst, k3_dcc_init, NULL, &k3_dcc_data_##inst,                    \
			      &k3_dcc_config_##inst, POST_KERNEL,                              \
			      CONFIG_CLOCK_MONITOR_INIT_PRIORITY, &k3_dcc_api);

DT_INST_FOREACH_STATUS_OKAY(K3_DCC_INIT)
