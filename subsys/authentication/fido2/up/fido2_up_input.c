/*
 * Copyright (c) 2026 Siratul Islam <email@sirat.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/input/input.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/authentication/fido2/fido2_up.h>

LOG_MODULE_DECLARE(fido2, CONFIG_FIDO2_LOG_LEVEL);

static K_SEM_DEFINE(up_sem, 0, 1);
static atomic_t up_cancelled;

static void up_input_cb(struct input_event *evt, void *user_data)
{
	ARG_UNUSED(user_data);

	if (evt->sync) {
		k_sem_give(&up_sem);
	}
}

INPUT_CALLBACK_DEFINE(DEVICE_DT_GET(DT_CHOSEN(zephyr_fido2_up_input)), up_input_cb, NULL);

int fido2_up_wait(void)
{
	int ret;

	atomic_clear(&up_cancelled);
	k_sem_reset(&up_sem);

	ret = k_sem_take(&up_sem, K_MSEC(CONFIG_FIDO2_UP_TIMEOUT_MS));
	if (ret) {
		return -ETIMEDOUT;
	}

	if (atomic_get(&up_cancelled)) {
		return -ECANCELED;
	}

	return 0;
}

void fido2_up_cancel(void)
{
	atomic_set(&up_cancelled, 1);
	k_sem_give(&up_sem);
}
