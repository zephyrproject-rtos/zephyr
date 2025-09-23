/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/logging/log.h>

#include <stm32_backup_domain.h>

#include <app_conf.h>
#include <bsp.h>

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(linklayer_plat_adapt);

#define RADIO_INTR_PRIO_HIGH_Z (RADIO_INTR_PRIO_HIGH + _IRQ_PRIO_OFFSET)
#define RADIO_INTR_PRIO_LOW_Z (RADIO_INTR_PRIO_LOW + _IRQ_PRIO_OFFSET)

/* 2.4GHz RADIO ISR callbacks */
typedef void (*radio_isr_cb_t) (void);

radio_isr_cb_t radio_callback;
radio_isr_cb_t low_isr_callback;

extern const struct device *rng_dev;

/* Radio critical sections */
volatile int32_t prio_high_isr_counter;
volatile int32_t prio_low_isr_counter;
volatile int32_t prio_sys_isr_counter;
volatile uint32_t local_basepri_value;
volatile uint8_t irq_counter;
static uint32_t primask_bit;

/* Radio SW low ISR global variable */
volatile uint8_t radio_sw_low_isr_is_running_high_prio;


void LINKLAYER_PLAT_DelayUs(uint32_t delay)
{
	k_busy_wait(delay);
}

void LINKLAYER_PLAT_GetRNG(uint8_t *ptr_rnd, uint32_t len)
{
	int ret;

	/* Read 32-bit random values from HW driver */
	ret = entropy_get_entropy_isr(rng_dev, (char *)ptr_rnd, len, 0);
	if (ret < 0) {
		LOG_ERR("Error: entropy_get_entropy failed: %d", ret);
	}
	LOG_DBG("n %d, val: %p", len, (void *)ptr_rnd);
}

void LINKLAYER_PLAT_SetupRadioIT(void (*intr_cb)())
{
	radio_callback = intr_cb;
}

void LINKLAYER_PLAT_SetupSwLowIT(void (*intr_cb)())
{
	low_isr_callback = intr_cb;
}

void radio_high_prio_isr(void)
{
	radio_callback();

	HAL_RCCEx_DisableRequestUponRadioWakeUpEvent();

	__ISB();

	ISR_DIRECT_PM();
}

void radio_low_prio_isr(void)
{
	irq_disable(RADIO_SW_LOW_INTR_NUM);

	low_isr_callback();

	/* Check if nested SW radio low interrupt has been requested*/
	if (radio_sw_low_isr_is_running_high_prio != 0) {
		NVIC_SetPriority((IRQn_Type) RADIO_SW_LOW_INTR_NUM, RADIO_INTR_PRIO_LOW);
		radio_sw_low_isr_is_running_high_prio = 0;
	}

	/* Re-enable SW radio low interrupt */
	irq_enable(RADIO_SW_LOW_INTR_NUM);

	ISR_DIRECT_PM();
}


void link_layer_register_isr(void)
{
	ARM_IRQ_DIRECT_DYNAMIC_CONNECT(RADIO_INTR_NUM, 0, 0, reschedule);

	/* Ensure the IRQ is disabled before enabling it at run time */
	irq_disable(RADIO_INTR_NUM);

	irq_connect_dynamic(RADIO_INTR_NUM, RADIO_INTR_PRIO_HIGH_Z,
			    (void (*)(const void *))radio_high_prio_isr, NULL, 0);

	irq_enable(RADIO_INTR_NUM);

	ARM_IRQ_DIRECT_DYNAMIC_CONNECT(RADIO_SW_LOW_INTR_NUM, 0, 0, reschedule);

	/* Ensure the IRQ is disabled before enabling it at run time */
	irq_disable(RADIO_SW_LOW_INTR_NUM);

	irq_connect_dynamic(RADIO_SW_LOW_INTR_NUM, RADIO_SW_LOW_INTR_PRIO,
			    (void (*)(const void *))radio_low_prio_isr, NULL, 0);

	irq_enable(RADIO_SW_LOW_INTR_NUM);
}


void LINKLAYER_PLAT_TriggerSwLowIT(uint8_t priority)
{
	uint8_t low_isr_priority = RADIO_INTR_PRIO_LOW_Z;

	LOG_DBG("Priority: %d", priority);

	/* Check if a SW low interrupt as already been raised.
	 * Nested call far radio low isr are not supported
	 **/
	if (NVIC_GetActive((IRQn_Type)RADIO_SW_LOW_INTR_NUM) == 0) {
		/* No nested SW low ISR, default behavior */
		if (priority == 0) {
			low_isr_priority = RADIO_SW_LOW_INTR_PRIO;
		}
		NVIC_SetPriority((IRQn_Type)RADIO_SW_LOW_INTR_NUM, low_isr_priority);
	} else {
		/* Nested call detected */
		/* No change for SW radio low interrupt priority for the moment */
		if (priority != 0) {
			/* At the end of current SW radio low ISR, this pending SW
			 * low interrupt will run with RADIO_INTR_PRIO_LOW_Z priority
			 **/
			radio_sw_low_isr_is_running_high_prio = 1;
		}
	}

	NVIC_SetPendingIRQ((IRQn_Type)RADIO_SW_LOW_INTR_NUM);
}

void LINKLAYER_PLAT_Assert(uint8_t condition)
{
	__ASSERT_NO_MSG(condition);
}

void LINKLAYER_PLAT_EnableIRQ(void)
{
	if (irq_counter > 0) {
		irq_counter--;
		if (irq_counter == 0) {
			__set_PRIMASK(primask_bit);
		}
	}
}

void LINKLAYER_PLAT_DisableIRQ(void)
{
	if (irq_counter == 0) {
		/* Save primask bit at first time interrupts disabling */
		primask_bit = __get_PRIMASK();
	}
	__disable_irq();
	irq_counter++;
}

void LINKLAYER_PLAT_EnableSpecificIRQ(uint8_t isr_type)
{

	LOG_DBG("isr_type: %d", isr_type);

	if ((isr_type & LL_HIGH_ISR_ONLY) != 0) {
		prio_high_isr_counter--;
		if (prio_high_isr_counter == 0) {
			irq_enable(RADIO_INTR_NUM);
		}
	}

	if ((isr_type & LL_LOW_ISR_ONLY) != 0) {
		prio_low_isr_counter--;
		if (prio_low_isr_counter == 0) {
			irq_enable(RADIO_SW_LOW_INTR_NUM);
		}
	}

	if ((isr_type & SYS_LOW_ISR) != 0) {
		prio_sys_isr_counter--;
		if (prio_sys_isr_counter == 0) {
			__set_BASEPRI(local_basepri_value);
		}
	}
}

void LINKLAYER_PLAT_DisableSpecificIRQ(uint8_t isr_type)
{

	LOG_DBG("isr_type: %d", isr_type);

	if ((isr_type & LL_HIGH_ISR_ONLY) != 0) {
		prio_high_isr_counter++;
		if (prio_high_isr_counter == 1) {
			irq_disable(RADIO_INTR_NUM);
		}
	}

	if ((isr_type & LL_LOW_ISR_ONLY) != 0) {
		prio_low_isr_counter++;
		if (prio_low_isr_counter == 1) {
			irq_disable(RADIO_SW_LOW_INTR_NUM);
		}
	}

	if ((isr_type & SYS_LOW_ISR) != 0) {
		prio_sys_isr_counter++;
		if (prio_sys_isr_counter == 1) {
			local_basepri_value = __get_BASEPRI();
			__set_BASEPRI_MAX(RADIO_INTR_PRIO_LOW_Z << 4);
		}
	}
}

void LINKLAYER_PLAT_EnableRadioIT(void)
{
	LOG_DBG("Enable RADIO IRQ");
	irq_enable(RADIO_INTR_NUM);
}

void LINKLAYER_PLAT_DisableRadioIT(void)
{
	LOG_DBG("Disable RADIO IRQ");
	irq_disable(RADIO_INTR_NUM);
}

void LINKLAYER_PLAT_StartRadioEvt(void)
{
	__HAL_RCC_RADIO_CLK_SLEEP_ENABLE();

	NVIC_SetPriority((IRQn_Type)RADIO_INTR_NUM, RADIO_INTR_PRIO_HIGH_Z);
}

void LINKLAYER_PLAT_StopRadioEvt(void)
{
	__HAL_RCC_RADIO_CLK_SLEEP_DISABLE();

	NVIC_SetPriority((IRQn_Type)RADIO_INTR_NUM, RADIO_INTR_PRIO_LOW_Z);
}

/* Link Layer notification for RCO calibration start */
void LINKLAYER_PLAT_RCOStartClbr(void)
{
/* Required only for RCO module usage in the context of LSI2 calibration */
}

/* Link Layer notification for RCO calibration end */
void LINKLAYER_PLAT_RCOStopClbr(void)
{
/* Required only for RCO module usage in the context of LSI2 calibration */
}

void LINKLAYER_PLAT_SCHLDR_TIMING_UPDATE_NOT(Evnt_timing_t *p_evnt_timing) {}

void LINKLAYER_PLAT_EnableOSContextSwitch(void)
{
	/* No implementation is needed. However, this function may be used to notify
	 * upper layers that the link layer has just finished a critical radio job
	 * (radio channels' calibration).
	 **/
}

void LINKLAYER_PLAT_DisableOSContextSwitch(void)
{
	/* No implementation is needed. However, this function may be used to notify upper layers
	 * that the link layer is running a critical radio job (radio channels' calibration);
	 * A sequence of radio ISRs will appear in the next few milli seconds.
	 **/
}

void LINKLAYER_PLAT_EnableBackupDomainAccess(void)
{
	stm32_backup_domain_enable_access();
}

void LINKLAYER_PLAT_DisableBackupDomainAccess(void)
{
	stm32_backup_domain_disable_access();
}
