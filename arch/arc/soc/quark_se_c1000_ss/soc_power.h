/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_POWER_H_
#define _SOC_POWER_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Bit 0 from GP0 register is used internally by the kernel
 * to handle PM multicore support. Any change on QMSI and/or
 * bootloader which affects this bit should take it in
 * consideration.
 */
#define GP0_BIT_SLEEP_READY BIT(0)

enum power_states {
	SYS_POWER_STATE_CPU_LPS,       /* SS1 state with Timer ON */
	SYS_POWER_STATE_CPU_LPS_1,     /* SS2 state */
	SYS_POWER_STATE_DEEP_SLEEP,    /* SS2 with LPSS enabled state */
	SYS_POWER_STATE_DEEP_SLEEP_1,  /* SLEEP state */
	SYS_POWER_STATE_DEEP_SLEEP_2,  /* SLEEP state with LPMODE enabled */

	SYS_POWER_STATE_MAX
};

/**
 * @brief Put processor into low power state
 *
 * This function implements the SoC specific details necessary
 * to put the processor into available power states.
 *
 * Wake up considerations:
 * -----------------------
 *
 * SYS_POWER_STATE_CPU_LPS: Any interrupt works as wake event.
 *
 * SYS_POWER_STATE_CPU_LPS_1: Any interrupt works as wake event except
 * the ARC TIMER which is disabled.
 *
 * SYS_POWER_STATE_CPU_LPS_2: SYS_POWER_STATE_DEEP_SLEEP wake events applies.
 *
 * SYS_POWER_STATE_DEEP_SLEEP: Only Always-On peripherals can wake up
 * the SoC. This consists of the Counter, RTC, GPIO 1 and AIO Comparator.
 *
 * SYS_POWER_STATE_DEEP_SLEEP_1: Only Always-On peripherals can wake up
 * the SoC. This consists of the Counter, RTC, GPIO 1 and AIO Comparator.
 *
 * SYS_POWER_STATE_DEEP_SLEEP_2: Only Always-On peripherals can wake up
 * the SoC. This consists of the Counter, RTC, GPIO 1 and AIO Comparator.
 *
 * Considerations around SYS_POWER_STATE_CPU_LPS (LPSS state):
 * -----------------------------------------------------------
 *
 * LPSS is a common power state between the x86 and ARC.
 * When the two applications enter SYS_POWER_STATE_CPU_LPS,
 * the SoC will enter this lower consumption mode.
 * After wake up, this state can only be entered again
 * if the ARC wakes up and transitions again to
 * SYS_POWER_STATE_CPU_LPS. This is not required on the x86 side.
 */
void _sys_soc_set_power_state(enum power_states state);

/**
 * @brief Do any SoC or architecture specific post ops after low power states.
 *
 * This function is a place holder to do any operations that may
 * be needed to be done after deep sleep exits.  Currently it enables
 * interrupts after resuming from deep sleep. In future, the enabling
 * of interrupts may be moved into the kernel.
 */
void _sys_soc_power_state_post_ops(enum power_states state);

#ifdef __cplusplus
}
#endif

#endif /* _SOC_POWER_H_ */
