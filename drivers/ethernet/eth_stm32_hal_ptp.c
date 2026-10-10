/*
 * Copyright (c) 2017 Erwin Rol <erwin@erwinrol.com>
 * Copyright (c) 2020 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
 * Copyright (c) 2021 Carbon Robotics
 * Copyright (c) 2025 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/precision_clock_output.h>
#include <zephyr/drivers/ptp_clock.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/sys/math_extras.h>
#include <soc.h>

#include <stdbool.h>

#include "eth_stm32_hal_priv.h"
#include "eth_stm32_hal_ptp_output.h"

#define DT_DRV_COMPAT snps_dwmac_ptp_clock

LOG_MODULE_REGISTER(eth_stm32_hal_ptp, CONFIG_ETHERNET_LOG_LEVEL);

#if defined(CONFIG_PTP_CLOCK_STM32_HAL_OUTPUT) && defined(ETH_MACHWF2R_PPSOUTNUM_Msk) &&           \
	defined(ETH_MACHWF2R_PPSOUTNUM_Pos) && defined(ETH_MACPPSCR_PPSCTRL_Msk) &&                \
	defined(ETH_MACPPSCR_PPSEN0_Msk) && defined(ETH_MACPPSCR_TRGTMODSEL0_Msk) &&               \
	defined(ETH_MACPPSTTNR_TRGTBUSY0_Msk)
#define ETH_STM32_HAS_FLEX_PPS 1
#else
#define ETH_STM32_HAS_FLEX_PPS 0
#endif

#define ETH_STM32_PPS_OUTPUT_CHANNELS_IMPLEMENTED 1U
#define ETH_STM32_PPS_MIN_LEAD_TIME_NS            1000000ULL
#define ETH_STM32_PPS_BUSY_TIMEOUT_US             1000U
#define ETH_STM32_PPS_SINGLE_PULSE                1U
#define ETH_STM32_PPS_START_PULSE_TRAIN           2U
#define ETH_STM32_PPS_CANCEL_START                3U
#define ETH_STM32_PPS_STOP_PULSE_TRAIN_IMMEDIATE  5U
#define ETH_STM32_PPS_TARGET_MODE_OUTPUT_ONLY     3U

/* ES0565 section 2.22.9 affects H5 pulse trains in fine timestamp mode. */
#define ETH_STM32_PPS_REARM_SINGLE_PULSES IS_ENABLED(CONFIG_PTP_CLOCK_STM32_HAL_OUTPUT_REARM)

#define ETH_STM32_PPS_REARM_GUARD_NS     1000000ULL
#define ETH_STM32_PPS_REARM_MAX_DELAY_NS NSEC_PER_SEC
#define ETH_STM32_PPS_REARM_PERIOD_NS    NSEC_PER_SEC
#define ETH_STM32_PPS_REARM_MAX_WIDTH_NS (NSEC_PER_SEC / 2U)
#define ETH_STM32_PPS_QUIESCE_POLL_MS    1U
#define ETH_STM32_PPS_RATE_SCALE_PCT     100U

#if ETH_STM32_HAS_FLEX_PPS
/*
 * Scheduled-output lifecycle.
 *
 * Transitions are serialized by output_lifecycle_lock, which may be held across
 * sleeping cleanup. The fast output_lock guards only the lifecycle state, the
 * effective configuration, the stored fault error, and the rearm bookkeeping
 * so status queries stay prompt; it is never held across a synchronous work
 * cancel, quiescence polling, or any sleep.
 *
 * Lock order: output_lifecycle_lock is always taken before output_lock. The
 * rearm worker takes only output_lock, so it can never block a stop that waits
 * for it with k_work_cancel_delayable_sync().
 */
enum ptp_clock_stm32_output_state {
	/* Channel unconfigured and hardware idle. */
	PTP_CLOCK_STM32_OUTPUT_IDLE = 0,
	/* Channel configured and pulse generation armed. */
	PTP_CLOCK_STM32_OUTPUT_RUNNING,
	/* Stop in progress; sleeping cleanup runs without holding output_lock. */
	PTP_CLOCK_STM32_OUTPUT_STOPPING,
	/* Rearm or cleanup failed; output_fault_error is surfaced to callers. */
	PTP_CLOCK_STM32_OUTPUT_FAULTED,
};

struct ptp_clock_stm32_data {
	struct k_mutex output_lifecycle_lock;
	struct k_mutex output_lock;
	enum ptp_clock_stm32_output_state output_state;
	int output_fault_error;
	struct precision_clock_output_raw_waveform_config output_config;
	uint32_t output_channel_count;
#if ETH_STM32_PPS_REARM_SINGLE_PULSES
	struct k_work_q output_rearm_queue;
	struct k_work_delayable output_rearm_work;
	struct k_work_sync output_rearm_sync;

	K_KERNEL_STACK_MEMBER(output_rearm_stack,
			      CONFIG_PTP_CLOCK_STM32_HAL_OUTPUT_REARM_STACK_SIZE);
	const struct device *dev;
	precision_time_t output_effective_width_ns;
	struct eth_stm32_ptp_output_rearm_timing output_rearm_timing;
#endif
};
#endif

/* Naming of the  ETH PTP Config Status changes depending on the stm32 series */
#if defined(CONFIG_SOC_SERIES_STM32F4X)
#define ETH_STM32_PTP_CONFIGURED     HAL_ETH_PTP_CONFIGURATED
#define ETH_STM32_PTP_NOT_CONFIGURED HAL_ETH_PTP_NOT_CONFIGURATED
#else
#define ETH_STM32_PTP_CONFIGURED HAL_ETH_PTP_CONFIGURED
#define ETH_STM32_PTP_NOT_CONFIGURED HAL_ETH_PTP_NOT_CONFIGURED
#endif /* stm32F7x or sm32F4x */

void HAL_ETH_TxPtpCallback(uint32_t *buff, ETH_TimeStampTypeDef *timestamp)
{
	struct eth_stm32_tx_context *ctx = (struct eth_stm32_tx_context *)buff;

	ctx->pkt->timestamp.second = timestamp->TimeStampHigh;
	ctx->pkt->timestamp.nanosecond = timestamp->TimeStampLow;

	net_if_add_tx_timestamp(ctx->pkt);
}

const struct device *eth_stm32_get_ptp_clock(const struct device *dev,
					     struct net_if *iface __unused)
{
	struct eth_stm32_hal_dev_data *dev_data = dev->data;

	return dev_data->ptp_clock;
}

static int ptp_clock_stm32_set(const struct device *dev,
			      struct net_ptp_time *tm)
{
	const struct device *eth_dev = dev->config;
	struct eth_stm32_hal_dev_data *eth_dev_data = eth_dev->data;
	ETH_HandleTypeDef *heth = &eth_dev_data->heth;
	unsigned int key;

	key = irq_lock();

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet)
	heth->Instance->MACSTSUR = tm->second;
	heth->Instance->MACSTNUR = tm->nanosecond;
	heth->Instance->MACTSCR |= ETH_MACTSCR_TSINIT;
	while (heth->Instance->MACTSCR & ETH_MACTSCR_TSINIT_Msk) {
		/* spin lock */
	}
#else
	heth->Instance->PTPTSHUR = tm->second;
	heth->Instance->PTPTSLUR = tm->nanosecond;
	heth->Instance->PTPTSCR |= ETH_PTPTSCR_TSSTI;
	while (heth->Instance->PTPTSCR & ETH_PTPTSCR_TSSTI_Msk) {
		/* spin lock */
	}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet) */

	irq_unlock(key);

	return 0;
}

static int ptp_clock_stm32_get(const struct device *dev,
			      struct net_ptp_time *tm)
{
	const struct device *eth_dev = dev->config;
	struct eth_stm32_hal_dev_data *eth_dev_data = eth_dev->data;
	ETH_HandleTypeDef *heth = &eth_dev_data->heth;
	unsigned int key;
	uint32_t second_2;

	key = irq_lock();

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet)
	tm->second = heth->Instance->MACSTSR;
	tm->nanosecond = heth->Instance->MACSTNR;
	second_2 = heth->Instance->MACSTSR;
#else
	tm->second = heth->Instance->PTPTSHR;
	tm->nanosecond = heth->Instance->PTPTSLR;
	second_2 = heth->Instance->PTPTSHR;
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet) */

	irq_unlock(key);

	if (tm->second != second_2 && tm->nanosecond < NSEC_PER_SEC / 2) {
		/* Second rollover has happened during first measurement: second register
		 * was read before second boundary and nanosecond register was read after.
		 * We will use second_2 as a new second value.
		 */
		tm->second = second_2;
	}

	return 0;
}

static int ptp_clock_stm32_adjust(const struct device *dev, int increment)
{
	const struct device *eth_dev = dev->config;
	struct eth_stm32_hal_dev_data *eth_dev_data = eth_dev->data;
	ETH_HandleTypeDef *heth = &eth_dev_data->heth;
	int key, ret;

	if ((increment <= (int32_t)(-NSEC_PER_SEC)) ||
			(increment >= (int32_t)NSEC_PER_SEC)) {
		ret = -EINVAL;
	} else {
		key = irq_lock();

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet)
		heth->Instance->MACSTSUR = 0;
		if (increment >= 0) {
			heth->Instance->MACSTNUR = increment;
		} else {
			heth->Instance->MACSTNUR = ETH_MACSTNUR_ADDSUB | (NSEC_PER_SEC + increment);
		}
		heth->Instance->MACTSCR |= ETH_MACTSCR_TSUPDT;
		while (heth->Instance->MACTSCR & ETH_MACTSCR_TSUPDT_Msk) {
			/* spin lock */
		}
#else
		heth->Instance->PTPTSHUR = 0;
		if (increment >= 0) {
			heth->Instance->PTPTSLUR = increment;
		} else {
			heth->Instance->PTPTSLUR = ETH_PTPTSLUR_TSUPNS | (-increment);
		}
		heth->Instance->PTPTSCR |= ETH_PTPTSCR_TSSTU;
		while (heth->Instance->PTPTSCR & ETH_PTPTSCR_TSSTU_Msk) {
			/* spin lock */
		}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet) */

		ret = 0;
		irq_unlock(key);
	}

	return ret;
}

static int ptp_clock_stm32_rate_adjust(const struct device *dev, double ratio)
{
	const struct device *eth_dev = dev->config;
	struct eth_stm32_hal_dev_data *eth_dev_data = eth_dev->data;
	ETH_HandleTypeDef *heth = &eth_dev_data->heth;
	int key, ret;
	uint32_t addend_val;

	key = irq_lock();

	/* Limit possible ratio */
	if (ratio * 100 < CONFIG_ETH_STM32_HAL_PTP_CLOCK_ADJ_MIN_PCT ||
			ratio * 100 > CONFIG_ETH_STM32_HAL_PTP_CLOCK_ADJ_MAX_PCT) {
		ret = -EINVAL;
		goto error;
	}

	/* Update addend register */
	addend_val = UINT32_MAX * (double)eth_dev_data->clk_ratio * ratio;

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet)
	heth->Instance->MACTSAR = addend_val;
	heth->Instance->MACTSCR |= ETH_MACTSCR_TSADDREG;
	while (heth->Instance->MACTSCR & ETH_MACTSCR_TSADDREG_Msk) {
		/* spin lock */
	}
#else
	heth->Instance->PTPTSAR = addend_val;
	heth->Instance->PTPTSCR |= ETH_PTPTSCR_TSARU;
	while (heth->Instance->PTPTSCR & ETH_PTPTSCR_TSARU_Msk) {
		/* spin lock */
	}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet) */

	ret = 0;

error:
	irq_unlock(key);

	return ret;
}

#if ETH_STM32_HAS_FLEX_PPS
static uint32_t ptp_clock_stm32_output_resolution_ns(void)
{
	return NSEC_PER_SEC / CONFIG_ETH_STM32_HAL_PTP_CLOCK_SRC_HZ;
}

static int ptp_clock_stm32_output_channel_validate(const struct device *dev, uint32_t channel)
{
	struct ptp_clock_stm32_data *data = dev->data;

	if (channel >= data->output_channel_count) {
		return -ENOTSUP;
	}

	return 0;
}

static int ptp_clock_stm32_output_wait_ready(ETH_HandleTypeDef *heth)
{
	for (uint32_t elapsed_us = 0U; elapsed_us < ETH_STM32_PPS_BUSY_TIMEOUT_US; elapsed_us++) {
		uint32_t control = heth->Instance->MACPPSCR;
		bool command_busy = (control & ETH_MACPPSCR_PPSEN0_Msk) != 0U &&
				    (control & ETH_MACPPSCR_PPSCTRL_Msk) != 0U;

		if ((heth->Instance->MACPPSTTNR & ETH_MACPPSTTNR_TRGTBUSY0_Msk) == 0U &&
		    !command_busy) {
			return 0;
		}

		k_busy_wait(1U);
	}

	return -EBUSY;
}

static void ptp_clock_stm32_output_command(ETH_HandleTypeDef *heth, uint32_t command,
					   uint32_t target_mode)
{
	uint32_t control = heth->Instance->MACPPSCR;

	control &= ~(ETH_MACPPSCR_PPSCTRL_Msk | ETH_MACPPSCR_PPSEN0_Msk |
		     ETH_MACPPSCR_TRGTMODSEL0_Msk);
#ifdef ETH_MACPPSCR_MCGREN0_Pos
	/* Some STM32Cube headers define MCGREN0_Msk with the unrelated TIMESEL position. */
	control &= ~BIT(ETH_MACPPSCR_MCGREN0_Pos);
#endif
	control |= (command << ETH_MACPPSCR_PPSCTRL_Pos) & ETH_MACPPSCR_PPSCTRL_Msk;
	control |= (target_mode << ETH_MACPPSCR_TRGTMODSEL0_Pos) & ETH_MACPPSCR_TRGTMODSEL0_Msk;
	control |= ETH_MACPPSCR_PPSEN0_Msk;
	heth->Instance->MACPPSCR = control;
}

static int ptp_clock_stm32_output_disable(ETH_HandleTypeDef *heth)
{
	int ret;

	ret = ptp_clock_stm32_output_wait_ready(heth);
	if (ret < 0) {
		return ret;
	}

	/* Cancel an armed start before stopping a pulse train which already began. */
	ptp_clock_stm32_output_command(heth, ETH_STM32_PPS_CANCEL_START,
				       ETH_STM32_PPS_TARGET_MODE_OUTPUT_ONLY);
	ret = ptp_clock_stm32_output_wait_ready(heth);
	if (ret < 0) {
		return ret;
	}

	ptp_clock_stm32_output_command(heth, ETH_STM32_PPS_STOP_PULSE_TRAIN_IMMEDIATE,
				       ETH_STM32_PPS_TARGET_MODE_OUTPUT_ONLY);

	return ptp_clock_stm32_output_wait_ready(heth);
}

static int ptp_clock_stm32_output_ticks(uint64_t duration_ns, uint64_t min_ticks,
					uint32_t resolution_ns, uint32_t *register_value)
{
	uint64_t ticks;

	if ((duration_ns % resolution_ns) != 0U) {
		return -ERANGE;
	}

	ticks = duration_ns / resolution_ns;
	if (ticks < min_ticks || ticks > UINT32_MAX) {
		return -ERANGE;
	}

	*register_value = (uint32_t)(ticks - 1U);

	return 0;
}

static bool ptp_clock_stm32_output_has_lead_time(const struct net_ptp_time *now,
						 const struct net_ptp_time *start)
{
	uint64_t second_delta;
	uint64_t lead_time_ns;

	if (start->second < now->second) {
		return false;
	}

	second_delta = start->second - now->second;
	if (second_delta > 1U) {
		return true;
	}

	if (second_delta == 0U) {
		if (start->nanosecond <= now->nanosecond) {
			return false;
		}

		lead_time_ns = start->nanosecond - now->nanosecond;
	} else {
		lead_time_ns = NSEC_PER_SEC - now->nanosecond + start->nanosecond;
	}

	return lead_time_ns >= ETH_STM32_PPS_MIN_LEAD_TIME_NS;
}

/*
 * Convert a signed raw clock-domain time (nanoseconds) into the STM32 target
 * time registers. Reject negative times and values whose seconds field does not
 * fit the 32-bit MACPPSTTSR register.
 */
static int ptp_clock_stm32_output_raw_to_ptp_time(precision_time_t raw_ns,
						  struct net_ptp_time *tm)
{
	uint64_t sec;
	uint32_t nsec;

	if (raw_ns < 0) {
		return -ERANGE;
	}

	sec = (uint64_t)(raw_ns / NSEC_PER_SEC);
	nsec = (uint32_t)(raw_ns % NSEC_PER_SEC);

	if (sec > UINT32_MAX) {
		return -ERANGE;
	}

	*tm = (struct net_ptp_time){
		.second = sec,
		.nanosecond = nsec,
	};

	return 0;
}

/*
 * Resolve the effective high-pulse width applied to each period. An exact width
 * is validated against the channel resolution and limits. The provider default
 * is a deterministic 50% high time rounded down to the output resolution and
 * clamped to the channel pulse-width limits.
 */
static int ptp_clock_stm32_output_effective_width(
	const struct precision_clock_output_raw_waveform_config *config, uint32_t resolution_ns,
	precision_time_t max_pulse_width_ns, precision_time_t *width_ns)
{
	precision_time_t width;

	switch (config->width_policy) {
	case PRECISION_CLOCK_OUTPUT_WIDTH_EXACT:
		width = config->pulse_width_ns;
		if (width <= 0 || width >= config->period_ns) {
			return -EINVAL;
		}
		if ((width % (precision_time_t)resolution_ns) != 0) {
			return -ERANGE;
		}
		if (width > max_pulse_width_ns) {
			return -ERANGE;
		}
		break;
	case PRECISION_CLOCK_OUTPUT_WIDTH_PROVIDER_DEFAULT:
		width = config->period_ns / 2;
		width -= width % (precision_time_t)resolution_ns;
		if (width > max_pulse_width_ns) {
			width = max_pulse_width_ns;
		}
		if (width < (precision_time_t)resolution_ns) {
			width = (precision_time_t)resolution_ns;
		}
		if (width >= config->period_ns) {
			return -ERANGE;
		}
		break;
	default:
		return -EINVAL;
	}

	*width_ns = width;

	return 0;
}

#if ETH_STM32_PPS_REARM_SINGLE_PULSES
static uint64_t ptp_clock_stm32_output_time_to_ns(const struct net_ptp_time *tm)
{
	return tm->second * NSEC_PER_SEC + tm->nanosecond;
}

static uint64_t ptp_clock_stm32_output_monotonic_ns(void)
{
	return k_ticks_to_ns_floor64(k_uptime_ticks());
}

static int ptp_clock_stm32_output_monotonic_deadline(uint64_t now_ns, uint64_t clock_duration_ns,
						     uint64_t *deadline_ns)
{
	uint64_t real_duration_ns;

	if (eth_stm32_ptp_output_real_duration_ceil(clock_duration_ns,
						    CONFIG_ETH_STM32_HAL_PTP_CLOCK_ADJ_MIN_PCT,
						    &real_duration_ns) < 0 ||
	    u64_add_overflow(real_duration_ns, k_ticks_to_ns_ceil64(1), &real_duration_ns) ||
	    u64_add_overflow(now_ns, real_duration_ns, deadline_ns)) {
		return -ERANGE;
	}

	return 0;
}

static int ptp_clock_stm32_output_wait_quiescent(struct ptp_clock_stm32_data *data)
{
	uint64_t clock_duration_ns;
	uint64_t safe_after_ns;
	uint64_t now_ns;
	int ret;

	if (u64_add_overflow((uint64_t)data->output_effective_width_ns,
			     ETH_STM32_PPS_REARM_GUARD_NS, &clock_duration_ns)) {
		return -ERANGE;
	}

	/*
	 * Cancel prevents a not-yet-executed single pulse, but cannot terminate one
	 * already high. Wait one complete worst-case pulse width from disable using
	 * uptime, so a PHC step cannot make the channel appear quiescent early.
	 */
	now_ns = ptp_clock_stm32_output_monotonic_ns();
	ret = ptp_clock_stm32_output_monotonic_deadline(now_ns, clock_duration_ns, &safe_after_ns);
	if (ret < 0) {
		return ret;
	}

	for (;;) {
		now_ns = ptp_clock_stm32_output_monotonic_ns();
		if (now_ns >= safe_after_ns) {
			return 0;
		}

		k_sleep(K_MSEC(ETH_STM32_PPS_QUIESCE_POLL_MS));
	}
}

static int ptp_clock_stm32_output_rearm_schedule(struct ptp_clock_stm32_data *data, uint64_t now_ns,
						 uint64_t now_monotonic_ns)
{
	uint64_t delay_ns;
	int ret;

	ret = eth_stm32_ptp_output_rearm_delay(
		&data->output_rearm_timing, now_ns, now_monotonic_ns,
		MAX(CONFIG_ETH_STM32_HAL_PTP_CLOCK_ADJ_MAX_PCT, ETH_STM32_PPS_RATE_SCALE_PCT),
		k_ticks_to_ns_ceil64(1), ETH_STM32_PPS_REARM_MAX_DELAY_NS, &delay_ns);
	if (ret < 0) {
		return ret;
	}

	ret = k_work_reschedule_for_queue(&data->output_rearm_queue, &data->output_rearm_work,
					  K_NSEC(delay_ns));

	return ret < 0 ? ret : 0;
}

static void ptp_clock_stm32_output_rearm_queue_start(struct ptp_clock_stm32_data *data)
{
	static const struct k_work_queue_config queue_config = {
		.name = "stm32_ptp_pps",
		.no_yield = true,
	};

	k_work_queue_init(&data->output_rearm_queue);
	k_work_queue_start(&data->output_rearm_queue, data->output_rearm_stack,
			   K_KERNEL_STACK_SIZEOF(data->output_rearm_stack),
			   CONFIG_PTP_CLOCK_STM32_HAL_OUTPUT_REARM_PRIORITY, &queue_config);
}

static void ptp_clock_stm32_output_rearm_fault(struct ptp_clock_stm32_data *data,
					       ETH_HandleTypeDef *heth, int error)
{
	/*
	 * Called with output_lock held; releases it. Latch the fault and its
	 * error under the fast status lock, then best-effort disable the channel
	 * without holding output_lock and without quiescence polling so status
	 * queries stay prompt. Ownership stays conservative (configured) so the
	 * channel never looks healthy until a stop performs the quiescent
	 * cleanup and finalizes the unconfigured state.
	 */
	data->output_state = PTP_CLOCK_STM32_OUTPUT_FAULTED;
	data->output_fault_error = error;
	k_mutex_unlock(&data->output_lock);

	(void)ptp_clock_stm32_output_disable(heth);

	LOG_ERR("Scheduled output faulted: PPS rearm failed (%d)", error);
}

static void ptp_clock_stm32_output_rearm(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct ptp_clock_stm32_data *data =
		CONTAINER_OF(dwork, struct ptp_clock_stm32_data, output_rearm_work);
	const struct device *dev = data->dev;
	const struct device *eth_dev = dev->config;
	struct eth_stm32_hal_dev_data *eth_dev_data = eth_dev->data;
	ETH_HandleTypeDef *heth = &eth_dev_data->heth;
	struct net_ptp_time next_target;
	struct net_ptp_time now;
	struct eth_stm32_ptp_output_rearm_timing next_rearm_timing;
	uint64_t next_target_ns;
	uint64_t now_monotonic_ns;
	uint64_t now_ns;
	unsigned int key;
	int ret;

	k_mutex_lock(&data->output_lock, K_FOREVER);
	if (data->output_state != PTP_CLOCK_STM32_OUTPUT_RUNNING) {
		goto unlock;
	}

	ret = ptp_clock_stm32_get(dev, &now);
	if (ret < 0) {
		ptp_clock_stm32_output_rearm_fault(data, heth, ret);
		return;
	}

	now_ns = ptp_clock_stm32_output_time_to_ns(&now);
	now_monotonic_ns = ptp_clock_stm32_output_monotonic_ns();
	/*
	 * Do not include an arbitrary first-edge lead in the monotonic falling-edge
	 * guard. Approach the target using PHC time, then anchor a full worst-case
	 * pulse duration in uptime once the edge can have occurred. This remains safe
	 * across a later PHC step without delaying the following period for a
	 * far-future first target.
	 */
	ret = eth_stm32_ptp_output_rearm_observe(
		&data->output_rearm_timing, now_ns, now_monotonic_ns,
		CONFIG_ETH_STM32_HAL_PTP_CLOCK_ADJ_MIN_PCT, k_ticks_to_ns_ceil64(1));
	if (ret < 0) {
		ptp_clock_stm32_output_rearm_fault(data, heth, ret);
		return;
	}
	if (!data->output_rearm_timing.monotonic_guard_active) {
		ret = ptp_clock_stm32_output_rearm_schedule(data, now_ns, now_monotonic_ns);
		if (ret < 0) {
			ptp_clock_stm32_output_rearm_fault(data, heth, ret);
			return;
		}
		goto unlock;
	}
	if (!eth_stm32_ptp_output_rearm_ready(&data->output_rearm_timing, now_ns,
					      now_monotonic_ns)) {
		ret = ptp_clock_stm32_output_rearm_schedule(data, now_ns, now_monotonic_ns);
		if (ret < 0) {
			ptp_clock_stm32_output_rearm_fault(data, heth, ret);
			return;
		}
		goto unlock;
	}

	if (data->output_rearm_timing.target_ns >
	    UINT64_MAX - (uint64_t)data->output_config.period_ns) {
		ptp_clock_stm32_output_rearm_fault(data, heth, -ERANGE);
		return;
	}
	next_target_ns =
		data->output_rearm_timing.target_ns + (uint64_t)data->output_config.period_ns;
	next_target.second = next_target_ns / NSEC_PER_SEC;
	next_target.nanosecond = next_target_ns % NSEC_PER_SEC;
	if (next_target.second > UINT32_MAX) {
		ptp_clock_stm32_output_rearm_fault(data, heth, -ERANGE);
		return;
	}
	ret = eth_stm32_ptp_output_rearm_timing_init(&next_rearm_timing, next_target_ns,
						     (uint64_t)data->output_effective_width_ns +
							     ETH_STM32_PPS_REARM_GUARD_NS);
	if (ret < 0) {
		ptp_clock_stm32_output_rearm_fault(data, heth, ret);
		return;
	}
	if (!ptp_clock_stm32_output_has_lead_time(&now, &next_target)) {
		ptp_clock_stm32_output_rearm_fault(data, heth, -ETIME);
		return;
	}

	ret = ptp_clock_stm32_output_wait_ready(heth);
	if (ret < 0) {
		ptp_clock_stm32_output_rearm_fault(data, heth, ret);
		return;
	}

	/* Keep the final lead-time check and command issue atomic with PHC updates. */
	key = irq_lock();
	ret = ptp_clock_stm32_get(dev, &now);
	if (ret < 0) {
		irq_unlock(key);
		ptp_clock_stm32_output_rearm_fault(data, heth, ret);
		return;
	}
	now_ns = ptp_clock_stm32_output_time_to_ns(&now);
	now_monotonic_ns = ptp_clock_stm32_output_monotonic_ns();
	if (!eth_stm32_ptp_output_rearm_ready(&data->output_rearm_timing, now_ns,
					      now_monotonic_ns)) {
		irq_unlock(key);
		ret = ptp_clock_stm32_output_rearm_schedule(data, now_ns, now_monotonic_ns);
		if (ret < 0) {
			ptp_clock_stm32_output_rearm_fault(data, heth, ret);
			return;
		}
		goto unlock;
	}
	if (!ptp_clock_stm32_output_has_lead_time(&now, &next_target)) {
		irq_unlock(key);
		ptp_clock_stm32_output_rearm_fault(data, heth, -ETIME);
		return;
	}
	/* RM0481 requires the next command only after the preceding falling edge. */
	heth->Instance->MACPPSTTNR = next_target.nanosecond;
	heth->Instance->MACPPSTTSR = (uint32_t)next_target.second;
	heth->Instance->MACPPSWR = (uint32_t)((uint64_t)data->output_effective_width_ns /
					      ptp_clock_stm32_output_resolution_ns()) -
				   1U;
	data->output_rearm_timing = next_rearm_timing;
	ptp_clock_stm32_output_command(heth, ETH_STM32_PPS_SINGLE_PULSE,
				       ETH_STM32_PPS_TARGET_MODE_OUTPUT_ONLY);
	irq_unlock(key);
	ret = ptp_clock_stm32_output_wait_ready(heth);
	if (ret < 0) {
		/* The pending command can still arm the next pulse; disable it conservatively. */
		ptp_clock_stm32_output_rearm_fault(data, heth, ret);
		return;
	}

	ret = ptp_clock_stm32_output_rearm_schedule(data, now_ns, now_monotonic_ns);
	if (ret < 0) {
		ptp_clock_stm32_output_rearm_fault(data, heth, ret);
		return;
	}

unlock:
	k_mutex_unlock(&data->output_lock);
}
#endif /* ETH_STM32_PPS_REARM_SINGLE_PULSES */

static int ptp_clock_stm32_output_get_caps(const struct device *dev, uint32_t channel,
					   struct precision_clock_output_caps *caps)
{
	uint32_t resolution_ns = ptp_clock_stm32_output_resolution_ns();
	precision_time_t min_period_ns;
	precision_time_t max_period_ns;
	precision_time_t max_pulse_width_ns;
#if !ETH_STM32_PPS_REARM_SINGLE_PULSES
	precision_time_t max_duration_ns = (precision_time_t)UINT32_MAX * resolution_ns;
#endif
	int ret;

	if (caps == NULL) {
		return -EINVAL;
	}

	ret = ptp_clock_stm32_output_channel_validate(dev, channel);
	if (ret < 0) {
		return ret;
	}

#if ETH_STM32_PPS_REARM_SINGLE_PULSES
	/*
	 * Each of the target-approach and guard-completion K_NSEC() timeouts can
	 * consume one tick while converting up and another for relative-timeout
	 * alignment. Reserve those four ticks, the explicit monotonic-guard tick,
	 * and the full post-command ready timeout. The remaining time must still
	 * leave the next target at least min_lead_time_ns in the future.
	 */
	const struct eth_stm32_ptp_output_rearm_limits limits = {
		.period_ns = ETH_STM32_PPS_REARM_PERIOD_NS,
		.min_lead_time_ns = ETH_STM32_PPS_MIN_LEAD_TIME_NS,
		.falling_edge_guard_ns = ETH_STM32_PPS_REARM_GUARD_NS,
		.scheduling_margin_ns = ETH_STM32_PPS_BUSY_TIMEOUT_US * NSEC_PER_USEC +
					5U * k_ticks_to_ns_ceil64(1),
		.hardware_max_width_ns = ETH_STM32_PPS_REARM_MAX_WIDTH_NS,
		.resolution_ns = resolution_ns,
		.min_rate_pct = CONFIG_ETH_STM32_HAL_PTP_CLOCK_ADJ_MIN_PCT,
		.max_rate_pct = MAX(CONFIG_ETH_STM32_HAL_PTP_CLOCK_ADJ_MAX_PCT,
				    ETH_STM32_PPS_RATE_SCALE_PCT),
	};
	uint64_t max_width_ns;

	min_period_ns = ETH_STM32_PPS_REARM_PERIOD_NS;
	max_period_ns = ETH_STM32_PPS_REARM_PERIOD_NS;
	ret = eth_stm32_ptp_output_max_pulse_width(&limits, &max_width_ns);
	if (ret < 0) {
		return ret;
	}
	max_pulse_width_ns = (precision_time_t)max_width_ns;
#else
	min_period_ns = 2 * (precision_time_t)resolution_ns;
	max_period_ns = max_duration_ns;
	max_pulse_width_ns = max_duration_ns;
#endif

	*caps = (struct precision_clock_output_caps){
		.flags = PRECISION_CLOCK_OUTPUT_CAP_WAVEFORM |
			 PRECISION_CLOCK_OUTPUT_CAP_PROGRAMMABLE_WIDTH,
		.channel_count = ((struct ptp_clock_stm32_data *)dev->data)->output_channel_count,
		.resolution_ns = resolution_ns,
		.min_lead_time_ns = ETH_STM32_PPS_MIN_LEAD_TIME_NS,
		.min_period_ns = min_period_ns,
		.max_period_ns = max_period_ns,
		.min_pulse_width_ns = resolution_ns,
		.max_pulse_width_ns = max_pulse_width_ns,
	};

	return 0;
}

static int ptp_clock_stm32_output_start_waveform(
	const struct device *dev, uint32_t channel,
	const struct precision_clock_output_raw_waveform_config *config)
{
	const struct device *eth_dev = dev->config;
	struct eth_stm32_hal_dev_data *eth_dev_data = eth_dev->data;
	struct ptp_clock_stm32_data *data = dev->data;
	ETH_HandleTypeDef *heth = &eth_dev_data->heth;
	uint32_t resolution_ns = ptp_clock_stm32_output_resolution_ns();
	struct precision_clock_output_caps caps;
	precision_time_t effective_width_ns;
	struct net_ptp_time start;
	struct net_ptp_time now;
	uint32_t interval;
	uint32_t width;
	unsigned int key;
#if ETH_STM32_PPS_REARM_SINGLE_PULSES
	struct eth_stm32_ptp_output_rearm_timing rearm_timing;
	uint64_t target_ns;
	uint64_t now_monotonic_ns;
	uint64_t now_ns;
	int rearm_ret;
#endif
	int ret;

	if (config == NULL) {
		return -EINVAL;
	}

	ret = ptp_clock_stm32_output_channel_validate(dev, channel);
	if (ret < 0) {
		return ret;
	}

	if (config->period_ns <= 0) {
		return -EINVAL;
	}

	ret = ptp_clock_stm32_output_get_caps(dev, channel, &caps);
	if (ret < 0) {
		return ret;
	}

	ret = ptp_clock_stm32_output_effective_width(config, resolution_ns, caps.max_pulse_width_ns,
						     &effective_width_ns);
	if (ret < 0) {
		return ret;
	}

	ret = ptp_clock_stm32_output_raw_to_ptp_time(config->first_rising_time, &start);
	if (ret < 0) {
		return ret;
	}

	if ((start.nanosecond % resolution_ns) != 0U) {
		return -ERANGE;
	}

#if ETH_STM32_PPS_REARM_SINGLE_PULSES
	if (config->period_ns != ETH_STM32_PPS_REARM_PERIOD_NS) {
		return -ERANGE;
	}
#endif

	ret = ptp_clock_stm32_output_ticks((uint64_t)config->period_ns, 2U, resolution_ns,
					   &interval);
	if (ret < 0) {
		return ret;
	}

	ret = ptp_clock_stm32_output_ticks((uint64_t)effective_width_ns, 1U, resolution_ns, &width);
	if (ret < 0) {
		return ret;
	}

#if ETH_STM32_PPS_REARM_SINGLE_PULSES
	target_ns = (uint64_t)config->first_rising_time;
	ret = eth_stm32_ptp_output_rearm_timing_init(&rearm_timing, target_ns,
						     (uint64_t)effective_width_ns +
							     ETH_STM32_PPS_REARM_GUARD_NS);
	if (ret < 0) {
		return ret;
	}
#endif

	k_mutex_lock(&data->output_lifecycle_lock, K_FOREVER);

	k_mutex_lock(&data->output_lock, K_FOREVER);
	if (data->output_state != PTP_CLOCK_STM32_OUTPUT_IDLE) {
		k_mutex_unlock(&data->output_lock);
		ret = -EBUSY;
		goto unlock_lifecycle;
	}
	k_mutex_unlock(&data->output_lock);

	ret = ptp_clock_stm32_output_wait_ready(heth);
	if (ret < 0) {
		goto unlock_lifecycle;
	}

	/* Keep the final lead-time check and command issue atomic with PHC updates. */
	key = irq_lock();
	ret = ptp_clock_stm32_get(dev, &now);
	if (ret < 0) {
		irq_unlock(key);
		goto unlock_lifecycle;
	}
#if ETH_STM32_PPS_REARM_SINGLE_PULSES
	now_ns = ptp_clock_stm32_output_time_to_ns(&now);
	now_monotonic_ns = ptp_clock_stm32_output_monotonic_ns();
#endif
	if (!ptp_clock_stm32_output_has_lead_time(&now, &start)) {
		irq_unlock(key);
		ret = -ETIME;
		goto unlock_lifecycle;
	}
	/* RM0481 requires target nanoseconds first and the control command last. */
	heth->Instance->MACPPSTTNR = start.nanosecond;
	heth->Instance->MACPPSTTSR = (uint32_t)start.second;
	heth->Instance->MACPPSIR = interval;
	heth->Instance->MACPPSWR = width;

	/* Activate pulse generation only after every parameter is valid. */
#if ETH_STM32_PPS_REARM_SINGLE_PULSES
	ptp_clock_stm32_output_command(heth, ETH_STM32_PPS_SINGLE_PULSE,
				       ETH_STM32_PPS_TARGET_MODE_OUTPUT_ONLY);
#else
	ptp_clock_stm32_output_command(heth, ETH_STM32_PPS_START_PULSE_TRAIN,
				       ETH_STM32_PPS_TARGET_MODE_OUTPUT_ONLY);
#endif
	irq_unlock(key);
	ret = ptp_clock_stm32_output_wait_ready(heth);

	/* Publish the requested configuration and retain any resolved value internally. */
	k_mutex_lock(&data->output_lock, K_FOREVER);
	data->output_config = *config;
	if (ret == 0) {
		data->output_state = PTP_CLOCK_STM32_OUTPUT_RUNNING;
		data->output_fault_error = 0;
	} else {
		/*
		 * The command can still arm the output after the wait times out.
		 * Preserve conservative ownership, but expose uncertain completion
		 * as a fault until stop confirms that the hardware is quiescent.
		 */
		data->output_state = PTP_CLOCK_STM32_OUTPUT_FAULTED;
		data->output_fault_error = ret;
	}
#if ETH_STM32_PPS_REARM_SINGLE_PULSES
	data->output_effective_width_ns = effective_width_ns;
	data->output_rearm_timing = rearm_timing;
#endif
	k_mutex_unlock(&data->output_lock);

	if (ret < 0) {
		goto unlock_lifecycle;
	}

#if ETH_STM32_PPS_REARM_SINGLE_PULSES
	rearm_ret = ptp_clock_stm32_output_rearm_schedule(data, now_ns, now_monotonic_ns);
	if (rearm_ret < 0) {
		k_mutex_lock(&data->output_lock, K_FOREVER);
		ptp_clock_stm32_output_rearm_fault(data, heth, rearm_ret);
		ret = rearm_ret;
	}
#endif

unlock_lifecycle:
	k_mutex_unlock(&data->output_lifecycle_lock);

	return ret;
}

static int ptp_clock_stm32_output_stop(const struct device *dev, uint32_t channel)
{
	const struct device *eth_dev = dev->config;
	struct eth_stm32_hal_dev_data *eth_dev_data = eth_dev->data;
	struct ptp_clock_stm32_data *data = dev->data;
	ETH_HandleTypeDef *heth = &eth_dev_data->heth;
	int ret;

	ret = ptp_clock_stm32_output_channel_validate(dev, channel);
	if (ret < 0) {
		return ret;
	}

	k_mutex_lock(&data->output_lifecycle_lock, K_FOREVER);

	k_mutex_lock(&data->output_lock, K_FOREVER);
	if (data->output_state == PTP_CLOCK_STM32_OUTPUT_IDLE) {
		k_mutex_unlock(&data->output_lock);
		k_mutex_unlock(&data->output_lifecycle_lock);
		return 0;
	}
	/*
	 * Mark the stop transition, then release the fast status lock so status
	 * queries stay prompt while the sleeping cleanup below runs. Lifecycle
	 * serialization keeps start and rearm out until the transition finalizes.
	 */
	data->output_state = PTP_CLOCK_STM32_OUTPUT_STOPPING;
	k_mutex_unlock(&data->output_lock);

#if ETH_STM32_PPS_REARM_SINGLE_PULSES
	/*
	 * Never hold output_lock across the synchronous cancel: the rearm worker
	 * takes output_lock, so holding it here would deadlock the cancel.
	 * A single pulse already asserted is allowed to finish; no later pulse is
	 * rearmed once the worker observes the non-running state.
	 */
	(void)k_work_cancel_delayable_sync(&data->output_rearm_work, &data->output_rearm_sync);
#endif

	ret = ptp_clock_stm32_output_disable(heth);
#if ETH_STM32_PPS_REARM_SINGLE_PULSES
	if (ret == 0) {
		ret = ptp_clock_stm32_output_wait_quiescent(data);
	}
#endif

	k_mutex_lock(&data->output_lock, K_FOREVER);
	if (ret == 0) {
		/* Disable and quiescence confirmed: finalize the unconfigured state. */
		data->output_state = PTP_CLOCK_STM32_OUTPUT_IDLE;
		data->output_fault_error = 0;
#if ETH_STM32_PPS_REARM_SINGLE_PULSES
		data->output_effective_width_ns = 0;
		data->output_rearm_timing = (struct eth_stm32_ptp_output_rearm_timing){0};
#endif
	} else {
		/*
		 * Timeout or uncertain completion: keep conservative ownership and
		 * surface the error so a later clean stop can retry the cleanup.
		 */
		data->output_state = PTP_CLOCK_STM32_OUTPUT_FAULTED;
		data->output_fault_error = ret;
	}
	k_mutex_unlock(&data->output_lock);

	k_mutex_unlock(&data->output_lifecycle_lock);

	return ret;
}

static int ptp_clock_stm32_output_get_status(const struct device *dev, uint32_t channel,
					     struct precision_clock_output_raw_status *status)
{
	struct ptp_clock_stm32_data *data = dev->data;
	int ret;

	if (status == NULL) {
		return -EINVAL;
	}

	ret = ptp_clock_stm32_output_channel_validate(dev, channel);
	if (ret < 0) {
		return ret;
	}

	k_mutex_lock(&data->output_lock, K_FOREVER);

	/* Never claim a physical hardware observation. */
	*status = (struct precision_clock_output_raw_status){
		.hardware_active_valid = false,
		.hardware_active = false,
	};

	switch (data->output_state) {
	case PTP_CLOCK_STM32_OUTPUT_IDLE:
		status->configured = false;
		ret = 0;
		break;
	case PTP_CLOCK_STM32_OUTPUT_RUNNING:
	case PTP_CLOCK_STM32_OUTPUT_STOPPING:
		status->configured = true;
		status->kind = PRECISION_CLOCK_OUTPUT_KIND_WAVEFORM;
		status->config.waveform = data->output_config;
		ret = 0;
		break;
	case PTP_CLOCK_STM32_OUTPUT_FAULTED:
	default:
		/*
		 * Surface the stored error and keep conservative ownership so the
		 * channel never looks healthy after a rearm or cleanup failure.
		 */
		status->configured = true;
		status->kind = PRECISION_CLOCK_OUTPUT_KIND_WAVEFORM;
		status->config.waveform = data->output_config;
		ret = (data->output_fault_error != 0) ? data->output_fault_error : -EIO;
		break;
	}

	k_mutex_unlock(&data->output_lock);

	return ret;
}
#endif /* ETH_STM32_HAS_FLEX_PPS */
static void eth_stm32_ptp_enable_timestamping(ETH_HandleTypeDef *heth)
{
	/* Mask the Timestamp Trigger interrupt and enable timestamping */
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet)
	heth->Instance->MACIER &= ~(ETH_MACIER_TSIE);
	heth->Instance->MACTSCR |= ETH_MACTSCR_TSENA;
#else
	heth->Instance->MACIMR &= ~(ETH_MACIMR_TSTIM);
	heth->Instance->PTPTSCR |= ETH_PTPTSCR_TSE;
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet) */
}

static void eth_stm32_ptp_set_addend(ETH_HandleTypeDef *heth, uint32_t addend_val)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet)
	heth->Instance->MACTSAR = addend_val;
	heth->Instance->MACTSCR |= ETH_MACTSCR_TSADDREG;
	while (heth->Instance->MACTSCR & ETH_MACTSCR_TSADDREG_Msk) {
		k_yield();
	}
#else
	heth->Instance->PTPTSAR = addend_val;
	heth->Instance->PTPTSCR |= ETH_PTPTSCR_TSARU;
	while (heth->Instance->PTPTSCR & ETH_PTPTSCR_TSARU_Msk) {
		k_yield();
	}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet) */
}

static void eth_stm32_ptp_enable_fine_timestamp_update(ETH_HandleTypeDef *heth)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet)
	heth->Instance->MACTSCR |= ETH_MACTSCR_TSCFUPDT;
#else
	heth->Instance->PTPTSCR |= ETH_PTPTSCR_TSFCU;
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet) */
}

static void eth_stm32_ptp_enable_nsec_rollover(ETH_HandleTypeDef *heth)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet)
	heth->Instance->MACTSCR |= ETH_MACTSCR_TSCTRLSSR;
#else
	heth->Instance->PTPTSCR |= ETH_PTPTSCR_TSSSR;
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet) */
}

static void eth_stm32_ptp_init_timestamp(ETH_HandleTypeDef *heth)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet)
	heth->Instance->MACSTSUR = 0;
	heth->Instance->MACSTNUR = 0;
	heth->Instance->MACTSCR |= ETH_MACTSCR_TSINIT;
	while (heth->Instance->MACTSCR & ETH_MACTSCR_TSINIT_Msk) {
		k_yield();
	}
#else
	heth->Instance->PTPTSHUR = 0;
	heth->Instance->PTPTSLUR = 0;
	heth->Instance->PTPTSCR |= ETH_PTPTSCR_TSSTI;
	while (heth->Instance->PTPTSCR & ETH_PTPTSCR_TSSTI_Msk) {
		k_yield();
	}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet) */
}

#if ETH_STM32_HAS_FLEX_PPS
/*
 * Protocol-neutral scheduled-output provider. The STM32 FlexPPS channel exposes
 * periodic waveforms with a programmable high-pulse width.
 */
static const struct precision_clock_output_provider ptp_clock_stm32_output_provider = {
	.get_caps = ptp_clock_stm32_output_get_caps,
	.start_waveform = ptp_clock_stm32_output_start_waveform,
	.stop = ptp_clock_stm32_output_stop,
	.get_status = ptp_clock_stm32_output_get_status,
};
#endif /* ETH_STM32_HAS_FLEX_PPS */

static DEVICE_API(ptp_clock, api) = {
	.set = ptp_clock_stm32_set,
	.get = ptp_clock_stm32_get,
	.adjust = ptp_clock_stm32_adjust,
	.rate_adjust = ptp_clock_stm32_rate_adjust,
#if ETH_STM32_HAS_FLEX_PPS
	.output = &ptp_clock_stm32_output_provider,
#endif
};

BUILD_ASSERT(NSEC_PER_SEC % CONFIG_ETH_STM32_HAL_PTP_CLOCK_SRC_HZ == 0,
	     "PTP clock period must be an integer nanosecond value");

BUILD_ASSERT(NSEC_PER_SEC / CONFIG_ETH_STM32_HAL_PTP_CLOCK_SRC_HZ <= UINT8_MAX,
	     "PTP clock period is more than 255 nanoseconds");

#if ETH_STM32_HAS_FLEX_PPS && ETH_STM32_PPS_REARM_SINGLE_PULSES
BUILD_ASSERT(CONFIG_ETH_STM32_HAL_PTP_CLOCK_ADJ_MIN_PCT > 0 &&
		     CONFIG_ETH_STM32_HAL_PTP_CLOCK_ADJ_MIN_PCT <= ETH_STM32_PPS_RATE_SCALE_PCT,
	     "PTP clock minimum adjustment must include the nominal rate");
#endif

static int ptp_stm32_init(const struct device *dev)
{
	const struct device *const eth_dev = dev->config;
	struct eth_stm32_hal_dev_data *eth_dev_data = eth_dev->data;
#if ETH_STM32_HAS_FLEX_PPS
	struct ptp_clock_stm32_data *data = dev->data;
#endif
	const struct eth_stm32_hal_dev_cfg *eth_cfg = eth_dev->config;
	ETH_HandleTypeDef *heth = &eth_dev_data->heth;
	int ret;
	uint32_t ptp_clk_rate;
	uint32_t ss_incr_ns = NSEC_PER_SEC / CONFIG_ETH_STM32_HAL_PTP_CLOCK_SRC_HZ;
	uint32_t addend_val;

#if ETH_STM32_HAS_FLEX_PPS
	k_mutex_init(&data->output_lifecycle_lock);
	k_mutex_init(&data->output_lock);
	data->output_state = PTP_CLOCK_STM32_OUTPUT_IDLE;
	data->output_fault_error = 0;
	data->output_config = (struct precision_clock_output_raw_waveform_config){0};
	data->output_channel_count = 0U;
#if ETH_STM32_PPS_REARM_SINGLE_PULSES
	k_work_init_delayable(&data->output_rearm_work, ptp_clock_stm32_output_rearm);
	data->dev = dev;
	data->output_effective_width_ns = 0;
	data->output_rearm_timing = (struct eth_stm32_ptp_output_rearm_timing){0};
#endif
#endif

	eth_dev_data->ptp_clock = dev;

	eth_stm32_ptp_enable_timestamping(heth);

	/* Query the MAC timestamp reference clock rate */
	clock_control_subsys_t rate_clk = (void *)&eth_cfg->pclken[eth_cfg->rate_pclken_idx];

	ret = clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE), rate_clk,
				     &ptp_clk_rate);
	if (ret) {
		LOG_ERR("Failed to query PTP reference clock");
		return -EIO;
	}

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet)
	heth->Instance->MACSSIR = ss_incr_ns << ETH_MACMACSSIR_SSINC_Pos;
#else
	heth->Instance->PTPSSIR = ss_incr_ns;
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_ethernet) */

	/* Program timestamp addend register */
	eth_dev_data->clk_ratio =
		((double)CONFIG_ETH_STM32_HAL_PTP_CLOCK_SRC_HZ) / ((double)ptp_clk_rate);
	/*
	 * clk_ratio is the ratio between the desired PTP clock frequency and the
	 * MAC timestamp reference clock. Because that reference is derived from a
	 * physical oscillator, it might drift due to manufacturing tolerances and
	 * environmental effects (e.g. temperature). It gets adjusted by calling
	 * ptp_clock_stm32_rate_adjust().
	 */
	addend_val =
		UINT32_MAX * eth_dev_data->clk_ratio;

	eth_stm32_ptp_set_addend(heth, addend_val);

	eth_stm32_ptp_enable_fine_timestamp_update(heth);

	eth_stm32_ptp_enable_nsec_rollover(heth);

	eth_stm32_ptp_init_timestamp(heth);

#if ETH_STM32_HAS_FLEX_PPS
	data->output_channel_count = MIN((heth->Instance->MACHWF2R & ETH_MACHWF2R_PPSOUTNUM_Msk) >>
						 ETH_MACHWF2R_PPSOUTNUM_Pos,
					 ETH_STM32_PPS_OUTPUT_CHANNELS_IMPLEMENTED);
	if (data->output_channel_count > 0U) {
		ret = ptp_clock_stm32_output_disable(heth);
		if (ret < 0) {
			LOG_WRN("Disabling scheduled output: PPS channel did not become idle");
			data->output_channel_count = 0U;
		}
#if ETH_STM32_PPS_REARM_SINGLE_PULSES
		if (data->output_channel_count > 0U) {
			ptp_clock_stm32_output_rearm_queue_start(data);
		}
#endif
	}
#endif

	/* Set PTP Configuration done */
	heth->IsPtpConfigured = ETH_STM32_PTP_CONFIGURED;

	return 0;
}

#if ETH_STM32_HAS_FLEX_PPS
#define PTP_CLOCK_STM32_DATA_DEFINE(n) static struct ptp_clock_stm32_data ptp_clock_stm32_data_##n;
#define PTP_CLOCK_STM32_DATA_GET(n)    &ptp_clock_stm32_data_##n
#else
#define PTP_CLOCK_STM32_DATA_DEFINE(n)
#define PTP_CLOCK_STM32_DATA_GET(n) NULL
#endif

#define PTP_CLOCK_STM32_INIT(n)                                                                    \
	PTP_CLOCK_STM32_DATA_DEFINE(n)                                                             \
	DEVICE_DT_INST_DEFINE(n, ptp_stm32_init, NULL, PTP_CLOCK_STM32_DATA_GET(n),                \
			      DEVICE_DT_GET(DT_INST_PARENT(n)), POST_KERNEL,                       \
			      CONFIG_PTP_CLOCK_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(PTP_CLOCK_STM32_INIT)
