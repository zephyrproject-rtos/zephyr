/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Core CMSE API
 *
 * CMSE API for Cortex-M23/M33 CPUs.
 */

#ifndef ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_CORTEX_M_CMSE_H_
#define ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_CORTEX_M_CMSE_H_

#ifdef _ASMLANGUAGE

/* nothing */

#else

#include <arm_cmse.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Address information retrieval based on the TT instructions.
 *
 * The TT instructions are used to check the access permissions that different
 * security states and privilege levels have on memory at a specified address
 */

/**
 * @brief Get the MPU region number of an address
 *
 * Return the non-negative MPU region that the address maps to,
 * or -EINVAL to indicate that an invalid MPU region was retrieved.
 *
 * Note:
 * Obtained region is valid only if:
 * - the function is called from privileged mode
 * - the MPU is implemented and enabled
 * - the given address matches a single, enabled MPU region
 *
 * @param addr The address for which the MPU region is requested
 *
 * @return a valid MPU region number or -EINVAL
 */
int arm_cmse_mpu_region_get(uint32_t addr);

/**
 * @brief Read accessibility of an address
 *
 * Evaluates whether a specified memory location can be read according to the
 * permissions of the current state MPU and the specified operation mode.
 *
 * This function shall always return zero:
 * - if executed from an unprivileged mode,
 * - if the address matches multiple MPU regions.
 *
 * @param addr The address for which the readability is requested
 * @param force_npriv Instruct to return the readability of the address
 *                    for unprivileged access, regardless of whether the current
 *                    mode is privileged or unprivileged.
 *
 * @return 1 if address is readable, 0 otherwise.
 */
int arm_cmse_addr_read_ok(uint32_t addr, int force_npriv);

/**
 * @brief Read and Write accessibility of an address
 *
 * Evaluates whether a specified memory location can be read/written according
 * to the permissions of the current state MPU and the specified operation
 * mode.
 *
 * This function shall always return zero:
 * - if executed from an unprivileged mode,
 * - if the address matches multiple MPU regions.
 *
 * @param addr The address for which the RW ability is requested
 * @param force_npriv Instruct to return the RW ability of the address
 *                    for unprivileged access, regardless of whether the current
 *                    mode is privileged or unprivileged.
 *
 * @return 1 if address is Read and Writable, 0 otherwise.
 */
int arm_cmse_addr_readwrite_ok(uint32_t addr, int force_npriv);

/**
 * @brief Read accessibility of an address range
 *
 * Evaluates whether a memory address range, specified by its base address
 * and size, can be read according to the permissions of the current state MPU
 * and the specified operation mode.
 *
 * This function shall always return zero:
 * - if executed from an unprivileged mode,
 * - if the address range overlaps with multiple MPU (and/or SAU/IDAU) regions.
 *
 * @param addr The base address of an address range,
 *             for which the readability is requested
 * @param size The size of the address range
 * @param force_npriv Instruct to return the readability of the address range
 *                    for unprivileged access, regardless of whether the current
 *                    mode is privileged or unprivileged.
 *
 * @return 1 if address range is readable, 0 otherwise.
 */
int arm_cmse_addr_range_read_ok(uint32_t addr, uint32_t size, int force_npriv);

/**
 * @brief Read and Write accessibility of an address range
 *
 * Evaluates whether a memory address range, specified by its base address
 * and size, can be read/written according to the permissions of the current
 * state MPU and the specified operation mode.
 *
 * This function shall always return zero:
 * - if executed from an unprivileged mode,
 * - if the address range overlaps with multiple MPU (and/or SAU/IDAU) regions.
 *
 * @param addr The base address of an address range,
 *             for which the RW ability is requested
 * @param size The size of the address range
 * @param force_npriv Instruct to return the RW ability of the address range
 *                    for unprivileged access, regardless of whether the current
 *                    mode is privileged or unprivileged.
 *
 * @return 1 if address range is Read and Writable, 0 otherwise.
 */
int arm_cmse_addr_range_readwrite_ok(uint32_t addr, uint32_t size, int force_npriv);

/* Required for C99 compilation (required for GCC-8.x version,
 * where typeof is used instead of __typeof__)
 */
#ifndef typeof
#define typeof  __typeof__
#endif

/**
 * @brief Read accessibility of an object
 *
 * Evaluates whether a given object can be read according to the
 * permissions of the current state MPU.
 *
 * The macro shall always evaluate to zero if called from an unprivileged mode.
 *
 * @param p_obj Pointer to the given object
 *              for which the readability is requested
 *
 * @pre Object is allocated in a single MPU (and/or SAU/IDAU) region.
 *
 * @return p_obj if object is readable, NULL otherwise.
 */
#define ARM_CMSE_OBJECT_READ_OK(p_obj) \
	cmse_check_pointed_object(p_obj, CMSE_MPU_READ)

/**
 * @brief Read accessibility of an object (nPRIV mode)
 *
 * Evaluates whether a given object can be read according to the
 * permissions of the current state MPU (unprivileged read).
 *
 * The macro shall always evaluate to zero if called from an unprivileged mode.
 *
 * @param p_obj Pointer to the given object
 *              for which the readability is requested
 *
 * @pre Object is allocated in a single MPU (and/or SAU/IDAU) region.
 *
 * @return p_obj if object is readable, NULL otherwise.
 */
#define ARM_CMSE_OBJECT_UNPRIV_READ_OK(p_obj) \
	cmse_check_pointed_object(p_obj, CMSE_MPU_UNPRIV | CMSE_MPU_READ)

/**
 * @brief Read and Write accessibility of an object
 *
 * Evaluates whether a given object can be read and written
 * according to the permissions of the current state MPU.
 *
 * The macro shall always evaluate to zero if called from an unprivileged mode.
 *
 * @param p_obj Pointer to the given object
 *              for which the read and write ability is requested
 *
 * @pre Object is allocated in a single MPU (and/or SAU/IDAU) region.
 *
 * @return p_obj if object is Read and Writable, NULL otherwise.
 */
#define ARM_CMSE_OBJECT_READWRITE_OK(p_obj) \
	cmse_check_pointed_object(p_obj, CMSE_MPU_READWRITE)

/**
 * @brief Read and Write accessibility of an object (nPRIV mode)
 *
 * Evaluates whether a given object can be read and written according
 * to the permissions of the current state MPU (unprivileged read/write).
 *
 * The macro shall always evaluate to zero if called from an unprivileged mode.
 *
 * @param p_obj Pointer to the given object
 *              for which the read and write ability is requested
 *
 * @pre Object is allocated in a single MPU (and/or SAU/IDAU) region.
 *
 * @return p_obj if object is Read and Writable, NULL otherwise.
 */
#define ARM_CMSE_OBJECT_UNPRIV_READWRITE_OK(p_obj) \
	cmse_check_pointed_object(p_obj, CMSE_MPU_UNPRIV | CMSE_MPU_READWRITE)

#if defined(CONFIG_ARM_SECURE_FIRMWARE)

/**
 * @brief Get the MPU (Non-Secure) region number of an address
 *
 * Return the non-negative MPU (Non-Secure) region that the address maps to,
 * or -EINVAL to indicate that an invalid MPU region was retrieved.
 *
 * Note:
 * Obtained region is valid only if:
 * - the function is called from Secure state
 * - the MPU is implemented and enabled
 * - the given address matches a single, enabled MPU region
 *
 * @param addr The address for which the MPU region is requested
 *
 * @return a valid MPU region number or -EINVAL
  */
int arm_cmse_mpu_nonsecure_region_get(uint32_t addr);

/**
 * @brief Get the SAU region number of an address
 *
 * Return the non-negative SAU (Non-Secure) region that the address maps to,
 * or -EINVAL to indicate that an invalid SAU region was retrieved.
 *
 * Note:
 * Obtained region is valid only if:
 * - the function is called from Secure state
 * - the SAU is implemented and enabled
 * - the given address is not exempt from the secure memory attribution
 *
 * @param addr The address for which the SAU region is requested
 *
 * @return a valid SAU region number or -EINVAL
  */
int arm_cmse_sau_region_get(uint32_t addr);

/**
 * @brief Get the IDAU region number of an address
 *
 * Return the non-negative IDAU (Non-Secure) region that the address maps to,
 * or -EINVAL to indicate that an invalid IDAU region was retrieved.
 *
 * Note:
 * Obtained region is valid only if:
 * - the function is called from Secure state
 * - the IDAU can provide a region number
 * - the given address is not exempt from the secure memory attribution
 *
 * @param addr The address for which the IDAU region is requested
 *
 * @return a valid IDAU region number or -EINVAL
  */
int arm_cmse_idau_region_get(uint32_t addr);

/**
 * @brief Security attribution of an address
 *
 * Evaluates whether a specified memory location belongs to a Secure region.
 * This function shall always return zero if executed from Non-Secure state.
 *
 * @param addr The address for which the security attribution is requested
 *
 * @return 1 if address is Secure, 0 otherwise.
 */
int arm_cmse_addr_is_secure(uint32_t addr);

/**
 * @brief Non-Secure Read accessibility of an address
 *
 * Evaluates whether a specified memory location can be read from Non-Secure
 * state according to the permissions of the Non-Secure state MPU and the
 * specified operation mode.
 *
 * This function shall always return zero:
 * - if executed from Non-Secure state
 * - if the address matches multiple MPU regions.
 *
 * @param addr The address for which the readability is requested
 * @param force_npriv Instruct to return the readability of the address
 *                    for unprivileged access, regardless of whether the current
 *                    mode is privileged or unprivileged.
 *
 * @return 1 if address is readable from Non-Secure state, 0 otherwise.
 */
int arm_cmse_addr_nonsecure_read_ok(uint32_t addr, int force_npriv);

/**
 * @brief Non-Secure Read and Write accessibility of an address
 *
 * Evaluates whether a specified memory location can be read/written from
 * Non-Secure state according to the permissions of the Non-Secure state MPU
 * and the specified operation mode.
 *
 * This function shall always return zero:
 * - if executed from Non-Secure  mode,
 * - if the address matches multiple MPU regions.
 *
 * @param addr The address for which the RW ability is requested
 * @param force_npriv Instruct to return the RW ability of the address
 *                    for unprivileged access, regardless of whether the current
 *                    mode is privileged or unprivileged.
 *
 * @return 1 if address is Read and Writable from Non-Secure state, 0 otherwise
 */
int arm_cmse_addr_nonsecure_readwrite_ok(uint32_t addr, int force_npriv);

/**
 * @brief Non-Secure Read accessibility of an address range
 *
 * Evaluates whether a memory address range, specified by its base address
 * and size, can be read according to the permissions of the Non-Secure state
 * MPU and the specified operation mode.
 *
 * This function shall always return zero:
 * - if executed from Non-Secure  mode,
 * - if the address matches multiple MPU (and/or SAU/IDAU) regions.
 *
 * @param addr The base address of an address range,
 *             for which the readability is requested
 * @param size The size of the address range
 * @param force_npriv Instruct to return the readability of the address range
 *                    for unprivileged access, regardless of whether the current
 *                    mode is privileged or unprivileged.
 *
 * @return 1 if address range is readable, 0 otherwise.
 */
int arm_cmse_addr_range_nonsecure_read_ok(uint32_t addr, uint32_t size,
	int force_npriv);

/**
 * @brief Non-Secure Read and Write accessibility of an address range
 *
 * Evaluates whether a memory address range, specified by its base address
 * and size, can be read and written according to the permissions of the
 * Non-Secure state MPU and the specified operation mode.
 *
 * This function shall always return zero:
 * - if executed from Non-Secure  mode,
 * - if the address matches multiple MPU (and/or SAU/IDAU) regions.
 *
 * @param addr The base address of an address range,
 *             for which Read and Write ability is requested
 * @param size The size of the address range
 * @param force_npriv Instruct to return the readability of the address range
 *                    for unprivileged access, regardless of whether the current
 *                    mode is privileged or unprivileged.
 *
 * @return 1 if address range is readable, 0 otherwise.
 */
int arm_cmse_addr_range_nonsecure_readwrite_ok(uint32_t addr, uint32_t size,
	int force_npriv);

/**
 * @brief Non-Secure Read accessibility of an object
 *
 * Evaluates whether a given object can be read according to the
 * permissions of the Non-Secure state MPU.
 *
 * The macro shall always evaluate to zero if called from Non-Secure state.
 *
 * @param p_obj Pointer to the given object
 *              for which the readability is requested
 *
 * @pre Object is allocated in a single MPU region.
 *
 * @return p_obj if object is readable from Non-Secure state, NULL otherwise.
 */
#define ARM_CMSE_OBJECT_NONSECURE_READ_OK(p_obj) \
	cmse_check_pointed_object(p_obj, CMSE_NONSECURE | CMSE_MPU_READ)

/**
 * @brief Non-Secure Read accessibility of an object (nPRIV mode)
 *
 * Evaluates whether a given object can be read according to the
 * permissions of the Non-Secure state MPU (unprivileged read).
 *
 * The macro shall always evaluate to zero if called from Non-Secure state.
 *
 * @param p_obj Pointer to the given object
 *              for which the readability is requested
 *
 * @pre Object is allocated in a single MPU region.
 *
 * @return p_obj if object is readable from Non-Secure state, NULL otherwise.
 */
#define ARM_CMSE_OBJECT_NONSECURE_UNPRIV_READ_OK(p_obj) \
	cmse_check_pointed_object(p_obj, \
		CMSE_NONSECURE | CMSE_MPU_UNPRIV | CMSE_MPU_READ)

/**
 * @brief Non-Secure Read and Write accessibility of an object
 *
 * Evaluates whether a given object can be read and written
 * according to the permissions of the Non-Secure state MPU.
 *
 * The macro shall always evaluate to zero if called from Non-Secure state.
 *
 * @param p_obj Pointer to the given object
 *              for which the read and write ability is requested
 *
 * @pre Object is allocated in a single MPU region.
 *
 * @return p_obj if object is Non-Secure Read and Writable, NULL otherwise.
 */
#define ARM_CMSE_OBJECT_NONSECURE_READWRITE_OK(p_obj) \
	cmse_check_pointed_object(p_obj, CMSE_NONSECURE | CMSE_MPU_READWRITE)

/**
 * @brief Non-Secure Read and Write accessibility of an object (nPRIV mode)
 *
 * Evaluates whether a given object can be read and written according
 * to the permissions of the Non-Secure state MPU (unprivileged read/write).
 *
 * The macro shall always evaluate to zero if called from Non-Secure state.
 *
 * @param p_obj Pointer to the given object
 *              for which the read and write ability is requested
 *
 * @pre Object is allocated in a single MPU region.
 *
 * @return p_obj if object is Non-Secure Read and Writable, NULL otherwise.
 */
#define ARM_CMSE_OBJECT_NON_SECURE_UNPRIV_READWRITE_OK(p_obj) \
	cmse_check_pointed_object(p_obj, \
			CMSE_NONSECURE | CMSE_MPU_UNPRIV | CMSE_MPU_READWRITE)

#endif /* CONFIG_ARM_SECURE_FIRMWARE */

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_ARM_INCLUDE_AARCH32_CORTEX_M_CMSE_H_ */
