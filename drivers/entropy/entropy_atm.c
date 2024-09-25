/*
 * Copyright (c) 2023-2024 Atmosic
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmosic_atm_trng

#include <zephyr/kernel.h>
#include <zephyr/drivers/entropy.h>
#include <string.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(entropy_atm, CONFIG_ENTROPY_LOG_LEVEL);
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>

#include "arch.h"
#include "at_wrpr.h"
#include "at_apb_pseq_regs_core_macro.h"
#include "rif_regs_core_macro.h"

#define TRNG_INTERNAL_DIRECT_INCLUDE_GUARD
#include "trng_internal.h"

static uint32_t volatile trng_ring[CONFIG_ENTROPY_ATM_RING_ENTRIES];
static uint32_t volatile trng_ring_head;
static uint32_t volatile trng_ring_tail;
static K_SEM_DEFINE(trng_ring_sem, 0, CONFIG_ENTROPY_ATM_RING_ENTRIES);

#define RING_NEXT(_idx) (((_idx) + 1) % CONFIG_ENTROPY_ATM_RING_ENTRIES)
#define RING_EMPTY() (trng_ring_head == trng_ring_tail)
#define RING_FULL() (trng_ring_head == RING_NEXT(trng_ring_tail))
#define RING_ENTRIES() ((trng_ring_tail - trng_ring_head) % CONFIG_ENTROPY_ATM_RING_ENTRIES)
// Potentially called from multiple threads and ISRs - ring access/update needs to be atomic
#define RING_POP() ({ \
	unsigned int key = irq_lock(); \
	uint32_t val = trng_ring[trng_ring_head]; \
	trng_ring_head = RING_NEXT(trng_ring_head); \
	irq_unlock(key); \
	val; \
})
// Only called from trng_handler() - no locking necessary
#define RING_PUSH(_val) do { \
	trng_ring[trng_ring_tail] = (_val); \
	trng_ring_tail = RING_NEXT(trng_ring_tail); \
} while (0)

#ifdef CONFIG_ENTROPY_ATM_STATS
static uint32_t entropy_atm_trng_timeout;
static uint32_t entropy_atm_trng_trouble;
static uint32_t entropy_atm_trng_good;
#endif

static bool trng_radio_locked;

static void trng_radio_lock(void)
{
	if (trng_radio_locked) {
		return;
	}
	trng_radio_locked = true;

#ifdef CONFIG_PM
	pm_policy_state_lock_get(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
#endif

	WRPR_CTRL_PUSH(CMSDK_RIF, WRPR_CTRL__CLK_ENABLE)
	{
		RIF_MODE_CNTL__RADIO_EN__SET(CMSDK_RIF->MODE_CNTL);
		RIF_MODE_CNTL__RADIO_EN_OVR__SET(CMSDK_RIF->MODE_CNTL);
	}
	WRPR_CTRL_POP();

	WRPR_CTRL_PUSH(CMSDK_PSEQ, WRPR_CTRL__CLK_ENABLE)
	{
		PSEQ_CTRL0__RADIO_EN_I_SRC__SET(CMSDK_PSEQ->CTRL0);
	}
	WRPR_CTRL_POP();
}

static void trng_radio_unlock(void)
{
	if (!trng_radio_locked) {
		return;
	}
	trng_radio_locked = false;

#ifdef CONFIG_PM
	pm_policy_state_lock_put(PM_STATE_SUSPEND_TO_RAM, PM_ALL_SUBSTATES);
#endif

	WRPR_CTRL_PUSH(CMSDK_PSEQ, WRPR_CTRL__CLK_ENABLE)
	{
		PSEQ_CTRL0__RADIO_EN_I_SRC__CLR(CMSDK_PSEQ->CTRL0);
	}
	WRPR_CTRL_POP();

	WRPR_CTRL_PUSH(CMSDK_RIF, WRPR_CTRL__CLK_ENABLE)
	{
		RIF_MODE_CNTL__RADIO_EN_OVR__CLR(CMSDK_RIF->MODE_CNTL);
		RIF_MODE_CNTL__RADIO_EN__CLR(CMSDK_RIF->MODE_CNTL);
	}
	WRPR_CTRL_POP();
}

static void trng_force_go_pulse(void)
{
	trng_radio_lock();
	trng_internal_force_go_pulse();
}

static int entropy_atm_get_entropy(const struct device *dev,
				   uint8_t *buffer, uint16_t length)
{
	ARG_UNUSED(dev);

	while (length) {
		if (k_sem_take(&trng_ring_sem, K_NO_WAIT)) {
			// Make sure HW sequence is atomic
			unsigned int key = irq_lock();
			trng_force_go_pulse();
			irq_unlock(key);

			if (k_is_in_isr()) {
				return -EAGAIN;
			}
			int err = k_sem_take(&trng_ring_sem, K_MSEC(30));
			if (err) {
#ifdef CONFIG_ENTROPY_ATM_STATS
				entropy_atm_trng_timeout++;
#endif
				return err;
			}
		}
		ASSERT_ERR(!RING_EMPTY());

		uint32_t value = RING_POP();
		size_t to_copy = MIN(length, sizeof(value));

		memcpy(buffer, &value, to_copy);
		buffer += to_copy;
		length -= to_copy;
	}

	return 0;
}

static int entropy_atm_get_entropy_isr(const struct device *dev,
				       uint8_t *buffer, uint16_t length,
				       uint32_t flags)
{
	ARG_UNUSED(dev);

	uint16_t cnt = length;

	while (length) {
		if (k_sem_take(&trng_ring_sem, K_NO_WAIT)) {
			// Make sure HW sequence is atomic
			unsigned int key = irq_lock();
			trng_force_go_pulse();
			irq_unlock(key);
			while (k_sem_take(&trng_ring_sem, K_NO_WAIT)) {
				if (!(flags & ENTROPY_BUSYWAIT)) {
					// Return whatever data is available
					return cnt - length;
				}
				YIELD();
			}
		}
		ASSERT_ERR(!RING_EMPTY());

		uint32_t value = RING_POP();
		size_t to_copy = MIN(length, sizeof(value));

		memcpy(buffer, &value, to_copy);
		buffer += to_copy;
		length -= to_copy;
	}

	return cnt - length;
}

static void trng_handler(void)
{
	uint32_t status = CMSDK_TRNG->INTERRUPT_STATUS;
	if (status & TRNG_INTERRUPT_STATUS__TRNG_TROUBLE__MASK) {
#ifdef CONFIG_ENTROPY_ATM_STATS
		entropy_atm_trng_trouble++;
#endif
		// Reset TRNG to clear rf_busy
		WRPR_CTRL_SET(CMSDK_TRNG, WRPR_CTRL__SRESET);
		trng_internal_config();
		goto done;
	}

	if (status & TRNG_INTERRUPT_STATUS__TRNG_READY__MASK) {
#ifdef CONFIG_ENTROPY_ATM_STATS
		entropy_atm_trng_good++;
#endif
		// FIXME: trng_internal_clear_synth_override();
		if (RING_FULL()) {
			LOG_WRN("RFULL! status:0x%" PRIx32 " ctrl:0x%" PRIx32, status,
				CMSDK_TRNG->CONTROL);
		} else {
			RING_PUSH(CMSDK_TRNG->TRNG);
			k_sem_give(&trng_ring_sem);
		}
	}

#ifdef __RIF_TRNG_CONF_MACRO__
	trng_internal_set_radio_warmup_cnt(false);
#endif

	CMSDK_TRNG->RESET_INTERRUPT = status;
	CMSDK_TRNG->RESET_INTERRUPT = 0;

done:
	if (RING_ENTRIES() < (CONFIG_ENTROPY_ATM_RING_ENTRIES / 2)) {
		trng_force_go_pulse();
	} else {
		bool notfull = !RING_FULL();
		TRNG_CONTROL__LAUNCH_ON_RADIO_UP__MODIFY(CMSDK_TRNG->CONTROL, notfull);
		trng_radio_unlock();
	}
}

static void trng_init(void)
{
	trng_internal_config();

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), trng_handler, DEVICE_DT_INST_GET(0),
		    0);
	irq_enable(DT_INST_IRQN(0));

	trng_force_go_pulse();
}

#ifdef __MDM_DCCAL_CTRL_MACRO__
static void trng_dccal_complete(void)
{
	MDM_IRQM__DCCAL_DONE__CLR(CMSDK_MDM->IRQM);
	irq_disable(MDM_IRQn);

	ASSERT_ERR(MDM_TIA_RETENT_DCCALRESULTS__DONE__READ(CMSDK_MDM->TIA_RETENT_DCCALRESULTS));

	trng_internal_dccal_complete();

	trng_init();
}

static void trng_dccal_init(void)
{
	if (trng_internal_dccal_init()) {
		trng_init();
		return;
	}

	MDM_IRQC__DCCAL_DONE__SET(CMSDK_MDM->IRQC);
	MDM_IRQC__DCCAL_DONE__CLR(CMSDK_MDM->IRQC);

	IRQ_CONNECT(MDM_IRQn, IRQ_PRI_NORMAL, trng_dccal_complete, 0, 0);
	irq_enable(MDM_IRQn);
	MDM_IRQM__DCCAL_DONE__SET(CMSDK_MDM->IRQM);
}
#endif

#if defined(__RIF_TRNG_CONF_MACRO__) && defined(CONFIG_PM)
static void entropy_atm_notify_pm_state_exit(enum pm_state state)
{
	if (state != PM_STATE_SUSPEND_TO_RAM) {
		return;
	}

	if (trng_internal_go_pulse_needed()) {
		trng_internal_set_radio_warmup_cnt(true);
		trng_internal_force_go_pulse();
	}
}

static struct pm_notifier entropy_atm_pm_notifier = {
	.state_exit = entropy_atm_notify_pm_state_exit,
};
#endif

static int entropy_atm_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	trng_internal_constructor();
#ifdef __MDM_DCCAL_CTRL_MACRO__
	trng_dccal_init();
#else
	trng_init();
#endif

#if defined(__RIF_TRNG_CONF_MACRO__) && defined(CONFIG_PM)
	pm_notifier_register(&entropy_atm_pm_notifier);
#endif

	return 0;
}

static const struct entropy_driver_api entropy_atm_api = {
	.get_entropy = entropy_atm_get_entropy,
	.get_entropy_isr = entropy_atm_get_entropy_isr
};

DEVICE_DT_INST_DEFINE(0, entropy_atm_init,
		      NULL, NULL, NULL,
		      PRE_KERNEL_2, CONFIG_ENTROPY_INIT_PRIORITY,
		      &entropy_atm_api);
