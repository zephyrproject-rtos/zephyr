/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/nfc.h>
#include <zephyr/drivers/nfc/nfc_emul.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

static const struct device *const offload = DEVICE_DT_GET(DT_NODELABEL(emul_offload));
static const struct device *const initiator = DEVICE_DT_GET(DT_NODELABEL(emul_initiator));
static const struct device *const target = DEVICE_DT_GET(DT_NODELABEL(emul_target));

static const uint8_t sens_req[] = {0x26};
static const uint8_t sens_res[] = {0x44, 0x00};

static struct nfc_target discovered;
static unsigned int discovered_count;

static enum nfc_target_event last_event;
static uint8_t last_frame[16];
static uint16_t last_frame_len;
static unsigned int event_count;

static void poll_cb(const struct device *dev, const struct nfc_target *found, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	discovered = *found;
	discovered_count++;
}

static void target_cb(const struct device *dev, enum nfc_target_event event, const uint8_t *data,
		      uint16_t len, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	last_event = event;
	last_frame_len = MIN(len, sizeof(last_frame));
	if (data != NULL && last_frame_len != 0U) {
		memcpy(last_frame, data, last_frame_len);
	}
	event_count++;
}

ZTEST(nfc_api, test_devices_ready)
{
	zassert_true(device_is_ready(offload));
	zassert_true(device_is_ready(initiator));
	zassert_true(device_is_ready(target));
}

ZTEST(nfc_api, test_backend_classification)
{
	uint8_t tx[1] = {0x00};
	uint8_t rx[4];
	uint16_t rx_len;

	rx_len = sizeof(rx);
	zassert_equal(nfc_initiator_transceive(offload, tx, sizeof(tx), 0U, rx, &rx_len), -ENOSYS);
	zassert_equal(nfc_target_send(offload, tx, sizeof(tx), 0U), -ENOSYS);

	zassert_equal(nfc_offload_poll_start(initiator, NFC_PROTO_ISO14443A, NULL, poll_cb, NULL),
		      -ENOSYS);
	zassert_equal(nfc_target_start(initiator, NFC_PROTO_ISO14443A, target_cb, NULL), -ENOSYS);

	rx_len = sizeof(rx);
	zassert_equal(nfc_initiator_transceive(target, tx, sizeof(tx), 0U, rx, &rx_len), -ENOSYS);
	zassert_equal(nfc_offload_exchange(target, NULL, tx, sizeof(tx), rx, &rx_len, 10U),
		      -ENOSYS);
}

ZTEST(nfc_api, test_claim_release_null_guard)
{
	zassert_equal(nfc_claim(offload), 0);
	zassert_ok(nfc_release(offload));

	zassert_equal(nfc_claim(target), 0);
	zassert_ok(nfc_release(target));
}

ZTEST(nfc_api, test_claim_reports_claimed_protocol)
{
	zassert_equal(nfc_claim(initiator), NFC_PROTO_ISO14443A);
	zassert_ok(nfc_release(initiator));
}

ZTEST(nfc_api, test_optional_ops_report_enosys)
{
	struct nfc_property prop = {.type = NFC_PROP_TIMEOUT};

	zassert_equal(nfc_load_protocol(offload, NFC_PROTO_ISO14443A, NFC_MODE_INITIATOR), -ENOSYS);
	zassert_equal(nfc_get_properties(offload, &prop, 1), -ENOSYS);
	zassert_equal(nfc_set_properties(offload, &prop, 1), -ENOSYS);
}

ZTEST(nfc_api, test_supported_protocols_and_modes)
{
	zassert_equal(nfc_supported_protocols(initiator), NFC_PROTO_ISO14443A);
	zassert_equal(nfc_supported_protocols(offload), NFC_PROTO_ISO14443A | NFC_PROTO_ISO14443B);

	zassert_equal(nfc_supported_modes(initiator, NFC_PROTO_ISO14443A) & NFC_MODE_ROLE_MASK,
		      NFC_MODE_INITIATOR);
	zassert_equal(nfc_supported_modes(target, NFC_PROTO_ISO14443A) & NFC_MODE_ROLE_MASK,
		      NFC_MODE_TARGET);

	zassert_equal(nfc_supported_modes(initiator, NFC_PROTO_FELICA), 0,
		      "an unsupported protocol must report no modes");
}

ZTEST(nfc_api, test_load_protocol)
{
	zassert_ok(nfc_load_protocol(initiator, NFC_PROTO_ISO14443A, NFC_MODE_INITIATOR));
	zassert_equal(nfc_load_protocol(initiator, NFC_PROTO_FELICA, NFC_MODE_INITIATOR), -ENOTSUP);
	zassert_equal(nfc_load_protocol(initiator, NFC_PROTO_ISO14443A, NFC_MODE_TARGET), -ENOTSUP);
}

ZTEST(nfc_api, test_properties_report_per_element_status)
{
	struct nfc_property written[] = {
		{.type = NFC_PROP_TIMEOUT, .timeout_us = 1234U},
		{.type = NFC_PROP_MFC_CRYPTO, .mfc_crypto_on = true},
		{.type = NFC_PROP_HW_TX_CRC, .hw_tx_crc = true},
	};
	struct nfc_property read[] = {
		{.type = NFC_PROP_TIMEOUT},
		{.type = NFC_PROP_HW_TX_CRC},
	};

	zassert_ok(nfc_set_properties(initiator, written, ARRAY_SIZE(written)));
	zassert_ok(written[0].status);
	zassert_equal(written[1].status, -ENOTSUP);
	zassert_ok(written[2].status);

	zassert_ok(nfc_get_properties(initiator, read, ARRAY_SIZE(read)));
	zassert_equal(read[0].timeout_us, 1234U);
	zassert_true(read[1].hw_tx_crc);
}

ZTEST(nfc_api, test_transceive_roundtrip)
{
	static const struct nfc_emul_frame script[] = {
		{
			.tx = sens_req,
			.tx_len = sizeof(sens_req),
			.tx_last_bits = 7U,
			.rx = sens_res,
			.rx_len = sizeof(sens_res),
		},
	};
	uint8_t rx[8];
	uint16_t rx_len = sizeof(rx);

	nfc_emul_load_script(initiator, script, ARRAY_SIZE(script));

	zassert_ok(
		nfc_initiator_transceive(initiator, sens_req, sizeof(sens_req), 7U, rx, &rx_len));
	zassert_equal(rx_len, sizeof(sens_res));
	zassert_mem_equal(rx, sens_res, sizeof(sens_res));
	zassert_equal(nfc_emul_script_remaining(initiator), 0);
}

ZTEST(nfc_api, test_transceive_reports_errors)
{
	static const uint8_t all_req[] = {0x52};
	static const struct nfc_emul_frame script[] = {
		{
			.tx = sens_req,
			.tx_len = sizeof(sens_req),
			.tx_last_bits = 7U,
			.rx = sens_res,
			.rx_len = sizeof(sens_res),
		},
	};
	uint8_t rx[8];
	uint16_t rx_len;

	nfc_emul_load_script(initiator, script, ARRAY_SIZE(script));
	rx_len = sizeof(rx);
	zassert_equal(
		nfc_initiator_transceive(initiator, all_req, sizeof(all_req), 7U, rx, &rx_len),
		-EIO);

	nfc_emul_load_script(initiator, script, ARRAY_SIZE(script));
	rx_len = sizeof(rx);
	zassert_equal(
		nfc_initiator_transceive(initiator, sens_req, sizeof(sens_req), 8U, rx, &rx_len),
		-EIO);

	nfc_emul_load_script(initiator, NULL, 0);
	rx_len = sizeof(rx);
	zassert_equal(
		nfc_initiator_transceive(initiator, sens_req, sizeof(sens_req), 7U, rx, &rx_len),
		-ENODATA);
}

ZTEST(nfc_api, test_offload_poll_and_exchange)
{
	static const uint8_t read_cmd[] = {0x30, 0x00};
	static const uint8_t read_res[] = {0x04, 0x11, 0x22, 0x33};
	static const struct nfc_emul_frame script[] = {
		{
			.tx = read_cmd,
			.tx_len = sizeof(read_cmd),
			.rx = read_res,
			.rx_len = sizeof(read_res),
		},
	};
	const struct nfc_target staged = {
		.tech = NFC_TECH_A,
		.proto = NFC_PROTO_ISO14443A,
		.a = {.uid = {0x04, 0x11, 0x22, 0x33}, .uid_len = 4U, .sak = 0x00U},
	};
	uint8_t rx[8];
	uint16_t rx_len = sizeof(rx);

	zassert_ok(nfc_emul_set_target(offload, &staged));
	zassert_ok(nfc_offload_poll_start(offload, NFC_PROTO_ISO14443A, NULL, poll_cb, NULL));

	zassert_equal(discovered_count, 1);
	zassert_equal(discovered.tech, NFC_TECH_A);
	zassert_equal(discovered.a.uid_len, 4U);
	zassert_mem_equal(discovered.a.uid, staged.a.uid, staged.a.uid_len);

	nfc_emul_load_script(offload, script, ARRAY_SIZE(script));
	zassert_ok(nfc_offload_exchange(offload, &discovered, read_cmd, sizeof(read_cmd), rx,
					&rx_len, 100U));
	zassert_equal(rx_len, sizeof(read_res));
	zassert_mem_equal(rx, read_res, sizeof(read_res));
	zassert_equal(nfc_emul_script_remaining(offload), 0);

	zassert_ok(nfc_offload_poll_stop(offload));

	zassert_equal(nfc_offload_poll_start(offload, NFC_PROTO_ISO15693, NULL, poll_cb, NULL),
		      -ENOTSUP);
}

ZTEST(nfc_api, test_target_mode_events)
{
	static const uint8_t request[] = {0x30, 0x04};
	static const uint8_t response[] = {0xDE, 0xAD, 0xBE, 0xEF};
	static const struct nfc_emul_frame script[] = {
		{
			.tx = response,
			.tx_len = sizeof(response),
		},
	};

	zassert_equal(nfc_emul_raise_target_event(target, NFC_TARGET_SELECTED, NULL, 0), -EPERM);
	zassert_equal(nfc_target_send(target, response, sizeof(response), 0U), -EPERM);

	zassert_ok(nfc_target_start(target, NFC_PROTO_ISO14443A, target_cb, NULL));

	zassert_ok(nfc_emul_raise_target_event(target, NFC_TARGET_SELECTED, NULL, 0));
	zassert_equal(last_event, NFC_TARGET_SELECTED);

	zassert_ok(nfc_emul_raise_target_event(target, NFC_TARGET_FRAME, request, sizeof(request)));
	zassert_equal(last_event, NFC_TARGET_FRAME);
	zassert_equal(last_frame_len, sizeof(request));
	zassert_mem_equal(last_frame, request, sizeof(request));

	nfc_emul_load_script(target, script, ARRAY_SIZE(script));
	zassert_ok(nfc_target_send(target, response, sizeof(response), 0U));
	zassert_equal(nfc_emul_script_remaining(target), 0);

	zassert_ok(nfc_emul_raise_target_event(target, NFC_TARGET_DESELECTED, NULL, 0));
	zassert_equal(last_event, NFC_TARGET_DESELECTED);
	zassert_equal(event_count, 3);

	zassert_ok(nfc_target_stop(target));
	zassert_equal(nfc_emul_raise_target_event(target, NFC_TARGET_SELECTED, NULL, 0), -EPERM);

	zassert_equal(nfc_target_start(target, NFC_PROTO_ISO15693, target_cb, NULL), -ENOTSUP);
}

static void nfc_api_before(void *fixture)
{
	ARG_UNUSED(fixture);

	nfc_emul_load_script(offload, NULL, 0);
	nfc_emul_load_script(initiator, NULL, 0);
	nfc_emul_load_script(target, NULL, 0);

	(void)nfc_offload_poll_stop(offload);
	(void)nfc_emul_set_target(offload, NULL);
	(void)nfc_target_stop(target);

	memset(&discovered, 0, sizeof(discovered));
	discovered_count = 0U;

	memset(last_frame, 0, sizeof(last_frame));
	last_frame_len = 0U;
	event_count = 0U;
}

ZTEST_SUITE(nfc_api, NULL, NULL, nfc_api_before, NULL, NULL);
