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

#ifndef ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_CORTEX_M_TZ_H_
#define ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_CORTEX_M_TZ_H_

#ifdef _ASMLANGUAGE

/* nothing */

#else

#include <arm_cmse.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 *
 * @brief Initial Non-Secure state configuration
 *
 * A convenient struct to include all required Non-Secure
 * state configuration.
 */
typedef struct tz_nonsecure_setup_conf {
	uint32_t msp_ns;
	uint32_t psp_ns;
	uint32_t vtor_ns;
	struct {
		uint32_t npriv:1;
		uint32_t spsel:1;
		uint32_t reserved:30;
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
 */
void tz_nonsecure_state_setup(const tz_nonsecure_setup_conf_t *p_ns_conf);

#if defined(CONFIG_ARMV8_M_MAINLINE)

/**
 *
 * @brief Setup Non-Secure Main Stack Pointer limit register
 *
 * Configure the Non-Secure instance of the MSPLIM register.
 *
 * @param val value to configure the MSPLIM_NS register with.
 *
 * Notes:
 *
 * This function shall only be called from Secure state.
 * Only ARMv8-M Mainline implementations have Non-Secure MSPLIM instance.
 *
 */
void tz_nonsecure_msplim_set(uint32_t val);

/**
 *
 * @brief Setup Non-Secure Process Stack Pointer limit register
 *
 * Configure the Non-Secure instance of the PSPLIM register.
 *
 * @param val value to configure the PSPLIM_NS register with.
 *
 * Notes:
 *
 * This function shall only be called from Secure state.
 * Only ARMv8-M Mainline implementations have Non-Secure PSPLIM instance.
 *
 */
void tz_nonsecure_psplim_set(uint32_t val);

#endif /* CONFIG_ARMV8_M_MAINLINE */

/**
 * @brief Block or permit Non-Secure System Reset Requests
 *
 * Function allows the user to configure the system to block or
 * permit the Non-Secure domain to issue System Reset Requests.
 *
 * @param block Flag indicating whether Non-Secure System Reset
 *                          Requests shall be blocked (1), or permitted (0).
 *
 * Note:
 *
 * This function shall only be called from Secure state.
 */
void tz_nonsecure_system_reset_req_block(int block);

/**
 * @brief Prioritize Secure exceptions
 *
 * Function allows the user to prioritize Secure exceptions over Non-Secure,
 * enabling Secure exception priority boosting.
 *
 * @param secure_boost Flag indicating whether Secure priority boosting
 *                 is desired; select 1 for priority boosting, otherwise 0.
 *
 * Note:
 *
 * This function shall only be called from Secure state.
 */
void tz_nonsecure_exception_prio_config(int secure_boost);

/**
 * @brief Set target state for exceptions not banked between security states
 *
 *  Function sets the security state (Secure or Non-Secure) target
 *  for ARMv8-M HardFault, NMI, and BusFault exception.
 *
 * @param secure_state 1 if target state is Secure, 0 if target state
 *                      is Non-Secure.
 *
 * Secure state: BusFault, HardFault, and NMI are Secure.
 * Non-Secure state: BusFault and NMI are Non-Secure and exceptions can
 * target Non-Secure HardFault.
 *
 * Notes:
 *
 * - This function shall only be called from Secure state.
 * - NMI and BusFault are not banked between security states; they
 * shall either target Secure or Non-Secure state based on user selection.
 * - HardFault exception generated through escalation will target the
 * security state of the original fault before its escalation to HardFault.
 * - If secure_state is set to 1 (Secure), all Non-Secure HardFaults are
 * escalated to Secure HardFaults.
 * - BusFault is present only if the Main Extension is implemented.
 */
void tz_nbanked_exception_target_state_set(int secure_state);

#if defined(CONFIG_ARMV7_M_ARMV8_M_FP)
/**
 * @brief Allow Non-Secure firmware to access the FPU
 *
 * Function allows the Non-Secure firmware to access the Floating Point Unit.
 *
 * Relevant for ARMv8-M MCUs supporting the Floating Point Extension.
 *
 * Note:
 *
 * This function shall only be called from Secure state.
 */
void tz_nonsecure_fpu_access_enable(void);
#endif /* CONFIG_ARMV7_M_ARMV8_M_FP */

/**
 *
 * @brief Configure SAU
 *
 * Configure (enable or disable) the ARMv8-M Security Attribution Unit.
 *
 * @param enable SAU enable flag: 1 if SAU is to be enabled, 0 if SAU is
 *               to be disabled.
 * @param allns SAU_CTRL.ALLNS flag: select 1 to set SAU_CTRL.ALLNS, 0
 *               to clear SAU_CTRL.ALLNS.
 *
 * Notes:
 *
 * SAU_CTRL.ALLNS bit: All Non-secure. When SAU_CTRL.ENABLE is 0
 *  this bit controls if the memory is marked as Non-secure or Secure.
 *  Values:
 *  Secure (not Non-Secure Callable): 0
 *  Non-Secure: 1
 *
 * This function shall only be called from Secure state, otherwise the
 * Non-Secure instance of SAU_CTRL register is RAZ/WI.
 *
 * This function shall be called before the Secure Firmware may transition
 * to Non-Secure state.
 *
 */
void tz_sau_configure(int enable, int allns);

/**
 *
 * @brief Get number of SAU regions
 *
 * Get the number of regions implemented by the Security Attribution Unit,
 * indicated by SAU_TYPE.SREGION (read-only) register field.
 *
 * Notes:
 *
 * The SREGION field reads as an IMPLEMENTATION DEFINED value.
 *
 * This function shall only be called from Secure state, otherwise the
 * Non-Secure instance of SAU_TYPE register is RAZ.
 *
 * @return The number of configured SAU regions.
 */
uint32_t tz_sau_number_of_regions_get(void);

#if defined(CONFIG_CPU_HAS_ARM_SAU)
/**
 *
 * @brief SAU Region configuration
 *
 * A convenient struct to include all required elements
 * for a SAU region configuration.
 */
typedef struct {
	uint8_t region_num;
	uint8_t enable:1;
	uint8_t nsc:1;
	uint32_t base_addr;
	uint32_t limit_addr;
} tz_sau_conf_t;


/**
 *
 * @brief Configure SAU Region
 *
 * Configure an existing ARMv8-M SAU region.
 *
 * @param p_sau_conf pointer to a tz_sau_conf_t structure
 *
 * This function shall only be called from Secure state, otherwise the
 * Non-Secure instances of SAU RNR, RLAR, RBAR registers are RAZ/WI.
 *
 * This function shall be called before the Secure Firmware may transition
 * to Non-Secure state.
 *
 * @return 1 if configuration is successful, otherwise 0.

 */
int tz_sau_region_configure(tz_sau_conf_t *p_sau_conf);

#endif /* CONFIG_CPU_HAS_ARM_SAU */

/**
 * @brief Non-Secure function type
 *
 * Defines a function pointer type to implement a non-secure function call,
 * i.e. a function call that switches state from Secure to Non-secure.
 *
 * Note:
 *
 * A non-secure function call can only happen through function pointers.
 * This is a consequence of separating secure and non-secure code into
 * separate executable files.
 */
typedef void __attribute__((cmse_nonsecure_call)) (*tz_ns_func_ptr_t) (void);

/* Required for C99 compilation (required for GCC-8.x version,
 * where typeof is used instead of __typeof__)
 */
#ifndef typeof
#define typeof  __typeof__
#endif

#if defined(CONFIG_ARM_FIRMWARE_HAS_SECURE_ENTRY_FUNCS)
/**
 * @brief Non-Secure entry function attribute.
 *
 * Declares a non-secure entry function that may be called from Non-Secure
 * or from Secure state using the CMSE _cmse_nonsecure_entry intrinsic.
 *
 * Note:
 *
 * The function must reside in Non-Secure Callable memory region.
 */
#define __TZ_NONSECURE_ENTRY_FUNC \
	__attribute__((cmse_nonsecure_entry, noinline))

#endif /* CONFIG_ARM_FIRMWARE_HAS_SECURE_ENTRY_FUNCS */

/**
 * @brief Declare a pointer of non-secure function type
 *
 * Note:
 *
 * A non-secure function type must only be used as a base type of pointer.
 */
#define TZ_NONSECURE_FUNC_PTR_DECLARE(fptr) tz_ns_func_ptr_t fptr

/**
 * @brief Define a non-secure function pointer
 *
 * A non-secure function pointer is a function pointer that has its LSB unset.
 * The macro uses the CMSE intrinsic: cmse_nsfptr_create(p) to return the
 * value of a pointer with its LSB cleared.
 */
#define TZ_NONSECURE_FUNC_PTR_CREATE(fptr) \
	((tz_ns_func_ptr_t)(cmse_nsfptr_create(fptr)))

/**
 * @brief  Check if pointer can be of non-secure function type
 *
 * A non-secure function pointer is a function pointer that has its LSB unset.
 * The macro uses the CMSE intrinsic: cmse_is_nsfptr(p) to evaluate whether
 * the supplied pointer has its LSB cleared and, thus, can be of non-secure
 * function type.
 *
 * @param fptr supplied pointer to be checked
 *
 * @return non-zero if pointer can be of non-secure function type
 *         (i.e. with LSB unset), zero otherwise.
 */
#define TZ_NONSECURE_FUNC_PTR_IS_NS(fptr) \
	cmse_is_nsfptr(fptr)

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_CORTEX_M_TZ_H_ */
