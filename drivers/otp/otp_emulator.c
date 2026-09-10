/*
 * Copyright (c) 2025 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/math_extras.h>

#define DT_DRV_COMPAT zephyr_otp_emul

static K_MUTEX_DEFINE(otp_emul_lock);

struct otp_emul_config {
	size_t size;
};

#if defined(CONFIG_OTP_PROGRAM)
static int otp_emul_program(const struct device *dev, off_t offset, const void *buf, size_t len)
{
	const struct otp_emul_config *config = dev->config;
	uint8_t *otp_mem = dev->data;
	size_t end;

	if (size_add_overflow(offset, len, &end) || end > config->size) {
		return -EINVAL;
	}

	k_mutex_lock(&otp_emul_lock, K_FOREVER);
	memcpy(&otp_mem[offset], buf, len);
	k_mutex_unlock(&otp_emul_lock);

	return 0;
}
#endif /* CONFIG_OTP_PROGRAM */

static int otp_emul_read(const struct device *dev, off_t offset, void *buf, size_t len)
{
	const struct otp_emul_config *config = dev->config;
	uint8_t *otp_mem = dev->data;
	size_t end;

	if (size_add_overflow(offset, len, &end) || end > config->size) {
		return -EINVAL;
	}

	k_mutex_lock(&otp_emul_lock, K_FOREVER);
	memcpy(buf, &otp_mem[offset], len);
	k_mutex_unlock(&otp_emul_lock);

	return 0;
}

static DEVICE_API(otp, otp_emul_api) = {
#if defined(CONFIG_OTP_PROGRAM)
	.program = otp_emul_program,
#endif
	.read = otp_emul_read,
};

#define OTP_EMU_INIT(n)										\
	static uint8_t otp_emul_mem_##n[DT_INST_PROP_OR(n, size, 0)];				\
												\
	static const struct otp_emul_config otp_emul_##n##_config = {				\
		.size = DT_INST_PROP_OR(n, size, 0),						\
	};											\
												\
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, &otp_emul_mem_##n, &otp_emul_##n##_config,		\
			      PRE_KERNEL_1, CONFIG_OTP_INIT_PRIORITY, &otp_emul_api);

DT_INST_FOREACH_STATUS_OKAY(OTP_EMU_INIT)
