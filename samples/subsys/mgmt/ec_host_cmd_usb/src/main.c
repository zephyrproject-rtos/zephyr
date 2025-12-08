/*
 * Copyright 2026 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <sample_usbd.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void)
{
	struct usbd_context *sample_usbd;
	int ret;

	LOG_INF("EC Host Command over USB sample started");

	sample_usbd = sample_usbd_setup_device(NULL);
	if (sample_usbd == NULL) {
		LOG_ERR("Failed to setup USB device");
		return -ENODEV;
	}

	ret = usbd_init(sample_usbd);
	if (ret) {
		LOG_ERR("Failed to initialize device support");
		return ret;
	}

	ret = usbd_enable(sample_usbd);
	if (ret) {
		LOG_ERR("Failed to enable device support");
		return ret;
	}

	k_sleep(K_FOREVER);

	return 0;
}
