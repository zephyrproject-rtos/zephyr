/*
 * Copyright (c) 2021 Intellinium <giuliano.franchetto@intellinium.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>
#include <nrfx.h>
#if defined(CONFIG_LORAWAN_SE_GENERATE_DEVEUI)
#include <drivers/hwinfo.h>
#endif

#include "LoRaMacHeaderTypes.h"

#include "secure-element.h"
#include "secure-element-nvm.h"
#include "settings_se_priv.h"
#include <logging/log.h>
#include <mbedtls/aes.h>
#include <mbedtls/cipher.h>
#include <mbedtls/cmac.h>
#include "../lorawan_se.h"

LOG_MODULE_REGISTER(settings_se, CONFIG_LORAWAN_LOG_LEVEL);

static SecureElementNvmData_t *se_nvm;

#if defined(CONFIG_LORAWAN_SE_SETTINGS_USE_ENCRYPTED)
static const uint8_t *enc_key;
static size_t enc_key_size;
#endif

#if defined(CONFIG_LORAWAN_SE_GENERATE_DEVEUI)
static int settings_gen_deveui(uint8_t *buf)
{
	ssize_t len;

	len = hwinfo_get_device_id(buf, 8);
	if (len < 0) {
		return len;
	}

	__ASSERT_NO_MSG(len == 8);
	return 0;
}

static int settings_se_check_or_gen_deveui(SecureElementNvmData_t *nvm)
{
	int err;
	bool init = false;
	uint8_t devEUI[8] = {0};

	for (int i = 0; i < sizeof(nvm->DevEui); i++) {
		if (nvm->DevEui[i] != 0x00) {
			init = true;
			break;
		}
	}

	err = settings_gen_deveui(devEUI);
	if (err) {
		return err;
	}

	if (!init) {
		memcpy(nvm->DevEui, devEUI, 8);
	} else {
		__ASSERT(memcmp(nvm->DevEui, devEUI, 8) == 0,
			 "The stored devEUI is not the generated one!");
	}

	return 0;
}
#endif

#if defined(CONFIG_LORAWAN_SE_SETTINGS_USE_ENCRYPTED)
static int settings_se_encrypt_key(uint8_t *in, uint8_t *out)
{
	int err;
	mbedtls_aes_context ctx = {0};

	mbedtls_aes_init(&ctx);

	err = mbedtls_aes_setkey_enc(&ctx, enc_key, 8 * enc_key_size);
	if (err != 0) {
		LOG_ERR("Could not set key, error %d", err);
		mbedtls_aes_free(&ctx);
		return SECURE_ELEMENT_ERROR;
	}

	mbedtls_internal_aes_encrypt(&ctx, in, out);
	mbedtls_aes_free(&ctx);

	return 0;
}

static int settings_se_decrypt_key(uint8_t *in, uint8_t *out)
{
	int err;
	mbedtls_aes_context ctx = {0};

	mbedtls_aes_init(&ctx);
	err = mbedtls_aes_setkey_dec(&ctx, enc_key, 8 * enc_key_size);
	if (err != 0) {
		mbedtls_aes_free(&ctx);
		return SECURE_ELEMENT_ERROR;
	}

	mbedtls_internal_aes_decrypt(&ctx, in, out);
	mbedtls_aes_free(&ctx);

	return SECURE_ELEMENT_SUCCESS;
}
#endif

static int settings_se_init(SecureElementNvmData_t *nvm)
{
	se_nvm = nvm;

#if defined(CONFIG_LORAWAN_SE_GENERATE_DEVEUI)
	int err;

	err = settings_se_check_or_gen_deveui(nvm);
	if (err) {
		return err;
	}
#endif

	/* Nothing to do with JoinEUI as it is managed by the
	 * NVM backend, as the JoinEUI is public
	 */

	return SECURE_ELEMENT_SUCCESS;
}

static int settings_se_set_key(KeyIdentifier_t keyID, uint8_t *key)
{
	int err = 0;
	struct settings_se_key se_key;

	if (!key) {
		return SECURE_ELEMENT_ERROR_NPE;
	}

	if (keyID == MC_KEY_0 ||
		keyID == MC_KEY_1 ||
		keyID == MC_KEY_2 ||
		keyID == MC_KEY_3) {

		if (SecureElementAesEncrypt(key,
					    16,
					    MC_KE_KEY,
					    se_key.value)) {
			return SECURE_ELEMENT_FAIL_ENCRYPT;
		}
	}

#if defined(CONFIG_LORAWAN_SE_SETTINGS_USE_ENCRYPTED)
	err = settings_se_encrypt_key(key, se_key.value);
	if (err) {
		return err;
	}
#else
	memcpy(se_key.value, key, 16);
#endif
	err = settings_se_keys_save(keyID, &se_key);
	if (err) {
		LOG_ERR("Could not save key, error %d", err);
		return SECURE_ELEMENT_ERROR;
	}

	return SECURE_ELEMENT_SUCCESS;
}

static int settings_se_get_decrypted_key(KeyIdentifier_t id, uint8_t *out)
{
	int err;
	struct settings_se_key se_key;

	err = settings_se_keys_load(id, &se_key);
	if (err) {
		return err;
	}

#if defined(CONFIG_LORAWAN_SE_SETTINGS_USE_ENCRYPTED)
	err = settings_se_decrypt_key(se_key.value, out);
	if (err) {
		return err;
	}
#else
	memcpy(out, se_key.value, 16);
#endif
	return SECURE_ELEMENT_SUCCESS;
}

/** Computes a CMAC of a message using provided initial Bx block
 *
 *  cmac = aes128_cmac(keyID, blocks[i].Buffer)
 *
 * @param[IN]  micBxBuffer    - Buffer containing the initial Bx block
 * @param[IN]  buffer         - Data buffer
 * @param[IN]  size           - Data buffer size
 * @param[IN]  keyID          - Key identifier to determine the AES key to be used
 * @param[OUT] cmac           - Computed cmac
 * @retval                    - Status of the operation
 */
static SecureElementStatus_t compute_cmac(uint8_t *micBxBuffer,
					  uint8_t *buffer,
					  uint16_t size,
					  KeyIdentifier_t keyID,
					  uint32_t *cmac)
{
	int err;
	uint8_t cmac_val[16];
	uint8_t key[16] = {0};
	mbedtls_cipher_context_t m_ctx;
	const mbedtls_cipher_info_t *cipher_info;

	if (!buffer || !cmac) {
		return SECURE_ELEMENT_ERROR_NPE;
	}

	cipher_info = mbedtls_cipher_info_from_type(MBEDTLS_CIPHER_AES_128_ECB);
	if (!cipher_info) {
		return SECURE_ELEMENT_FAIL_CMAC;
	}

	mbedtls_cipher_init(&m_ctx);

	if (mbedtls_cipher_setup(&m_ctx, cipher_info)) {
		mbedtls_cipher_free(&m_ctx);
		return SECURE_ELEMENT_FAIL_CMAC;
	}

	err = settings_se_get_decrypted_key(keyID, key);
	if (err) {
		mbedtls_cipher_free(&m_ctx);
		return err;
	}

	err = mbedtls_cipher_cmac_starts(&m_ctx, key, 16 * 8);
	if (err) {
		mbedtls_cipher_free(&m_ctx);
		return SECURE_ELEMENT_FAIL_CMAC;
	}

	if (micBxBuffer != NULL) {
		err = mbedtls_cipher_cmac_update(&m_ctx, micBxBuffer, 16);
		if (err) {
			mbedtls_cipher_free(&m_ctx);
			return SECURE_ELEMENT_FAIL_CMAC;
		}
	}

	err = mbedtls_cipher_cmac_update(&m_ctx, buffer, size);
	if (err) {
		mbedtls_cipher_free(&m_ctx);
		return SECURE_ELEMENT_FAIL_CMAC;
	}

	err = mbedtls_cipher_cmac_finish(&m_ctx, cmac_val);
	if (err) {
		mbedtls_cipher_free(&m_ctx);
		return SECURE_ELEMENT_FAIL_CMAC;
	}

	*cmac = (uint32_t) ((uint32_t) cmac_val[3] << 24 |
		(uint32_t) cmac_val[2] << 16 |
		(uint32_t) cmac_val[1] << 8 |
		(uint32_t) cmac_val[0]);

	mbedtls_cipher_free(&m_ctx);
	return SECURE_ELEMENT_SUCCESS;
}

static int settings_se_compute_cmac(uint8_t *micBxBuffer,
				    uint8_t *buffer,
				    uint16_t size,
				    KeyIdentifier_t keyID,
				    uint32_t *cmac)
{
	if (keyID >= LORAMAC_CRYPTO_MULTICAST_KEYS) {
		return SECURE_ELEMENT_ERROR_INVALID_KEY_ID;
	}

	return compute_cmac(micBxBuffer, buffer, size, keyID, cmac);
}

static int settings_se_verify_cmac(uint8_t *buffer, uint16_t size,
				   uint32_t expectedCmac,
				   KeyIdentifier_t keyID)
{
	uint32_t compCmac = 0;
	SecureElementStatus_t err;

	if (buffer == NULL) {
		return SECURE_ELEMENT_ERROR_NPE;
	}

	err = compute_cmac(NULL, buffer, size, keyID, &compCmac);
	if (err != SECURE_ELEMENT_SUCCESS) {
		return err;
	}

	if (expectedCmac != compCmac) {
		err = SECURE_ELEMENT_FAIL_CMAC;
	}

	return err;
}

static int settings_se_encrypt(uint8_t *buffer,
			       uint16_t size,
			       KeyIdentifier_t keyID,
			       uint8_t *encBuffer)
{
	uint8_t local_key[16] = {0};
	SecureElementStatus_t err;
	mbedtls_aes_context ctx = {0};

	if (!buffer || !encBuffer) {
		return SECURE_ELEMENT_ERROR_NPE;
	}

	/* Check if the size is divisible by 16 */
	if ((size % 16) != 0) {
		return SECURE_ELEMENT_ERROR_BUF_SIZE;
	}

	err = settings_se_get_decrypted_key(keyID, local_key);
	if (err) {
		return err;
	}

	mbedtls_aes_init(&ctx);

	err = mbedtls_aes_setkey_enc(&ctx, local_key, 8 * 16);
	if (err != 0) {
		memset(local_key, 0, 16);
		mbedtls_aes_free(&ctx);
		LOG_ERR("Could not set shadow KMU ECB encrypt key.");
		return err;
	}

	for (int i = 0; i < size / 16; i++) {
		mbedtls_internal_aes_encrypt(&ctx,
					     &buffer[i * 16],
					     &encBuffer[i * 16]);
	}

	memset(local_key, 0, 16);
	mbedtls_aes_free(&ctx);

	return 0;
}

static int settings_se_derive(uint8_t *input,
			      KeyIdentifier_t rootKeyID,
			      KeyIdentifier_t targetKeyID)
{
	uint8_t key[16] = {0};
	SecureElementStatus_t err;

	if (!input) {
		return SECURE_ELEMENT_ERROR_NPE;
	}

	/* In case of MC_KE_KEY, only McRootKey can be used as root key */
	if (targetKeyID == MC_KE_KEY && rootKeyID != MC_ROOT_KEY) {
		return SECURE_ELEMENT_ERROR_INVALID_KEY_ID;
	}

	err = SecureElementAesEncrypt(input, 16, rootKeyID, key);
	if (err != SECURE_ELEMENT_SUCCESS) {
		return err;
	}

	err = SecureElementSetKey(targetKeyID, key);
	if (err != SECURE_ELEMENT_SUCCESS) {
		return err;
	}

	return SECURE_ELEMENT_SUCCESS;
}

int settings_process_join_accept(JoinReqIdentifier_t joinReqType,
				 uint8_t *joinEui,
				 uint16_t devNonce,
				 uint8_t *encJoinAccept,
				 uint8_t encJoinAcceptSize,
				 uint8_t *decJoinAccept,
				 uint8_t *versionMinor)
{
#if (USE_LRWAN_1_1_X_CRYPTO == 0)
	ARG_UNUSED(joinEui);
	ARG_UNUSED(devNonce);
#endif

	uint32_t mic;
	KeyIdentifier_t encKeyID = NWK_KEY;

	if (!encJoinAccept ||
		!decJoinAccept ||
		!versionMinor) {
		return SECURE_ELEMENT_ERROR_NPE;
	}

	/* Check that frame size isn't bigger than a
	 * JoinAccept with CFList size
	 */
	if (encJoinAcceptSize > LORAMAC_JOIN_ACCEPT_FRAME_MAX_SIZE) {
		return SECURE_ELEMENT_ERROR_BUF_SIZE;
	}

	if (joinReqType != JOIN_REQ) {
		encKeyID = J_S_ENC_KEY;
	}

	memcpy(decJoinAccept, encJoinAccept, encJoinAcceptSize);

	if (SecureElementAesEncrypt(encJoinAccept + LORAMAC_MHDR_FIELD_SIZE,
				    encJoinAcceptSize - LORAMAC_MHDR_FIELD_SIZE,
				    encKeyID,
				    decJoinAccept + LORAMAC_MHDR_FIELD_SIZE)) {
		return SECURE_ELEMENT_FAIL_ENCRYPT;
	}

	*versionMinor = ((decJoinAccept[11] & 0x80) == 0x80) ? 1 : 0;

	mic = ((uint32_t) decJoinAccept[encJoinAcceptSize
		- LORAMAC_MIC_FIELD_SIZE] << 0);
	mic |= ((uint32_t) decJoinAccept[encJoinAcceptSize
		- LORAMAC_MIC_FIELD_SIZE + 1] << 8);
	mic |= ((uint32_t) decJoinAccept[encJoinAcceptSize
		- LORAMAC_MIC_FIELD_SIZE + 2] << 16);
	mic |= ((uint32_t) decJoinAccept[encJoinAcceptSize
		- LORAMAC_MIC_FIELD_SIZE + 3] << 24);

	/*  - Header buffer to be used for MIC computation
	 *        - LoRaWAN 1.0.x : micHeader = [MHDR(1)]
	 *        - LoRaWAN 1.1.x : micHeader = [JoinReqType(1), JoinEUI(8), DevNonce(2), MHDR(1)]
	 */

	/* Verify mic */
	if (*versionMinor == 0) {
		/* For LoRaWAN 1.0.x
		 *   cmac = aes128_cmac(NwkKey, MHDR |  JoinNonce | NetID |
		 *       DevAddr | DLSettings | RxDelay | CFList |
		 *   CFListType)
		 */
		size_t size = encJoinAcceptSize - LORAMAC_MIC_FIELD_SIZE;

		if (SecureElementVerifyAesCmac(decJoinAccept,
					       size,
					       mic,
					       NWK_KEY)) {
			return SECURE_ELEMENT_FAIL_CMAC;
		}
	}
#if (USE_LRWAN_1_1_X_CRYPTO == 1)
		/* TODO */
#endif
	else {
		return SECURE_ELEMENT_ERROR_INVALID_LORAWAM_SPEC_VERSION;
	}

	return SECURE_ELEMENT_SUCCESS;
}

static int settings_se_set_deveui(uint8_t *devEui)
{
#if defined(CONFIG_LORAWAN_SE_GENERATE_DEVEUI)
	ARG_UNUSED(devEui);

	return SECURE_ELEMENT_SUCCESS;
#else
	if (!devEui) {
		return SECURE_ELEMENT_ERROR_NPE;
	}
	memcpy(se_nvm->DevEui, devEui, SE_EUI_SIZE);
	return SECURE_ELEMENT_SUCCESS;
#endif
}

uint8_t *settings_get_deveui(void)
{
	return se_nvm->DevEui;
}

static int settings_se_set_join_eui(uint8_t *joinEui)
{
	if (!joinEui) {
		return SECURE_ELEMENT_ERROR_NPE;
	}

	memcpy(se_nvm->JoinEui, joinEui, SE_EUI_SIZE);

	return SECURE_ELEMENT_SUCCESS;
}

uint8_t *settings_get_join_eui(void)
{
	return se_nvm->JoinEui;
}

static int settings_se_set_pin(uint8_t *pin)
{
	ARG_UNUSED(pin);

	return SECURE_ELEMENT_SUCCESS;
}

uint8_t *settings_se_get_pin(void)
{
	return se_nvm->Pin;
}

#if defined(CONFIG_LORAWAN_SE_SETTINGS_USE_ENCRYPTED)
int lorawan_se_set_enc_key(const uint8_t *key, size_t size)
{
	enc_key = key;
	enc_key_size = size;

	return 0;
}
#endif

static const struct lorawan_se settings_se = {
	.init = settings_se_init,
	.set_key = settings_se_set_key,
	.compute_cmac = settings_se_compute_cmac,
	.verify_cmac = settings_se_verify_cmac,
	.encrypt = settings_se_encrypt,
	.derive = settings_se_derive,
	.process_join_accept = settings_process_join_accept,
	.set_deveui = settings_se_set_deveui,
	.get_deveui = settings_get_deveui,
	.set_joineui = settings_se_set_join_eui,
	.get_joineui = settings_get_join_eui,
	.set_pin = settings_se_set_pin,
	.get_pin = settings_se_get_pin
};

#include <init.h>
static int settings_se_register(const struct device *device)
{
	ARG_UNUSED(device);

	lorawan_se_register(&settings_se);

	return 0;
}
SYS_INIT(settings_se_register, POST_KERNEL, 0);
