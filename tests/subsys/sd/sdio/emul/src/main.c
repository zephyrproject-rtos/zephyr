/*
 * Copyright 2026 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sdhc/sdhc_emul.h>
#include <zephyr/sd/sdio.h>
#include <zephyr/sd/sd_spec.h>

static const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(sdhc0));
static struct sd_card card;
static struct sdio_func func1;

static void sdio_up(void)
{
	zassert_true(device_is_ready(dev), "SDHC emulator not ready");
	zassert_ok(sd_init(dev, &card));
	func1.num = SDIO_FUNC_NUM_1;
	func1.card = &card;
	func1.block_size = 64;
	func1.cis.max_blk_size = 512;
}

ZTEST(sdio_emul, test_fbr_function1)
{
	uint8_t val = 0xFF;

	sdio_up();
	zassert_ok(sdio_read_byte(&func1, SDIO_FBR_BASE(1), &val));
	zassert_equal(val, 0x01, "FBR standard interface code mismatch");
}

ZTEST(sdio_emul, test_sdio_invalid_function_boundary)
{
	struct sdio_func bad = {0};
	uint8_t val;

	sdio_up();
	bad.num = 3;
	bad.card = &card;
	zassert_not_equal(sdio_read_byte(&bad, 0, &val), 0,
			  "CMD52 with invalid func should fail");
}

ZTEST(sdio_emul, test_fifo_readback)
{
	uint8_t wbuf[64] = "mock_udp_payload";
	uint8_t rbuf[64];

	sdio_up();
	zassert_ok(sdio_write_fifo(&func1, 0, wbuf, sizeof(wbuf)));
	zassert_ok(sdio_read_fifo(&func1, 0, rbuf, sizeof(rbuf)));
	zassert_mem_equal(wbuf, rbuf, sizeof(wbuf));
}

/* CMD52 write/read of a CCCR byte init never touches. */
ZTEST(sdio_emul, test_cccr_bus_if_write_readback)
{
	struct sdio_func func0 = {
		.num = SDIO_FUNC_NUM_0,
		.card = &card,
	};
	uint8_t val;

	sdio_up();
	zassert_ok(sdio_write_byte(&func0, SDIO_CCCR_BUS_IF, SDIO_CCCR_BUS_IF_WIDTH_4_BIT));
	zassert_ok(sdio_read_byte(&func0, SDIO_CCCR_BUS_IF, &val));
	zassert_equal(val & SDIO_CCCR_BUS_IF_WIDTH_MASK, SDIO_CCCR_BUS_IF_WIDTH_4_BIT,
		      "CCCR bus-width readback mismatch");
}

/* CMD53 that walks past the 4096-byte function buffer. */
ZTEST(sdio_emul, test_cmd53_out_of_bounds)
{
	uint8_t wbuf[64];

	sdio_up();
	zassert_not_equal(sdio_write_addr(&func1, 4090, wbuf, sizeof(wbuf)), 0,
			  "Write past end of SDIO function buffer should fail");
}

static int irq_fire_count;
static int irq_last_reason;

static void sdio_irq_cb(const struct device *irq_dev, int reason, const void *user_data)
{
	ARG_UNUSED(irq_dev);
	ARG_UNUSED(user_data);
	irq_last_reason = reason;
	irq_fire_count++;
}

/* Enable, fire, disable. Callback must stop after disable. */
ZTEST(sdio_emul, test_sdio_irq_delivery)
{
	sdio_up();
	irq_fire_count = 0;
	irq_last_reason = 0;

	zassert_ok(sdhc_enable_interrupt(dev, sdio_irq_cb, SDHC_INT_SDIO, NULL));
	sdhc_emul_trigger_sdio_irq(dev, SDIO_FUNC_NUM_1);
	zassert_equal(irq_fire_count, 1, "SDIO IRQ callback did not fire");
	zassert_equal(irq_last_reason, SDHC_INT_SDIO, "Wrong IRQ reason reported");

	zassert_ok(sdhc_disable_interrupt(dev, SDHC_INT_SDIO));
	sdhc_emul_trigger_sdio_irq(dev, SDIO_FUNC_NUM_1);
	zassert_equal(irq_fire_count, 1, "IRQ fired after being disabled");
}

ZTEST_SUITE(sdio_emul, NULL, NULL, NULL, NULL, NULL);
