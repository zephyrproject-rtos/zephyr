/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stm32_ll_pwr.h>
#include <zephyr/spinlock.h>
#include <zephyr/logging/log.h>
#include <stm32_backup_domain.h>
#include <stddef.h>

LOG_MODULE_REGISTER(stm32_backup_domain, CONFIG_SOC_LOG_LEVEL);

#ifdef PWR_BDCR1_DBD3P
#define ENABLE_BKUP_ACCESS	LL_PWR_EnableBkUpD3Access
#define DISABLE_BKUP_ACCESS	LL_PWR_DisableBkUpD3Access
#define IS_ENABLED_BKUP_ACCESS	LL_PWR_IsEnabledBkUpD3Access
#else
#define ENABLE_BKUP_ACCESS	LL_PWR_EnableBkUpAccess
#define DISABLE_BKUP_ACCESS	LL_PWR_DisableBkUpAccess
#define IS_ENABLED_BKUP_ACCESS	LL_PWR_IsEnabledBkUpAccess
#endif

static struct k_spinlock lock;
static size_t refcount;

void stm32_backup_domain_enable_access(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	if (refcount == 0U) {
		ENABLE_BKUP_ACCESS();
		while (!IS_ENABLED_BKUP_ACCESS()) {
		}
	}
	refcount++;

	k_spin_unlock(&lock, key);
}

void stm32_backup_domain_disable_access(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	if (refcount == 0U) {
		LOG_WRN_ONCE("Unbalanced backup domain access refcount");
	} else {
		if (refcount == 1U) {
			DISABLE_BKUP_ACCESS();
		}
		refcount--;
	}

	k_spin_unlock(&lock, key);
}
