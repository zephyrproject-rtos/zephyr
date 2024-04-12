/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/ztest.h>

#define SPI_MODE (SPI_MODE_CPOL | SPI_WORD_SET(8) | SPI_LINES_SINGLE)
#define SPIM_OP	 (SPI_OP_MODE_MASTER | SPI_MODE)
#define SPIS_OP	 (SPI_OP_MODE_SLAVE | SPI_MODE)

static struct spi_dt_spec spim = SPI_DT_SPEC_GET(DT_NODELABEL(dut_spi_dt), SPIM_OP, 0);
static struct spi_dt_spec spis = SPI_DT_SPEC_GET(DT_NODELABEL(dut_spis_dt), SPIS_OP, 0);

#define MEMORY_SECTION(node)                                                                       \
	COND_CODE_1(DT_NODE_HAS_PROP(node, memory_regions),                                        \
		    (__attribute__((__section__(                                                   \
			    LINKER_DT_NODE_REGION_NAME(DT_PHANDLE(node, memory_regions)))))),      \
		    ())

static uint8_t spim_buffer[32] MEMORY_SECTION(DT_NODELABEL(dut_spi));
static uint8_t spis_buffer[32] MEMORY_SECTION(DT_NODELABEL(dut_spis));

struct test_data {
	int spim_alloc_idx;
	int spis_alloc_idx;
	struct spi_buf_set sets[4];
	struct spi_buf_set *mtx_set;
	struct spi_buf_set *mrx_set;
	struct spi_buf_set *stx_set;
	struct spi_buf_set *srx_set;
	struct spi_buf bufs[4];
};

static struct test_data tdata;

/* Allocate buffer from spim or spis space. */
static uint8_t *buf_alloc(size_t len, bool spim)
{
	int *idx = spim ? &tdata.spim_alloc_idx : &tdata.spis_alloc_idx;
	uint8_t *buf = spim ? spim_buffer : spis_buffer;
	size_t total = spim ? sizeof(spim_buffer) : sizeof(spis_buffer);
	uint8_t *rv;

	if (*idx + len > total) {
		zassert_false(true);

		return NULL;
	}

	rv = &buf[*idx];
	*idx += len;

	return rv;
}

ZTEST(spi_error_cases, test_SPI_HALF_DUPLEX_not_supported)
{
	int rv;
	int slave_rv;
	struct spi_dt_spec spim_invalid = spim;
	struct spi_dt_spec spis_invalid = spis;

	spim_invalid.config.operation |= SPI_HALF_DUPLEX;
	spis_invalid.config.operation |= SPI_HALF_DUPLEX;

	rv = spi_transceive_dt(&spim_invalid, tdata.stx_set, tdata.srx_set);
	zassert_equal(rv, -ENOTSUP, "Got %d instead", rv);
	slave_rv = spi_transceive_dt(&spis_invalid, tdata.stx_set, tdata.srx_set);
	zassert_equal(slave_rv, -ENOTSUP, "Got %d instead", slave_rv);
}

ZTEST(spi_error_cases, test_SPI_OP_MODE_invalid)
{
	int rv;
	int slave_rv;
	struct spi_dt_spec spim_invalid = spim;
	struct spi_dt_spec spis_invalid = spis;

	spim_invalid.config.operation |= SPI_OP_MODE_SLAVE;
	spis_invalid.config.operation &= !SPI_OP_MODE_SLAVE;

	/* Check that Operation Mode Slave on spim is not supported */
	rv = spi_transceive_dt(&spim_invalid, tdata.stx_set, tdata.srx_set);
	zassert_equal(rv, -EINVAL, "Got %d instead", rv);
	/* Check that Operation Mode Master on spis is not supported */
	slave_rv = spi_transceive_dt(&spis_invalid, tdata.stx_set, tdata.srx_set);
	zassert_equal(slave_rv, -EINVAL, "Got %d instead", slave_rv);
}

ZTEST(spi_error_cases, test_SPI_MODE_LOOP_not_supported)
{
	int rv;
	int slave_rv;
	struct spi_dt_spec spim_invalid = spim;
	struct spi_dt_spec spis_invalid = spis;

	spim_invalid.config.operation |= SPI_MODE_LOOP;
	spis_invalid.config.operation |= SPI_MODE_LOOP;

	rv = spi_transceive_dt(&spim_invalid, tdata.stx_set, tdata.srx_set);
	zassert_equal(rv, -EINVAL, "Got %d instead", rv);
	slave_rv = spi_transceive_dt(&spis_invalid, tdata.stx_set, tdata.srx_set);
	zassert_equal(slave_rv, -EINVAL, "Got %d instead", slave_rv);
}

ZTEST(spi_error_cases, test_only_SPI_LINES_SINGLE_supported)
{
	int rv;
	int slave_rv;
	struct spi_dt_spec spim_invalid = spim;
	struct spi_dt_spec spis_invalid = spis;

	spim_invalid.config.operation |= SPI_LINES_DUAL;
	spis_invalid.config.operation |= SPI_LINES_DUAL;

	rv = spi_transceive_dt(&spim_invalid, tdata.stx_set, tdata.srx_set);
	zassert_equal(rv, -EINVAL, "Got %d instead", rv);
	slave_rv = spi_transceive_dt(&spis_invalid, tdata.stx_set, tdata.srx_set);
	zassert_equal(slave_rv, -EINVAL, "Got %d instead", slave_rv);

	spim_invalid = spim;
	spis_invalid = spis;
	spim_invalid.config.operation |= SPI_LINES_QUAD;
	spis_invalid.config.operation |= SPI_LINES_QUAD;

	rv = spi_transceive_dt(&spim_invalid, tdata.stx_set, tdata.srx_set);
	zassert_equal(rv, -EINVAL, "Got %d instead", rv);
	slave_rv = spi_transceive_dt(&spis_invalid, tdata.stx_set, tdata.srx_set);
	zassert_equal(slave_rv, -EINVAL, "Got %d instead", slave_rv);

	spim_invalid = spim;
	spis_invalid = spis;
	spim_invalid.config.operation |= SPI_LINES_OCTAL;
	spis_invalid.config.operation |= SPI_LINES_OCTAL;

	rv = spi_transceive_dt(&spim_invalid, tdata.stx_set, tdata.srx_set);
	zassert_equal(rv, -EINVAL, "Got %d instead", rv);
	slave_rv = spi_transceive_dt(&spis_invalid, tdata.stx_set, tdata.srx_set);
	zassert_equal(slave_rv, -EINVAL, "Got %d instead", slave_rv);
}

ZTEST(spi_error_cases, test_only_8BIT_supported)
{
	int rv;
	int slave_rv;
	struct spi_dt_spec spim_invalid = spim;
	struct spi_dt_spec spis_invalid = spis;

	spim_invalid.config.operation |= SPI_WORD_SET(16);
	spis_invalid.config.operation |= SPI_WORD_SET(16);

	rv = spi_transceive_dt(&spim_invalid, tdata.stx_set, tdata.srx_set);
	zassert_equal(rv, -EINVAL, "Got %d instead", rv);
	slave_rv = spi_transceive_dt(&spis_invalid, tdata.stx_set, tdata.srx_set);
	zassert_equal(slave_rv, -EINVAL, "Got %d instead", slave_rv);
}

ZTEST(spi_error_cases, test_unsupported_frequency)
{
	int rv;
	struct spi_dt_spec spim_invalid = spim;

	spim_invalid.config.frequency = 124999;

	rv = spi_transceive_dt(&spim_invalid, tdata.stx_set, tdata.srx_set);
	zassert_equal(rv, -EINVAL, "Got %d instead", rv);
}

ZTEST(spi_error_cases, test_CS_unsupported_on_slave)
{
	int slave_rv;
	struct spi_dt_spec spis_invalid = spis;
	struct gpio_dt_spec test_gpio = { DEVICE_DT_GET(DT_NODELABEL(gpio1)), 10, GPIO_ACTIVE_LOW };

	spis_invalid.config.cs.gpio = test_gpio;

	slave_rv = spi_transceive_dt(&spis_invalid, tdata.stx_set, tdata.srx_set);
	zassert_equal(slave_rv, -EINVAL, "Got %d instead", slave_rv);
}

ZTEST(spi_error_cases, test_spis_scattered_tx_buf_not_supported)
{
	int slave_rv;

	tdata.sets[2].count = 2;
	slave_rv = spi_transceive_dt(&spis, tdata.stx_set, tdata.srx_set);
	zassert_equal(slave_rv, -ENOTSUP, "Got %d instead", slave_rv);
}

ZTEST(spi_error_cases, test_spis_scattered_rx_buf_not_supported)
{
	int slave_rv;

	tdata.sets[3].count = 2;
	slave_rv = spi_transceive_dt(&spis, tdata.stx_set, tdata.srx_set);
	zassert_equal(slave_rv, -ENOTSUP, "Got %d instead", slave_rv);
}

ZTEST(spi_error_cases, test_spis_tx_buf_too_big)
{
	int slave_rv;

	tdata.bufs[2].len = (size_t)65536;
	slave_rv = spi_transceive_dt(&spis, tdata.stx_set, tdata.srx_set);
	zassert_equal(slave_rv, -EINVAL, "Got %d instead", slave_rv);
}

ZTEST(spi_error_cases, test_spis_rx_buf_too_big)
{
	int slave_rv;

	tdata.bufs[3].len = (size_t)65536;
	slave_rv = spi_transceive_dt(&spis, tdata.stx_set, tdata.srx_set);
	zassert_equal(slave_rv, -EINVAL, "Got %d instead", slave_rv);
}

ZTEST(spi_error_cases, test_spis_tx_buf_not_in_ram)
{
	int slave_rv;

	tdata.bufs[2].buf = (void *)0x12345678;
	slave_rv = spi_transceive_dt(&spis, tdata.stx_set, tdata.srx_set);
	zassert_equal(slave_rv, -ENOTSUP, "Got %d instead", slave_rv);
}

static void before(void *not_used)
{
	ARG_UNUSED(not_used);
	size_t len = 16;

	memset(&tdata, 0, sizeof(tdata));
	for (size_t i = 0; i < sizeof(spim_buffer); i++) {
		spim_buffer[i] = (uint8_t)i;
	}
	for (size_t i = 0; i < sizeof(spis_buffer); i++) {
		spis_buffer[i] = (uint8_t)i;
	}

	for (int i = 0; i < 4; i++) {
		tdata.bufs[i].buf = buf_alloc(len, i < 2);
		tdata.bufs[i].len = len;
		tdata.sets[i].buffers = &tdata.bufs[i];
		tdata.sets[i].count = 1;
	}

	tdata.mtx_set = &tdata.sets[0];
	tdata.mrx_set = &tdata.sets[1];
	tdata.stx_set = &tdata.sets[2];
	tdata.srx_set = &tdata.sets[3];
}

static void *suite_setup(void)
{
	return NULL;
}

ZTEST_SUITE(spi_error_cases, NULL, suite_setup, before, NULL, NULL);
