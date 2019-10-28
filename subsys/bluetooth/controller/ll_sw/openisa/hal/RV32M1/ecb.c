/*
 * Copyright (c) 2016-2017 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <sys/dlist.h>
#include <sys/mempool_base.h>

#include "util/mem.h"
#include "hal/ecb.h"

#define LOG_MODULE_NAME bt_ctlr_rv32m1_ecb
#include "common/log.h"
#include "hal/debug.h"
#include "fsl_cau3_ble.h"

void ecb_encrypt_be(u8_t const *const key_be, u8_t const *const clear_text_be,
		    u8_t * const cipher_text_be)
{
	uint8_t keyAes[16] __attribute__((aligned));
	uint8_t clear[16];
	uint8_t cipher[16];
	status_t status;

	cau3_handle_t handle;

	memcpy(&keyAes, key_be, sizeof(keyAes));
	memcpy(&clear, clear_text_be, sizeof(clear));

	/* CAU3 driver supports 4 key slots. */
	handle.keySlot = kCAU3_KeySlot1;

	/* After enryption/decryption request is sent to CAU3, the Host CPU will
	 * execute WFE instruction opcode until CAU3 signals task done by setting
	 * the event.
	 */
	handle.taskDone = kCAU3_TaskDonePoll;

	/* Loads the key into CAU3's DMEM memory and expands the AES key schedule */
	status = CAU3_AES_SetKey(CAU3, &handle, keyAes, sizeof(keyAes));
	if (status != kStatus_Success) {
		LOG_ERR("CAUv3 AES key set failed %d", status);
		return;
	}

	status = CAU3_AES_Encrypt(CAU3, &handle, clear, cipher);
	if (status != kStatus_Success) {
		LOG_ERR("CAUv3 AES encrypt failed %d", status);
		return;
	}

	memcpy(cipher_text_be, &cipher, sizeof(cipher));
}

void ecb_encrypt(u8_t const *const key_le, u8_t const *const clear_text_le,
		 u8_t * const cipher_text_le, u8_t * const cipher_text_be)
{
	uint8_t keyAes[16] __attribute__((aligned));
	uint8_t clear[16];
	uint8_t cipher[16];
	status_t status;

	cau3_handle_t handle;

	/* The security function e of the cryptographic toolbox in CAU as in STD
	 * The most significant octet of key corresponds to key[0], the most
	 * significant octet of plaintextData corresponds to in[0] and the most
	 * significant octet of encryptedData corresponds to out[0] using the
	 * notation specified in FIPS-197
	 * Thus, reverse the input parameters that are LSB to MSB format (le) */
	mem_rcopy((u8_t *)&keyAes, key_le, sizeof(keyAes));
	mem_rcopy((u8_t *)&clear, clear_text_le, sizeof(clear));

	/* CAU3 driver supports 4 key slots. */
	handle.keySlot = kCAU3_KeySlot1;

	/* After encryption/decryption request is sent to CAU3, the Host CPU will
	 * execute WFE instruction opcode until CAU3 signals task done by setting
	 * the event.
	 */
	handle.taskDone = kCAU3_TaskDonePoll;

	/* Loads the key into CAU3's DMEM memory and expands the AES key schedule */
	status = CAU3_AES_SetKey(CAU3, &handle, keyAes, sizeof(keyAes));
	if (status != kStatus_Success) {
		LOG_ERR("CAUv3 AES key set failed %d", status);
		return;
	}

	status = CAU3_AES_Encrypt(CAU3, &handle, clear, cipher);
	if (status != kStatus_Success) {
		LOG_ERR("CAUv3 AES encrypt failed %d", status);
		return;
	}

	if (cipher_text_le) {
		/* STD e function outputs in MSB to LSB thus reverse for the (le) */
		mem_rcopy(cipher_text_le, &cipher[0], sizeof(cipher));
	}

	if (cipher_text_be) {
		memcpy(cipher_text_be, &cipher, sizeof(cipher));
	}
}

u32_t ecb_encrypt_nonblocking(struct ecb *ecb)
{
	return 0;
}

void isr_ecb(void *param)
{
}

/*
 * Used by ULL as in this example:
 * ltk[16] is copied from the HCI packet, preserving the LSO to MSO format
 * skd[16] is copied from the PDU, preserving the LSO to MSO format
 * calc the Session Key and retrieve the MSO to LSO format because that is how
 * the PDU payload uses the encrypted data; MIC is also MSO to LSO.
	ecb_encrypt(&conn->llcp_enc.ltk[0],
				&conn->llcp.encryption.skd[0], NULL,
				&lll->ccm_rx.key[0]);
 * */
u32_t ecb_ut1(void)
{
	/*
	 * LTK = 0x4C68384139F574D836BCF34E9DFB01BF (MSO to LSO)
	 * SKD = SKDm || SKDs
	 * SKD (LSO to MSO)
	 * :0x13:0x02:0xF1:0xE0:0xDF:0xCE:0xBD:0xAC:0x79:0x68:0x57:0x46:0x35:0x24:0x13:0x02:
	 * SK = Encrypt(LTK, SKD)
	 * SK (LSO to MSO)
	 * :0x66:0xC6:0xC2:0x27:0x8E:0x3B:0x8E:0x05:0x3E:0x7E:0xA3:0x26:0x52:0x1B:0xAD:0x99:
	 * */
	static const u8_t ltk_le[16] =
		{0xbf, 0x01, 0xfb, 0x9d, 0x4e, 0xf3, 0xbc, 0x36,
		 0xd8, 0x74, 0xf5, 0x39, 0x41, 0x38, 0x68, 0x4c};
	static const u8_t skd_le[16] =
		{0x13, 0x02, 0xF1, 0xE0, 0xDF, 0xCE, 0xBD, 0xAC,
		 0x79, 0x68, 0x57, 0x46, 0x35, 0x24, 0x13, 0x02};
	u8_t key_le[16] = {};
	u8_t key_ref_le[16] =
		{0x66, 0xC6, 0xC2, 0x27, 0x8E, 0x3B, 0x8E, 0x05,
		 0x3E, 0x7E, 0xA3, 0x26, 0x52, 0x1B, 0xAD, 0x99};
	u8_t *key;
	u32_t status = kStatus_Success;

	/* calc the Session Key and compare vs the ref_le in LSO to MSO format */
	ecb_encrypt(ltk_le, skd_le, key = key_le, NULL);

	if (memcmp(key_ref_le, key, 16)) {
		printk("Failed session key unit test\n");
		status = kStatus_Fail;
	}

	printk("Session key: %02x %02x %02x %02x %02x %02x %02x %02x "
			"%02x %02x %02x %02x %02x %02x %02x %02x\n",
			key[0], key[1], key[2], key[3],
			key[4], key[5], key[6], key[7],
			key[8], key[9], key[10], key[11],
			key[12], key[13], key[14], key[15]);

	return status;
}

u32_t ecb_ut(void)
{
	static const uint8_t keyAes128[] __attribute__((aligned)) =
		{0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
		 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
	static const uint8_t plainAes128[] =
		{0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
		 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a};
	static const uint8_t cipherAes128[] =
		{0x3a, 0xd7, 0x7b, 0xb4, 0x0d, 0x7a, 0x36, 0x60,
		 0xa8, 0x9e, 0xca, 0xf3, 0x24, 0x66, 0xef, 0x97};
	uint8_t cipher[16];
	uint8_t output[16];
	status_t status;

	cau3_handle_t handle;

	handle.keySlot = kCAU3_KeySlot1;
	handle.taskDone = kCAU3_TaskDoneEvent;

	status = CAU3_AES_SetKey(CAU3, &handle, keyAes128, 16);
	if (status != kStatus_Success)
		return status;

	CAU3_AES_Encrypt(CAU3, &handle, plainAes128, cipher);
	if (memcmp(cipher, cipherAes128, 16) != 0)
		return kStatus_Fail;

	CAU3_AES_Decrypt(CAU3, &handle, cipher, output);
	if (memcmp(output, plainAes128, 16) != 0)
		return kStatus_Fail;

	return kStatus_Success;
}
