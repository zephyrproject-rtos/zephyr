/*
 * Copyright (c) 2016-2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include "hal/cpu.h"
#include "hal/rand.h"

K_MUTEX_DEFINE(mutex_rand);

int bt_rand(void *buf, size_t len)
{
	while (len) {
		k_mutex_lock(&mutex_rand, K_FOREVER);
		len = rand_get(len, buf);
		k_mutex_unlock(&mutex_rand);
		if (len) {
			cpu_sleep();
		}
	}

	return 0;
}
