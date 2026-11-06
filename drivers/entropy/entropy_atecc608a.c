/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_atecc608a_trng

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/drivers/mfd/atecc608a.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(entropy_atecc608a, CONFIG_ENTROPY_LOG_LEVEL);

/* Random opcode and 32-byte payload, ATECC608A datasheet. */
#define ATECC608A_OP_RANDOM          0x1BU
#define ATECC608A_RANDOM_LEN         32U
#define ATECC608A_RANDOM_MODE_NOSEED 0x01U
#define ATECC608A_RANDOM_WAIT        K_MSEC(30)
#define ATECC608A_RANDOM_TIMEOUT     K_MSEC(100)

struct entropy_atecc608a_config {
	const struct device *mfd;
};

static int entropy_atecc608a_get_entropy(const struct device *dev, uint8_t *buffer, uint16_t length)
{
	const struct entropy_atecc608a_config *cfg = dev->config;
	uint8_t rnd[ATECC608A_RANDOM_LEN];
	uint16_t remaining = length;
	int ret;

	if (buffer == NULL || length == 0U) {
		return -EINVAL;
	}

	while (remaining > 0U) {
		uint16_t chunk;
		bool test_pattern = true;

		ret = mfd_atecc608a_execute(cfg->mfd, ATECC608A_OP_RANDOM,
					    ATECC608A_RANDOM_MODE_NOSEED, 0U, NULL, 0U, rnd,
					    sizeof(rnd), ATECC608A_RANDOM_WAIT,
					    ATECC608A_RANDOM_TIMEOUT);
		if (ret < 0) {
			LOG_ERR("Random failed (%d)", ret);
			return ret;
		}

		/* Unlocked config zone returns FF FF 00 00... (datasheet Random). */
		for (size_t i = 0U; i < sizeof(rnd); i += 4U) {
			if ((rnd[i] != 0xffU) || (rnd[i + 1U] != 0xffU) ||
			    (rnd[i + 2U] != 0x00U) || (rnd[i + 3U] != 0x00U)) {
				test_pattern = false;
				break;
			}
		}

		if (test_pattern) {
			LOG_ERR("Random test pattern; lock the configuration zone first");
			return -EIO;
		}

		chunk = MIN(remaining, (uint16_t)sizeof(rnd));
		memcpy(buffer, rnd, chunk);
		buffer += chunk;
		remaining -= chunk;
	}

	return 0;
}

static int entropy_atecc608a_init(const struct device *dev)
{
	const struct entropy_atecc608a_config *cfg = dev->config;

	if (!device_is_ready(cfg->mfd)) {
		LOG_ERR("ATECC608A MFD device not ready");
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(entropy, entropy_atecc608a_api) = {
	.get_entropy = entropy_atecc608a_get_entropy,
};

#define ENTROPY_ATECC608A_DEFINE(inst)                                                             \
	static const struct entropy_atecc608a_config entropy_atecc608a_cfg_##inst = {              \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                        \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, entropy_atecc608a_init, NULL, NULL,                            \
			      &entropy_atecc608a_cfg_##inst, POST_KERNEL,                          \
			      CONFIG_ENTROPY_ATECC608A_INIT_PRIORITY, &entropy_atecc608a_api);

DT_INST_FOREACH_STATUS_OKAY(ENTROPY_ATECC608A_DEFINE)
