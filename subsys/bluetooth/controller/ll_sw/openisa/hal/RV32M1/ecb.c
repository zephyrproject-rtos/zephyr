/*
 * Copyright (c) 2016-2017 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 * Copyright 2019-2020 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <sys/dlist.h>
#include <sys/mempool_base.h>
#include <sys/byteorder.h>

#include "hal/ecb.h"

#define LOG_MODULE_NAME bt_ctlr_rv32m1_ecb
#include "common/log.h"
#include "hal/debug.h"
#include "fsl_cau3_ble.h"

void ecb_encrypt_be(uint8_t const *const key_be, uint8_t const *const clear_text_be,
		    uint8_t *const cipher_text_be)
{
	uint8_t keyAes[16] __aligned(4);
	status_t status;

	cau3_handle_t handle;

	memcpy(&keyAes, key_be, sizeof(keyAes));

	/* CAU3 driver supports 4 key slots. */
	handle.keySlot = kCAU3_KeySlot1;

	/* After encrypt/decrypt req is sent to CAU3, the Host CPU will
	 * execute WFE() until CAU3 signals task done by setting the event.
	 */
	handle.taskDone = kCAU3_TaskDonePoll;

	/* Loads the key into CAU3's DMEM and expands the AES key schedule */
	status = CAU3_AES_SetKey(CAU3, &handle, keyAes, sizeof(keyAes));
	if (status != kStatus_Success) {
		LOG_ERR("CAUv3 AES key set failed %d", status);
		return;
	}

	status = CAU3_AES_Encrypt(CAU3, &handle, clear_text_be, cipher_text_be);
	if (status != kStatus_Success) {
		LOG_ERR("CAUv3 AES encrypt failed %d", status);
		return;
	}
}

void ecb_encrypt(uint8_t const *const key_le, uint8_t const *const clear_text_le,
		 uint8_t *const cipher_text_le, uint8_t *const cipher_text_be)
{
	uint8_t keyAes[16] __aligned(4);
	uint8_t clear[16];
	uint8_t cipher[16];
	status_t status;

	cau3_handle_t handle;

	/* The security function e of the cryptographic toolbox in CAU as in STD
	 * The most significant octet of key corresponds to key[0], the most
	 * significant octet of plaintextData corresponds to in[0] and the most
	 * significant octet of encryptedData corresponds to out[0] using the
	 * notation specified in FIPS-197
	 * Thus, reverse the input parameters that are LSB to MSB format (le)
	 */
	sys_memcpy_swap(&keyAes, key_le, sizeof(keyAes));
	sys_memcpy_swap(&clear, clear_text_le, sizeof(clear));

	/* CAU3 driver supports 4 key slots. */
	handle.keySlot = kCAU3_KeySlot1;

	/* After encrypt/decrypt req is sent to CAU3, the Host CPU will
	 * execute WFE() until CAU3 signals task done by setting the event.
	 */
	handle.taskDone = kCAU3_TaskDonePoll;

	/* Loads the key into CAU3's DMEM and expands the AES key schedule */
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
		/* STD e function outputs in MSB thus reverse for the (le) */
		sys_memcpy_swap(cipher_text_le, &cipher[0], sizeof(cipher));
	}

	if (cipher_text_be) {
		memcpy(cipher_text_be, &cipher, sizeof(cipher));
	}
}

uint32_t ecb_encrypt_nonblocking(struct ecb *ecb)
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
 */
uint32_t ecb_ut(void)
{
	/*
	 * LTK = 0x4C68384139F574D836BCF34E9DFB01BF (MSO to LSO)
	 * SKD = SKDm || SKDs
	 * SKD (LSO to MSO)
	 * :0x13:0x02:0xF1:0xE0:0xDF:0xCE:0xBD:0xAC
	 * :0x79:0x68:0x57:0x46:0x35:0x24:0x13:0x02
	 * SK = Encrypt(LTK, SKD)
	 * SK (LSO to MSO)
	 * :0x66:0xC6:0xC2:0x27:0x8E:0x3B:0x8E:0x05
	 * :0x3E:0x7E:0xA3:0x26:0x52:0x1B:0xAD:0x99
	 */
	static const uint8_t ltk_le[16] = {
		0xbf, 0x01, 0xfb, 0x9d, 0x4e, 0xf3, 0xbc, 0x36,
		0xd8, 0x74, 0xf5, 0x39, 0x41, 0x38, 0x68, 0x4c
	};
	static const uint8_t skd_le[16] = {
		0x13, 0x02, 0xF1, 0xE0, 0xDF, 0xCE, 0xBD, 0xAC,
		0x79, 0x68, 0x57, 0x46, 0x35, 0x24, 0x13, 0x02
	};
	uint8_t key_le[16] = {};
	uint8_t key_ref_le[16] = {
		0x66, 0xC6, 0xC2, 0x27, 0x8E, 0x3B, 0x8E, 0x05,
		0x3E, 0x7E, 0xA3, 0x26, 0x52, 0x1B, 0xAD, 0x99
	};
	uint32_t status = kStatus_Success;
	uint8_t *key;

	/* calc the Session Key and compare vs the ref_le in LSO format */
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
