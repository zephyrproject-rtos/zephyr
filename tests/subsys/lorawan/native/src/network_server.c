/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network_server.h"
#include "region/test_region.h"

#include <string.h>

#include <psa/crypto.h>
#include <zephyr/device.h>
#include <zephyr/drivers/lora/lora_fake.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/lorawan/packets.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#define AES_BLOCK_SIZE 16U

#define RX_DELAY 1U

#define DEV_NONCE 0x1234
#define DEV_ADDR  0x12345678

#define MAX_DATA_FRAME_SIZE 255U
#define CFLIST_FREQ_STEP_HZ 100U
#define APP_S_KEY_TYPE      0x02
#define FNWK_S_INT_KEY_TYPE 0x01

static const struct device *const fake_lora_dev = DEVICE_DT_GET(DT_NODELABEL(fake_lora));

static uint8_t dev_eui[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
static uint8_t join_eui[] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17};
static uint8_t nwk_key[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
};
/*
 * LoRaWAN 1.0.x OTAA vector from lorawan-parser:
 * https://github.com/tanupoo/lorawan-parser
 */
static uint8_t vector_dev_eui[] = {0xbe, 0xef, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
static uint8_t vector_join_eui[] = {0xbe, 0xef, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
static uint8_t vector_nwk_key[] = {
	0xbe, 0xef, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
	0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
};
static const uint8_t vector_join_request[] = {
	0x00, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00, 0xef, 0xbe, 0x05, 0x04, 0x03,
	0x02, 0x01, 0x00, 0xef, 0xbe, 0x87, 0xbc, 0xc5, 0x4c, 0x5b, 0x5f,
};
static const uint8_t vector_join_accept[] = {
	0x20, 0xb4, 0x6c, 0x00, 0x22, 0xda, 0xe9, 0x1f, 0x38,
	0xe1, 0x72, 0xac, 0x03, 0x12, 0xd4, 0xbc, 0x5b,
};
static const uint8_t vector_uplink[] = {
	0x40, 0x34, 0x09, 0xbe, 0x2b, 0x80, 0x00, 0x00, 0x02, 0xc7, 0xfb, 0x89,
	0x63, 0x47, 0x6d, 0x5b, 0xf4, 0x09, 0x0e, 0x6b, 0x86, 0x7a, 0x40, 0xb5,
	0x97, 0x04, 0x72, 0x41, 0xeb, 0x80, 0xae, 0xf7, 0x9d, 0xf6,
};
static const uint8_t join_nonce[] = {0x01, 0x02, 0x03};
static const uint8_t net_id[] = {0x04, 0x05, 0x06};

static uint8_t expected_tx_frame[MAX_DATA_FRAME_SIZE];
static size_t expected_tx_frame_len;
static uint8_t join_accept[sizeof(struct lorawan_pkt_join_accept) + LORAWAN_PKT_CFLIST_SIZE +
			   LORAWAN_PKT_MIC_SIZE];
static size_t join_accept_len;
static uint8_t ack_downlink[sizeof(struct lorawan_pkt_data_header) + LORAWAN_PKT_MIC_SIZE];
static uint8_t application_downlink[MAX_DATA_FRAME_SIZE];
static size_t application_downlink_len;
static uint32_t join_accept_rx_window;
static uint32_t downlink_rx_window;
static enum network_server_invalid_join_response invalid_join_response;
static enum network_server_invalid_downlink invalid_downlink;
static bool invalid_join_response_in_rx1_only;
static uint32_t receive_attempt;
static uint32_t transmit_attempt;
static uint32_t ack_tx_attempt;
static uint32_t expected_tx_frequency;
static uint32_t tx_frequency;
static uint32_t rx_window;
static enum lorawan_datarate expected_uplink_datarate;
static enum lorawan_datarate expected_rx2_datarate;
static uint32_t expected_rx_timeout_ms;
static uint32_t expected_rx1_delay_ms;
static uint32_t expected_fcnt_up;
static int64_t tx_complete_time;

DEFINE_FFF_GLOBALS;

static void calculate_mic(const uint8_t key[AES_BLOCK_SIZE], const uint8_t *data, size_t data_len,
			  uint8_t mic[LORAWAN_PKT_MIC_SIZE])
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	uint8_t cmac[AES_BLOCK_SIZE];
	psa_key_id_t key_id;
	psa_status_t status;
	size_t cmac_len;

	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_MESSAGE);
	psa_set_key_algorithm(&attributes, PSA_ALG_CMAC);
	psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attributes, PSA_BYTES_TO_BITS(AES_BLOCK_SIZE));

	status = psa_import_key(&attributes, key, AES_BLOCK_SIZE, &key_id);
	zassert_equal(status, PSA_SUCCESS);

	status = psa_mac_compute(key_id, PSA_ALG_CMAC, data, data_len, cmac, sizeof(cmac),
				 &cmac_len);
	zassert_equal(status, PSA_SUCCESS);
	zassert_equal(cmac_len, sizeof(cmac));
	memcpy(mic, cmac, LORAWAN_PKT_MIC_SIZE);

	zassert_equal(psa_destroy_key(key_id), PSA_SUCCESS);
}

static void encrypt_aes_block(const uint8_t key[AES_BLOCK_SIZE],
			      const uint8_t plaintext[AES_BLOCK_SIZE],
			      uint8_t ciphertext[AES_BLOCK_SIZE])
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id;
	psa_status_t status;
	size_t ciphertext_len;

	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_ENCRYPT);
	psa_set_key_algorithm(&attributes, PSA_ALG_ECB_NO_PADDING);
	psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attributes, PSA_BYTES_TO_BITS(AES_BLOCK_SIZE));

	status = psa_import_key(&attributes, key, AES_BLOCK_SIZE, &key_id);
	zassert_equal(status, PSA_SUCCESS);

	status = psa_cipher_encrypt(key_id, PSA_ALG_ECB_NO_PADDING, plaintext, AES_BLOCK_SIZE,
				    ciphertext, AES_BLOCK_SIZE, &ciphertext_len);
	zassert_equal(status, PSA_SUCCESS);
	zassert_equal(ciphertext_len, AES_BLOCK_SIZE);

	zassert_equal(psa_destroy_key(key_id), PSA_SUCCESS);
}

static void encrypt_join_accept(const uint8_t *plaintext, size_t plaintext_len, uint8_t *ciphertext)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id;
	psa_status_t status;
	size_t ciphertext_len;

	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DECRYPT);
	psa_set_key_algorithm(&attributes, PSA_ALG_ECB_NO_PADDING);
	psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attributes, PSA_BYTES_TO_BITS(AES_BLOCK_SIZE));

	status = psa_import_key(&attributes, nwk_key, sizeof(nwk_key), &key_id);
	zassert_equal(status, PSA_SUCCESS);

	/* LoRaWAN encrypts a join-accept with the AES decrypt operation. */
	status = psa_cipher_decrypt(key_id, PSA_ALG_ECB_NO_PADDING, plaintext, plaintext_len,
				    ciphertext, plaintext_len, &ciphertext_len);
	zassert_equal(status, PSA_SUCCESS);
	zassert_equal(ciphertext_len, plaintext_len);

	zassert_equal(psa_destroy_key(key_id), PSA_SUCCESS);
}

static void build_expected_join_request(struct lorawan_pkt_join_request *frame)
{
	frame->mhdr = LORAWAN_PKT_MHDR_JOIN_REQUEST;
	sys_memcpy_swap(frame->join_eui, join_eui, sizeof(join_eui));
	sys_memcpy_swap(frame->dev_eui, dev_eui, sizeof(dev_eui));
	sys_put_le16(DEV_NONCE, frame->dev_nonce);
	calculate_mic(nwk_key, (const uint8_t *)frame,
		      offsetof(struct lorawan_pkt_join_request, mic), frame->mic);
}

static void build_join_accept(bool include_cflist,
			      enum network_server_invalid_join_response invalid_response,
			      uint8_t rx_delay)
{
	uint8_t plaintext_frame[sizeof(join_accept)] = {0};
	struct lorawan_pkt_join_accept *plaintext =
		(struct lorawan_pkt_join_accept *)plaintext_frame;
	size_t cflist_len = include_cflist ? LORAWAN_PKT_CFLIST_SIZE : 0U;
	size_t mic_offset = sizeof(*plaintext) + cflist_len;
	size_t encrypted_len;

	plaintext->mhdr = LORAWAN_PKT_MHDR_JOIN_ACCEPT;
	memcpy(plaintext->join_nonce, join_nonce, sizeof(join_nonce));
	memcpy(plaintext->net_id, net_id, sizeof(net_id));
	sys_put_le32(DEV_ADDR, plaintext->dev_addr);
	plaintext->dl_settings =
		(test_region_get()->rx1_dr_offset << 4) | test_region_get()->rx2_datarate;
	if (invalid_response == NETWORK_SERVER_INVALID_JOIN_RESPONSE_INVALID_RX1_OFFSET) {
		plaintext->dl_settings = 0x70U | test_region_get()->rx2_datarate;
	} else if (invalid_response == NETWORK_SERVER_INVALID_JOIN_RESPONSE_INVALID_RX2_DATARATE) {
		plaintext->dl_settings = (test_region_get()->rx1_dr_offset << 4) | 0x0fU;
	}
	plaintext->rx_delay = rx_delay;

	for (size_t i = 0;
	     i < MIN(test_region_get()->cflist_frequency_count, LORAWAN_PKT_CFLIST_SIZE / 3U) &&
	     include_cflist;
	     i++) {
		sys_put_le24(test_region_get()->cflist_frequencies[i] / CFLIST_FREQ_STEP_HZ,
			     &plaintext->payload_and_mic[i * 3U]);
	}

	calculate_mic(nwk_key, plaintext_frame, mic_offset, &plaintext_frame[mic_offset]);
	if (invalid_response == NETWORK_SERVER_INVALID_JOIN_RESPONSE_BAD_MIC) {
		plaintext_frame[mic_offset] ^= BIT(0);
	}

	join_accept_len = mic_offset + LORAWAN_PKT_MIC_SIZE;
	encrypted_len = join_accept_len - LORAWAN_PKT_MHDR_SIZE;
	join_accept[0] = LORAWAN_PKT_MHDR_JOIN_ACCEPT;
	encrypt_join_accept(&plaintext_frame[LORAWAN_PKT_MHDR_SIZE], encrypted_len,
			    &join_accept[LORAWAN_PKT_MHDR_SIZE]);
}

static void derive_session_key(uint8_t key_type, uint8_t key[AES_BLOCK_SIZE])
{
	uint8_t input[AES_BLOCK_SIZE] = {key_type};

	memcpy(&input[1], join_nonce, sizeof(join_nonce));
	memcpy(&input[4], net_id, sizeof(net_id));
	sys_put_le16(DEV_NONCE, &input[7]);
	encrypt_aes_block(nwk_key, input, key);
}

static void crypt_payload(const uint8_t key[AES_BLOCK_SIZE], uint32_t frame_counter,
			  uint8_t direction, uint8_t *payload, size_t payload_len)
{
	uint8_t encryption_block[AES_BLOCK_SIZE] = {0};
	uint8_t key_stream[AES_BLOCK_SIZE];

	zassert_true(payload_len <= sizeof(key_stream));

	encryption_block[0] = 0x01;
	encryption_block[5] = direction;
	sys_put_le32(DEV_ADDR, &encryption_block[6]);
	sys_put_le32(frame_counter, &encryption_block[10]);
	encryption_block[15] = 0x01;
	encrypt_aes_block(key, encryption_block, key_stream);

	for (size_t i = 0; i < payload_len; i++) {
		payload[i] ^= key_stream[i];
	}
}

static void calculate_data_mic(const uint8_t key[AES_BLOCK_SIZE], uint32_t frame_counter,
			       uint8_t direction, const uint8_t *frame, size_t frame_len,
			       uint8_t mic[LORAWAN_PKT_MIC_SIZE])
{
	uint8_t mic_input[sizeof(struct lorawan_pkt_data_mic_block) + MAX_DATA_FRAME_SIZE] = {0};
	struct lorawan_pkt_data_mic_block *block = (struct lorawan_pkt_data_mic_block *)mic_input;

	block->type = LORAWAN_PKT_DATA_MIC_BLOCK_TYPE;
	block->direction = direction;
	sys_put_le32(DEV_ADDR, block->dev_addr);
	sys_put_le32(frame_counter, block->fcnt);
	block->msg_len = frame_len;
	memcpy(&mic_input[sizeof(*block)], frame, frame_len);
	calculate_mic(key, mic_input, sizeof(*block) + frame_len, mic);
}

static size_t build_expected_uplink(enum lorawan_message_type type, bool ack, uint8_t port,
				    const uint8_t *payload, size_t payload_len, uint32_t fcnt_up,
				    uint8_t frame[MAX_DATA_FRAME_SIZE])
{
	struct lorawan_pkt_data_header *header = (struct lorawan_pkt_data_header *)frame;
	uint8_t app_s_key[AES_BLOCK_SIZE];
	uint8_t fnwk_s_int_key[AES_BLOCK_SIZE];
	size_t message_len = sizeof(*header) + 1U + payload_len;
	uint8_t *port_field = header->payload_and_mic;
	uint8_t *payload_field = &port_field[1];

	zassert_true(message_len + LORAWAN_PKT_MIC_SIZE <= MAX_DATA_FRAME_SIZE);

	header->mhdr = type == LORAWAN_MSG_CONFIRMED ? LORAWAN_PKT_MHDR_CONF_DATA_UP
						     : LORAWAN_PKT_MHDR_UNCONF_DATA_UP;
	sys_put_le32(DEV_ADDR, header->dev_addr);
	header->fctrl = ack ? LORAWAN_PKT_FCTRL_ACK : 0U;
	sys_put_le16((uint16_t)fcnt_up, header->fcnt);
	*port_field = port;
	memcpy(payload_field, payload, payload_len);

	derive_session_key(APP_S_KEY_TYPE, app_s_key);
	crypt_payload(app_s_key, fcnt_up, LORAWAN_PKT_DIR_UPLINK, payload_field, payload_len);

	derive_session_key(FNWK_S_INT_KEY_TYPE, fnwk_s_int_key);
	calculate_data_mic(fnwk_s_int_key, fcnt_up, LORAWAN_PKT_DIR_UPLINK, frame, message_len,
			   &frame[message_len]);

	return message_len + LORAWAN_PKT_MIC_SIZE;
}

static void build_ack_downlink(void)
{
	struct lorawan_pkt_data_header *header = (struct lorawan_pkt_data_header *)ack_downlink;
	uint8_t fnwk_s_int_key[AES_BLOCK_SIZE];
	size_t message_len = sizeof(*header);

	header->mhdr = LORAWAN_PKT_MHDR_UNCONF_DATA_DOWN;
	sys_put_le32(DEV_ADDR, header->dev_addr);
	header->fctrl = LORAWAN_PKT_FCTRL_ACK;
	sys_put_le16(0U, header->fcnt);

	derive_session_key(FNWK_S_INT_KEY_TYPE, fnwk_s_int_key);
	calculate_data_mic(fnwk_s_int_key, 0U, LORAWAN_PKT_DIR_DOWNLINK, ack_downlink, message_len,
			   header->payload_and_mic);
}

static void build_application_downlink(bool confirmed, bool data_pending, uint8_t port,
				       const uint8_t *payload, size_t payload_len)
{
	struct lorawan_pkt_data_header *header =
		(struct lorawan_pkt_data_header *)application_downlink;
	uint8_t app_s_key[AES_BLOCK_SIZE];
	uint8_t fnwk_s_int_key[AES_BLOCK_SIZE];
	size_t message_len = sizeof(*header) + LORAWAN_PKT_FPORT_SIZE + payload_len;
	uint8_t *port_field = header->payload_and_mic;
	uint8_t *payload_field = &port_field[LORAWAN_PKT_FPORT_SIZE];

	zassert_true(message_len + LORAWAN_PKT_MIC_SIZE <= sizeof(application_downlink));

	header->mhdr =
		confirmed ? LORAWAN_PKT_MHDR_CONF_DATA_DOWN : LORAWAN_PKT_MHDR_UNCONF_DATA_DOWN;
	sys_put_le32(DEV_ADDR, header->dev_addr);
	header->fctrl = data_pending ? LORAWAN_PKT_FCTRL_FPENDING : 0U;
	sys_put_le16(0U, header->fcnt);
	*port_field = port;
	memcpy(payload_field, payload, payload_len);

	derive_session_key(APP_S_KEY_TYPE, app_s_key);
	crypt_payload(app_s_key, 0U, LORAWAN_PKT_DIR_DOWNLINK, payload_field, payload_len);

	derive_session_key(FNWK_S_INT_KEY_TYPE, fnwk_s_int_key);
	calculate_data_mic(fnwk_s_int_key, 0U, LORAWAN_PKT_DIR_DOWNLINK, application_downlink,
			   message_len, &application_downlink[message_len]);
	application_downlink_len = message_len + LORAWAN_PKT_MIC_SIZE;
}

static int validate_config(const struct device *dev, const struct lora_modem_config *config)
{
	const struct test_region *region = test_region_get();
	enum lorawan_datarate expected_datarate;

	zexpect_equal_ptr(dev, fake_lora_dev);
	zexpect_not_null(config);

	if (config == NULL) {
		return -EINVAL;
	}

	zexpect_equal(config->coding_rate, CR_4_5);
	zexpect_equal(config->preamble_len, 8U);
	zexpect_true(config->public_network);

	if (config->tx) {
		expected_datarate = expected_uplink_datarate;
		zexpect_true(expected_datarate < region->datarate_count);
		zexpect_equal(config->bandwidth, region->datarates[expected_datarate].bandwidth);
		zexpect_equal(config->datarate,
			      region->datarates[expected_datarate].spreading_factor);

		if (expected_tx_frequency == 0U) {
			zexpect_true(region->is_default_channel(config->frequency));
		} else {
			zexpect_equal(config->frequency, expected_tx_frequency);
		}

		zexpect_equal(config->tx_power, 16);
		zexpect_false(config->iq_inverted);
		zexpect_false(config->packet_crc_disable);
		tx_frequency = config->frequency;
		rx_window = 0U;
	} else {
		rx_window++;
		if (rx_window == 1U) {
			expected_datarate =
				expected_uplink_datarate > region->rx1_dr_offset
					? expected_uplink_datarate - region->rx1_dr_offset
					: LORAWAN_DR_0;
		} else {
			expected_datarate = expected_rx2_datarate;
		}
		zexpect_true(expected_datarate < region->datarate_count);
		zexpect_equal(config->bandwidth, region->datarates[expected_datarate].bandwidth);
		zexpect_equal(config->datarate,
			      region->datarates[expected_datarate].spreading_factor);
		zexpect_equal(config->frequency,
			      rx_window == 1U ? tx_frequency : region->rx2_frequency);
		zexpect_true(config->iq_inverted);
		zexpect_true(config->packet_crc_disable);
		expected_rx_timeout_ms = region->datarates[expected_datarate].rx_timeout_ms;
	}

	return 0;
}

static int capture_send(const struct device *dev, uint8_t *data, uint32_t data_len,
			struct k_poll_signal *done_signal)
{
	zexpect_equal_ptr(dev, fake_lora_dev);
	zexpect_not_null(data);
	zexpect_equal(data_len, expected_tx_frame_len);
	zexpect_not_null(done_signal);

	if (data == NULL || data_len != expected_tx_frame_len || done_signal == NULL) {
		return -EINVAL;
	}

	zexpect_mem_equal(data, expected_tx_frame, expected_tx_frame_len);
	transmit_attempt++;
	tx_complete_time = k_uptime_get();

	return k_poll_signal_raise(done_signal, 0);
}

static bool validate_receive_args(const struct device *dev, uint8_t *data, uint8_t size,
				  k_timeout_t timeout, int16_t *rssi, int8_t *snr)
{
	zexpect_equal_ptr(dev, fake_lora_dev);
	zexpect_not_null(data);
	zexpect_equal(size, MAX_DATA_FRAME_SIZE);
	zexpect_true(K_TIMEOUT_EQ(timeout, K_MSEC(expected_rx_timeout_ms)));
	zexpect_not_null(rssi);
	zexpect_not_null(snr);
	if (expected_rx1_delay_ms != 0U) {
		int64_t elapsed = k_uptime_get() - tx_complete_time;
		uint32_t expected_delay = expected_rx1_delay_ms - 50U + (rx_window - 1U) * 1000U;

		zexpect_true(elapsed >= expected_delay && elapsed <= expected_delay + 50U,
			     "RX%u started after %lld ms, expected %u ms", rx_window, elapsed,
			     expected_delay);
	}

	return data != NULL && size == MAX_DATA_FRAME_SIZE && rssi != NULL && snr != NULL;
}

static int provide_join_accept(const struct device *dev, uint8_t *data, uint8_t size,
			       k_timeout_t timeout, int16_t *rssi, int8_t *snr)
{
	size_t response_len = join_accept_len;
	bool send_invalid_response;

	if (!validate_receive_args(dev, data, size, timeout, rssi, snr)) {
		return -EINVAL;
	}

	*rssi = -42;
	*snr = 7;
	receive_attempt++;
	send_invalid_response =
		invalid_join_response != NETWORK_SERVER_INVALID_JOIN_RESPONSE_NONE &&
		(!invalid_join_response_in_rx1_only || receive_attempt == 1U);

	if (receive_attempt < join_accept_rx_window) {
		return 0;
	}

	if (send_invalid_response &&
	    invalid_join_response == NETWORK_SERVER_INVALID_JOIN_RESPONSE_EMPTY) {
		return 0;
	}

	memcpy(data, join_accept, join_accept_len);
	if (!send_invalid_response) {
		return join_accept_len;
	}

	switch (invalid_join_response) {
	case NETWORK_SERVER_INVALID_JOIN_RESPONSE_NONE:
		break;
	case NETWORK_SERVER_INVALID_JOIN_RESPONSE_WRONG_MHDR:
		data[0] = LORAWAN_PKT_MHDR_UNCONF_DATA_DOWN;
		break;
	case NETWORK_SERVER_INVALID_JOIN_RESPONSE_UNSUPPORTED_MAJOR:
		data[0] |= LORAWAN_PKT_MHDR_MAJOR_MASK;
		break;
	case NETWORK_SERVER_INVALID_JOIN_RESPONSE_SHORT:
		response_len--;
		break;
	case NETWORK_SERVER_INVALID_JOIN_RESPONSE_INTERMEDIATE:
		response_len++;
		break;
	case NETWORK_SERVER_INVALID_JOIN_RESPONSE_OVERSIZE:
		response_len += LORAWAN_PKT_CFLIST_SIZE + 1U;
		break;
	case NETWORK_SERVER_INVALID_JOIN_RESPONSE_BAD_CIPHERTEXT:
		data[join_accept_len - 1U] ^= BIT(0);
		break;
	case NETWORK_SERVER_INVALID_JOIN_RESPONSE_EMPTY:
	case NETWORK_SERVER_INVALID_JOIN_RESPONSE_BAD_MIC:
	case NETWORK_SERVER_INVALID_JOIN_RESPONSE_INVALID_RX1_OFFSET:
	case NETWORK_SERVER_INVALID_JOIN_RESPONSE_INVALID_RX2_DATARATE:
		break;
	}

	return response_len;
}

static int provide_no_response(const struct device *dev, uint8_t *data, uint8_t size,
			       k_timeout_t timeout, int16_t *rssi, int8_t *snr)
{
	if (!validate_receive_args(dev, data, size, timeout, rssi, snr)) {
		return -EINVAL;
	}

	*rssi = 0;
	*snr = 0;

	return -EAGAIN;
}

static int provide_ack(const struct device *dev, uint8_t *data, uint8_t size, k_timeout_t timeout,
		       int16_t *rssi, int8_t *snr)
{
	if (!validate_receive_args(dev, data, size, timeout, rssi, snr)) {
		return -EINVAL;
	}

	*rssi = -42;
	*snr = 7;

	if (transmit_attempt < ack_tx_attempt) {
		return 0;
	}

	memcpy(data, ack_downlink, sizeof(ack_downlink));

	return sizeof(ack_downlink);
}

static int provide_application_downlink(const struct device *dev, uint8_t *data, uint8_t size,
					k_timeout_t timeout, int16_t *rssi, int8_t *snr)
{
	size_t response_len = application_downlink_len;

	if (!validate_receive_args(dev, data, size, timeout, rssi, snr)) {
		return -EINVAL;
	}

	*rssi = -42;
	*snr = 7;
	receive_attempt++;
	if (receive_attempt != downlink_rx_window) {
		return 0;
	}

	memcpy(data, application_downlink, application_downlink_len);
	switch (invalid_downlink) {
	case NETWORK_SERVER_INVALID_DOWNLINK_NONE:
		break;
	case NETWORK_SERVER_INVALID_DOWNLINK_WRONG_MHDR:
		data[0] = LORAWAN_PKT_MHDR_UNCONF_DATA_UP;
		break;
	case NETWORK_SERVER_INVALID_DOWNLINK_UNSUPPORTED_MAJOR:
		data[0] |= LORAWAN_PKT_MHDR_MAJOR_MASK;
		break;
	case NETWORK_SERVER_INVALID_DOWNLINK_SHORT:
		response_len = sizeof(struct lorawan_pkt_data_header) + LORAWAN_PKT_MIC_SIZE - 1U;
		break;
	case NETWORK_SERVER_INVALID_DOWNLINK_WRONG_DEV_ADDR:
		sys_put_le32(DEV_ADDR + 1U, ((struct lorawan_pkt_data_header *)data)->dev_addr);
		break;
	case NETWORK_SERVER_INVALID_DOWNLINK_BAD_MIC:
		data[application_downlink_len - 1U] ^= BIT(0);
		break;
	case NETWORK_SERVER_INVALID_DOWNLINK_BAD_FOPTS_LEN:
		((struct lorawan_pkt_data_header *)data)->fctrl = LORAWAN_PKT_FCTRL_FOPTS_LEN_MASK;
		break;
	}

	return response_len;
}

static void prepare_server(void)
{
	RESET_FAKE(fake_lora_config);
	RESET_FAKE(fake_lora_send_async);
	RESET_FAKE(fake_lora_recv);
	transmit_attempt = 0U;
	receive_attempt = 0U;
	expected_tx_frequency = 0U;
	expected_uplink_datarate = LORAWAN_DR_0;
	expected_rx2_datarate = test_region_get()->rx2_datarate;
	expected_rx1_delay_ms = 0U;
	tx_frequency = 0U;
	rx_window = 0U;
	fake_lora_config_fake.custom_fake = validate_config;
	fake_lora_send_async_fake.custom_fake = capture_send;
}

static void expect_join_request(void)
{
	struct lorawan_pkt_join_request *frame =
		(struct lorawan_pkt_join_request *)expected_tx_frame;

	build_expected_join_request(frame);
	expected_tx_frame_len = sizeof(*frame);
}

static void expect_uplink(enum lorawan_message_type type, bool ack, uint8_t port,
			  const uint8_t *payload, size_t payload_len)
{
	expected_tx_frame_len = build_expected_uplink(type, ack, port, payload, payload_len,
						      expected_fcnt_up, expected_tx_frame);
	expected_fcnt_up++;
}

struct lorawan_join_config network_server_join_config(void)
{
	return (struct lorawan_join_config){
		.mode = LORAWAN_ACT_OTAA,
		.dev_eui = dev_eui,
		.otaa = {.join_eui = join_eui, .nwk_key = nwk_key, .dev_nonce = DEV_NONCE},
	};
}

struct lorawan_join_config network_server_vector_join_config(void)
{
	return (struct lorawan_join_config){
		.mode = LORAWAN_ACT_OTAA,
		.dev_eui = vector_dev_eui,
		.otaa = {.join_eui = vector_join_eui,
			 .nwk_key = vector_nwk_key,
			 .dev_nonce = 0xbc87U},
	};
}

void network_server_prepare_vector_join(void)
{
	prepare_server();
	expected_fcnt_up = 0U;
	memcpy(expected_tx_frame, vector_join_request, sizeof(vector_join_request));
	expected_tx_frame_len = sizeof(vector_join_request);
	memcpy(join_accept, vector_join_accept, sizeof(vector_join_accept));
	join_accept_len = sizeof(vector_join_accept);
	invalid_join_response = NETWORK_SERVER_INVALID_JOIN_RESPONSE_NONE;
	invalid_join_response_in_rx1_only = false;
	expected_rx2_datarate = LORAWAN_DR_2;
	join_accept_rx_window = 1U;
	receive_attempt = 0U;
	fake_lora_recv_fake.custom_fake = provide_join_accept;
}

void network_server_prepare_vector_uplink(void)
{
	prepare_server();
	memcpy(expected_tx_frame, vector_uplink, sizeof(vector_uplink));
	expected_tx_frame_len = sizeof(vector_uplink);
	expected_rx2_datarate = LORAWAN_DR_2;
	fake_lora_recv_fake.custom_fake = provide_no_response;
}

void network_server_prepare_join(uint32_t rx_window)
{
	prepare_server();
	expected_fcnt_up = 0U;
	expect_join_request();
	build_join_accept(false, NETWORK_SERVER_INVALID_JOIN_RESPONSE_NONE, RX_DELAY);
	invalid_join_response = NETWORK_SERVER_INVALID_JOIN_RESPONSE_NONE;
	join_accept_rx_window = rx_window;
	invalid_join_response_in_rx1_only = false;
	receive_attempt = 0U;
	fake_lora_recv_fake.custom_fake = provide_join_accept;
}

void network_server_prepare_invalid_join_response(void)
{
	prepare_server();
	expected_fcnt_up = 0U;
	expect_join_request();
	build_join_accept(false, NETWORK_SERVER_INVALID_JOIN_RESPONSE_NONE, RX_DELAY);
	join_accept_rx_window = 1U;
	invalid_join_response = NETWORK_SERVER_INVALID_JOIN_RESPONSE_BAD_CIPHERTEXT;
	invalid_join_response_in_rx1_only = true;
	receive_attempt = 0U;
	fake_lora_recv_fake.custom_fake = provide_join_accept;
}

void network_server_prepare_invalid_join_responses(
	enum network_server_invalid_join_response response)
{
	prepare_server();
	expected_fcnt_up = 0U;
	expect_join_request();
	build_join_accept(false, response, RX_DELAY);
	join_accept_rx_window = 1U;
	invalid_join_response = response;
	invalid_join_response_in_rx1_only = false;
	receive_attempt = 0U;
	fake_lora_recv_fake.custom_fake = provide_join_accept;
}

void network_server_prepare_join_with_cflist(uint32_t rx_window)
{
	prepare_server();
	expected_fcnt_up = 0U;
	expect_join_request();
	build_join_accept(true, NETWORK_SERVER_INVALID_JOIN_RESPONSE_NONE, RX_DELAY);
	invalid_join_response = NETWORK_SERVER_INVALID_JOIN_RESPONSE_NONE;
	join_accept_rx_window = rx_window;
	invalid_join_response_in_rx1_only = false;
	receive_attempt = 0U;
	fake_lora_recv_fake.custom_fake = provide_join_accept;
}

void network_server_prepare_join_no_response(void)
{
	prepare_server();
	expected_fcnt_up = 0U;
	expect_join_request();
	fake_lora_recv_fake.custom_fake = provide_no_response;
}

void network_server_prepare_join_with_rx_delay(uint8_t rx_delay)
{
	prepare_server();
	expected_fcnt_up = 0U;
	expect_join_request();
	build_join_accept(false, NETWORK_SERVER_INVALID_JOIN_RESPONSE_NONE, rx_delay);
	invalid_join_response = NETWORK_SERVER_INVALID_JOIN_RESPONSE_NONE;
	invalid_join_response_in_rx1_only = false;
	join_accept_rx_window = 1U;
	receive_attempt = 0U;
	fake_lora_recv_fake.custom_fake = provide_join_accept;
}

void network_server_prepare_uplink_no_response(enum lorawan_message_type type, uint8_t port,
					       const uint8_t *payload, size_t payload_len)
{
	prepare_server();
	expect_uplink(type, false, port, payload, payload_len);
	fake_lora_recv_fake.custom_fake = provide_no_response;
}

void network_server_prepare_uplink_ack(uint8_t port, const uint8_t *payload, size_t payload_len)
{
	prepare_server();
	expect_uplink(LORAWAN_MSG_UNCONFIRMED, true, port, payload, payload_len);
	fake_lora_recv_fake.custom_fake = provide_no_response;
}

void network_server_prepare_downlink(uint32_t rx_window, uint8_t port, const uint8_t *payload,
				     size_t payload_len)
{
	prepare_server();
	expect_uplink(LORAWAN_MSG_UNCONFIRMED, false, port, payload, payload_len);
	build_application_downlink(false, false, port, payload, payload_len);
	downlink_rx_window = rx_window;
	invalid_downlink = NETWORK_SERVER_INVALID_DOWNLINK_NONE;
	fake_lora_recv_fake.custom_fake = provide_application_downlink;
}

void network_server_prepare_data_pending_downlink(uint32_t rx_window, uint8_t port,
						  const uint8_t *payload, size_t payload_len)
{
	prepare_server();
	expect_uplink(LORAWAN_MSG_UNCONFIRMED, false, port, payload, payload_len);
	build_application_downlink(false, true, port, payload, payload_len);
	downlink_rx_window = rx_window;
	invalid_downlink = NETWORK_SERVER_INVALID_DOWNLINK_NONE;
	fake_lora_recv_fake.custom_fake = provide_application_downlink;
}

void network_server_prepare_confirmed_downlink(uint32_t rx_window, uint8_t port,
					       const uint8_t *payload, size_t payload_len)
{
	prepare_server();
	expect_uplink(LORAWAN_MSG_UNCONFIRMED, false, port, payload, payload_len);
	build_application_downlink(true, false, port, payload, payload_len);
	downlink_rx_window = rx_window;
	invalid_downlink = NETWORK_SERVER_INVALID_DOWNLINK_NONE;
	fake_lora_recv_fake.custom_fake = provide_application_downlink;
}

void network_server_prepare_invalid_downlink(enum network_server_invalid_downlink response,
					     uint8_t port, const uint8_t *payload,
					     size_t payload_len)
{
	prepare_server();
	expect_uplink(LORAWAN_MSG_UNCONFIRMED, false, port, payload, payload_len);
	build_application_downlink(false, false, port, payload, payload_len);
	downlink_rx_window = 1U;
	invalid_downlink = response;
	fake_lora_recv_fake.custom_fake = provide_application_downlink;
}

void network_server_prepare_ack(uint32_t tx_attempt, uint8_t port, const uint8_t *payload,
				size_t payload_len)
{
	prepare_server();
	expect_uplink(LORAWAN_MSG_CONFIRMED, false, port, payload, payload_len);
	build_ack_downlink();
	ack_tx_attempt = tx_attempt;
	fake_lora_recv_fake.custom_fake = provide_ack;
}

void network_server_expect_tx_frequency(uint32_t frequency)
{
	expected_tx_frequency = frequency;
}

void network_server_expect_uplink_datarate(enum lorawan_datarate datarate)
{
	zassert_true(datarate < test_region_get()->datarate_count);
	expected_uplink_datarate = datarate;
}

void network_server_expect_rx_delay(uint8_t delay_seconds)
{
	zassert_true(delay_seconds > 0U);
	expected_rx1_delay_ms = delay_seconds * MSEC_PER_SEC;
}
