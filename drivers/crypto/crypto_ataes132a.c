/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <kernel.h>
#include <string.h>
#include <device.h>
#include <drivers/i2c.h>
#include <assert.h>
#include <crypto/cipher.h>

#include "crypto_ataes132a_priv.h"

#define D10D24S 11
#define MAX_RETRIES 3
#define ATAES132A_AES_KEY_SIZE 16

/* ATAES132A can store up to 16 different crypto keys */
#define CRYPTO_MAX_SESSION 16

#define LOG_LEVEL CONFIG_CRYPTO_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(ataes132a);

static struct ataes132a_driver_state ataes132a_state[CRYPTO_MAX_SESSION];

static void ataes132a_init_states(void)
{
	int i;

	for (i = 0; i < ATAES132A_AES_KEY_SIZE; i++) {
		ataes132a_state[i].in_use = false;
		ataes132a_state[i].key_id = i;
	}
}

static int ataes132a_send_command(struct device *dev, u8_t opcode,
				  u8_t mode, u8_t *params,
				  u8_t nparams, u8_t *response,
				  u8_t *nresponse)
{
	int retry_count = 0;
	struct ataes132a_device_data *data = dev->driver_data;
	const struct ataes132a_device_config *cfg = dev->config->config_info;
	u8_t count;
	u8_t status;
	u8_t crc[2];
	int i, i2c_return;

	count = nparams + 5;
	if (count > 64) {
		LOG_ERR("command too large for command buffer");
		return -EDOM;
	}

	/* If there is a command in progress, idle wait until it is available.
	 * If there is concurrency protection around the driver, this should
	 * never happen.
	 */
	read_reg_i2c(data->i2c, cfg->i2c_addr, ATAES_STATUS_REG, &status);

	while (status & ATAES_STATUS_WIP) {
		k_busy_wait(D10D24S);
		read_reg_i2c(data->i2c, cfg->i2c_addr,
			     ATAES_STATUS_REG, &status);
	}

	data->command_buffer[0] = count;
	data->command_buffer[1] = opcode;
	data->command_buffer[2] = mode;
	for (i = 0; i < nparams; i++) {
		data->command_buffer[i + 3] = params[i];
	}

	/*Calculate command CRC*/
	ataes132a_atmel_crc(data->command_buffer, nparams + 3, crc);
	data->command_buffer[nparams + 3] = crc[0];
	data->command_buffer[nparams + 4] = crc[1];

	/*Reset i/O address start before sending a command*/
	write_reg_i2c(data->i2c, cfg->i2c_addr,
		      ATAES_COMMAND_ADDRR_RESET, 0x0);

	/*Send a command through the command buffer*/
	i2c_return = burst_write_i2c(data->i2c, cfg->i2c_addr,
				     ATAES_COMMAND_MEM_ADDR,
				     data->command_buffer, count);

	LOG_DBG("BURST WRITE RETURN: %d", i2c_return);

	/* Idle-waiting for the command completion*/
	do {
		k_busy_wait(D10D24S);
		read_reg_i2c(data->i2c, cfg->i2c_addr,
			     ATAES_STATUS_REG, &status);
	} while (status & ATAES_STATUS_WIP);

	if (status & ATAES_STATUS_CRC) {
		LOG_ERR("incorrect CRC command");
		return -EINVAL;
	}

	if (!(status & ATAES_STATUS_RDY)) {
		LOG_ERR("expected response is not in place");
		return -EINVAL;
	}

	/* Read the response */
	burst_read_i2c(data->i2c, cfg->i2c_addr,
		       ATAES_COMMAND_MEM_ADDR,
		       data->command_buffer, 64);

	count = data->command_buffer[0];

	/* Calculate and validate response CRC */
	ataes132a_atmel_crc(data->command_buffer, count - 2, crc);

	LOG_DBG("COMMAND CRC %x%x", data->command_buffer[count - 2],
		     data->command_buffer[count - 1]);
	LOG_DBG("CALCULATED CRC %x%x", crc[0], crc[1]);

	/* If CRC fails retry reading MAX RETRIES times */
	while (crc[0] != data->command_buffer[count - 2] ||
	       crc[1] != data->command_buffer[count - 1]) {
		if (retry_count > MAX_RETRIES - 1) {
			LOG_ERR("response crc validation rebase"
				    " max retries");
			return -EINVAL;
		}

		burst_read_i2c(data->i2c, cfg->i2c_addr,
			       ATAES_COMMAND_MEM_ADDR,
			       data->command_buffer, 64);

		count = data->command_buffer[0];

		ataes132a_atmel_crc(data->command_buffer, count -  2, crc);
		retry_count++;

		LOG_DBG("COMMAND RETRY %d", retry_count);
		LOG_DBG("COMMAND CRC %x%x",
			    data->command_buffer[count - 2],
			    data->command_buffer[count - 1]);
		LOG_DBG("CALCULATED CRC %x%x", crc[0], crc[1]);
		}

	if ((status & ATAES_STATUS_ERR) || data->command_buffer[1] != 0x00) {
		LOG_ERR("command execution error %x",
			    data->command_buffer[1]);
		return -EIO;
	}

	LOG_DBG("Read the response count: %d", count);

	for (i = 0; i < count - 3; i++) {
		response[i] = data->command_buffer[i + 1];
	}

	*nresponse = count - 3;

	return 0;
}

int ataes132a_init(struct device *dev)
{
	struct ataes132a_device_data *ataes132a = dev->driver_data;
	const struct ataes132a_device_config *cfg = dev->config->config_info;
	u32_t i2c_cfg;

	LOG_DBG("ATAES132A INIT");

	ataes132a->i2c = device_get_binding((char *)cfg->i2c_port);
	if (!ataes132a->i2c) {
		LOG_DBG("ATAE132A master controller not found!");
		return -EINVAL;
	}

	i2c_cfg = I2C_MODE_MASTER | I2C_SPEED_SET(ATAES132A_BUS_SPEED);

	i2c_configure(ataes132a->i2c, i2c_cfg);

	k_sem_init(&ataes132a->device_sem, 1, UINT_MAX);

	ataes132a_init_states();

	return 0;
}

int ataes132a_aes_ccm_decrypt(struct device *dev,
			      u8_t key_id,
			      struct ataes132a_mac_mode *mac_mode,
			      struct ataes132a_mac_packet *mac_packet,
			      struct cipher_aead_pkt *aead_op,
			      u8_t *nonce_buf)
{
	u8_t command_mode = 0x0;
	struct ataes132a_device_data *data = dev->driver_data;
	u8_t out_len;
	u8_t in_buf_len;
	u8_t return_code;
	u8_t expected_out_len;
	u8_t param_buffer[52];

	if (!aead_op) {
		LOG_ERR("Parameter cannot be null");
		return -EINVAL;
	}

	if (!aead_op->pkt) {
		LOG_ERR("Parameter cannot be null");
		return -EINVAL;
	}

	in_buf_len = aead_op->pkt->in_len;
	expected_out_len = aead_op->pkt->out_len;

	/*The KeyConfig[EKeyID].ExternalCrypto bit must be 1b.*/
	if (!(ataes132a_state[key_id].key_config & ATAES_KEYCONFIG_EXTERNAL)) {
		LOG_ERR("key %x external mode disabled", key_id);
		return -EINVAL;
	}

	if (in_buf_len != 16U && in_buf_len != 32U) {
		LOG_ERR("ccm mode only accepts input blocks of 16"
			    " and 32 bytes");
		return -EINVAL;
	}

	if (expected_out_len > 32) {
		LOG_ERR("ccm mode cannot generate more than"
			    " 32 output bytes");
		return -EINVAL;
	}

	/* If KeyConfig[key_id].AuthKey is set, then prior authentication
	 * is required
	 */
	if (!(ataes132a_state[key_id].key_config & ATAES_KEYCONFIG_AUTHKEY)) {
		LOG_DBG("keep in mind key %x will require"
			    " previous authentication", key_id);
	}

	if (!aead_op->pkt->in_buf || !aead_op->pkt->out_buf) {
		return 0;
	}

	/* If the KeyConfig[EKeyID].RandomNonce bit is set
	 * the current nonce register content will be used.
	 * If there is an invalid random nonce or if there
	 * is no nonce synchronization between device
	 * the decrypt operation will fail accordingly.
	 */
	if (ataes132a_state[key_id].key_config & ATAES_KEYCONFIG_RAND_NONCE) {
		LOG_DBG("key %x requires random nonce,"
			    " nonce_buf will be ignored", key_id);

		LOG_DBG("current nonce register will be used");

	}

	k_sem_take(&data->device_sem, K_FOREVER);

	/* If the KeyConfig[EKeyID].RandomNonce bit is not set
	 * then the nonce send as parameter will be loaded into
	 * the nonce register.
	 */
	if (!(ataes132a_state[key_id].key_config & ATAES_KEYCONFIG_RAND_NONCE)
	    && nonce_buf) {
		param_buffer[0] = 0x0;
		param_buffer[1] = 0x0;
		param_buffer[2] = 0x0;
		param_buffer[3] = 0x0;
		memcpy(param_buffer + 4,  nonce_buf, 12);

		return_code = ataes132a_send_command(dev, ATAES_NONCE_OP,
						     0x0, param_buffer, 16,
						     param_buffer, &out_len);

		if (return_code != 0U) {
			LOG_ERR("nonce command ended with code %d",
				    return_code);
			k_sem_give(&data->device_sem);
			return -EINVAL;
		}

		if (param_buffer[0] != 0U) {
			LOG_ERR("nonce command failed with error"
				    " code %d", param_buffer[0]);
			k_sem_give(&data->device_sem);
			return -EIO;
		}
	}

	/* If the KeyConfig[EKeyID].RandomNonce bit is not set
	 * and the nonce send as parameter is a null value,
	 * the command will use the current nonce register value.
	 */
	if (!(ataes132a_state[key_id].key_config & ATAES_KEYCONFIG_RAND_NONCE)
	    && !nonce_buf) {
		LOG_DBG("current nonce register will be used");
	}

	/* Client decryption mode requires a MAC packet to specify the
	 * encryption key id and the MAC count of the encryption device
	 * to synchronize MAC generation
	 */
	if (mac_packet) {
		param_buffer[0] = mac_packet->encryption_key_id;
		param_buffer[2] = mac_packet->encryption_mac_count;
	} else {
		param_buffer[0] = 0x0;
		param_buffer[2] = 0x0;
		LOG_DBG("normal decryption mode"
			    " ignores mac_packet parameter");
	}

	/* Client decryption mode requires a MAC packet to specify
	 * if MAC counter, serial number and small zone number are
	 * included in MAC generation.
	 */
	if (mac_mode) {
		if (mac_mode->include_counter) {
			LOG_DBG("including usage counter in the MAC: "
				    "decrypt and encrypt dev must be the same");
			command_mode = command_mode | ATAES_MAC_MODE_COUNTER;
		}

		if (mac_mode->include_serial) {
			LOG_DBG("including serial number in the MAC: "
				    "decrypt and encrypt dev must be the same");
			command_mode = command_mode | ATAES_MAC_MODE_SERIAL;
		}

		if (mac_mode->include_smallzone) {
			LOG_DBG("including small zone in the MAC: "
				    "decrypt and encrypt dev share the "
				    "first four bytes of their small zone");
			command_mode = command_mode | ATAES_MAC_MODE_SMALLZONE;
		}
	}

	param_buffer[1] = key_id;
	param_buffer[3] = expected_out_len;
	if (aead_op->tag) {
		memcpy(param_buffer + 4,  aead_op->tag, 16);
	}
	memcpy(param_buffer + 20, aead_op->pkt->in_buf, in_buf_len);

	return_code = ataes132a_send_command(dev, ATAES_DECRYPT_OP,
					     command_mode, param_buffer,
					     in_buf_len + 4, param_buffer,
					     &out_len);

	if (return_code != 0U) {
		LOG_ERR("decrypt command ended with code %d", return_code);
		k_sem_give(&data->device_sem);
		return -EINVAL;
	}

	if (out_len < 2 || out_len > 33) {
		LOG_ERR("decrypt command response has invalid"
			    " size %d", out_len);
		k_sem_give(&data->device_sem);
		return -EINVAL;
	}

	if (param_buffer[0] != 0U) {
		LOG_ERR("legacy command failed with error"
			    " code %d", param_buffer[0]);
		k_sem_give(&data->device_sem);
		return -param_buffer[0];
	}

	if (expected_out_len != out_len - 1) {
		LOG_ERR("decrypted output data size %d and expected data"
			    " size %d  are different", out_len - 1,
			    expected_out_len);
		k_sem_give(&data->device_sem);
		return -EINVAL;
	}

	memcpy(aead_op->pkt->out_buf, param_buffer + 1, out_len - 1);

	k_sem_give(&data->device_sem);

	return 0;
}

int ataes132a_aes_ccm_encrypt(struct device *dev,
			      u8_t key_id,
			      struct ataes132a_mac_mode *mac_mode,
			      struct cipher_aead_pkt *aead_op,
			      u8_t *nonce_buf,
			      u8_t *mac_count)
{
	u8_t command_mode = 0x0;
	struct ataes132a_device_data *data = dev->driver_data;
	u8_t buf_len;
	u8_t out_len;
	u8_t return_code;
	u8_t param_buffer[40];

	if (!aead_op) {
		LOG_ERR("Parameter cannot be null");
		return -EINVAL;
	}

	if (!aead_op->pkt) {
		LOG_ERR("Parameter cannot be null");
		return -EINVAL;
	}

	buf_len = aead_op->pkt->in_len;

	/*The KeyConfig[EKeyID].ExternalCrypto bit must be 1b.*/
	if (!(ataes132a_state[key_id].key_config & ATAES_KEYCONFIG_EXTERNAL)) {
		LOG_ERR("key %x external mode disabled", key_id);
		return -EINVAL;
	}

	if (buf_len > 32) {
		LOG_ERR("only up to 32 bytes accepted for ccm mode");
			return -EINVAL;
	}

	/* If KeyConfig[key_id].AuthKey is set, then prior authentication
	 * is required
	 */
	if (!(ataes132a_state[key_id].key_config & ATAES_KEYCONFIG_AUTHKEY)) {
		LOG_DBG("keep in mind key %x will require"
			    " previous authentication", key_id);
	}

	if (!aead_op->pkt->in_buf || !aead_op->pkt->out_buf) {
		return 0;
	}

	/* If the KeyConfig[EKeyID].RandomNonce bit is set
	 * the current nonce register content will be used.
	 * If there is an invalid random nonce or if there
	 * is no nonce synchronization between device
	 * the decrypt operation will fail accordingly.
	 */
	if (ataes132a_state[key_id].key_config & ATAES_KEYCONFIG_RAND_NONCE) {
		LOG_DBG("key %x requires random nonce,"
			    " nonce_buf will be ignored", key_id);

		LOG_DBG("current nonce register will be used");

	}

	k_sem_take(&data->device_sem, K_FOREVER);

	/* If the KeyConfig[EKeyID].RandomNonce bit is not set
	 * then the nonce send as parameter will be loaded into
	 * the nonce register.
	 */
	if (!(ataes132a_state[key_id].key_config & ATAES_KEYCONFIG_RAND_NONCE)
	    && nonce_buf) {
		param_buffer[0] = 0x0;
		param_buffer[1] = 0x0;
		param_buffer[2] = 0x0;
		param_buffer[3] = 0x0;
		memcpy(param_buffer + 4,  nonce_buf, 12);

		return_code = ataes132a_send_command(dev, ATAES_NONCE_OP,
						     0x0, param_buffer, 16,
						     param_buffer, &out_len);

		if (return_code != 0U) {
			LOG_ERR("nonce command ended with code %d",
				    return_code);
			k_sem_give(&data->device_sem);
			return -EINVAL;
		}

		if (param_buffer[0] != 0U) {
			LOG_ERR("nonce command failed with error"
				    " code %d", param_buffer[0]);
			k_sem_give(&data->device_sem);
			return -EIO;
		}
	}
	/* If the KeyConfig[EKeyID].RandomNonce bit is not set
	 * and the nonce send as parameter is a null value,
	 * the command will use the current nonce register value.
	 */
	if (!(ataes132a_state[key_id].key_config & ATAES_KEYCONFIG_RAND_NONCE)
	    && !nonce_buf) {
		LOG_DBG("current nonce register will be used");
	}

	/* MAC packet to specify if MAC counter, serial number and small zone
	 * number are included in MAC generation.
	 */
	if (mac_mode) {
		if (mac_mode->include_counter) {
			LOG_DBG("including usage counter in the MAC: "
				    "decrypt and encrypt dev must be the same");
			command_mode = command_mode | ATAES_MAC_MODE_COUNTER;
		}

		if (mac_mode->include_serial) {
			LOG_DBG("including serial number in the MAC: "
				    "decrypt and encrypt dev must be the same");
			command_mode = command_mode | ATAES_MAC_MODE_SERIAL;
		}

		if (mac_mode->include_smallzone) {
			LOG_DBG("including small zone in the MAC: "
				    "decrypt and encrypt dev share the "
				    "first four bytes of their small zone");
			command_mode = command_mode | ATAES_MAC_MODE_SMALLZONE;
		}
	}

	param_buffer[0] = key_id;
	param_buffer[1] = buf_len;
	memcpy(param_buffer + 2, aead_op->pkt->in_buf, buf_len);

	return_code = ataes132a_send_command(dev, ATAES_ENCRYPT_OP,
					     command_mode, param_buffer,
					     buf_len + 2, param_buffer,
					     &out_len);

	if (return_code != 0U) {
		LOG_ERR("encrypt command ended with code %d", return_code);
		k_sem_give(&data->device_sem);
		return -EINVAL;
	}

	if (out_len < 33 || out_len > 49) {
		LOG_ERR("encrypt command response has invalid"
			    " size %d", out_len);
		k_sem_give(&data->device_sem);
		return -EINVAL;
	}

	if (param_buffer[0] != 0U) {
		LOG_ERR("encrypt command failed with error"
			    " code %d", param_buffer[0]);
		k_sem_give(&data->device_sem);
		return -EIO;
	}

	if (aead_op->tag) {
		memcpy(aead_op->tag, param_buffer + 1, 16);
	}
	memcpy(aead_op->pkt->out_buf, param_buffer + 17, out_len - 17U);

	if (mac_mode) {
		if (mac_mode->include_counter) {
			param_buffer[0] = 0x0;
			param_buffer[1] = 0x0;
			param_buffer[2] = 0x0;
			param_buffer[3] = 0x0;
			ataes132a_send_command(dev, ATAES_INFO_OP, 0x0,
					       param_buffer,	4,
					       param_buffer, &out_len);
			if (param_buffer[0] != 0U) {
				LOG_ERR("info command failed with error"
					    " code %d", param_buffer[0]);
				k_sem_give(&data->device_sem);
				return -EIO;
			}
			if (mac_count) {
				*mac_count = param_buffer[2];
			}
		}
	}

	k_sem_give(&data->device_sem);

	return 0;
}

int ataes132a_aes_ecb_block(struct device *dev,
			    u8_t key_id,
			    struct cipher_pkt *pkt)
{
	struct ataes132a_device_data *data = dev->driver_data;
	u8_t buf_len;
	u8_t out_len;
	u8_t return_code;
	u8_t param_buffer[19];

	if (!pkt) {
		LOG_ERR("Parameter cannot be null");
		return -EINVAL;
	}

	buf_len = pkt->in_len;
	if (buf_len > 16) {
		LOG_ERR("input block cannot be above 16 bytes");
		return -EINVAL;
	}

	/* AES ECB can only be executed if the ChipConfig.LegacyE configuration
	 * is set to 1 and if KeyConfig[key_id].LegacyOK is set to 1.
	 */
	if (!(ataes132a_state[key_id].chip_config & ATAES_CHIPCONFIG_LEGACYE)) {
		LOG_ERR("legacy mode disabled");
		return -EINVAL;
	}

	if (!(ataes132a_state[key_id].key_config & ATAES_KEYCONFIG_LEGACYOK)) {
		LOG_ERR("key %x legacy mode disabled", key_id);
		return -EINVAL;
	}

	LOG_DBG("Chip config: %x", ataes132a_state[key_id].chip_config);
	LOG_DBG("Key ID: %d", key_id);
	LOG_DBG("Key config: %x", ataes132a_state[key_id].key_config);

	/* If KeyConfig[key_id].AuthKey is set, then prior authentication
	 * is required
	 */
	if (!(ataes132a_state[key_id].key_config & ATAES_KEYCONFIG_AUTHKEY)) {
		LOG_DBG("keep in mind key %x will require"
			    " previous authentication", key_id);
	}

	if (!pkt->in_buf || !pkt->out_buf) {
		return 0;
	}

	k_sem_take(&data->device_sem, K_FOREVER);

	param_buffer[0] = 0x0;
	param_buffer[1] = key_id;
	param_buffer[2] = 0x0;
	memcpy(param_buffer + 3, pkt->in_buf, buf_len);
	(void)memset(param_buffer + 3 + buf_len, 0x0, 16 - buf_len);

	return_code = ataes132a_send_command(dev, ATAES_LEGACY_OP, 0x00,
					     param_buffer, buf_len + 3,
					     param_buffer, &out_len);

	if (return_code != 0U) {
		LOG_ERR("legacy command ended with code %d", return_code);
		k_sem_give(&data->device_sem);
		return -EINVAL;
	}

	if (out_len != 17U) {
		LOG_ERR("legacy command response has invalid"
			    " size %d", out_len);
		k_sem_give(&data->device_sem);
		return -EINVAL;
	}
	if (param_buffer[0] != 0U) {
		LOG_ERR("legacy command failed with error"
			    " code %d", param_buffer[0]);
		k_sem_give(&data->device_sem);
		return -EIO;
	}

	memcpy(pkt->out_buf, param_buffer + 1, 16);

	k_sem_give(&data->device_sem);

	return 0;
}

static int do_ccm_encrypt_mac(struct cipher_ctx *ctx,
			      struct cipher_aead_pkt *aead_op, u8_t *nonce)
{
	struct device *dev = ctx->device;
	struct ataes132a_driver_state *state = ctx->drv_sessn_state;
	struct ataes132a_mac_mode mac_mode;
	u8_t key_id;

	key_id = state->key_id;

	assert(*(u8_t *)ctx->key.handle == key_id);

	/* Removing all this salt from the MAC reduces the protection
	 * but allows any other crypto implementations to authorize
	 * the message.
	 */
	mac_mode.include_counter = false;
	mac_mode.include_serial = false;
	mac_mode.include_smallzone = false;

	if (aead_op->pkt->in_len <= 16 &&
	    aead_op->pkt->out_buf_max < 16) {
		LOG_ERR("Not enough space available in out buffer.");
		return -EINVAL;
	}

	if (aead_op->pkt->in_len > 16 &&
	    aead_op->pkt->out_buf_max < 32) {
		LOG_ERR("Not enough space available in out buffer.");
		return -EINVAL;
	}

	if (aead_op->pkt->in_len <= 16) {
		aead_op->pkt->out_len = 16;
	} else  if (aead_op->pkt->in_len > 16) {
		aead_op->pkt->out_len = 32;
	}

	if (aead_op->ad != NULL || aead_op->ad_len != 0U) {
		LOG_ERR("Associated data is not supported.");
		return -EINVAL;
	}

	ataes132a_aes_ccm_encrypt(dev, key_id, &mac_mode,
				  aead_op, nonce, NULL);

	return 0;
}

static int do_ccm_decrypt_auth(struct cipher_ctx *ctx,
			       struct cipher_aead_pkt *aead_op, u8_t *nonce)
{
	struct device *dev = ctx->device;
	struct ataes132a_driver_state *state = ctx->drv_sessn_state;
	struct ataes132a_mac_mode mac_mode;
	u8_t key_id;

	key_id = state->key_id;

	assert(*(u8_t *)ctx->key.handle == key_id);

	/* Removing all this salt from the MAC reduces the protection
	 * but allows any other crypto implementations to authorize
	 * the message.
	 */
	mac_mode.include_counter = false;
	mac_mode.include_serial = false;
	mac_mode.include_smallzone = false;

	if (aead_op->pkt->in_len <= 16 &&
	    aead_op->pkt->out_buf_max < 16) {
		LOG_ERR("Not enough space available in out buffer.");
		return -EINVAL;
	}

	if (aead_op->pkt->in_len > 16 &&
	    aead_op->pkt->out_buf_max < 32) {
		LOG_ERR("Not enough space available in out buffer.");
		return -EINVAL;
	}

	aead_op->pkt->ctx = ctx;

	if (aead_op->ad != NULL || aead_op->ad_len != 0U) {
		LOG_ERR("Associated data is not supported.");
		return -EINVAL;
	}

	/* Normal Decryption Mode will only decrypt host generated packets */
	ataes132a_aes_ccm_decrypt(dev, key_id, &mac_mode,
				  NULL, aead_op, nonce);

	return 0;
}

static int do_block(struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
	struct device *dev = ctx->device;
	struct ataes132a_driver_state *state = ctx->drv_sessn_state;
	u8_t key_id;

	key_id = state->key_id;

	assert(*(u8_t *)ctx->key.handle == key_id);

	if (pkt->out_buf_max < 16) {
		LOG_ERR("Not enough space available in out buffer.");
		return -EINVAL;
	}

	pkt->out_len = 16;

	return ataes132a_aes_ecb_block(dev, key_id, pkt);
}

static int ataes132a_session_free(struct device *dev,
				  struct cipher_ctx *session)
{
	struct ataes132a_driver_state *state = session->drv_sessn_state;

	ARG_UNUSED(dev);

	state->in_use = false;

	return 0;
}

static int ataes132a_session_setup(struct device *dev, struct cipher_ctx *ctx,
				   enum cipher_algo algo, enum cipher_mode mode,
				   enum cipher_op op_type)
{
	u8_t key_id = *((u8_t *)ctx->key.handle);
	struct ataes132a_device_data *data = dev->driver_data;
	const struct ataes132a_device_config *cfg = dev->config->config_info;
	u8_t config;

	if (ataes132a_state[key_id].in_use) {
		LOG_ERR("Session in progress");
		return -EINVAL;
	}
	if (mode == CRYPTO_CIPHER_MODE_CCM &&
	    ctx->mode_params.ccm_info.tag_len != 16U) {
		LOG_ERR("ATAES132A support 16 byte tag only.");
		return -EINVAL;
	}
	if (mode == CRYPTO_CIPHER_MODE_CCM &&
	    ctx->mode_params.ccm_info.nonce_len != 12U) {
		LOG_ERR("ATAES132A support 12 byte nonce only.");
		return -EINVAL;
	}

	ataes132a_state[key_id].in_use = true;
	read_reg_i2c(data->i2c, cfg->i2c_addr,
		     ATAES_KEYCFG_REG(key_id),
		     &config);
	ataes132a_state[key_id].key_config = config;
	read_reg_i2c(data->i2c, cfg->i2c_addr,
		     ATAES_CHIPCONFIG_REG,
		     &config);
	ataes132a_state[key_id].chip_config = config;

	ctx->drv_sessn_state = &ataes132a_state[key_id];
	ctx->device = dev;

	if (algo != CRYPTO_CIPHER_ALGO_AES) {
		LOG_ERR("ATAES132A unsupported algorithm");
		return -EINVAL;
	}

	/*ATAES132A support I2C polling only*/
	if (!(ctx->flags & CAP_SYNC_OPS)) {
		LOG_ERR("Async not supported by this driver");
		return -EINVAL;
	}

	if (ctx->keylen != ATAES132A_AES_KEY_SIZE) {
		LOG_ERR("ATAES132A unsupported key size");
		return -EINVAL;
	}

	if (op_type == CRYPTO_CIPHER_OP_ENCRYPT) {
		switch (mode) {
		case CRYPTO_CIPHER_MODE_ECB:
			ctx->ops.block_crypt_hndlr = do_block;
			break;
		case CRYPTO_CIPHER_MODE_CCM:
			ctx->ops.ccm_crypt_hndlr = do_ccm_encrypt_mac;
			break;
		default:
			LOG_ERR("ATAES132A unsupported mode");
			return -EINVAL;
		}
	} else {
		switch (mode) {
		case CRYPTO_CIPHER_MODE_ECB:
			ctx->ops.block_crypt_hndlr = do_block;
			break;
		case CRYPTO_CIPHER_MODE_CCM:
			ctx->ops.ccm_crypt_hndlr = do_ccm_decrypt_auth;
			break;
		default:
			LOG_ERR("ATAES132A unsupported mode");
			return -EINVAL;
		}
	}

	ctx->ops.cipher_mode = mode;

	return 0;
}

static int ataes132a_query_caps(struct device *dev)
{
	return (CAP_OPAQUE_KEY_HNDL | CAP_SEPARATE_IO_BUFS |
		CAP_SYNC_OPS | CAP_AUTONONCE);
}

const struct ataes132a_device_config ataes132a_config = {
	.i2c_port = CONFIG_CRYPTO_ATAES132A_I2C_PORT_NAME,
	.i2c_addr = CONFIG_CRYPTO_ATAES132A_I2C_ADDR,
	.i2c_speed = ATAES132A_BUS_SPEED,
};

static struct crypto_driver_api crypto_enc_funcs = {
	.begin_session = ataes132a_session_setup,
	.free_session = ataes132a_session_free,
	.crypto_async_callback_set = NULL,
	.query_hw_caps = ataes132a_query_caps,
};

struct ataes132a_device_data ataes132a_data;

DEVICE_AND_API_INIT(ataes132a, CONFIG_CRYPTO_ATAES132A_DRV_NAME, ataes132a_init,
		    &ataes132a_data, &ataes132a_config, POST_KERNEL,
		    CONFIG_CRYPTO_INIT_PRIORITY, (void *)&crypto_enc_funcs);
