/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief TrustZone API
 *
 * TrustZone API for Cortex-M23/M33 CPUs implementing the Security Extension.
 */

#ifndef _ARM_CORTEXM_TZ__H_
#define _ARM_CORTEXM_TZ__H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASMLANGUAGE

/* nothing */

#else

#include <stdint.h>

/**
 *
 * @brief Initial Non-Secure state configuration
 *
 * A convenient struct to include all required Non-Secure
 * state configuration.
 */
typedef struct tz_nonsecure_setup_conf {
	u32_t msp_ns;
	u32_t psp_ns;
	u32_t vtor_ns;
	struct {
		u32_t npriv:1;
		u32_t spsel:1;
		u32_t reserved:30;
	} control_ns;
} tz_nonsecure_setup_conf_t;


/**
 *
 * @brief Setup Non-Secure state core registers
 *
 * Configure the Non-Secure instances of the VTOR, MSP, PSP,
 * and CONTROL register.
 *
 * @param p_ns_conf Pointer to a structure holding the desired configuration.
 *
 * Notes:
 *
 * This function shall only be called from Secure state, otherwise the
 * Non-Secure instances of the core registers are RAZ/WI.
 *
 * This function shall be called before the Secure Firmware may transition
 * to Non-Secure state.
 *
 * @return N/A
 */
void tz_nonsecure_state_setup(const tz_nonsecure_setup_conf_t *p_ns_conf);

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _ARM_CORTEXM_TZ__H_ */
