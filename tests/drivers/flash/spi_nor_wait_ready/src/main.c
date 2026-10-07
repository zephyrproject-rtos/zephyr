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

#define NOR_SHORT   DT_NODELABEL(nor_short)
#define NOR_LONG    DT_NODELABEL(nor_long)
#define NOR_DEFAULT DT_NODELABEL(nor_default)

#define SHORT_TIMEOUT_MS   DT_PROP(NOR_SHORT, wait_ready_timeout_ms)
#define LONG_TIMEOUT_MS    DT_PROP(NOR_LONG, wait_ready_timeout_ms)
#define DEFAULT_TIMEOUT_MS 1500000

#define TIMEOUT_MARGIN_MS (2 * CONFIG_SPI_NOR_SLEEP_ERASE_MS)

static const uint8_t jedec_id[] = DT_PROP(NOR_SHORT, jedec_id);

static bool busy;
static int busy_polls;
static int rdsr_count;
static int64_t clear_at_ms;

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
		if (clear_at_ms != 0 && k_uptime_get() >= clear_at_ms) {
			busy = false;
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

#define SPI_NOR_EMUL_DEFINE(inst)                                                                  \
	EMUL_DT_INST_DEFINE(inst, spi_nor_emul_init, NULL, NULL, &spi_nor_emul_api, NULL);

DT_INST_FOREACH_STATUS_OKAY(SPI_NOR_EMUL_DEFINE)

static const struct device *const nor_short = DEVICE_DT_GET(NOR_SHORT);
static const struct device *const nor_long = DEVICE_DT_GET(NOR_LONG);
static const struct device *const nor_default = DEVICE_DT_GET(NOR_DEFAULT);

static void assert_erase_times_out_after(const struct device *dev, int64_t timeout_ms)
{
	int64_t start = k_uptime_get();
	int64_t elapsed;

	zassert_true(device_is_ready(dev));
	zassert_equal(flash_erase(dev, 0, 4096), -ETIMEDOUT);

	elapsed = k_uptime_get() - start;
	zassert_true(elapsed >= timeout_ms, "timed out after %lld ms", (long long)elapsed);
	zassert_true(elapsed <= timeout_ms + TIMEOUT_MARGIN_MS, "timed out after %lld ms",
		     (long long)elapsed);
	zassert_true(rdsr_count > 1);
	zassert_true(busy);
}

static void spi_nor_wait_ready_before(void *fixture)
{
	ARG_UNUSED(fixture);

	busy = false;
	busy_polls = 0;
	rdsr_count = 0;
	clear_at_ms = 0;
}

ZTEST(spi_nor_wait_ready, test_erase_completes)
{
	busy_polls = 3;

	zassert_true(device_is_ready(nor_short));
	zassert_ok(flash_erase(nor_short, 0, 4096));
	zassert_true(rdsr_count >= 3);
}

ZTEST(spi_nor_wait_ready, test_erase_times_out_when_wip_stays_set)
{
	assert_erase_times_out_after(nor_short, SHORT_TIMEOUT_MS);
}

ZTEST(spi_nor_wait_ready, test_timeout_is_per_instance)
{
	assert_erase_times_out_after(nor_long, LONG_TIMEOUT_MS);
}

ZTEST(spi_nor_wait_ready, test_default_timeout)
{
	assert_erase_times_out_after(nor_default, DEFAULT_TIMEOUT_MS);
}

ZTEST(spi_nor_wait_ready, test_erase_completes_when_wip_clears_at_deadline)
{
	int64_t start = k_uptime_get();

	clear_at_ms = start + SHORT_TIMEOUT_MS;

	zassert_true(device_is_ready(nor_short));
	zassert_ok(flash_erase(nor_short, 0, 4096));
	zassert_true(k_uptime_get() - start >= SHORT_TIMEOUT_MS);
}

ZTEST_SUITE(spi_nor_wait_ready, NULL, NULL, spi_nor_wait_ready_before, NULL, NULL);
