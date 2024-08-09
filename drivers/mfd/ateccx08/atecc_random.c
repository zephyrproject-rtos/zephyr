/*
 * Copyright (c) 2024 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "atecc_priv.h"
LOG_MODULE_DECLARE(ateccx08);

#define RANDOM_SEED_UPDATE    0x00U
#define RANDOM_NO_SEED_UPDATE 0x01U
#define RANDOM_NUM_SIZE       32U
#define RANDOM_RSP_SIZE       35U

int atecc_random(const struct device *dev, uint8_t *rand_out, bool update_seed)
{
	__ASSERT(dev != NULL, "device is NULL");
	__ASSERT(rand_out != NULL, "rand_out is NULL");

	struct ateccx08_packet packet = {0};
	int ret;

	packet.param1 = (update_seed ? RANDOM_SEED_UPDATE : RANDOM_NO_SEED_UPDATE);

	atecc_command(ATECC_RANDOM, &packet);

	ret = atecc_execute_command_pm(dev, &packet);
	if (ret < 0) {
		LOG_ERR("%s: failed: %d", __func__, ret);
		return ret;
	}

	if (packet.data[ATECC_COUNT_IDX] != RANDOM_RSP_SIZE) {
		LOG_ERR("Wrong response size");
		return -EBADMSG;
	}

	memcpy(rand_out, &packet.data[ATECC_RSP_DATA_IDX], RANDOM_NUM_SIZE);

	return 0;
}
