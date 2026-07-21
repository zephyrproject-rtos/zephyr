/*
 * Copyright (c) 2026 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM64 Pointer Authentication (PAC) internal APIs
 *
 * Internal kernel APIs for PAC key management. Not for application use.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM64_PAC_H_
#define ZEPHYR_INCLUDE_ARCH_ARM64_PAC_H_

/** @cond INTERNAL_HIDDEN */

#include <zephyr/types.h>

/* Forward declaration */
struct k_thread;

/**
 * @brief PAC key structure
 *
 * Each PAC key is 128 bits, stored as high and low 64-bit values.
 */
struct pac_key {
	uint64_t lo;  /* Low 64 bits of key */
	uint64_t hi;  /* High 64 bits of key */
};

/**
 * @brief PAC key set for a thread/process
 *
 * Contains all 5 PAC keys used for pointer authentication.
 * These keys provide isolation between threads/processes.
 *
 * The union allows efficient initialization of all keys with a single
 * sys_rand_get() call while maintaining individual key access.
 */
struct pac_keys {
	union {
		struct {
			struct pac_key apia;  /* Instruction Pointer Authentication Key A */
			struct pac_key apib;  /* Instruction Pointer Authentication Key B */
			struct pac_key apda;  /* Data Pointer Authentication Key A */
			struct pac_key apdb;  /* Data Pointer Authentication Key B */
			struct pac_key apga;  /* Generic Authentication Key */
		};
		uint8_t raw_bytes[5 * 2 * 8];  /* 5 keys × 2 (hi/lo) × 8 bytes = 80 bytes */
	};
};

/**
 * @brief Generate random PAC keys
 *
 * @param keys Pointer to pac_keys structure to populate
 */
void z_arm64_pac_keys_generate(struct pac_keys *keys);

/**
 * @brief Save current PAC keys from hardware registers
 *
 * @param keys Pointer to pac_keys structure to store keys
 */
void z_arm64_pac_keys_save(struct pac_keys *keys);

/**
 * @brief Restore PAC keys to hardware registers
 *
 * @param keys Pointer to pac_keys structure containing keys to restore
 */
void z_arm64_pac_keys_restore(const struct pac_keys *keys);

/**
 * @brief Handle PAC key context switching between threads
 *
 * Internal function called during thread context switches.
 *
 * @param new_thread Pointer to the thread being switched to
 * @param old_thread Pointer to the thread being switched from
 */
void z_arm64_pac_thread_context_switch(struct k_thread *new_thread, struct k_thread *old_thread);

/** @endcond */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_PAC_H_ */
