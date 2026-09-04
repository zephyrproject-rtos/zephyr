/*
 * Copyright (c) 2026 Deere & Company
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "CriticalSectionManager.h"

/* TODO: we could definitely reduce the scope of this lock */
K_MUTEX_DEFINE(j1939_lock);

void CriticalSection_Lock(void)
{
	k_mutex_lock(&j1939_lock, K_FOREVER);
}

void CriticalSection_Unlock(void)
{
	k_mutex_unlock(&j1939_lock);
}
