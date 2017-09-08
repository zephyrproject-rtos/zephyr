/*
* Copyright (c) 2017 RDA
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef __TC_RDA5981A_PRIV_H__
#define __TC_RDA5981A_PRIV_H__

#if RDA_HW_ACCELERATE_ENABLE
#define RDA_HW_AES_ENABLE   1
#endif

#define RDA_AES_ENC_MODE AES_ENC_MODE
#define RDA_AES_DEC_MODE AES_DEC_MODE

#define RDA_AES_KEY_TART	(1 << 1)
#define RDA_AES_KEY_SIZE	(16)

/* 128 bits iv value 16 bytes*/
#define RDA_AES_IV_LENGTH	(16)

struct tc_rda5981a_drv_state {
	int in_use;
	struct k_sem device_sem;
};

extern int rda_aes_crypt(struct device *dev, unsigned int *rk_buf,
			dma_mode mode, /* DEC/ENC*/
			unsigned int length, /* bytes */
			unsigned char *iv, /*CBC only*/
			const unsigned char *input,
			unsigned char *output);
#endif  /* __TC_RDA5981A_PRIV_H__ */
