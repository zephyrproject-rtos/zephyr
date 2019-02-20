/*
 * Copyright (c) 2019 Benjamin Valentin
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <power.h>
#include <soc.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

static inline void sam0_idle(uint8_t mode)
{
	SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
	PM->SLEEP.reg = mode;
	__DSB();
	__WFI();
}

static inline void sam0_deep_sleep(void)
{
	SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
	__DSB();
	__WFI();
}

/*
 * Errata 1.14.2: In Standby, Idle1, and Idle2 Sleep modes, the device may not
 *    (13140)     wake from sleep.
 *
 * Workaround: The SLEEPPRM bits in the NVMCTRL.CTRLB register must be written
 *             to 3 (NVMCTRL -CTRLB.bit.SLEEPPRM = 3) to ensure correct
 *             operation of the device.
 *             The average power consumption of the device will increase by 20 μA.
 */
static inline void _samd21_errata(void)
{
#if defined(CONFIG_SOC_SERIES_SAMD21) || defined(CONFIG_SOC_SERIES_SAMR21)
	if (DSU->DID.bit.REVISION <= 0x2)
		NVMCTRL->CTRLB.bit.SLEEPPRM = NVMCTRL_CTRLB_SLEEPPRM_DISABLED_Val;
#endif
}

/* Invoke Low Power/System Off specific Tasks */
void sys_set_power_state(enum power_states state)
{
	switch (state) {

#ifdef CONFIG_HAS_SYS_POWER_STATE_SLEEP_1
	case SYS_POWER_STATE_SLEEP_1:

		sam0_idle(0);

		break;
#endif
#ifdef CONFIG_HAS_SYS_POWER_STATE_SLEEP_2
	case SYS_POWER_STATE_SLEEP_2:

		_samd21_errata();
		sam0_idle(1);

		break;
#endif
#ifdef CONFIG_HAS_SYS_POWER_STATE_SLEEP_3
	case SYS_POWER_STATE_SLEEP_3:

		_samd21_errata();
		sam0_idle(2);

		break;
#endif

#ifdef CONFIG_HAS_SYS_POWER_STATE_DEEP_SLEEP_1
	case SYS_POWER_STATE_DEEP_SLEEP_1:

		_samd21_errata();
		sam0_deep_sleep();

		break;
#endif

	default:
		LOG_ERR("Unsupported power state %u", state);
		break;
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
void sys_power_state_post_ops(enum power_states state)
{
	/*
	 * System is now in active mode. Reenable interrupts which were disabled
	 * when OS started idling code.
	 */
	irq_unlock(0);
}
