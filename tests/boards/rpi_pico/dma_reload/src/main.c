/*
 * Copyright (c) 2026 Robin Sachsenweger Ballantyne <makenenjoy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/misc/pio_rpi_pico/pio_rpi_pico.h>
#include <stdlib.h>
#include <zephyr/drivers/dma.h>

#if defined(CONFIG_SOC_SERIES_RP2040) /* include RPI_PICO_DMA_DREQ_TO_SLOT */
#include <zephyr/dt-bindings/dma/rpi-pico-dma-rp2040.h>
#elif defined(CONFIG_SOC_SERIES_RP2350)
#include <zephyr/dt-bindings/dma/rpi-pico-dma-rp2350.h>
#endif

/**
 * @file
 * @brief Verify calling dma_reload, followed by dma_start for repeated DMA transfers.
 * @details
 * - Test Steps
 *   -# Perform a DMA transfer to PIO loopback
 *   -# Read out data from PIO ISR
 *   -# Perform another DMA transfer with different data using either:
 *        - dma_reload(...); dma_start(...); or (test: test_dma_reload_start)
 *        - dma_reload(...)                     (test: test_dma_reload)
 *   -# Read out data from PIO ISR.
 * - Expected Results
 *   -# The data read out from PIO ISR in both cases
 *      is equal to what was sent.
 *   -# The callback is run after the transfer ends.
 */

RPI_PICO_PIO_DEFINE_PROGRAM(loopback_pio, 0, 1,
			    /* .wrap_target */
			    0x6020, /*  0: out    x, 32 */
			    0x4020, /*  1: in     x, 32 */
			    /* .wrap */
);

const struct device *dma = DEVICE_DT_GET(DT_NODELABEL(tst_dma0));

struct dma_pico_reload_suite_fixture {
	PIO pio;
	size_t sm;
	struct k_sem callback_triggered_sem;
};

static int loopback_program_init(PIO pio, size_t sm)
{
	uint32_t offset;

	if (!pio_can_add_program(pio, RPI_PICO_PIO_GET_PROGRAM(loopback_pio))) {
		return -EBUSY;
	}

	offset = pio_add_program(pio, RPI_PICO_PIO_GET_PROGRAM(loopback_pio));
	pio_sm_config sm_config = pio_get_default_sm_config();

	sm_config_set_wrap(&sm_config, offset + RPI_PICO_PIO_GET_WRAP_TARGET(loopback_pio),
			   offset + RPI_PICO_PIO_GET_WRAP(loopback_pio));

	sm_config_set_out_shift(&sm_config, false, true, 32);
	sm_config_set_in_shift(&sm_config, false, true, 32);
	pio_sm_init(pio, sm, offset, &sm_config);
	pio_sm_set_enabled(pio, sm, true);
	return 0;
}

static int pio_loopback_init(PIO *pio, size_t *sm)
{
	const struct device *piodev = DEVICE_DT_GET(DT_NODELABEL(loopback_pio));
	int retval;
	*pio = pio_rpi_pico_get_pio(piodev);

	retval = pio_rpi_pico_allocate_sm(piodev, sm);
	if (retval < 0) {
		TC_PRINT("pio_rpi_pico_allocate_sm failed with ret = %d", retval);
		return retval;
	}

	retval = loopback_program_init(*pio, *sm);
	if (retval < 0) {
		TC_PRINT("loopback_program_init failed with ret = %d", retval);
		return retval;
	}

	return 0;
}

static void test_done(const struct device *dma_dev, void *f, uint32_t id, int status)
{
	struct dma_pico_reload_suite_fixture *fixture = (struct dma_pico_reload_suite_fixture *)f;

	k_sem_give(&fixture->callback_triggered_sem);
}

#define DMA_CHAN_ID        0
#define DMA_DATA_ALIGNMENT DT_PROP_OR(DT_NODELABEL(tst_dma0), dma_buf_addr_alignment, 32)
#define TEST_BUF_SIZE      (48)
__aligned(DMA_DATA_ALIGNMENT) char tx_data[TEST_BUF_SIZE] =
	"It is harder to be kind than to be wise........";

__aligned(DMA_DATA_ALIGNMENT) char tx_data1[TEST_BUF_SIZE] =
	"iT IS HARDER TO BE KIND THAN TO BE WISE,,,,,,,,";

static int setup_dma(struct dma_pico_reload_suite_fixture *fixture)
{
	PIO pio = fixture->pio;
	size_t sm = fixture->sm;

	struct dma_config dma_cfg = {0};
	struct dma_block_config dma_block_cfg = {0};

	if (!device_is_ready(dma)) {
		return -EBUSY;
	}

	dma_cfg.channel_direction = MEMORY_TO_PERIPHERAL;
	dma_cfg.source_data_size = 4U;
	dma_cfg.dest_data_size = 4U;
	dma_cfg.source_burst_length = 1;
	dma_cfg.dest_burst_length = 1;
	dma_cfg.dma_callback = test_done;
	dma_cfg.user_data = (void *)fixture;
	dma_cfg.complete_callback_en = 0U;
	dma_cfg.error_callback_dis = 0U;
	dma_cfg.block_count = 1U;
	dma_cfg.head_block = &dma_block_cfg;
	dma_cfg.dma_slot = RPI_PICO_DMA_DREQ_TO_SLOT(pio_get_dreq(pio, sm, true));

	TC_PRINT("Preparing DMA Controller: Name=%s, Chan_ID=%u\n", dma->name, dma_cfg.dma_slot);

	dma_block_cfg.block_size = TEST_BUF_SIZE;
	dma_block_cfg.source_address = (uint32_t)tx_data;
	dma_block_cfg.dest_address = (uint32_t)&pio->txf[sm];
	dma_block_cfg.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	dma_block_cfg.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;

	return dma_config(dma, DMA_CHAN_ID, &dma_cfg);
}

static void *setup_tests(void)
{
	struct dma_pico_reload_suite_fixture *fixture =
		malloc(sizeof(struct dma_pico_reload_suite_fixture));

	zassert_ok(pio_loopback_init(&fixture->pio, &fixture->sm));
	k_sem_init(&fixture->callback_triggered_sem, 0, 1);
	return fixture;
}

static void reset_pio(void *f)
{
	struct dma_pico_reload_suite_fixture *fixture = (struct dma_pico_reload_suite_fixture *)f;

	zassert_ok(setup_dma(fixture));

	pio_sm_restart(fixture->pio, fixture->sm);

	/* reset semaphore */
	k_sem_take(&fixture->callback_triggered_sem, K_MSEC(0));
}

static void free_fixture(void *f)
{
	free(f);
}

ZTEST_SUITE(dma_pico_reload_suite, NULL, setup_tests, reset_pio, NULL, free_fixture);

ZTEST_F(dma_pico_reload_suite, test_dma_reload_start)
{
	PIO pio = fixture->pio;
	size_t sm = fixture->sm;

	TC_PRINT("Starting the transfer\n");
	zassert_ok(dma_start(dma, DMA_CHAN_ID));

	for (int i = 0; i < TEST_BUF_SIZE / 4; i++) {
		uint32_t rx_data = pio_sm_get_blocking(pio, sm);

		uint32_t *slice_tx = (uint32_t *)tx_data;

		zassert_equal(slice_tx[i], rx_data);
	}

	zassert_ok(k_sem_take(&fixture->callback_triggered_sem, K_MSEC(500)));

	TC_PRINT("Restarting the transfer with new data\n");

	zassert_ok(dma_reload(dma, DMA_CHAN_ID, (uint32_t)tx_data1, (uint32_t)&pio->txf[sm],
			      TEST_BUF_SIZE));

	zassert_ok(dma_start(dma, DMA_CHAN_ID));

	for (int i = 0; i < TEST_BUF_SIZE / 4; i++) {
		uint32_t rx_data = pio_sm_get_blocking(pio, sm);

		char *slice = (char *)&rx_data;
		uint32_t *slice_tx = (uint32_t *)tx_data1;

		char str[5] = {slice[0], slice[1], slice[2], slice[3], '\0'};

		TC_PRINT("data = %x rx_data = %x str = %s\n", slice_tx[i], rx_data, str);
		zassert_equal(slice_tx[i], rx_data);
	}

	zassert_ok(k_sem_take(&fixture->callback_triggered_sem, K_MSEC(500)),
		   "Callback did not trigger.");
}

ZTEST_F(dma_pico_reload_suite, test_dma_reload)
{
	PIO pio = fixture->pio;
	size_t sm = fixture->sm;

	TC_PRINT("Starting the transfer\n");
	zassert_ok(dma_start(dma, DMA_CHAN_ID));

	for (int i = 0; i < TEST_BUF_SIZE / 4; i++) {
		uint32_t rx_data = pio_sm_get_blocking(pio, sm);

		uint32_t *slice_tx = (uint32_t *)tx_data;

		zassert_equal(slice_tx[i], rx_data);
	}

	zassert_ok(k_sem_take(&fixture->callback_triggered_sem, K_MSEC(500)),
		   "Callback did not trigger.");

	TC_PRINT("Restarting the transfer with new data\n");

	zassert_ok(dma_reload(dma, DMA_CHAN_ID, (uint32_t)tx_data1, (uint32_t)&pio->txf[sm],
			      TEST_BUF_SIZE));

	for (int i = 0; i < TEST_BUF_SIZE / 4; i++) {
		uint32_t rx_data = pio_sm_get_blocking(pio, sm);

		char *slice = (char *)&rx_data;
		uint32_t *slice_tx = (uint32_t *)tx_data1;

		char str[5] = {slice[0], slice[1], slice[2], slice[3], '\0'};

		TC_PRINT("data = %x rx_data = %x str = %s\n", slice_tx[i], rx_data, str);
		zassert_equal(slice_tx[i], rx_data);
	}

	zassert_ok(k_sem_take(&fixture->callback_triggered_sem, K_MSEC(500)),
		   "Callback did not trigger.");
}

/* Trivial test to simply demonstrate that the PIO loopback program is functional. */
ZTEST_F(dma_pico_reload_suite, test_pio_loopback)
{
	PIO pio = fixture->pio;
	size_t sm = fixture->sm;

	uint32_t tx_word = 0x89ABCDEAU;

	for (int i = 0; i < 10; i++) {
		pio_sm_put_blocking(pio, sm, tx_word);

		uint32_t rx_word = pio_sm_get_blocking(pio, sm);

		zassert_equal(rx_word, tx_word);

		tx_word++;
	}
}
