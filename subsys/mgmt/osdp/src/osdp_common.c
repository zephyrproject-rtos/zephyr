/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>

#include <device.h>
#include <sys/crc.h>
#include <logging/log.h>

#include "osdp_common.h"

LOG_MODULE_DECLARE(osdp, CONFIG_OSDP_LOG_LEVEL);

void osdp_dump(const char *head, uint8_t *buf, int len)
{
	LOG_HEXDUMP_DBG(buf, len, head);
}

uint16_t osdp_compute_crc16(const uint8_t *buf, size_t len)
{
	return crc16_itu_t(0x1D0F, buf, len);
}

int64_t osdp_millis_now(void)
{
	return (int64_t) k_uptime_get();
}

int64_t osdp_millis_since(int64_t last)
{
	int64_t tmp = last;

	return (int64_t) k_uptime_delta(&tmp);
}

struct osdp_cmd *osdp_cmd_alloc(struct osdp_pd *pd)
{
	struct osdp_cmd *cmd = NULL;

	if (k_mem_slab_alloc(&pd->cmd.slab, (void **)&cmd, K_MSEC(100))) {
		LOG_ERR("Memory allocation time-out");
		return NULL;
	}
	return cmd;
}

void osdp_cmd_free(struct osdp_pd *pd, struct osdp_cmd *cmd)
{
	k_mem_slab_free(&pd->cmd.slab, (void **)&cmd);
}
