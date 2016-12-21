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
	SYS_POWER_STATE_CPU_LPS,       /* C2LP state */
	SYS_POWER_STATE_CPU_LPS_1,     /* C2 state */
	SYS_POWER_STATE_CPU_LPS_2,     /* C1 state */
	SYS_POWER_STATE_DEEP_SLEEP,    /* DEEP SLEEP state */
	SYS_POWER_STATE_DEEP_SLEEP_1,  /* SLEEP state */
	SYS_POWER_STATE_DEEP_SLEEP_2,  /* Multicore support for
					* DEEP_SLEEP state.
					*/
	SYS_POWER_STATE_MAX
};

/**
 * @brief Put processor into low power state
 *
 * This function implements the SoC specific details necessary
 * to put the processor into available power states.
 *
 * Wake up considerations:
 * SYS_POWER_STATE_CPU_LPS_2: Any interrupt works as wake event.
 *
 * SYS_POWER_STATE_CPU_LPS_1: Any interrupt works as wake event except
 * if the core enters LPSS where SYS_POWER_STATE_DEEP_SLEEP wake events
 * applies.
 *
 * SYS_POWER_STATE_CPU_LPS: Any interrupt works as wake event except the
 * PIC timer which is gated. If the core enters LPSS only
 * SYS_POWER_STATE_DEEP_SLEEP wake events applies.
 *
 * SYS_POWER_STATE_DEEP_SLEEP: Only Always-On peripherals can wake up
 * the SoC. This consists of the Counter, RTC, GPIO 1 and AIO Comparator.
 *
 * SYS_POWER_STATE_DEEP_SLEEP_1: Only Always-On peripherals can wake up
 * the SoC. This consists of the Counter, RTC, GPIO 1 and AIO Comparator.
 *
 * SYS_POWER_STATE_DEEP_SLEEP_2: Only Always-On peripherals can wake up
 * the SoC. This consists of the Counter, RTC, GPIO 1 and AIO Comparator.
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

/**
 * @brief Check if ARC core is ready to enter in DEEP_SLEEP states.
 *
 * @retval true If ARC is ready.
 * @retval false Otherwise.
 */
bool _sys_soc_power_state_is_arc_ready(void);

#ifdef __cplusplus
}
#endif

#endif /* _SOC_POWER_H_ */
