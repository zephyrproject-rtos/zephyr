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

#if CONFIG_SOC_SERIES_NRF54HX

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
	nrfx_err_t err;

	err = nrfx_power_constlat_mode_request();

	return (err == NRFX_SUCCESS || err == NRFX_ERROR_ALREADY) ? 0 : -EAGAIN;
}

int nrf_sys_event_release_global_constlat(void)
{
	nrfx_err_t err;

	err = nrfx_power_constlat_mode_free();

	return (err == NRFX_SUCCESS || err == NRFX_ERROR_BUSY) ? 0 : -EAGAIN;
}

#endif

#ifdef CONFIG_NRF_SYS_EVENT_IRQ_LATENCY
BUILD_ASSERT(IS_ENABLED(CONFIG_NRF_SYS_EVENT_IRQ_LATENCY_MANUAL) ||
	     (CONFIG_NRF_SYS_EVENT_GRTC_CHAN_CNT > 0),
	     "If manual mode is not available then at least 1 GRTC channel need to be used.");

static uint32_t event_ref_cnt;
static uint32_t chan_mask;

#define NVM_HW_WAKEUP_US 16
#define NVM_MANUAL_SUPPORT IS_ENABLED(CONFIG_NRF_SYS_EVENT_IRQ_LATENCY_MANUAL)
/* Due to software performance and risk of waking up too early (then RRAMC may go
 * to sleep before interrupt), it's better to adjust a bit.
 */
#define NVM_WAKEUP_US (NVM_HW_WAKEUP_US - 1)

static void irq_low_latency_on(bool enable)
{
#ifdef RRAMC_POWER_LOWPOWERCONFIG_MODE_Standby
#if NRFX_RELEASE_VER_MAJOR == 4
#error "Update with nrf_rramc_lp_mode_set"
#endif
	NRF_RRAMC->POWER.LOWPOWERCONFIG =
		(enable ? RRAMC_POWER_LOWPOWERCONFIG_MODE_Standby :
			RRAMC_POWER_LOWPOWERCONFIG_MODE_PowerOff) <<
		RRAMC_POWER_LOWPOWERCONFIG_MODE_Pos;
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

int event_register(union nrf_sys_evt_us us, bool force, bool abs)
{
	int rv;

	LOCKED() {
		if ((CONFIG_NRF_SYS_EVENT_GRTC_CHAN_CNT > 0) &&
		    ((abs == false) && ((us.rel >= NVM_WAKEUP_US) || !NVM_MANUAL_SUPPORT)) &&
		    (chan_mask != 0)) {
			rv = __builtin_ctz(chan_mask);
			chan_mask &= ~BIT(rv);
			if (abs) {
				nrfy_grtc_sys_counter_cc_set(NRF_GRTC, rv,
							us.abs - NVM_WAKEUP_US);
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
			rv = 32;
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

	if (handle < 32) {
		if (cancel) {
			nrf_grtc_sys_counter_compare_event_disable(NRF_GRTC, handle);
		}
		atomic_or((atomic_t *)&chan_mask, BIT(handle));
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

	uint8_t ppi_ch;
	uint32_t tsk = nrf_rramc_task_address_get(NRF_RRAMC, NRF_RRAMC_TASK_WAKEUP);
	uint32_t ch = __builtin_ctz(chan_mask);
	nrf_grtc_event_t cc_evt = nrf_grtc_sys_counter_compare_event_get(ch);
	uint32_t evt = nrf_grtc_event_address_get(NRF_GRTC, cc_evt);
	uint32_t grtc_ppi_ch;
	uint32_t chan_mask_cpy = chan_mask & ~BIT(ch);
	nrfx_err_t err;

	/* Setup a PPI connection between one of allocated GRTC channels and RRAMC wake up task. */
	err = nrfx_gppi_channel_alloc(&ppi_ch);
	if (err != NRFX_SUCCESS) {
		return -ENOMEM;
	}

	nrfx_gppi_channel_endpoints_setup(ppi_ch, evt, tsk);
	nrfx_gppi_channels_enable(BIT(ppi_ch));

	/* Get PPI channel used in GRTC channel and set the same channel for other channels so
	 * that all GRTC channels from the pool publish to the same PPI which triggers RRAMC
	 * wake up.
	 */
	grtc_ppi_ch = nrf_grtc_publish_get(NRF_GRTC, cc_evt) & BIT_MASK(30);
	while (chan_mask_cpy) {
		ch = __builtin_ctz(chan_mask_cpy);
		chan_mask_cpy &= ~BIT(ch);
		cc_evt = nrf_grtc_sys_counter_compare_event_get(ch);
		nrf_grtc_publish_set(NRF_GRTC, cc_evt, grtc_ppi_ch);
	}

	return 0;
}

SYS_INIT(nrf_sys_event_init, PRE_KERNEL_1, 0);
#endif /* CONFIG_NRF_SYS_EVENT_GRTC_CHAN_CNT > 0 */
#endif /* CONFIG_NRF_SYS_EVENT_IRQ_LATENCY */
