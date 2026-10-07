/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT jedec_spi_nor

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "spi_nor.h"

static const uint8_t jedec_id[] = DT_INST_PROP(0, jedec_id);

static bool busy;
static int busy_polls;
static int rdsr_count;

static int spi_nor_emul_io(const struct emul *target, const struct spi_config *config,
			   const struct spi_buf_set *tx_bufs, const struct spi_buf_set *rx_bufs)
{
	const uint8_t opcode = *(const uint8_t *)tx_bufs->buffers[0].buf;
	uint8_t *data = NULL;
	size_t len = 0;

	ARG_UNUSED(target);
	ARG_UNUSED(config);

	if (rx_bufs != NULL && rx_bufs->count > 1) {
		data = rx_bufs->buffers[1].buf;
		len = rx_bufs->buffers[1].len;
	}

	switch (opcode) {
	case SPI_NOR_CMD_RDID:
		memcpy(data, jedec_id, MIN(len, sizeof(jedec_id)));
		break;
	case SPI_NOR_CMD_RDSR:
		rdsr_count++;
		if (busy && busy_polls > 0) {
			busy_polls--;
			busy = busy_polls > 0;
		}
		data[0] = busy ? SPI_NOR_WIP_BIT : 0;
		break;
	case SPI_NOR_CMD_SE:
		busy = true;
		break;
	default:
		break;
	}

	return 0;
}

static const struct spi_emul_api spi_nor_emul_api = {
	.io = spi_nor_emul_io,
};

static int spi_nor_emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(target);
	ARG_UNUSED(parent);

	return 0;
}

EMUL_DT_INST_DEFINE(0, spi_nor_emul_init, NULL, NULL, &spi_nor_emul_api, NULL);

static const struct device *const flash_dev = DEVICE_DT_INST_GET(0);

static void spi_nor_wait_ready_before(void *fixture)
{
	ARG_UNUSED(fixture);

	busy = false;
	busy_polls = 0;
	rdsr_count = 0;
}

ZTEST(spi_nor_wait_ready, test_erase_completes)
{
	busy_polls = 3;

	zassert_true(device_is_ready(flash_dev));
	zassert_ok(flash_erase(flash_dev, 0, 4096));
	zassert_true(rdsr_count >= 3);
}

ZTEST(spi_nor_wait_ready, test_erase_times_out_when_wip_stays_set)
{
	int64_t start = k_uptime_get();

	zassert_true(device_is_ready(flash_dev));
	zassert_equal(flash_erase(flash_dev, 0, 4096), -ETIMEDOUT);
	zassert_true(k_uptime_get() - start >= CONFIG_SPI_NOR_WAIT_READY_TIMEOUT_MS);
	zassert_true(rdsr_count > 1);
	zassert_true(busy);
}

ZTEST_SUITE(spi_nor_wait_ready, NULL, NULL, spi_nor_wait_ready_before, NULL, NULL);
