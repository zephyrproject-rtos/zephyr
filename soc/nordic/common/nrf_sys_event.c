/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <nrf_sys_event.h>
#include <helpers/nrfx_gppi.h>
#include <zephyr/logging/log.h>
#ifdef CONFIG_NRF_SYS_EVENT_IRQ_LATENCY
#include <nrfx_grtc.h>
#include <zephyr/drivers/timer/nrf_grtc_timer.h>
#ifdef RRAMC_PRESENT
#include <hal/nrf_rramc.h>
#endif
#endif
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

#if CONFIG_SOC_SERIES_NRF54H

/*
 * The 54HX is not yet supported by an nrfx driver nor the system controller so
 * we implement an ISR and concurrent access safe reference counting implementation
 * here using the nrfx hal.
 */

#include <hal/nrf_lrcconf.h>

static struct k_spinlock global_constlat_lock;
static uint16_t global_constlat_count;

int nrf_sys_event_request_global_constlat(void)
{
	K_SPINLOCK(&global_constlat_lock) {
		if (global_constlat_count == 0) {
#if CONFIG_SOC_NRF54H20_CPUAPP
			nrf_lrcconf_task_trigger(NRF_LRCCONF010,
						 NRF_LRCCONF_TASK_CONSTLAT_ENABLE);
#elif CONFIG_SOC_NRF54H20_CPURAD
			nrf_lrcconf_task_trigger(NRF_LRCCONF000,
						 NRF_LRCCONF_TASK_CONSTLAT_ENABLE);
			nrf_lrcconf_task_trigger(NRF_LRCCONF020,
						 NRF_LRCCONF_TASK_CONSTLAT_ENABLE);
#else
#error "unsupported"
#endif
		}

		global_constlat_count++;
	}

	return 0;
}

int nrf_sys_event_release_global_constlat(void)
{
	K_SPINLOCK(&global_constlat_lock) {
		if (global_constlat_count == 1) {
#if CONFIG_SOC_NRF54H20_CPUAPP
			nrf_lrcconf_task_trigger(NRF_LRCCONF010,
						 NRF_LRCCONF_TASK_CONSTLAT_DISABLE);
#elif CONFIG_SOC_NRF54H20_CPURAD
			nrf_lrcconf_task_trigger(NRF_LRCCONF000,
						 NRF_LRCCONF_TASK_CONSTLAT_DISABLE);
			nrf_lrcconf_task_trigger(NRF_LRCCONF020,
						 NRF_LRCCONF_TASK_CONSTLAT_DISABLE);
#else
#error "unsupported"
#endif
		}

		global_constlat_count--;
	}

	return 0;
}

#else

/*
 * The nrfx power driver already contains an ISR and concurrent access safe reference
 * counting API so we just use it directly when available.
 */

#include <nrfx_power.h>

int nrf_sys_event_request_global_constlat(void)
{
	int err;

	err = nrfx_power_constlat_mode_request();

	return (err == 0 || err == -EALREADY) ? 0 : -EAGAIN;
}

int nrf_sys_event_release_global_constlat(void)
{
	int err;

	err = nrfx_power_constlat_mode_free();

	return (err == 0 || err == -EBUSY) ? 0 : -EAGAIN;
}

#endif

#ifdef CONFIG_NRF_SYS_EVENT_IRQ_LATENCY
BUILD_ASSERT(IS_ENABLED(CONFIG_NRF_SYS_EVENT_IRQ_LATENCY_MANUAL) ||
	     (CONFIG_NRF_SYS_EVENT_GRTC_CHAN_CNT > 0),
	     "If manual mode is not available then at least 1 GRTC channel need to be used.");

static uint32_t event_ref_cnt;
static uint32_t chan_mask;

/* Handle returned by the registering function can be a GRTC channel that was used which indicates
 * that PPI RRAMC wake up is used. If manual mode is used (changing RRAMC power mode) than that
 * handle value is used which exceeds any potential GRTC channel number.
 */
#define NRF_SYS_EVENT_MANUAL_HANDLE 32

#define NVM_HW_WAKEUP_US 16
#define NVM_MANUAL_SUPPORT IS_ENABLED(CONFIG_NRF_SYS_EVENT_IRQ_LATENCY_MANUAL)
/* Due to software performance and risk of waking up too early (then RRAMC may go
 * to sleep before interrupt), it's better to adjust a bit.
 */
#define NVM_WAKEUP_US (NVM_HW_WAKEUP_US - 1)

static void irq_low_latency_on(bool enable)
{
#ifdef RRAMC_POWER_LOWPOWERCONFIG_MODE_Standby
	nrf_rramc_lp_mode_set(NRF_RRAMC, enable ? NRF_RRAMC_LP_STANDBY : NRF_RRAMC_LP_POWER_OFF);
#endif
}

#ifdef CONFIG_ZERO_LATENCY_IRQS
static uint32_t full_irq_lock(void)
{
	uint32_t mcu_critical_state;

	mcu_critical_state = __get_PRIMASK();
	__disable_irq();

	return mcu_critical_state;
}

static void full_irq_unlock(uint32_t mcu_critical_state)
{
	__set_PRIMASK(mcu_critical_state);
}

#define LOCKED(lock) \
	for (uint32_t __tmp = 0, __key = full_irq_lock(); !__tmp; full_irq_unlock(__key), __tmp = 1)
#else
static struct k_spinlock event_lock;
#define LOCKED() K_SPINLOCK(&event_lock)
#endif

union nrf_sys_evt_us {
	uint32_t rel;
	uint64_t abs;
};

static int event_register(union nrf_sys_evt_us us, bool force, bool abs)
{
	int rv;

	LOCKED() {
		if ((CONFIG_NRF_SYS_EVENT_GRTC_CHAN_CNT > 0) &&
		    ((abs == true) || ((us.rel >= NVM_WAKEUP_US) || !NVM_MANUAL_SUPPORT)) &&
		    (chan_mask != 0)) {
			rv = __builtin_ctz(chan_mask);
			chan_mask &= ~BIT(rv);
			if (abs) {
				nrfy_grtc_sys_counter_cc_set(NRF_GRTC, rv, us.abs - NVM_WAKEUP_US);
			} else {
				uint32_t val = (NVM_MANUAL_SUPPORT || (us.rel >= NVM_WAKEUP_US)) ?
					(us.rel - NVM_WAKEUP_US) : 1;

				nrfx_grtc_syscounter_cc_rel_set(rv, val,
								NRFX_GRTC_CC_RELATIVE_SYSCOUNTER);
			}
		} else if ((chan_mask == 0) && (force == false)) {
			rv = -ENOSYS;
		} else {
			if (event_ref_cnt == 0) {
				irq_low_latency_on(true);
			}
			event_ref_cnt++;
			rv = NRF_SYS_EVENT_MANUAL_HANDLE;
		}
	}

	return rv;
}

int nrf_sys_event_register(uint32_t us, bool force)
{
	return event_register((union nrf_sys_evt_us)us, force, false);
}

int nrf_sys_event_abs_register(uint64_t us, bool force)
{
	return event_register((union nrf_sys_evt_us)us, force, true);
}

int nrf_sys_event_unregister(int handle, bool cancel)
{
	__ASSERT_NO_MSG(handle >= 0);
	int rv = 0;

	if (handle != NRF_SYS_EVENT_MANUAL_HANDLE) {
		if (cancel) {
			nrf_grtc_sys_counter_compare_event_disable(NRF_GRTC, handle);
		}
		atomic_or((atomic_t *)&chan_mask, BIT(handle));
		return rv;
	}

	LOCKED() {
		if (IS_ENABLED(CONFIG_TRUSTED_EXECUTION_NONSECURE)) {
			rv = -EINVAL;
		} else {
			__ASSERT_NO_MSG(event_ref_cnt > 0);
			event_ref_cnt--;
			if (event_ref_cnt == 0) {
				irq_low_latency_on(false);
			}
		}
	}

	return rv;
}

#if CONFIG_NRF_SYS_EVENT_GRTC_CHAN_CNT > 0
int nrf_sys_event_init(void)
{
	/* Attempt to allocate requested amount of GRTC channels. */
	for (int i = 0; i < CONFIG_NRF_SYS_EVENT_GRTC_CHAN_CNT; i++) {
		int ch = z_nrf_grtc_timer_chan_alloc();

		if (ch < 0) {
			LOG_WRN("Allocated less GRTC channels (%d) than requested (%d)",
				i, CONFIG_NRF_SYS_EVENT_GRTC_CHAN_CNT);
			break;
		}
		chan_mask |= BIT(ch);
	}

	uint32_t chan_mask_cpy = chan_mask;
	uint32_t tsk = nrf_rramc_task_address_get(NRF_RRAMC, NRF_RRAMC_TASK_WAKEUP);
	bool first = true;
	nrfx_gppi_handle_t ppi_handle;
	nrf_grtc_event_t cc_evt;
	uint32_t evt;
	uint32_t ch;
	int err;

	/* Setup a PPI connection between allocated GRTC channels and RRAMC wake up task. */
	while (chan_mask_cpy) {
		ch = __builtin_ctz(chan_mask_cpy);
		chan_mask_cpy &= ~BIT(ch);
		cc_evt = nrf_grtc_sys_counter_compare_event_get(ch);
		evt = nrf_grtc_event_address_get(NRF_GRTC, cc_evt);
		if (first) {
			first = false;
			err = nrfx_gppi_conn_alloc(evt, tsk, &ppi_handle);
			if (err < 0) {
				return err;
			}
		} else {
			nrfx_gppi_ep_attach(evt, ppi_handle);
		}
	}

	nrfx_gppi_conn_enable(ppi_handle);

	return 0;
}

SYS_INIT(nrf_sys_event_init, PRE_KERNEL_1, 0);
#endif /* CONFIG_NRF_SYS_EVENT_GRTC_CHAN_CNT > 0 */
#endif /* CONFIG_NRF_SYS_EVENT_IRQ_LATENCY */
