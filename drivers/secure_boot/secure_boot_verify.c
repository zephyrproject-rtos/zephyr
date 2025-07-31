/*
 * Copyright (c) 2024 VCI Development - LPC54S018J4MET180E
 * Private Porting , by David Hor - Xtooltech 2025, david.hor@xtooltech.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/flash.h>
#include <lpc_boot_image.h>

LOG_MODULE_REGISTER(secure_boot_verify, CONFIG_SECURE_BOOT_LOG_LEVEL);

/* External functions from crypto drivers */
extern int lpc_aes_cmac_authenticate(const uint8_t *key, size_t key_len,
				     const uint8_t *data, size_t data_len,
				     uint8_t *mac);
extern int lpc_sha256_hash(const uint8_t *data, size_t data_len, uint8_t *digest);
extern bool secure_boot_check_version(uint8_t image_version);
extern int lpc_ecdsa_verify_image(const uint8_t *pubkey_x, const uint8_t *pubkey_y,
				  const uint8_t *hash, const uint8_t *sig_r, const uint8_t *sig_s);

/* Default keys for development - REPLACE WITH SECURE KEYS IN PRODUCTION */
#ifdef CONFIG_LPC54S018_DEV_KEYS
static const uint8_t dev_cmac_key[16] = {
	0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
	0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
};

/* ECDSA P-256 public key for development */
static const uint8_t dev_ecdsa_pubkey_x[32] = {
	0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
	0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00,
	0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
	0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00
};

static const uint8_t dev_ecdsa_pubkey_y[32] = {
	0x00, 0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99,
	0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11,
	0x00, 0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99,
	0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11
};
#endif

/* Verify CMAC authentication */
static int verify_cmac(const struct lpc_boot_header *header,
		       const uint8_t *image_data, size_t image_size)
{
	uint8_t calculated_mac[16];
	const uint8_t *stored_mac;
	int ret;

	/* For LPC54S018, authentication data typically follows the image */
	/* Calculate location based on load_length */
	size_t auth_offset = header->load_length;
	
	if (auth_offset >= image_size - 16) {
		LOG_ERR("Invalid authentication data location");
		return -EINVAL;
	}

	/* MAC is stored after the image data */
	stored_mac = image_data + auth_offset;

	/* Calculate CMAC over header + payload (excluding MAC itself) */
	size_t auth_len = auth_offset;

#ifdef CONFIG_LPC54S018_DEV_KEYS
	ret = lpc_aes_cmac_authenticate(dev_cmac_key, sizeof(dev_cmac_key),
					image_data, auth_len, calculated_mac);
#else
	/* TODO: Get key from secure storage or PUF */
	LOG_ERR("Production keys not implemented");
	return -ENOTSUP;
#endif

	if (ret != 0) {
		LOG_ERR("CMAC calculation failed: %d", ret);
		return ret;
	}

	/* Compare MACs */
	if (memcmp(calculated_mac, stored_mac, 16) != 0) {
		LOG_ERR("CMAC verification failed");
		return -EBADMSG;
	}

	LOG_INF("CMAC verification successful");
	return 0;
}

/* Verify ECDSA signature (simplified - full implementation would use mbedTLS) */
static int verify_ecdsa(const struct lpc_boot_header *header,
			const uint8_t *image_data, size_t image_size)
{
	const struct lpc_ecdsa_signature *sig;
	uint8_t hash[32];
	int ret;

	/* For LPC54S018, signature typically follows the image */
	size_t sig_offset = header->load_length;
	
	if (sig_offset >= image_size - sizeof(struct lpc_ecdsa_signature)) {
		LOG_ERR("Invalid signature location");
		return -EINVAL;
	}

	/* Signature is stored after the image data */
	sig = (const struct lpc_ecdsa_signature *)(image_data + sig_offset);

	/* Calculate SHA-256 hash of header + payload (excluding signature) */
	size_t hash_len = sig_offset;
	ret = lpc_sha256_hash(image_data, hash_len, hash);
	if (ret != 0) {
		LOG_ERR("SHA-256 calculation failed: %d", ret);
		return ret;
	}

#ifdef CONFIG_LPC54S018_DEV_KEYS
	/* Use development public key */
	ret = lpc_ecdsa_verify_image(dev_ecdsa_pubkey_x, dev_ecdsa_pubkey_y,
				     hash, sig->r, sig->s);
#else
	/* TODO: Get public key from OTP or secure storage */
	LOG_ERR("Production ECDSA keys not implemented");
	return -ENOTSUP;
#endif

	if (ret != 0) {
		LOG_ERR("ECDSA verification failed: %d", ret);
		return -EBADMSG;
	}

	LOG_INF("ECDSA verification successful");
	return 0;
}

/* Main secure boot verification function */
int secure_boot_verify_image(const uint8_t *image_data, size_t image_size)
{
	const struct lpc_boot_header *header;
	uint32_t calculated_crc;
	int ret;

	if (!image_data || image_size < sizeof(struct lpc_boot_header)) {
		LOG_ERR("Invalid image data");
		return -EINVAL;
	}

	header = (const struct lpc_boot_header *)image_data;

	/* Validate header */
	ret = lpc_boot_validate_header(header);
	if (ret != 0) {
		LOG_ERR("Header validation failed: %d", ret);
		return ret;
	}

	/* Check load length */
	if (header->load_length > image_size) {
		LOG_ERR("Load length exceeds image size: load_length=%u, image_size=%u",
			header->load_length, image_size);
		return -EINVAL;
	}

	/* Verify CRC32 if not plain image */
	if (header->crc32 != 0) {
		/* CRC32 excludes the CRC field itself */
		calculated_crc = lpc_boot_crc32(image_data, header->load_length);
		if (calculated_crc != header->crc32) {
			LOG_ERR("CRC32 mismatch: expected=0x%08X, calculated=0x%08X",
				header->crc32, calculated_crc);
			return -EBADMSG;
		}
	}

	/* Check version against revocation */
	if (!secure_boot_check_version(header->version)) {
		LOG_ERR("Image version %u is revoked", header->version);
		return -EACCES;
	}

	/* Verify authentication based on image type */
	uint32_t auth_type = LPC_BOOT_AUTH_TYPE(header->image_type);
	
	switch (auth_type) {
	case LPC_IMAGE_TYPE_PLAIN:
		LOG_WRN("Plain image - no authentication");
		/* Only allowed if secure boot is disabled */
#ifdef CONFIG_LPC54S018_SECURE_BOOT
		LOG_ERR("Plain images not allowed when secure boot is enabled");
		return -EACCES;
#endif
		break;

	case LPC_IMAGE_TYPE_CRC:
		LOG_INF("CRC32 verification passed");
		break;

	case LPC_IMAGE_TYPE_CMAC:
		ret = verify_cmac(header, image_data, image_size);
		if (ret != 0) {
			return ret;
		}
		break;

	case LPC_IMAGE_TYPE_ECDSA:
		ret = verify_ecdsa(header, image_data, image_size);
		if (ret != 0) {
			return ret;
		}
		break;

	default:
		LOG_ERR("Unknown authentication type: %u", auth_type);
		return -EINVAL;
	}

	/* Check if image is encrypted */
	if (LPC_BOOT_IS_ENCRYPTED(header->image_type)) {
		LOG_ERR("Encrypted images not yet supported");
		return -ENOTSUP;
	}

	LOG_INF("Image verification successful");
	LOG_INF("  Type: 0x%08X", header->image_type);
	LOG_INF("  Version: %u", header->version);
	LOG_INF("  Load Address: 0x%08X", header->load_address);
	LOG_INF("  Length: %u bytes", header->load_length);

	return 0;
}

/* Verify and execute image */
int secure_boot_verify_and_jump(uint32_t image_address)
{
	const uint8_t *image_data = (const uint8_t *)image_address;
	const struct lpc_boot_header *header;
	typedef void (*app_entry_t)(void);
	app_entry_t app_entry;
	int ret;

	/* Read and verify image */
	ret = secure_boot_verify_image(image_data, 4 * 1024 * 1024); /* Max 4MB */
	if (ret != 0) {
		LOG_ERR("Image verification failed: %d", ret);
		return ret;
	}

	header = (const struct lpc_boot_header *)image_data;

	/* The entry point is the ResetISR, which is the second entry in vector table */
	uint32_t *vectors = (uint32_t *)image_address;
	uint32_t reset_handler = vectors[1];

	/* Prepare for jump */
	LOG_INF("Jumping to application at 0x%08X", reset_handler);

	/* Disable interrupts */
	irq_lock();

	/* Set vector table to the start of the image */
	/* SCB->VTOR = image_address & 0xFFFFFF00; */

	/* Jump to application reset handler */
	app_entry = (app_entry_t)reset_handler;
	app_entry();

	/* Should never return */
	return -EFAULT;
}