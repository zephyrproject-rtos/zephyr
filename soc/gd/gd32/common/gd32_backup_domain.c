/*
 * Copyright (c) 2025 STMicroelectronics
 * Derived from soc/st/stm32/common/stm32_backup_domain.c
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <soc.h>
#include <gd32_backup_domain.h>

LOG_MODULE_REGISTER(gd32_backup_domain, CONFIG_SOC_LOG_LEVEL);

static struct k_spinlock lock;
static size_t refcount;

void gd32_backup_domain_enable_access(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	if (refcount == 0U) {
		PMU_CTL |= PMU_CTL_BKPWEN;
	}
	refcount++;

	k_spin_unlock(&lock, key);
}

void gd32_backup_domain_disable_access(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	if (refcount == 0U) {
		LOG_WRN_ONCE("Unbalanced backup domain access refcount");
	} else {
		if (refcount == 1U) {
			PMU_CTL &= ~PMU_CTL_BKPWEN;
		}
		refcount--;
	}

	k_spin_unlock(&lock, key);
}
