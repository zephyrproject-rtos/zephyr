/*
 * Copyright 2026 Linumiz
 * Copyright (c) 2026 Texas Instruments Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CRYPTO_MSP_AES_H_
#define ZEPHYR_DRIVERS_CRYPTO_MSP_AES_H_

#include <zephyr/crypto/cipher.h>
#include <zephyr/crypto/crypto.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#define AES_MSP_PWREN_MASK				BIT(0)
#define AES_MSP_PWREN_KEY_MASK			        GENMASK(31, 24)
#define AES_MSP_PWREN_KEY				FIELD_PREP(AES_MSP_PWREN_KEY_MASK, 0x26)

#ifdef CONFIG_CRYPTO_MSPM0_AES
#define AES_MSPM0_IMASK_AESRDY_MASK			BIT(0)
#define AES_MSPM0_ICLR_AESRDY_MASK			BIT(0)
#else
#define AESADV_MSP_IMASK_OUTPUTRDY_MASK			BIT(0)
#define AESADV_MSP_IMASK_INPUTRDY_MASK			BIT(1)
#define AESADV_MSP_IMASK_SAVEDCNTXTRDY_MASK		BIT(2)
#define AESADV_MSP_IMASK_CNTXTRDY_MASK			BIT(3)
#define AESADV_MSP_IMASK_INT_MASK			GENMASK(3, 0)

#define AESADV_MSP_ICLR_OUTPUTRDY_MASK			BIT(0)
#define AESADV_MSP_ICLR_INPUTRDY_MASK			BIT(1)
#define AESADV_MSP_ICLR_SAVEDCNTXTRDY_MASK		BIT(2)
#define AESADV_MSP_ICLR_CNTXTRDY_MASK			BIT(3)
#define AESADV_MSP_ICLR_INT_MASK			GENMASK(3, 0)
#endif

#ifdef CONFIG_CRYPTO_MSPM0_AES
#define AES_MSPM0_AESACTL0_SWRST			BIT(7)
#define AES_MSPM0_AESACTL0_CMEN				BIT(15)
#define AES_MSPM0_AESACTL0_CMX_MASK			GENMASK(6, 5) /* AES cipher mode select */
#define AES_MSPM0_AESACTL0_KLX_MASK			GENMASK(3, 2) /* AES key length */
#define AES_MSPM0_AESACTL0_OPX_MASK			GENMASK(1, 0) /* AES operation */

#define AES_MSPM0_AESACTL0_CMX_ECB			FIELD_PREP(AES_MSPM0_AESACTL0_CMX_MASK, 0x0)
#define AES_MSPM0_AESACTL0_CMX_CBC			FIELD_PREP(AES_MSPM0_AESACTL0_CMX_MASK, 0x1)
#define AES_MSPM0_AESACTL0_CMX_OFB			FIELD_PREP(AES_MSPM0_AESACTL0_CMX_MASK, 0x2)
#define AES_MSPM0_AESACTL0_CMX_CFB			FIELD_PREP(AES_MSPM0_AESACTL0_CMX_MASK, 0x3)

#define AES_MSPM0_AESACTL0_KLX_128			FIELD_PREP(AES_MSPM0_AESACTL0_KLX_MASK, 0x0)
#define AES_MSPM0_AESACTL0_KLX_256			FIELD_PREP(AES_MSPM0_AESACTL0_KLX_MASK, 0x2)

#define AES_MSPM0_AESACTL0_OPX_ENCRYPT			FIELD_PREP(AES_MSPM0_AESACTL0_OPX_MASK, 0x0)
#define AES_MSPM0_AESACTL0_OPX_DECRYPT			FIELD_PREP(AES_MSPM0_AESACTL0_OPX_MASK, 0x1)
#define AES_MSPM0_AESACTL0_OPX_GEN_FIRST_KEY		FIELD_PREP(AES_MSPM0_AESACTL0_OPX_MASK, 0x2)
#define AES_MSPM0_AESACTL0_OPX_DECRYPT_FIRST_KEY	FIELD_PREP(AES_MSPM0_AESACTL0_OPX_MASK, 0x3)

#define AES_MSPM0_AESACTL0_CMX_CTR			AES_MSPM0_AESACTL0_OPX_ENCRYPT

#define AES_MSPM0_AESASTAT_KEYWR			BIT(1)
#else
#define AESADV_MSP_CTRL_OUTPUT_RDY			BIT(0)
#define AESADV_MSP_CTRL_INPUT_RDY			BIT(1)
#define AESADV_MSP_CTRL_DIR				BIT(2)
#define AESADV_MSP_CTRL_KEYSIZE				GENMASK(4, 3)
#define AESADV_MSP_CTRL_CBC				BIT(5)
#define AESADV_MSP_CTRL_CTR				BIT(6)
#define AESADV_MSP_CTRL_CTR_WIDTH			GENMASK(8, 7)
#define AESADV_MSP_CTRL_ICM				BIT(9)
#define AESADV_MSP_CTRL_CFB				BIT(10)
#define AESADV_MSP_CTRL_CBCMAC				BIT(15)
#define AESADV_MSP_CTRL_GCM				GENMASK(17, 16)
#define AESADV_MSP_CTRL_CCM				BIT(18)
#define AESADV_MSP_CTRL_CCML				GENMASK(21, 19)
#define AESADV_MSP_CTRL_CCMM				GENMASK(24, 22)
#define AESADV_MSP_CTRL_OFB_GCM_CCM_CONT		BIT(26)
#define AESADV_MSP_CTRL_GET_DIGEST			BIT(27)
#define AESADV_MSP_CTRL_GCM_CONT			BIT(28)
#define AESADV_MSP_CTRL_SAVE_CNTXT			BIT(29)
#define AESADV_MSP_CTRL_SAVED_CNTXT_RDY			BIT(30)
#define AESADV_MSP_CTRL_CNTXT_RDY			BIT(31)

#define AESADV_MSP_CTRL_KEYSIZE_128                     FIELD_PREP(AESADV_MSP_CTRL_KEYSIZE, 0x1)
#define AESADV_MSP_CTRL_KEYSIZE_256                     FIELD_PREP(AESADV_MSP_CTRL_KEYSIZE, 0x3)
#endif

#define AES_HW_CAPS	(CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS | CAP_NO_IV_PREFIX)
#define AES_BLOCK_SIZE	16
#define AES_BLOCK_WORDS	AES_BLOCK_SIZE / 4

#define AES_128_KEY_WORDS    4U
#define AES_256_KEY_WORDS    8U

/*
 * The block cycle for AES module is 300 cycles (MSPM0_AES_BLOCK_CYC)
 * AES_BLOCK_TIMEOUT applies a safety margin i.e. 300 << 1 = 600 cycles
 * K_CYC(AES_BLOCK_TIMEOUT) converts this cycle count to a timeout
 * period in system ticks AES_SEM_TIMEOUT.
 */
#define MSPM0_AES_BLOCK_CYC	300
#define AES_BLOCK_TIMEOUT	(MSPM0_AES_BLOCK_CYC << 1)
#define AES_WAIT_TIMEOUT	K_USEC(10)
#define AES_SEM_TIMEOUT		K_CYC(AES_BLOCK_TIMEOUT)

#ifdef CONFIG_CRYPTO_MSPM0_AES
typedef struct {
	uint32_t reserved0[0x200];
	volatile uint32_t pwren;	/* Power Enable				@0x800h */
	volatile uint32_t rstctl;	/* Reset Control			@0x804h */
	uint32_t reserved1[3];
	volatile uint32_t stat;		/* Status Register			@0x814h */
	uint32_t reserved2[0x200];
	volatile uint32_t pdbgctl;	/* Peripheral Debug Control		@0x1018h */
	uint32_t reserved3[1];
	volatile uint32_t iidx;		/* Interrupt Index Register		@0x1020h */
	uint32_t reserved4[1];
	volatile uint32_t imask;	/* Interrupt Mask			@0x1028h */
	uint32_t reserved5[1];
	volatile uint32_t ris;		/* Raw Interrupt Status			@0x1030h */
	uint32_t reserved6[1];
	volatile uint32_t mis;		/* Masked Interrupt Status		@0x1038h */
	uint32_t reserved7[1];
	volatile uint32_t iset;		/* Interrupt Set			@0x1040h */
	uint32_t reserved8[1];
	volatile uint32_t iclr;		/* Interrupt Clear			@0x1048h */
	uint32_t reserved9[1];
	volatile uint32_t iidx1;	/* Interrupt Index Register DMA0	@0x1050h */
	uint32_t reserved10[1];
	volatile uint32_t imask1;	/* Interrupt Mask DMA0			@0x1058h */
	uint32_t reserved11[1];
	volatile uint32_t ris1;		/* Raw Interrupt Status DMA0		@0x1060h */
	uint32_t reserved12[1];
	volatile uint32_t mis1;		/* Masked Interrupt Status DMA0		@0x1068h */
	uint32_t reserved13[1];
	volatile uint32_t iset1;	/* Interrupt Set DMA0			@0x1070h */
	uint32_t reserved14[1];
	volatile uint32_t iclr1;	/* Interrupt Clear DMA0			@0x1078h */
	uint32_t reserved15[1];
	volatile uint32_t iidx2;	/* Interrupt Index Register DMA1	@0x1080h */
	uint32_t reserved16[1];
	volatile uint32_t imask2;	/* Interrupt Mask DMA1			@0x1088h */
	uint32_t reserved17[1];
	volatile uint32_t ris2;		/* Raw Interrupt Status DMA1		@0x1090h */
	uint32_t reserved18[1];
	volatile uint32_t mis2;		/* Masked Interrupt Status DMA1		@0x1098h */
	uint32_t reserved19[1];
	volatile uint32_t iset2;	/* Interrupt Set DMA1			@0x10A0h */
	uint32_t reserved20[1];
	volatile uint32_t iclr2;	/* Interrupt Clear DMA1			@0x10A8h */
	uint32_t reserved21[1];
	volatile uint32_t iidx3;	/* Interrupt Index Register DMA2	@0x10B0h */
	uint32_t reserved22[1];
	volatile uint32_t imask3;	/* Interrupt Mask DMA2			@0x10B8h */
	uint32_t reserved23[1];
	volatile uint32_t ris3;		/* Raw Interrupt Status DMA2		@0x10C0h */
	uint32_t reserved24[1];
	volatile uint32_t mis3;		/* Masked Interrupt Status DMA2		@0x10C8h */
	uint32_t reserved25[1];
	volatile uint32_t iset3;	/* Interrupt Set DMA2			@0x10D0h */
	uint32_t reserved26[1];
	volatile uint32_t iclr3;	/* Interrupt Clear DMA2			@0x10D8h */
	uint32_t reserved27[1];
	volatile uint32_t evt_mode;	/* Event Mode				@0x10E0h */
	uint32_t reserved28[7];
	volatile uint32_t aesactl0;	/* AES Control Register 0		@0x1100h */
	volatile uint32_t aesactl1;	/* AES Control Register 1		@0x1104h */
	volatile uint32_t aesastat;	/* AES Status Register			@0x1108h */
	volatile uint32_t aesakey;	/* AES Key Register			@0x110Ch */
	volatile uint32_t aesadin;	/* AES Data In Register			@0x1110h */
	volatile uint32_t aesadout;	/* AES Data Out Register		@0x1114h */
	volatile uint32_t aesaxdin;	/* AES XORed Data In Register		@0x1118h */
	volatile uint32_t aesaxin;	/* AES XORed Data In (no trigger)	@0x111Ch */
} aes_ti_mspm0_reg_t;
#else
typedef struct {
	uint32_t reserved0[0x200];
	volatile uint32_t pwren;		/* Power Enable			@0x800h  */
	volatile uint32_t rstctl;		/* Reset Control		@0x804h  */
	uint32_t reserved1[3];
	volatile uint32_t stat;			/* Status Register		@0x814h  */
	uint32_t reserved2[0x200];
	volatile uint32_t pdbgctl;		/* Peripheral Debug Control	@0x1018h */
	uint32_t reserved3[1];
	volatile uint32_t iidx;			/* Interrupt Index		@0x1020h */
	uint32_t reserved4[1];
	volatile uint32_t imask;		/* Interrupt Mask		@0x1028h */
	uint32_t reserved5[1];
	volatile uint32_t ris;			/* Raw Interrupt Status		@0x1030h */
	uint32_t reserved6[1];
	volatile uint32_t mis;			/* Masked Interrupt Status	@0x1038h */
	uint32_t reserved7[1];
	volatile uint32_t iset;			/* Interrupt Set		@0x1040h */
	uint32_t reserved8[1];
	volatile uint32_t iclr;			/* Interrupt Clear		@0x1048h */
	uint32_t reserved9[1];
	volatile uint32_t iidx_datain;		/* DMA Trig0 Index		@0x1050h */
	uint32_t reserved10[1];
	volatile uint32_t imask_datain;		/* DMA Trig0 Mask		@0x1058h */
	uint32_t reserved11[1];
	volatile uint32_t ris_datain;		/* DMA Trig0 RIS		@0x1060h */
	uint32_t reserved12[1];
	volatile uint32_t mis_datain;		/* DMA Trig0 MIS		@0x1068h */
	uint32_t reserved13[1];
	volatile uint32_t iset_datain;		/* DMA Trig0 Set		@0x1070h */
	uint32_t reserved14[1];
	volatile uint32_t iclr_datain;		/* DMA Trig0 Clear		@0x1078h */
	uint32_t reserved15[1];
	volatile uint32_t iidx_dataout;		/* DMA Trig1 Index		@0x1080h */
	uint32_t reserved16[1];
	volatile uint32_t imask_dataout;	/* DMA Trig1 Mask		@0x1088h */
	uint32_t reserved17[1];
	volatile uint32_t ris_dataout;		/* DMA Trig1 RIS		@0x1090h */
	uint32_t reserved18[1];
	volatile uint32_t mis_dataout;		/* DMA Trig1 MIS		@0x1098h */
	uint32_t reserved19[1];
	volatile uint32_t iset_dataout;		/* DMA Trig1 Set		@0x10A0h */
	uint32_t reserved20[1];
	volatile uint32_t iclr_dataout;		/* DMA Trig1 Clear		@0x10A8h */
	uint32_t reserved21[13];
	volatile uint32_t evt_mode;		/* Event Mode			@0x10E0h */
	uint32_t reserved22[7];
	volatile uint32_t gcmccm_tag[4];	/* GCM/CCM Intermediate TAG	@0x1100h */
	volatile uint32_t ghash_h[4];		/* GCM Hash Key / CCM Key2	@0x1110h */
	volatile uint32_t key[8];		/* Key (128 or 256-bit)		@0x1120h */
	volatile uint32_t iv[4];		/* Initialization Vector	@0x1140h */
	volatile uint32_t ctrl;			/* Mode and Control		@0x1150h */
	volatile uint32_t c_length_0;		/* Crypto Data Length LSW	@0x1154h */
	volatile uint32_t c_length_1;		/* Crypto Data Length MSW	@0x1158h */
	volatile uint32_t aad_length;		/* AAD Data Length		@0x115Ch */
	volatile uint32_t data[4];		/* Data In / Data Out		@0x1160h */
	volatile uint32_t tag[4];		/* Authentication Result	@0x1170h */
	volatile uint32_t status;		/* Status			@0x1180h */
	volatile uint32_t data_in;		/* Data In Alias		@0x1184h */
	volatile uint32_t data_out;		/* Data Out Alias		@0x1188h */
	uint32_t reserved23[17];
	volatile uint32_t force_in_av;		/* Data Control			@0x11D0h */
	volatile uint32_t ccm_aln_wrd;		/* CCM AAD Alignment Word	@0x11D4h */
	volatile uint32_t blk_cnt0;		/* Block Counter LSW		@0x11D8h */
	volatile uint32_t blk_cnt1;		/* Block Counter MSW		@0x11DCh */
	uint32_t reserved24[5];
	volatile uint32_t dma_hs;		/* DMA Handshake Control	@0x11F4h */
} aes_adv_ti_msp_reg_t;
#endif

struct crypto_msp_aes_config {
#ifdef CONFIG_CRYPTO_MSPM0_AES
	aes_ti_mspm0_reg_t *regs;
#else
	aes_adv_ti_msp_reg_t *regs;
#endif
	void (*irq_config_func)(const struct device *dev);
};

struct msp_aes_session {
	uint32_t keylen;
	uint32_t aesconfig;
	enum cipher_op op;
	bool in_use;
};

struct crypto_msp_aes_data {
	struct msp_aes_session sessions[CONFIG_CRYPTO_MSP_AES_MAX_SESSION];
	struct k_mutex device_mutex;
	struct k_sem aes_done;
};
#endif
