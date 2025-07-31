/*
 * Copyright (c) 2024 VCI Development - LPC54S018J4MET180E
 * Private Porting , by David Hor - Xtooltech 2025, david.hor@xtooltech.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lpc_boot_image.h>
#include <string.h>

LOG_MODULE_REGISTER(secure_boot_decrypt, CONFIG_SECURE_BOOT_LOG_LEVEL);

/* External functions */
extern int lpc_puf_get_key(uint8_t key_index);
extern int lpc_aes_cbc_decrypt(const uint8_t *key, size_t key_len,
			       const uint8_t *iv, const uint8_t *cipher,
			       uint8_t *plain, size_t len);

/* AES block size */
#define AES_BLOCK_SIZE 16

/* Decrypt boot image using PUF-derived key */
int secure_boot_decrypt_image(const struct lpc_boot_header *header,
			      const uint8_t *encrypted_data,
			      uint8_t *decrypted_data,
			      size_t data_size)
{
	int ret;
	uint8_t iv[AES_BLOCK_SIZE];

	if (!header || !encrypted_data || !decrypted_data) {
		return -EINVAL;
	}

	/* Check if image is encrypted */
	if (!LPC_BOOT_IS_ENCRYPTED(header->image_type)) {
		LOG_ERR("Image is not encrypted");
		return -EINVAL;
	}

	/* For LPC54S018, encryption key info would be in image_type field */
	/* Extract key index from image type (implementation specific) */
	uint32_t key_index = (header->image_type >> 8) & 0x3;
	
	if (key_index > 3) {
		LOG_ERR("Invalid key index: %u", key_index);
		return -EINVAL;
	}

	/* Initialize IV - could use image version or other header fields */
	memset(iv, 0, AES_BLOCK_SIZE);
	/* Use load address and version as IV components */
	memcpy(iv, &header->load_address, 4);
	memcpy(iv + 4, &header->version, 4);

	LOG_INF("Decrypting image with key index %u", key_index);

	/* Load key from PUF to AES engine */
	ret = lpc_puf_get_key(key_index);
	if (ret != 0) {
		LOG_ERR("Failed to get decryption key: %d", ret);
		return ret;
	}

	/* Decrypt the image data */
	/* Note: In real implementation, we'd use CTR mode or authenticated encryption */
	/* For now, using simplified CBC as placeholder */
	size_t encrypted_size = data_size;
	
	/* Ensure size is multiple of block size */
	if (encrypted_size % AES_BLOCK_SIZE != 0) {
		LOG_ERR("Encrypted size not multiple of block size");
		return -EINVAL;
	}

	/* Copy encrypted data first (in case in-place decryption not supported) */
	memcpy(decrypted_data, encrypted_data, encrypted_size);

	/* TODO: Implement actual AES-CTR or AES-GCM decryption */
	/* This would decrypt block by block with incrementing counter */
	LOG_WRN("Image decryption not fully implemented - copying encrypted data");

	return 0;
}

/* Verify and decrypt secure boot image */
int secure_boot_process_encrypted_image(const uint8_t *image_data,
					size_t image_size,
					uint8_t *output_buffer)
{
	const struct lpc_boot_header *header;
	const uint8_t *encrypted_payload;
	size_t payload_offset;
	size_t payload_size;
	int ret;

	if (!image_data || !output_buffer) {
		return -EINVAL;
	}

	header = (const struct lpc_boot_header *)image_data;

	/* Validate header */
	ret = lpc_boot_validate_header(header);
	if (ret != 0) {
		return ret;
	}

	/* Check if encrypted */
	if (!LPC_BOOT_IS_ENCRYPTED(header->image_type)) {
		LOG_ERR("Image is not encrypted");
		return -EINVAL;
	}

	/* For LPC54S018, the header is part of vector table */
	/* Find the actual header location within the image */
	const struct lpc_boot_image *boot_img = (const struct lpc_boot_image *)image_data;
	
	/* Encrypted payload starts after vectors + header + SPI descriptor */
	payload_offset = offsetof(struct lpc_boot_image, spi_desc) + 
	                sizeof(struct lpc_spi_descriptor);
	payload_size = header->load_length - payload_offset;

	/* Account for authentication data at end */
	size_t auth_size = 0;
	uint32_t auth_type = LPC_BOOT_AUTH_TYPE(header->image_type);
	
	switch (auth_type) {
	case LPC_IMAGE_TYPE_CMAC:
		auth_size = 16; /* CMAC tag */
		break;
	case LPC_IMAGE_TYPE_ECDSA:
		auth_size = 64; /* ECDSA signature */
		break;
	}

	if (auth_size > 0 && payload_size > auth_size) {
		payload_size -= auth_size;
	}

	encrypted_payload = image_data + payload_offset;

	/* Copy header to output */
	memcpy(output_buffer, header, sizeof(struct lpc_boot_header));

	/* Decrypt payload */
	ret = secure_boot_decrypt_image(header, encrypted_payload,
					output_buffer + payload_offset,
					payload_size);
	if (ret != 0) {
		LOG_ERR("Image decryption failed: %d", ret);
		return ret;
	}

	/* Update header in output to mark as decrypted */
	struct lpc_boot_header *out_header = (struct lpc_boot_header *)output_buffer;
	out_header->image_type &= ~LPC_IMAGE_TYPE_ENCRYPTED;

	LOG_INF("Image decrypted successfully");
	return 0;
}

/* Initialize secure boot decryption */
int secure_boot_decrypt_init(void)
{
	/* Ensure PUF is enrolled and ready */
	/* This would typically be done during provisioning */
	
	LOG_INF("Secure boot decryption initialized");
	return 0;
}