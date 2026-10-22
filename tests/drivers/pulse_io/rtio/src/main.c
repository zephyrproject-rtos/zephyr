/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/pulse_io.h>
#include <zephyr/rtio/rtio.h>
#include <string.h>

#include "loopback_iodev.h"

static const struct device *pio_dev = DEVICE_DT_GET(DT_NODELABEL(pulse_io0));

/* One bit-time unit in ticks. */
#define T 4U

/*
 * A WS2812-like bit template: zero = short high then long low, one =
 * long high then short low. pulse_symbol is {duration, level}.
 */
static const struct pulse_io_bit_template tmpl = {
	.zero = {{T, 1}, {3 * T, 0}},
	.one = {{3 * T, 1}, {T, 0}},
	.msb_first = true,
};

RTIO_DEFINE(pio_rtio, 4, 4);

/*
 * A minimal iodev that routes submissions to the pulse_io device's own
 * submit op via pulse_io_iodev_submit, exercising the device-backed RTIO
 * path (what a real backend like RMT implements).
 */
static void dev_iodev_submit(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct device *d = ((const struct rtio_iodev *)iodev_sqe->sqe.iodev)->data;

	pulse_io_iodev_submit(d, iodev_sqe);
}

static const struct rtio_iodev_api dev_iodev_api = {
	.submit = dev_iodev_submit,
};

static struct rtio_iodev dev_iodev = {
	.api = &dev_iodev_api,
	.data = (void *)DEVICE_DT_GET(DT_NODELABEL(pulse_io0)),
};

/*
 * End-to-end RTIO round-trip: encode bytes into symbols, submit them on
 * the loopback iodev as a TX, submit an RX that gets the same symbols
 * back, then decode and compare. This exercises the proposed RTIO
 * integration shape (encoder/decoder vtables + submit) with no hardware.
 */
ZTEST(pulse_io_rtio, test_encode_submit_decode_roundtrip)
{
	const struct pulse_io_encoder_api *enc;
	const struct pulse_io_decoder_api *dec;

	zassert_ok(pulse_io_get_encoder(pio_dev, &enc));
	zassert_ok(pulse_io_get_decoder(pio_dev, &dec));
	struct rtio_iodev *iodev = pulse_io_loopback_iodev();

	const uint8_t msg[] = {'H', 'i'};
	struct pulse_symbol syms[sizeof(msg) * 8 * 2];
	struct pulse_symbol rx_syms[sizeof(msg) * 8 * 2];
	uint8_t decoded[sizeof(msg)];
	size_t produced = 0;
	int rc;

	zassert_not_null(enc);
	zassert_not_null(dec);
	zassert_not_null(iodev);

	rc = enc->encode(&tmpl, msg, sizeof(msg), syms, ARRAY_SIZE(syms), &produced);
	zassert_ok(rc, "encode failed: %d", rc);
	zassert_equal(produced, sizeof(msg) * 8 * 2, "wrong symbol count");

	struct rtio_sqe *tx = rtio_sqe_acquire(&pio_rtio);

	zassert_not_null(tx);
	rtio_sqe_prep_write(tx, iodev, RTIO_PRIO_NORM, (const uint8_t *)syms,
			    produced * sizeof(struct pulse_symbol), NULL);
	rc = rtio_submit(&pio_rtio, 1);
	zassert_ok(rc, "tx submit failed: %d", rc);

	struct rtio_cqe *tx_cqe = rtio_cqe_consume(&pio_rtio);

	zassert_not_null(tx_cqe);
	zassert_ok(tx_cqe->result, "tx result: %d", tx_cqe->result);
	rtio_cqe_release(&pio_rtio, tx_cqe);

	struct rtio_sqe *rx = rtio_sqe_acquire(&pio_rtio);

	zassert_not_null(rx);
	rtio_sqe_prep_read(rx, iodev, RTIO_PRIO_NORM, (uint8_t *)rx_syms, sizeof(rx_syms), NULL);
	rc = rtio_submit(&pio_rtio, 1);
	zassert_ok(rc, "rx submit failed: %d", rc);

	struct rtio_cqe *rx_cqe = rtio_cqe_consume(&pio_rtio);

	zassert_not_null(rx_cqe);
	zassert_true(rx_cqe->result > 0, "rx result: %d", rx_cqe->result);
	rtio_cqe_release(&pio_rtio, rx_cqe);

	produced = 0;
	rc = dec->decode(&tmpl, T - 1U, rx_syms, ARRAY_SIZE(rx_syms), decoded, sizeof(decoded),
			 &produced);
	zassert_ok(rc, "decode failed: %d", rc);
	zassert_equal(produced, sizeof(msg), "wrong byte count");
	zassert_mem_equal(decoded, msg, sizeof(msg), "payload mismatch");
}

/*
 * Chained transaction: submit TX then RX as one chain (TX flagged
 * RTIO_SQE_CHAINED) in a single rtio_submit. This is the pattern a
 * protocol like one-wire or IR uses to drive a write then read without
 * nested callbacks, the main reason to adopt RTIO.
 */
ZTEST(pulse_io_rtio, test_chained_tx_then_rx)
{
	const struct pulse_io_encoder_api *enc;
	const struct pulse_io_decoder_api *dec;

	zassert_ok(pulse_io_get_encoder(pio_dev, &enc));
	zassert_ok(pulse_io_get_decoder(pio_dev, &dec));
	struct rtio_iodev *iodev = pulse_io_loopback_iodev();

	const uint8_t msg[] = {'O', 'k'};
	struct pulse_symbol syms[sizeof(msg) * 8 * 2];
	struct pulse_symbol rx_syms[sizeof(msg) * 8 * 2];
	uint8_t decoded[sizeof(msg)];
	size_t produced = 0;
	int rc;

	rc = enc->encode(&tmpl, msg, sizeof(msg), syms, ARRAY_SIZE(syms), &produced);
	zassert_ok(rc, "encode failed: %d", rc);

	struct rtio_sqe *tx = rtio_sqe_acquire(&pio_rtio);

	zassert_not_null(tx);
	rtio_sqe_prep_write(tx, iodev, RTIO_PRIO_NORM, (const uint8_t *)syms,
			    produced * sizeof(struct pulse_symbol), NULL);
	tx->flags |= RTIO_SQE_CHAINED;

	struct rtio_sqe *rx = rtio_sqe_acquire(&pio_rtio);

	zassert_not_null(rx);
	rtio_sqe_prep_read(rx, iodev, RTIO_PRIO_NORM, (uint8_t *)rx_syms, sizeof(rx_syms), NULL);

	rc = rtio_submit(&pio_rtio, 2);
	zassert_ok(rc, "chain submit failed: %d", rc);

	struct rtio_cqe *cqe = rtio_cqe_consume(&pio_rtio);

	zassert_not_null(cqe);
	zassert_ok(cqe->result, "tx in chain failed: %d", cqe->result);
	rtio_cqe_release(&pio_rtio, cqe);

	cqe = rtio_cqe_consume(&pio_rtio);
	zassert_not_null(cqe);
	zassert_true(cqe->result > 0, "rx in chain failed: %d", cqe->result);
	rtio_cqe_release(&pio_rtio, cqe);

	produced = 0;
	rc = dec->decode(&tmpl, T - 1U, rx_syms, ARRAY_SIZE(rx_syms), decoded, sizeof(decoded),
			 &produced);
	zassert_ok(rc, "decode failed: %d", rc);
	zassert_mem_equal(decoded, msg, sizeof(msg), "payload mismatch");
}

/*
 * Device-routed codec: a backend that opts into RTIO exposes encoder and
 * decoder vtables through the driver API. Fetch them from a real device
 * and round-trip a payload, proving the optional RTIO path is reachable.
 */
ZTEST(pulse_io_rtio, test_device_encoder_decoder)
{
	const struct pulse_io_encoder_api *enc;
	const struct pulse_io_decoder_api *dec;
	const uint8_t msg[] = {'R', 'T'};
	struct pulse_symbol syms[sizeof(msg) * 8 * 2];
	uint8_t decoded[sizeof(msg)];
	size_t produced = 0;
	int rc;

	zassert_true(device_is_ready(pio_dev));

	rc = pulse_io_get_encoder(pio_dev, &enc);
	zassert_ok(rc, "get_encoder failed: %d", rc);
	zassert_not_null(enc);

	rc = pulse_io_get_decoder(pio_dev, &dec);
	zassert_ok(rc, "get_decoder failed: %d", rc);
	zassert_not_null(dec);

	rc = enc->encode(&tmpl, msg, sizeof(msg), syms, ARRAY_SIZE(syms), &produced);
	zassert_ok(rc, "encode failed: %d", rc);

	produced = 0;
	rc = dec->decode(&tmpl, T - 1U, syms, ARRAY_SIZE(syms), decoded, sizeof(decoded),
			 &produced);
	zassert_ok(rc, "decode failed: %d", rc);
	zassert_mem_equal(decoded, msg, sizeof(msg), "payload mismatch");
}

/*
 * Device-routed submit: drive TX then RX through the backend's own
 * submit op (via pulse_io_iodev_submit), not the standalone loopback.
 */
ZTEST(pulse_io_rtio, test_device_routed_submit)
{
	const struct pulse_io_encoder_api *enc;
	const struct pulse_io_decoder_api *dec;

	zassert_ok(pulse_io_get_encoder(pio_dev, &enc));
	zassert_ok(pulse_io_get_decoder(pio_dev, &dec));
	const uint8_t msg[] = {'D', 'v'};
	struct pulse_symbol syms[sizeof(msg) * 8 * 2];
	struct pulse_symbol rx_syms[sizeof(msg) * 8 * 2];
	uint8_t decoded[sizeof(msg)];
	size_t produced = 0;
	int rc;

	rc = enc->encode(&tmpl, msg, sizeof(msg), syms, ARRAY_SIZE(syms), &produced);
	zassert_ok(rc);

	struct rtio_sqe *tx = rtio_sqe_acquire(&pio_rtio);

	rtio_sqe_prep_write(tx, &dev_iodev, RTIO_PRIO_NORM, (const uint8_t *)syms,
			    produced * sizeof(struct pulse_symbol), NULL);
	zassert_ok(rtio_submit(&pio_rtio, 1));

	struct rtio_cqe *cqe = rtio_cqe_consume(&pio_rtio);

	zassert_not_null(cqe);
	zassert_ok(cqe->result, "device tx failed: %d", cqe->result);
	rtio_cqe_release(&pio_rtio, cqe);

	struct rtio_sqe *rx = rtio_sqe_acquire(&pio_rtio);

	rtio_sqe_prep_read(rx, &dev_iodev, RTIO_PRIO_NORM, (uint8_t *)rx_syms, sizeof(rx_syms),
			   NULL);
	zassert_ok(rtio_submit(&pio_rtio, 1));

	cqe = rtio_cqe_consume(&pio_rtio);
	zassert_not_null(cqe);
	zassert_true(cqe->result > 0, "device rx failed: %d", cqe->result);
	rtio_cqe_release(&pio_rtio, cqe);

	produced = 0;
	rc = dec->decode(&tmpl, T - 1U, rx_syms, ARRAY_SIZE(rx_syms), decoded, sizeof(decoded),
			 &produced);
	zassert_ok(rc);
	zassert_mem_equal(decoded, msg, sizeof(msg), "payload mismatch");
}

static void *rtio_setup(void)
{
	pulse_io_loopback_iodev_init();
	return NULL;
}

ZTEST_SUITE(pulse_io_rtio, NULL, rtio_setup, NULL, NULL, NULL);
