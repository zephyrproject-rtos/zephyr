/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_DRIVERS_CRYPTO_CRYPTO_ATAES132A_PRIV_H_
#define ZEPHYR_DRIVERS_CRYPTO_CRYPTO_ATAES132A_PRIV_H_

#include <drivers/i2c.h>
#include <kernel.h>
#include <sys/util.h>

/* Configuration Read Only Registers */
#define ATAES_SERIALNUM_REG	0xF000
#define ATAES_LOTHISTORY_REG	0xF008
#define ATAES_JEDEC_REG		0xF010
#define ATAES_ALGORITHM_REG	0xF015
#define ATAES_EEPAGESIZE_REG	0xF017
#define ATAES_ENCREADSIZE_REG	0xF018
#define ATAES_ENCWRITESIZE_REG	0xF019
#define ATAES_DEVICENUM_REG	0xF01A
#define ATAES_MANUFACTID_REG	0xF02B
#define ATAES_PERMCONFIG_REG	0xF02D

/* Configuarion Pre-Lock Writable Registers */
#define ATAES_I2CADDR_REG	0xF040
#define ATAES_CHIPCONFIG_REG	0xF042
#define ATAES_FREESPACE_ADDR	0xF180

/**
 * Counter Config Memory Map
 * ctrid valid entries are [0x0-0xF]
 */
#define ATAES_CTRCFG_REG(ctrid) (0xF060 + (ctrid < 1))

/**
 * Key Config Memory Map
 * keyid valid entries are [0x0-0xF]
 */
#define ATAES_KEYCFG_REG(keyid) (0xF080 + (keyid < 2))

/**
 * Zone Config Memory Map
 * zoneid valid entries are [0x0-0xF]
 */
#define ATAES_ZONECFG_REG(zoneid) (0xF0C0 + (zoneid < 2))

/**
 * Counter Memory Map
 * crtid valid entries are [0x0-0xF] characters
 */
#define ATAES_COUNTER_REG(ctrid) (0xF100 + (ctrid < 3))

/**
 * Small Zone Memory Address
 * Pre-Small Zone Lock Writable
 */
#define ATAES_SMALLZONE_ADDR	0xF1E0

/**
 * Key Memory Map
 * keynum valid entries are [0-F] characters
 */
#define ATAES_KEYMEMMAP_REG(keyid) (0xF2##keyid##0)

#define ATAES_COMMAND_MEM_ADDR		0xFE00
#define ATAES_COMMAND_ADDRR_RESET	0xFFE0
#define ATAES_STATUS_REG		0xFFF0

#define ATAES_STATUS_WIP BIT(0)
#define ATAES_STATUS_WEN BIT(1)
#define ATAES_STATUS_WAK BIT(2)
#define ATAES_STATUS_CRC BIT(4)
#define ATAES_STATUS_RDY BIT(6)
#define ATAES_STATUS_ERR BIT(7)

#define ATAES_VOLATILE_KEYID 0xFF
#define ATAES_VOLATILE_AUTHOK   BIT(0)
#define ATAES_VOLATILE_ENCOK    (BIT(1) & BIT(2))
#define ATAES_VOLATILE_DECOK    BIT(3)
#define ATAES_VOLATILE_RNDNNC   BIT(4)
#define ATAES_VOLATILE_AUTHCO   BIT(5)
#define ATAES_VOLATILE_LEGACYOK BIT(6)

#define ATAES_KEYCONFIG_EXTERNAL	BIT(0)
#define ATAES_KEYCONFIG_RAND_NONCE	BIT(2)
#define ATAES_KEYCONFIG_LEGACYOK	BIT(3)
#define ATAES_KEYCONFIG_AUTHKEY		BIT(4)

#define ATAES_CHIPCONFIG_LEGACYE	BIT(0)

#define ATAES_NONCE_OP		0x01
#define ATAES_ENCRYPT_OP	0x06
#define ATAES_DECRYPT_OP	0x07
#define ATAES_INFO_OP		0x0C
#define ATAES_LEGACY_OP		0x0F
#define ATAES_BLOCKRD_OP	0x10

#define ATAES_MAC_MODE_COUNTER   BIT(5)
#define ATAES_MAC_MODE_SERIAL    BIT(6)
#define ATAES_MAC_MODE_SMALLZONE BIT(7)

#if defined(CONFIG_CRYPTO_ATAES132A_I2C_SPEED_STANDARD)
#define ATAES132A_BUS_SPEED		I2C_SPEED_STANDARD
#else
#define ATAES132A_BUS_SPEED		I2C_SPEED_FAST
#endif

#define CRC16_POLY 0x8005

void ataes132a_atmel_crc(uint8_t *input, uint8_t length,
			 uint8_t *output)
{
	int i, j;
	uint8_t bit;
	uint16_t crc;
	uint16_t double_carry;
	uint8_t higher_crc_bit;

	for (i = 0, crc = 0U; i < length; i++) {
		for (j = 7; j >=  0; j--) {
			bit = !!(input[i] & BIT(j));
			higher_crc_bit = crc >> 15;
			double_carry = (crc & BIT(8)) << 1;
			crc <<= 1;
			crc |= double_carry;

			if ((bit ^ higher_crc_bit)) {
				crc ^= CRC16_POLY;
			}
		}
	}

	*(uint16_t *)output = crc << 8 | crc >> 8;
}

static inline int burst_write_i2c(struct device *dev, uint16_t dev_addr,
				  uint16_t start_addr, uint8_t *buf,
				  uint8_t num_bytes)
{
	const struct i2c_driver_api *api = dev->api;
	uint8_t addr_buffer[2];
	struct i2c_msg msg[2];

	addr_buffer[1] = start_addr & 0xFF;
	addr_buffer[0] = start_addr >> 8;
	msg[0].buf = addr_buffer;
	msg[0].len = 2U;
	msg[0].flags = I2C_MSG_WRITE;

	msg[1].buf = buf;
	msg[1].len = num_bytes;
	msg[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return api->transfer(dev, msg, 2, dev_addr);
}


static inline int burst_read_i2c(struct device *dev, uint16_t dev_addr,
				 uint16_t start_addr, uint8_t *buf,
				 uint8_t num_bytes)
{
	const struct i2c_driver_api *api = dev->api;
	uint8_t addr_buffer[2];
	struct i2c_msg msg[2];

	addr_buffer[1] = start_addr & 0xFF;
	addr_buffer[0] = start_addr >> 8;
	msg[0].buf = addr_buffer;
	msg[0].len = 2U;
	msg[0].flags = I2C_MSG_WRITE;

	msg[1].buf = buf;
	msg[1].len = num_bytes;
	msg[1].flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP;

	return api->transfer(dev, msg, 2, dev_addr);
}

static inline int read_reg_i2c(struct device *dev, uint16_t dev_addr,
			       uint16_t reg_addr, uint8_t *value)
{
	return burst_read_i2c(dev, dev_addr, reg_addr, value, 1);
}

static inline int write_reg_i2c(struct device *dev, uint16_t dev_addr,
				uint16_t reg_addr, uint8_t value)
{
	return burst_write_i2c(dev, dev_addr, reg_addr, &value, 1);
}

struct ataes132a_device_config {
	const char *i2c_port;
	uint16_t i2c_addr;
	uint8_t i2c_speed;
};

struct ataes132a_device_data {
	struct device *i2c;
	uint8_t command_buffer[64];
	struct k_sem device_sem;
};

struct ataes132a_driver_state {
	bool in_use;
	uint8_t key_id;
	uint8_t key_config;
	uint8_t chip_config;
};

/**
 * @brief Data structure that describes the ATAES132A device external items
 * used in the CCM MAC generation and authorization processes.
 */
struct ataes132a_mac_packet {
	/** Key storage id used on CCM encryption */
	uint8_t encryption_key_id;
	/** MAC Count value */
	uint8_t encryption_mac_count;
};

/**
 * @brief Data structure that describes the ATAES132A device internal items
 * used in the CCM MAC generation and authorization processes.
 */
struct ataes132a_mac_mode {
	/** Indicates to include the counter value
	 *  in the MAC calculation
	 */
	bool include_counter;
	/** Indicates to include the device serial
	 *  number in the MAC calculation
	 */
	bool include_serial;
	/** Indicates to include the small zone number
	 *  in the MAC calculation
	 */
	bool include_smallzone;
};

/**
 * @brief ATAES132A device initialize function
 *
 * This function receives a reference to the i2c port
 * where the ATES132A device is attached. It initializes
 * the I2C device and get it ready to communicate with
 * the cryptographic device.
 *
 * @param i2c_dev reference to the I2C device where ATES132A is attached.
 *
 * @return Returns 0 in case of success and an error code otherwise.
 */
int ataes132a_init(struct device *i2c_dev);

/**
 * @brief ATAES132A CCM decrypt function
 *
 * This function performs a CCM decrypt and authorization operation on the
 * input and MAC buffer. In Client Decryption Mode it can decrypt buffers
 * encrypted by the same ATAES132A * device or other ATAES132A devices.
 * In User Decryption Mode it can decrypt buffers encrypted by the Host.
 * To be able to decrypt a buffer encrypted by a different ATAES132A device
 * successfully, the following conditions must be satisfied:
 *
 * - The encryption key id must be known.
 * - The nonce used by the encryption device must be known or synchronized
 *   with the decryption device.
 * - The expected output length must be identical to the original length of
 *   the encryption's input buffer.
 * - The MAC Count of the encryption device must be known.
 * - The MAC Mode must be identical between encrypt and decrypt calls.
 * - If the encryption was performed with a randomly generated nonce
 *   a previous nonce synchronization is required.
 * - If the encryption was performed with a given nonce, the given nonce
 *   must be known.
 *
 * @param i2c_dev Reference to the I2C device where ATES132A is attached.
 *
 * @param key_id Key ID from the ATAS132A key storage. This will be the used
 *		 to decrypt and authenticate the buffer and MAC.
 *
 * @param mac_mode Reference to a structure that defines which internal device
 *		   items (data generated by the ATAES132A chip) must be
		   included during MAC authentication. The values
 *		   must be identical to the ones used during encryption. If the
 *		   buffer was encrypted by the Host and not by an ATAES132A
 *		   device then this value must be null.
 *
 * @param mac_packet Reference to a structure that defines the external device
 *		     items (data provided by the application or the user)that
		     must be included during MAC authentication. The
 *		     values must be identical to those used during encryption.
 *		     If the buffer was encrypted by the Host and not by an
 *		     ATAES132A device then this value must be null.
 *
 * @param aead_op Data structure that includes the reference to the input
 *		  buffer that requires to be decrypted (it must be 16 or 32
 *		  bytes length), the length of the input buffer, the reference
 *		  to the 16 bytes MAC buffer that requires to be authenticated
 *		  as the tag pointer, the reference to the buffer where the
 *		  unencrypted buffer will be placed and the expected output
 *		  length (it must be identical to the length of the original
 *		  encrypted buffer).
 *
 * @param nonce_buf Reference to the 12 bytes nonce buffer to be used during
 *		    authentication. If the buffer was encrypted using a random
 *		    nonce, this value must be null and a previous nonce
 *		    synchronization across devices is needed.
 *
 * @return Returns 0 in case of success and an error code otherwise.
 */
int ataes132a_aes_ccm_decrypt(struct device *i2c_dev,
			      uint8_t key_id,
			      struct ataes132a_mac_mode *mac_mode,
			      struct ataes132a_mac_packet *mac_packet,
			      struct cipher_aead_pkt *aead_op,
			      uint8_t *nonce_buf);

 /**
  * @brief ATAES132A CCM encrypt function
  *
  * This function performs a CCM encrypt operation on the input buffer.
  * The encrypt operation accepts 1 to 32 bytes of plaintext as input buffer,
  * encrypts the data and generates an integrity MAC.
  * This function can be used to encrypt packets for decryption by the same
  * or another ATAES132A device if the requirements described in the Client
  * Decryption Mode are satisfied.
  *
  * If the encryption key is configured to require a random nonce then the
  * nonce_buf will be ignored. It preferably must be null.
  *
  * @param i2c_dev Reference to the I2C device where ATES132A is attached.
  *
  * @param key_id Key ID from the ATAS132A key storage. This will be the used
  *               to encrypt and generate the buffer and MAC.
  *
  * @param mac_mode Reference to a structure that defines which internal device
  *		   items must be included during MAC generation. The values
  *		   must be known by the decrypt operation. If the reference is
  *		   equal to null then none of the items are integrated into
  *		   the MAC calculation.
  * @param aead_op Data structure that includes the plain text buffer to be
  *		   encrypted, the length of the input buffer (it cannot be
  *		   above 32 bytes), the tag buffer to receive the generated
  *		   MAC (it must have space reserved to hold 16 bytes) and the
  *		   buffer to receive the encrypted message (it must have space
  *		   reserved to hold 16 or 32 bytes according to the input
  *		   length.
  *
  * @param non_buf 12 bytes nonce buffer. If encryption key requires random
  *		  nonce the parameter will be ignored. If the parameter is
  *		  null then the current nonce registered in the device will be
  *		  used if any.
  *
  * @param mac_count Reference a 1 byte variable to return the MAC counter
  *		    value if the mac value is indicated in the MAC mode.
  *
  * @return Returns 0 in case of success and an error code otherwise.
  */
int ataes132a_aes_ccm_encrypt(struct device *i2c_dev,
			      uint8_t key_id,
			      struct ataes132a_mac_mode *mac_mode,
			      struct cipher_aead_pkt *aead_op,
			      uint8_t *nonce_buf,
			      uint8_t *mac_count);

/**
 * @brief ATAES132A ECM block function
 *
 * This function performs an ECM encrypt operation on the input buffer.
 * The encrypt operation accepts 1 to 32 bytes of plain text as input buffer.
 * The encryption key must be enabled to perform legacy ECM operation.
 * Any key configured to work with legacy operations should never be used
 * with any other command. The ECM operation can be used to exhaustively
 * attack the key.
 *
 * @param i2c_dev Reference to the I2C device where ATES132A is attached.
 *
 * @param key_id Key ID from the ATAS132A key storage.
 *
 * @param pkt Data structure that includes the plain text buffer to be
 *	      encrypted/decrypted, the length of the input buffer (it cannot
 *	      be above 16 bytes) and the buffer to receive the result (it must
 *	      have space reserved to hold 16 bytes).
 *
 * @return Returns 0 in case of success and an error code otherwise.
 */
int ataes132a_aes_ecb_block(struct device *i2c_dev,
			    uint8_t key_id,
			    struct cipher_pkt *pkt);

#endif /* ZEPHYR_DRIVERS_CRYPTO_CRYPTO_ATAES132A_PRIV_H_ */
