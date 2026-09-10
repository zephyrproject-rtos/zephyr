/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2020 Lingao Meng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/net_buf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/mesh.h>
#include <zephyr/bluetooth/uuid.h>

#include "crypto.h"
#include "mesh.h"
#include "net.h"
#include "access.h"
#include "foundation.h"
#include "prov.h"

#define LOG_LEVEL CONFIG_BT_MESH_PROV_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_prov);

/* 10 power 32 represents 14 bytes maximum in little-endian format */
#define MAX_NUMERIC_OOB_BYTES 14

struct bt_mesh_prov_link bt_mesh_prov_link;
const struct bt_mesh_prov *bt_mesh_prov;

/* Verify specification defined length: */
BUILD_ASSERT(sizeof(bt_mesh_prov_link.conf_inputs) == 145,
	     "Confirmation inputs shall be 145 bytes");

int bt_mesh_prov_reset_state(void)
{
	int err;
	const size_t offset = offsetof(struct bt_mesh_prov_link, addr);

	atomic_clear(bt_mesh_prov_link.flags);
	(void)memset((uint8_t *)&bt_mesh_prov_link + offset, 0,
		     sizeof(bt_mesh_prov_link) - offset);

	err = bt_mesh_pub_key_gen();
	if (err) {
		LOG_ERR("Failed to generate public key (%d)", err);
		return err;
	}
	return 0;
}

static bt_mesh_output_action_t output_action(uint8_t action)
{
	switch (action) {
	case OUTPUT_OOB_BLINK:
		return BT_MESH_BLINK;
	case OUTPUT_OOB_BEEP:
		return BT_MESH_BEEP;
	case OUTPUT_OOB_VIBRATE:
		return BT_MESH_VIBRATE;
	case OUTPUT_OOB_NUMBER:
		return BT_MESH_DISPLAY_NUMBER;
	case OUTPUT_OOB_STRING:
		return BT_MESH_DISPLAY_STRING;
	default:
		return BT_MESH_NO_OUTPUT;
	}
}

static bt_mesh_input_action_t input_action(uint8_t action)
{
	switch (action) {
	case INPUT_OOB_PUSH:
		return BT_MESH_PUSH;
	case INPUT_OOB_TWIST:
		return BT_MESH_TWIST;
	case INPUT_OOB_NUMBER:
		return BT_MESH_ENTER_NUMBER;
	case INPUT_OOB_STRING:
		return BT_MESH_ENTER_STRING;
	default:
		return BT_MESH_NO_INPUT;
	}
}

static int check_output_auth(bt_mesh_output_action_t output, uint8_t size)
{
	if (!output) {
		return -EINVAL;
	}

	if (!(bt_mesh_prov->output_actions & output)) {
		return -EINVAL;
	}

	if (size > bt_mesh_prov->output_size || size == 0) {
		return -EINVAL;
	}

	return 0;
}

static int check_input_auth(bt_mesh_input_action_t input, uint8_t size)
{
	if (!input) {
		return -EINVAL;
	}

	if (!(bt_mesh_prov->input_actions & input)) {
		return -EINVAL;
	}

	if (size > bt_mesh_prov->input_size || size == 0) {
		return -EINVAL;
	}

	return 0;
}

static void get_auth_string(char *str, uint8_t size)
{
	static const char characters[36] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	bt_rand(str, size);

	for (int i = 0; i < size; i++) {
		str[i] = characters[(uint8_t)str[i] % (uint8_t)36];
	}

	memcpy(bt_mesh_prov_link.auth, str, size);
	memset(bt_mesh_prov_link.auth + size, 0,
			sizeof(bt_mesh_prov_link.auth) - size);
}

/* Computes 10^n - 1 in little endian array */
static void compute_pow10_minus1(uint8_t n, uint8_t *out, size_t *out_len)
{
	out[0] = 1; /* Start with value 1 at LSB */
	size_t len = 1;

	/* Compute 10^n */
	for (uint8_t i = 0; i < n; ++i) {
		uint16_t carry = 0;

		for (size_t j = 0; j < len; ++j) {
			uint16_t prod = out[j] * 10 + carry;

			out[j] = prod & 0xFF;
			carry = prod >> 8;
		}

		if (carry > 0 && len < MAX_NUMERIC_OOB_BYTES) {
			out[len] = carry;
			len++;
		}
	}

	/* Subtract 1 from the result (10^n - 1) */
	size_t j = 0;

	while (j < len) {
		if (out[j] > 0) {
			out[j] -= 1;
			break;
		}
		out[j] = 0xFF;
		j++;
	}

	/* Adjust length if highest byte becomes 0 */
	while (len > 1 && out[len - 1] == 0) {
		len--;
	}

	*out_len = len;
}

static void random_byte_shift(uint8_t max_inclusive, uint8_t *out)
{
	if (max_inclusive == 0) {
		*out = 0;
		return;
	}

	while (*out > max_inclusive) {
		*out >>= 1;
	}
}

static void generate_random_below_pow10(uint8_t n, uint8_t *output, size_t *out_len)
{
	uint8_t max_val[MAX_NUMERIC_OOB_BYTES] = {0};
	size_t max_len = 0;

	compute_pow10_minus1(n, max_val, &max_len);
	bt_rand(output, max_len);

	/* Generate random number less than max_val, starting from MSB in little-endian */
	for (int i = max_len - 1; i >= 0; --i) {
		random_byte_shift(max_val[i], &output[i]);
		if (output[i] < max_val[i]) {
			break;
		}
	}

	/* Ensure the result is not all zero */
	int all_zero = 1;

	for (size_t i = 0; i < max_len; ++i) {
		if (output[i] != 0) {
			all_zero = 0;
			break;
		}
	}

	if (all_zero) {
		output[0] = 1;
	}

	*out_len = max_len;
}

static void get_auth_number(uint8_t *rand_bytes, size_t *size)
{
	uint8_t auth_size = bt_mesh_prov_auth_size_get();

	generate_random_below_pow10(*size, rand_bytes, size);
	sys_memcpy_swap(bt_mesh_prov_link.auth + (auth_size - *size), rand_bytes, *size);
	memset(bt_mesh_prov_link.auth, 0, auth_size - *size);
}

int bt_mesh_prov_auth(bool is_provisioner, uint8_t method, uint8_t action, size_t size)
{
	bt_mesh_output_action_t output;
	bt_mesh_input_action_t input;
	uint8_t rand_bytes[PROV_IO_OOB_SIZE_MAX + 1] = {0};
	uint8_t auth_size = bt_mesh_prov_auth_size_get();
	int err;

	size = MIN(size, PROV_IO_OOB_SIZE_MAX);

	switch (method) {
	case AUTH_METHOD_NO_OOB:
		if (action || size) {
			return -EINVAL;
		}

		(void)memset(bt_mesh_prov_link.auth, 0, auth_size);
		return 0;
	case AUTH_METHOD_STATIC:
		if (action || size) {
			return -EINVAL;
		}

		atomic_set_bit(bt_mesh_prov_link.flags, OOB_STATIC_KEY);

		return 0;

	case AUTH_METHOD_OUTPUT:
		output = output_action(action);
		err = check_output_auth(output, size);
		if (err) {
			return err;
		}

		if (is_provisioner) {
			if (output == BT_MESH_DISPLAY_STRING) {
				input = BT_MESH_ENTER_STRING;
				atomic_set_bit(bt_mesh_prov_link.flags, WAIT_STRING);
			} else {
				input = BT_MESH_ENTER_NUMBER;
				atomic_set_bit(bt_mesh_prov_link.flags, WAIT_NUMBER);
			}

			return bt_mesh_prov->input(input, size);
		}

		atomic_set_bit(bt_mesh_prov_link.flags, NOTIFY_INPUT_COMPLETE);

		if (output == BT_MESH_DISPLAY_STRING) {
			get_auth_string(rand_bytes, MIN(size, auth_size));
			return bt_mesh_prov->output_string(rand_bytes);
		}

		get_auth_number(rand_bytes, &size);
#if defined CONFIG_BT_MESH_PROV_OOB_API_LEGACY
		uint32_t output_num;

		memcpy(&output_num, rand_bytes, sizeof(output_num));
		return bt_mesh_prov->output_number(output, output_num);
#else
		return bt_mesh_prov->output_numeric(output, rand_bytes, size);
#endif

	case AUTH_METHOD_INPUT:
		input = input_action(action);
		err = check_input_auth(input, size);
		if (err) {
			return err;
		}

		if (!is_provisioner) {
			if (input == BT_MESH_ENTER_STRING) {
				atomic_set_bit(bt_mesh_prov_link.flags, WAIT_STRING);
			} else {
				atomic_set_bit(bt_mesh_prov_link.flags, WAIT_NUMBER);
			}

			return bt_mesh_prov->input(input, size);
		}

		atomic_set_bit(bt_mesh_prov_link.flags, NOTIFY_INPUT_COMPLETE);

		if (input == BT_MESH_ENTER_STRING) {
			get_auth_string(rand_bytes, MIN(size, auth_size));
			return bt_mesh_prov->output_string(rand_bytes);
		}

		get_auth_number(rand_bytes, &size);
		output = BT_MESH_DISPLAY_NUMBER;
#if defined CONFIG_BT_MESH_PROV_OOB_API_LEGACY
		uint32_t input_num;

		memcpy(&input_num, rand_bytes, sizeof(input_num));
		return bt_mesh_prov->output_number(output, input_num);
#else
		return bt_mesh_prov->output_numeric(output, rand_bytes, size);
#endif

	default:
		return -EINVAL;
	}
}

#if defined CONFIG_BT_MESH_PROV_OOB_API_LEGACY
int bt_mesh_input_number(uint32_t num)
{
	uint8_t auth_size = bt_mesh_prov_auth_size_get();

	LOG_DBG("%u", num);

	if (!atomic_test_and_clear_bit(bt_mesh_prov_link.flags, WAIT_NUMBER)) {
		return -EINVAL;
	}

	sys_put_be32(num, &bt_mesh_prov_link.auth[auth_size - sizeof(num)]);

	bt_mesh_prov_link.role->input_complete();

	return 0;
}
#endif

int bt_mesh_input_numeric(uint8_t *numeric, size_t size)
{
	uint8_t auth_size = bt_mesh_prov_auth_size_get();

	LOG_HEXDUMP_DBG(numeric, size, "");

	if (size > MAX_NUMERIC_OOB_BYTES) {
		return -ENOTSUP;
	}

	if (!atomic_test_and_clear_bit(bt_mesh_prov_link.flags, WAIT_NUMBER)) {
		return -EINVAL;
	}

	sys_memcpy_swap(bt_mesh_prov_link.auth + (auth_size - size), numeric, size);
	memset(bt_mesh_prov_link.auth, 0, auth_size - size);

	bt_mesh_prov_link.role->input_complete();

	return 0;
}

int bt_mesh_input_string(const char *str)
{
	size_t size = strlen(str);

	LOG_DBG("%s", str);

	if (size > PROV_IO_OOB_SIZE_MAX || size > bt_mesh_prov_link.oob_size) {
		return -ENOTSUP;
	}

	if (!atomic_test_and_clear_bit(bt_mesh_prov_link.flags, WAIT_STRING)) {
		return -EINVAL;
	}

	memcpy(bt_mesh_prov_link.auth, str, size);
	memset(bt_mesh_prov_link.auth + size, 0, sizeof(bt_mesh_prov_link.auth) - size);

	bt_mesh_prov_link.role->input_complete();

	return 0;
}

const struct bt_mesh_prov *bt_mesh_prov_get(void)
{
	return bt_mesh_prov;
}

bool bt_mesh_prov_active(void)
{
	return atomic_test_bit(bt_mesh_prov_link.flags, LINK_ACTIVE);
}

static void prov_recv(const struct prov_bearer *bearer, void *cb_data,
		      struct net_buf_simple *buf)
{
	static const uint8_t op_len[10] = {
		[PROV_INVITE]         = PDU_LEN_INVITE,
		[PROV_CAPABILITIES]   = PDU_LEN_CAPABILITIES,
		[PROV_START]          = PDU_LEN_START,
		[PROV_PUB_KEY]        = PDU_LEN_PUB_KEY,
		[PROV_INPUT_COMPLETE] = PDU_LEN_INPUT_COMPLETE,
		[PROV_CONFIRM]        = PDU_LEN_CONFIRM,
		[PROV_RANDOM]         = PDU_LEN_RANDOM,
		[PROV_DATA]           = PDU_LEN_DATA,
		[PROV_COMPLETE]       = PDU_LEN_COMPLETE,
		[PROV_FAILED]         = PDU_LEN_FAILED,
	};

	uint8_t type = buf->data[0];

	LOG_DBG("type 0x%02x len %u", type, buf->len);

	if (type >= ARRAY_SIZE(bt_mesh_prov_link.role->op)) {
		LOG_ERR("Unknown provisioning PDU type 0x%02x", type);
		bt_mesh_prov_link.role->error(PROV_ERR_NVAL_PDU);
		return;
	}

	if ((type != PROV_FAILED && type != bt_mesh_prov_link.expect) ||
	    !bt_mesh_prov_link.role->op[type]) {
		LOG_WRN("Unexpected msg 0x%02x != 0x%02x", type, bt_mesh_prov_link.expect);
		bt_mesh_prov_link.role->error(PROV_ERR_UNEXP_PDU);
		return;
	}

	uint8_t expected = 1 + op_len[type];

	if (type == PROV_CONFIRM || type == PROV_RANDOM) {
		/* Expected length depends on Auth size */
		expected = 1 + bt_mesh_prov_auth_size_get();
	}

	if (buf->len != expected) {
		LOG_ERR("Invalid length %u for type 0x%02x", buf->len, type);
		bt_mesh_prov_link.role->error(PROV_ERR_NVAL_FMT);
		return;
	}

	bt_mesh_prov_link.role->op[type](&buf->data[1]);
}

static void prov_link_opened(const struct prov_bearer *bearer, void *cb_data)
{
	atomic_set_bit(bt_mesh_prov_link.flags, LINK_ACTIVE);

	if (bt_mesh_prov->link_open) {
		bt_mesh_prov->link_open(bearer->type);
	}

	bt_mesh_prov_link.bearer = bearer;

	if (bt_mesh_prov_link.role->link_opened) {
		bt_mesh_prov_link.role->link_opened();
	}
}

static void prov_link_closed(const struct prov_bearer *bearer, void *cb_data,
			     enum prov_bearer_link_status reason)
{
	LOG_DBG("%u", reason);

	if (bt_mesh_prov_link.role->link_closed) {
		bt_mesh_prov_link.role->link_closed(reason);
	}

	if (bt_mesh_prov->link_close) {
		bt_mesh_prov->link_close(bearer->type);
	}
}

static void prov_bearer_error(const struct prov_bearer *bearer, void *cb_data,
			      uint8_t err)
{
	if (bt_mesh_prov_link.role->error) {
		bt_mesh_prov_link.role->error(err);
	}
}

static const struct prov_bearer_cb prov_bearer_cb = {
	.link_opened = prov_link_opened,
	.link_closed = prov_link_closed,
	.error = prov_bearer_error,
	.recv = prov_recv,
};

const struct prov_bearer_cb *bt_mesh_prov_bearer_cb_get(void)
{
	return &prov_bearer_cb;
}

void bt_mesh_prov_complete(uint16_t net_idx, uint16_t addr)
{
	if (bt_mesh_prov->complete) {
		bt_mesh_prov->complete(net_idx, addr);
	}
}

void bt_mesh_prov_reset(void)
{
	if (IS_ENABLED(CONFIG_BT_MESH_PB_ADV)) {
		bt_mesh_pb_adv_reset();
	}

	if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT)) {
		bt_mesh_pb_gatt_reset();
	}

	bt_mesh_prov_reset_state();

	if (bt_mesh_prov->reset) {
		bt_mesh_prov->reset();
	}
}

int bt_mesh_prov_init(const struct bt_mesh_prov *prov_info)
{
	if (!prov_info) {
		LOG_ERR("No provisioning context provided");
		return -EINVAL;
	}

	bt_mesh_prov = prov_info;

	if (IS_ENABLED(CONFIG_BT_MESH_PB_ADV)) {
		bt_mesh_pb_adv_init();
	}

	if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT)) {
		bt_mesh_pb_gatt_init();
	}

	return bt_mesh_prov_reset_state();
}
