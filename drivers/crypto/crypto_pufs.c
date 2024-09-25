/*
 * Copyright (c) 2024 Rapid Silicon
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "crypto_pufs.h"

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/types.h>

#define LOG_LEVEL CONFIG_CRYPTO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(crypto_puf_security);

#if DT_HAS_COMPAT_STATUS_OKAY(pufsecurity_pufcc)
  #define DT_DRV_COMPAT pufsecurity_pufcc
#else
  #error No PUF Security HW Crypto Accelerator in device tree
#endif

#define PUFS_HW_CAP (CAP_RAW_KEY | CAP_INPLACE_OPS | \
                     CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS | CAP_ASYNC_OPS | \
                     CAP_NO_IV_PREFIX | CAP_NO_ENCRYPTION | CAP_NO_SIGNING)

/*****************************************************************************
 * Macros
 ****************************************************************************/
#define SG_DMA_MAX_DSCS_SIZE (512 - 8)  // Enough for 15 descriptors
#define BUFFER_SIZE 512
#define PUFCC_MAX_BUSY_COUNT 8000000  // Max busy count for processing 10MB data
#define CTR_MODE_BLOCK_SIZE 16

/*****************************************************************************
 * Local variable declarations
 ****************************************************************************/
uint8_t __pufcc_descriptors[BUFFER_SIZE];
static struct pufcc_sg_dma_desc *sg_dma_descs =
    (struct pufcc_sg_dma_desc *)__pufcc_descriptors;
static uint8_t pufcc_buffer[BUFFER_SIZE];

// PUFcc microprogram for RSA2048
static uint32_t rsa_2048_mprog[] = {
    0x33cdac81, 0x6817434e, 0x4283ad5d, 0x27499978, 0x8a000040, 0x0a1080c0,
    0xc3800b00, 0x081810c6, 0xfc000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000};

// PUFcc microprogram for ECDSA256
static uint32_t p256_ecdsa_mprog[] = {
    0xb1703302, 0x0f91d3f8, 0x004ae67d, 0x8f7093c5, 0x8a000068, 0x0a014088,
    0xc3000000, 0xa0624000, 0x43000100, 0x20824000, 0x0a014090, 0xc3000000,
    0x20624800, 0x43000100, 0xa0824800, 0x0a014090, 0xc3000600, 0x8900101e,
    0x8e000028, 0x8a000068, 0x8a014800, 0x8a028070, 0x43000400, 0x0901101e,
    0x8e000028, 0x8a000068, 0x8a014800, 0x0a028088, 0x43000400, 0x0902101e,
    0x8e000048, 0x8a028058, 0x0a03c060, 0x92050020, 0x8a064808, 0x41801600,
    0x8900101e, 0x09011028, 0x8e000048, 0x0a028078, 0x8a03c080, 0x92050020,
    0x8a064810, 0x41801600, 0x0902101e, 0x89031028, 0x8e000048, 0x8a028800,
    0x0a03c808, 0x0a050810, 0x0a064818, 0xc1000700, 0x20a25000, 0x8900101e,
    0x8e000028, 0x8a000068, 0x8a014800, 0x43000200, 0x8900101e, 0x1c110800,
    0x18025800, 0xfc000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000};

// EC NIST-P256 parameters
struct pufcc_ecc_param ecc_param_nistp256 = {
    "\xff\xff\xff\xff\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff",  // prime
    "\xff\xff\xff\xff\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xfc",  // a
    "\x5a\xc6\x35\xd8\xaa\x3a\x93\xe7\xb3\xeb\xbd\x55\x76\x98\x86"
    "\xbc\x65\x1d\x06\xb0\xcc\x53\xb0\xf6\x3b\xce\x3c\x3e\x27\xd2"
    "\x60\x4b",  // b
    "\x6b\x17\xd1\xf2\xe1\x2c\x42\x47\xf8\xbc\xe6\xe5\x63\xa4\x40"
    "\xf2\x77\x03\x7d\x81\x2d\xeb\x33\xa0\xf4\xa1\x39\x45\xd8\x98"
    "\xc2\x96",  // px
    "\x4f\xe3\x42\xe2\xfe\x1a\x7f\x9b\x8e\xe7\xeb\x4a\x7c\x0f\x9e"
    "\x16\x2b\xce\x33\x57\x6b\x31\x5e\xce\xcb\xb6\x40\x68\x37\xbf"
    "\x51\xf5",  // py
    "\xff\xff\xff\xff\x00\x00\x00\x00\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xbc\xe6\xfa\xad\xa7\x17\x9e\x84\xf3\xb9\xca\xc2\xfc\x63"
    "\x25\x51"  // order
};

// Base register addresses of different PUFcc modules
static struct pufcc_dma_regs *dma_regs;
static struct pufcc_hmac_regs *hmac_regs;
static struct pufcc_crypto_regs *crypto_regs;
static struct pufcc_sp38a_regs *sp38a_regs;
static struct pufcc_pkc_regs *pkc_regs;

/*****************************************************************************
 * Local function declarations
 ****************************************************************************/
static enum pufcc_status rsa_p1v15_verify(const uint8_t *dec_msg,
                                          struct pufs_crypto_addr *msg_addr);
static void reverse(uint8_t *dst, const uint8_t *src, size_t len);
static uint32_t be2le(uint32_t var);
static enum pufcc_status busy_wait(volatile uint32_t *status_reg_addr,
                                   uint32_t error_mask);

static char *session_to_str(enum pufs_session_type inSession) {
  return ((inSession ==  PUFS_SESSION_HASH_CALCULATION)?"Hash":\
            ((inSession ==  PUFS_SESSION_DECRYPTION)?"Decryption":\
            ((inSession ==  PUFS_SESSION_SIGN_VERIFICATION)?"Sign_Verification":"Unknown")));
}                                      

/*****************************************************************************
 * API functions
 ****************************************************************************/
/**
 * @fn    pufcc_calc_sha256_hash
 * @brief Calculates SHA256 hash
 *
 * @param[in]     ctx    Context of the ecdsa hash calculation.
 * @param[inout]  pkt    Input and output data related parameters for hash calculation.
 * @return               PUFCC_SUCCESS on success, otherwise an error code.
 */
static enum pufcc_status pufcc_calc_sha256_hash(struct hash_ctx *ctx, 
                                                struct hash_pkt *pkt) 
{
  enum pufcc_status status;
  struct pufcc_intrpt_reg intrpt_reg = {0};
  struct pufcc_dma_cfg_0_reg dma_cfg_0_reg = {0};
  struct pufcc_dma_dsc_cfg_4_reg dma_dsc_cfg_4_reg = {0};
  struct pufcc_dma_key_cfg0_reg dma_key_cfg0_reg = {0};
  struct pufcc_start_reg start_reg = {0};

  // Set 'intrpt' register values
  intrpt_reg.intrpt_st = 1;  // Write 1 to clear interrupt
  intrpt_reg.intrpt_en = 0;  // Disable interrupt

  // Set dma_dsc_cfg_4 reg values
  dma_dsc_cfg_4_reg.head = 1;
  dma_dsc_cfg_4_reg.tail = 1;

  // set values for key_cfg_0 register
  dma_key_cfg0_reg.key_dst = PUFCC_DMA_KEY_DST_HASH;

  // Set values for start register
  start_reg.start_p = 1;  // Start operation bit

  // Set values for HMAC config register
  struct pufcc_hmac_config_reg hmac_config_reg = {
      .variant = PUFCC_HMAC_VARIANT_SHA256,  // Shah 256
      .function = PUFCC_HMAC_FUNCTION_HASH,  // Hash
  };

  /*** Configure DMA registers ***/
  // Set dma_cfg_0 register
  REG_WRITE_32(&dma_regs->cfg_0, &dma_cfg_0_reg);

  // Set data source address in dsc_cfg_0 register
  dma_regs->dsc_cfg_0 = (uint32_t)pkt->in_buf;

  // Set data length in dsc_cfg_2 register
  dma_regs->dsc_cfg_2 = pkt->in_len;

  // Set the dsc_cfg_4 register to values defined above
  REG_WRITE_32(&dma_regs->dsc_cfg_4, &dma_dsc_cfg_4_reg);

  // Set the key_cfg_0 register to values defined above
  REG_WRITE_32(&dma_regs->key_cfg_0, &dma_key_cfg0_reg);

  // Write intrpt register to values defined above
  REG_WRITE_32(&dma_regs->interrupt, &intrpt_reg);

  /** Configure HMAC registers **/
  // Set the config register to values defined above
  REG_WRITE_32(&hmac_regs->cfg, &hmac_config_reg);

  // Write previous length in HMAC plen register
  hmac_regs->plen = hmac_regs->alen;

  // Write intrpt register to values defined above
  REG_WRITE_32(&hmac_regs->interrupt, &intrpt_reg);

  // Start the DMA operation by writing to its start register
  REG_WRITE_32(&dma_regs->start, &start_reg);

  // Poll on busy status
  status = busy_wait(&dma_regs->status_0, PUFCC_DMA_ERROR_MASK);

  if (status != PUFCC_SUCCESS) {
    return status;
  }

  if (hmac_regs->status) {
    return PUFCC_E_ERROR;
  }

  // Read the calculated hash
  for (uint8_t i = 0; i < (PUFCC_SHA_256_LEN / PUFCC_WORD_SIZE); i++) {
    ((uint32_t *)pkt->out_buf)[i] = be2le(crypto_regs->dgst_out[i]);
  }

  pkt->out_len = PUFCC_SHA_256_LEN;

  return PUFCC_SUCCESS;
}

/**
 * @fn    pufcc_calc_sha256_hash_sg
 * @brief Calculates SHA256 hash of non-contiguous data.
 *        All non contiguous data addresses can be passed in as a single linked
 *        list in the form of 'hash_pkr' struct as 'pkt' parameter
 *        or this function can be invoked multiple times with partial data
 *        address info in the linked list by setting the 'head' and 'tail'
 *        params accordingly as detailed below against each param. In case of
 *        partial data address info and hence multiple invocations, we also need
 *        to pass in previously calculated hash values to every invocation
 *        after the head one and the accumulated length of all previous
 *        messages/data.
 *
 *        Note: In case of multiple data chunks either in a single linked list
 *        or as partial linked lists using multiple invocations the sizes of all
 *        chunks must be multiples of 64 bytes except the last chunk.
 *
 * @param[in]     ctx    Context of the scatter gather hash calculation.
 * @param[inout]  pkt    Input and output data related parameters for hash calculation.
 * @return               PUFCC_SUCCESS on success, otherwise an error code.
 */
static enum pufcc_status pufcc_calc_sha256_hash_sg(struct hash_ctx *ctx, 
                                                   struct hash_pkt *pkt) {
  enum pufcc_status status;
  uint32_t plen = 0;
  uint8_t desc_count = 0;
  bool lvHead = pkt->head, lvTail = pkt->tail; 
  struct pufcc_dma_cfg_0_reg dma_cfg_0_reg = {0};
  struct pufcc_dma_dsc_cfg_4_reg dma_dsc_cfg_4_reg = {0};
  struct pufcc_dma_key_cfg0_reg dma_key_cfg0_reg = {0};
  struct pufcc_intrpt_reg intrpt_reg = {0};
  struct pufcc_start_reg start_reg = {0};
  struct pufcc_hmac_config_reg hmac_config_reg = {0};

  if (!lvHead) plen = *pkt->prev_len;

  // Set DMA 'intrpt' register values
  intrpt_reg.intrpt_st = 1;  // Write 1 to clear interrupt
  intrpt_reg.intrpt_en = 0;  // Disable interrupt

  // Set values for start register
  start_reg.start_p = 1;  // Start operation

  // Set values for HMAC config register
  hmac_config_reg.variant = PUFCC_HMAC_VARIANT_SHA256;
  hmac_config_reg.function = PUFCC_HMAC_FUNCTION_HASH;

  // Set key_cfg_0 for hash calculation
  dma_key_cfg0_reg.key_dst = PUFCC_DMA_KEY_DST_HASH;

  // Set previous hash value if it's not the first data block
  if (!lvHead) {
    for (uint8_t i = 0; i < (PUFCC_SHA_256_LEN / PUFCC_WORD_SIZE); i++) {
      crypto_regs->dgst_in[i] = be2le(((uint32_t *)pkt->in_hash)[i]);
    }
  }

  // Set SGDMA descriptors
  do {
    sg_dma_descs[desc_count].read_addr = be2le((uint32_t)pkt->in_buf);
    sg_dma_descs[desc_count].length = be2le(pkt->in_len);
    sg_dma_descs[desc_count].next =
        be2le((uint32_t)&sg_dma_descs[desc_count + 1]);

    sg_dma_descs[desc_count].key_cfg = be2le(*(uint32_t *)&dma_key_cfg0_reg);
    sg_dma_descs[desc_count].cypt_cfg[0] = be2le(*(uint32_t *)&hmac_config_reg);
    sg_dma_descs[desc_count].cypt_cfg[1] = be2le(plen);

    *(uint32_t *)&dma_dsc_cfg_4_reg = 0;
    dma_dsc_cfg_4_reg.offset = plen % 16;

    plen += pkt->in_len;
    pkt = pkt->next;

    if (!desc_count && lvHead) {
      dma_dsc_cfg_4_reg.head = 1;
    }

    // Mark this descriptor as last if there is no more data
    if (!pkt) {
      dma_dsc_cfg_4_reg.dn_pause = 1;
      if (lvTail) {
        dma_dsc_cfg_4_reg.tail = 1;
      }
    }

    sg_dma_descs[desc_count].dsc_cfg_4 = be2le(*(uint32_t *)&dma_dsc_cfg_4_reg);
    desc_count++;
  } while (pkt && ((desc_count * sizeof(struct pufcc_sg_dma_desc)) <
                         SG_DMA_MAX_DSCS_SIZE));

  if (pkt) {
    // No enough descriptors available
    return PUFCC_E_OVERFLOW;
  }

  // Update accumulated data length
  *pkt->prev_len = plen;

  /*** Configure DMA registers ***/
  // Enable SGDMA in dma_cfg_0 register
  dma_cfg_0_reg.sg_en = 1;
  REG_WRITE_32(&dma_regs->cfg_0, &dma_cfg_0_reg);

  // Set dsc_cfg_2 register to indicate it's an SGDMA operation
  dma_regs->dsc_cfg_2 = PUFCC_DMA_DSC_CFG2_SGDMA_VAL;

  // Set starting address of SGDMA descriptors in dma_dsc_cfg3 register
  dma_regs->dsc_cfg_3 = (uint32_t)sg_dma_descs;

  // Clear and disable DMA and HMAC interrupts
  REG_WRITE_32(&dma_regs->interrupt, &intrpt_reg);
  REG_WRITE_32(&hmac_regs->interrupt, &intrpt_reg);

  // Start the DMA operation by writing to its start register
  REG_WRITE_32(&dma_regs->start, &start_reg);

  // Poll on busy status
  status = busy_wait(&dma_regs->status_0, PUFCC_DMA_ERROR_MASK);

  if (status != PUFCC_SUCCESS) {
    return status;
  }

  if (hmac_regs->status) {
    return PUFCC_E_ERROR;
  }

  // Read the calculated hash value
  for (uint8_t i = 0; i < (PUFCC_SHA_256_LEN / PUFCC_WORD_SIZE); i++) {
    ((uint32_t *)pkt->out_buf)[i] = be2le(crypto_regs->dgst_out[i]);
  }

  pkt->out_len = PUFCC_SHA_256_LEN;

  return PUFCC_SUCCESS;
}

/**
 * @fn    pufcc_decrypt_aes
 * @brief Decrypt passed in data using AES decryption algorithm
 *
 * @param[in]  out_addr    Address to write decrypted data to
 * @param[in]  in_addr     Address of the encrypted data
 * @param[in]  in_len      Length of the input data in bytes
 * @param[in]  prev_len    Length of the previously decrypted data in case
 *                         decrypting data in chunks; 0 for first or single
 *                         chunk
 * @param[in]  key_type    Key type i.e. SW or OTP
 * @param[in]  key_addr    Address or OTP slot of the decryption key
 * @param[in]  key_len     Length of the decryption key
 * @param[in]  iv_addr     IV address
 * @param[in]  iv_len      IV length
 * @param[in]  write_type  Write type i.e. write decrypted data to fixed address
 *                         or auto-increment the destination address
 * @param[in]  readback_iv If true, read back updated IV value corresponding to
 *                         number of processed data blocks into passed in
 *                         iv_addr buffer. In this case iv_addr buffer must be
 *                         writable.
 *
 * @return                 PUFCC_SUCCESS on success, otherwise an error code
 */
static enum pufcc_status pufcc_decrypt_aes(uint32_t out_addr, uint32_t in_addr,
                                    uint32_t in_len, uint32_t prev_len,
                                    enum pufcc_key_type key_type,
                                    uint32_t key_addr, uint32_t key_len,
                                    uint32_t iv_addr, uint32_t iv_len,
                                    enum pufcc_dma_rw_type write_type,
                                    bool readback_iv) {
  enum pufcc_status status;
  uint32_t temp32;

  // Configure DMA intrpt register
  temp32 = 0;
  struct pufcc_intrpt_reg *intrpt_reg = (struct pufcc_intrpt_reg *)&temp32;
  intrpt_reg->intrpt_st = 1;  // Write 1 to clear interrupt
  REG_WRITE_32(&dma_regs->interrupt, intrpt_reg);

  // Set dma_cfg_0 register
  temp32 = 0;
  REG_WRITE_32(&dma_regs->cfg_0, &temp32);

  struct pufcc_dma_cfg_1_reg *cfg_1_reg = (struct pufcc_dma_cfg_1_reg *)&temp32;
  cfg_1_reg->rbst_max = 0xF;
  cfg_1_reg->rbst_min = 0xF;
  cfg_1_reg->wbst_max = 0xF;
  cfg_1_reg->wbst_min = 0xF;
  
  REG_WRITE_32(&dma_regs->cfg_1, cfg_1_reg);

  // Set data source address in dsc_cfg_0 register
  REG_WRITE_32(&dma_regs->dsc_cfg_0, &in_addr);

  // Set decrypted data destination address in dsc_cfg_1 register
  REG_WRITE_32(&dma_regs->dsc_cfg_1, &out_addr);

  // Set data length in dsc_cfg_2 register
  REG_WRITE_32(&dma_regs->dsc_cfg_2, &in_len);

  // Configure dma_dsc_cfg_4 register
  temp32 = 0;
  struct pufcc_dma_dsc_cfg_4_reg *dsc_cfg_4_reg =
      (struct pufcc_dma_dsc_cfg_4_reg *)&temp32;
  dsc_cfg_4_reg->fw = write_type;
  dsc_cfg_4_reg->fr = AUTO_INCREMENT;  // Autoincrement source address
  dsc_cfg_4_reg->offset = prev_len % CTR_MODE_BLOCK_SIZE;
  REG_WRITE_32(&dma_regs->dsc_cfg_4, dsc_cfg_4_reg);

  // Configure key_cfg_0 register
  struct pufcc_dma_key_cfg0_reg *key_cfg_0_reg =
      (struct pufcc_dma_key_cfg0_reg *)&temp32;
  key_cfg_0_reg->key_src = key_type;
  key_cfg_0_reg->key_dst = PUFCC_DMA_KEY_DST_SP38A;  // SP38a
  key_cfg_0_reg->key_size = key_len * 8;             // Key length in bits

  // Configure the decryption key
  if (key_type == PUFCC_SW_KEY) {
    for (uint32_t i = 0; i < (key_len / PUFCC_WORD_SIZE); i++) {
      crypto_regs->sw_key[i] = be2le(((uint32_t *)key_addr)[i]);
    }
  } else {
    key_cfg_0_reg->key_idx = (enum pufcc_key_type)key_addr;
  }

  REG_WRITE_32(&dma_regs->key_cfg_0, key_cfg_0_reg);

  // Configure IV
  for (uint32_t i = 0; i < (iv_len / PUFCC_WORD_SIZE); i++) {
    crypto_regs->iv[i] = be2le(((uint32_t *)iv_addr)[i]);
  }

  // Configure SP38a intrpt register
  temp32 = 0;
  intrpt_reg = (struct pufcc_intrpt_reg *)&temp32;
  intrpt_reg->intrpt_st = 1;  // Clear interrupt by writing 1
  REG_WRITE_32(&sp38a_regs->interrupt, intrpt_reg);

  // Configure SP38a config register
  struct pufcc_sp38a_config_reg *sp38a_config_reg =
      (struct pufcc_sp38a_config_reg *)&temp32;
  sp38a_config_reg->variant =
      (key_len == PUFCC_CRYPTO_AES128_KEY_LEN ? PUFCC_AES128 : PUFCC_AES256);
  sp38a_config_reg->mode = PUFCC_CTR128;
  sp38a_config_reg->enc_dec = 0;  // Decryption
  REG_WRITE_32(&sp38a_regs->cfg, sp38a_config_reg);

  // Start DMA operation
  struct pufcc_start_reg *start_reg = (struct pufcc_start_reg *)&temp32;
  start_reg->start_p = 1;
  REG_WRITE_32(&dma_regs->start, start_reg);

  // Poll on busy status
  status = busy_wait(&dma_regs->status_0, PUFCC_DMA_ERROR_MASK);

  if (status != PUFCC_SUCCESS) {
    return status;
  }

  if (sp38a_regs->status & PUFCC_SP38A_STATUS_ERROR_MASK) {
    return PUFCC_E_ERROR;
  }

  // Read back updated IV
  if (readback_iv) {
    for (uint8_t i = 0; i < iv_len / PUFCC_WORD_SIZE; i++) {
      ((uint32_t *)iv_addr)[i] = be2le(crypto_regs->iv[i]);
    }
  }

  return PUFCC_SUCCESS;
}

/**
 * @fn    pufcc_rsa2048_sign_verify
 * @brief Verify RSA2048 signature of the input message data
 *
 * @param[in]     ctx    Context of the ecdsa sign verification.
 * @param[inout]  pkt    Input and output data related parameters for sign verification.
 * @return               PUFCC_SUCCESS on success, otherwise an error code.
 */
static int pufcc_rsa2048_sign_verify(struct sign_ctx *ctx, struct sign_pkt *pkt) {
  enum pufcc_status status = PUFCC_SUCCESS;
  uint32_t temp32;
  uint8_t dec_msg[PUFCC_RSA_2048_LEN];
  const uint8_t *sig = ctx->sig;  
  struct pufs_crypto_rsa2048_puk *pub_key = (struct pufs_crypto_rsa2048_puk *)ctx->pub_key;

  ((struct pufs_data*)ctx->device->data)->pufs_pkt.sign_pkt = pkt;
  ((struct pufs_data*)ctx->device->data)->pufs_ctx.sign_ctx = ctx;

  struct pufs_crypto_addr msg_addr = {
    .read_addr = (uint32_t)pkt->in_buf,
    .len = pkt->in_len
  };

  // Configure signature scheme
  temp32 = 0;
  struct pufcc_pkc_ecp_ec_reg *ecp_ec_reg =
      (struct pufcc_pkc_ecp_ec_reg *)&temp32;
  ecp_ec_reg->field = PUFCC_RSA_2048;
  REG_WRITE_32(&pkc_regs->ecp_ec, ecp_ec_reg);

  // Reverse public key modulus
  reverse(pufcc_buffer, pub_key->n, PUFCC_RSA_2048_LEN);

  // Write reversed public key modulus to ECP data field at proper offset
  memcpy((uint8_t *)&pkc_regs->ecp_data + PUFCC_DATA_RSA2048_MODULUS_OFFSET,
         pufcc_buffer, PUFCC_RSA_2048_LEN);

  // Write public key exponent to ecp_e_short register
  REG_WRITE_32(&pkc_regs->ecp_e_short, &pub_key->e);

  // Reverse signature
  reverse(pufcc_buffer, sig, PUFCC_RSA_2048_LEN);

  // Write reversed signature to ECP data field at proper offset
  memcpy((uint8_t *)&pkc_regs->ecp_data + PUFCC_DATA_RSA2048_SIGN_OFFSET,
         pufcc_buffer, PUFCC_RSA_2048_LEN);

  // Write microprogram for RSA2048
  memcpy((void *)&pkc_regs->ecp_mac, rsa_2048_mprog, sizeof(rsa_2048_mprog));

  // Clear and disable PKC interrupt
  temp32 = 0;
  struct pufcc_intrpt_reg *intrpt_reg = (struct pufcc_intrpt_reg *)&temp32;
  intrpt_reg->intrpt_st = 1;
  REG_WRITE_32(&pkc_regs->interrupt, intrpt_reg);

  // Start PKC operation
  struct pufcc_start_reg *start_reg = (struct pufcc_start_reg *)&temp32;
  start_reg->start_p = 1;
  REG_WRITE_32(&pkc_regs->start, start_reg);

  // Poll on busy status
  status = busy_wait(&pkc_regs->status, PUFCC_PKC_ERROR_MASK);

  if (status != PUFCC_SUCCESS) {
    LOG_ERR("%s(%d) PUFs Error:%d\n", __func__, __LINE__, (int)status);
    return -ECANCELED;
  }

  // Read decrypted message from proper offset in ECP data field and reverse it
  memcpy(pufcc_buffer,
         (uint8_t *)&pkc_regs->ecp_data + PUFCC_DATA_RSA2048_SIGN_OFFSET,
         PUFCC_RSA_2048_LEN);
  reverse(dec_msg, pufcc_buffer, PUFCC_RSA_2048_LEN);

  status = rsa_p1v15_verify(dec_msg, &msg_addr);

  if (status != PUFCC_SUCCESS) {
    LOG_ERR("%s(%d) PUFs Verification Error:%d\n", __func__, __LINE__, (int)status);
    return -ECANCELED;
  }  
  
  return PUFCC_SUCCESS;
}

/**
 * @fn    pufcc_ecdsa256_sign_verify
 * @brief Verify ECDSA256 signature of the input message data
 *
 * @param[in]  ctx    Context of the ecdsa sign verification.
 * @param[in]  pkt    Input and output data related parameters for sign verification.
 * @return            PUFCC_SUCCESS on success, otherwise an error code.
 */
static int pufcc_ecdsa256_sign_verify(struct sign_ctx *ctx, struct sign_pkt *pkt) {

  struct pufs_crypto_ec256_sig *sig = (struct pufs_crypto_ec256_sig *)ctx->sig;
  struct rs_crypto_ec256_puk *pub_key = (struct rs_crypto_ec256_puk *)ctx->pub_key;
  uint32_t temp32, prev_len = 0;
  enum pufcc_status status;
  struct pufs_crypto_hash hash;

  ((struct pufs_data*)ctx->device->data)->pufs_pkt.sign_pkt = pkt;
  ((struct pufs_data*)ctx->device->data)->pufs_ctx.sign_ctx = ctx;

  struct hash_ctx lvHashCtx = {
    .device = ctx->device,
    .drv_sessn_state = NULL,
    .hash_hndlr = NULL,
    .started = false,
    .flags = (CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS)
  };

  struct hash_pkt lvHashPkt = {
    .in_buf = pkt->in_buf,
    .in_hash = NULL,
    .in_len = pkt->in_len,
    .prev_len = &prev_len,
    .out_buf = (uint8_t*)hash.val,
    .out_len = 0,
    .next = NULL,
    .head = true,
    .tail = true,
    .ctx = &lvHashCtx
  };

  // Calculate hash of the message
  if (pufcc_calc_sha256_hash_sg(&lvHashCtx, &lvHashPkt) != PUFCC_SUCCESS) {
    return PUFCC_E_ERROR;
  }

  // Set the EC NIST P256 parameters after reversing them
  reverse(pufcc_buffer, ecc_param_nistp256.prime, PUFCC_ECDSA_256_LEN);
  memcpy((uint8_t *)&pkc_regs->ecp_data + PUFCC_DATA_ECDSA_PRIME_OFFSET,
         pufcc_buffer, PUFCC_ECDSA_256_LEN);

  reverse(pufcc_buffer, ecc_param_nistp256.a, PUFCC_ECDSA_256_LEN);
  memcpy((uint8_t *)&pkc_regs->ecp_data + PUFCC_DATA_ECDSA_EC_A_OFFSET,
         pufcc_buffer, PUFCC_ECDSA_256_LEN);

  reverse(pufcc_buffer, ecc_param_nistp256.b, PUFCC_ECDSA_256_LEN);
  memcpy((uint8_t *)&pkc_regs->ecp_data + PUFCC_DATA_ECDSA_EC_B_OFFSET,
         pufcc_buffer, PUFCC_ECDSA_256_LEN);

  reverse(pufcc_buffer, ecc_param_nistp256.px, PUFCC_ECDSA_256_LEN);
  memcpy((uint8_t *)&pkc_regs->ecp_data + PUFCC_DATA_ECDSA_PX_OFFSET,
         pufcc_buffer, PUFCC_ECDSA_256_LEN);

  reverse(pufcc_buffer, ecc_param_nistp256.py, PUFCC_ECDSA_256_LEN);
  memcpy((uint8_t *)&pkc_regs->ecp_data + PUFCC_DATA_ECDSA_PY_OFFSET,
         pufcc_buffer, PUFCC_ECDSA_256_LEN);

  reverse(pufcc_buffer, ecc_param_nistp256.order, PUFCC_ECDSA_256_LEN);
  memcpy((uint8_t *)&pkc_regs->ecp_data + PUFCC_DATA_ECDSA_ORDER_OFFSET,
         pufcc_buffer, PUFCC_ECDSA_256_LEN);

  // Configure signature scheme
  temp32 = 0;
  struct pufcc_pkc_ecp_ec_reg *ecp_ec_reg =
      (struct pufcc_pkc_ecp_ec_reg *)&temp32;
  ecp_ec_reg->field = PUFCC_ECDSA256;
  ecp_ec_reg->h = 1;  // EC cofactor h
  REG_WRITE_32(&pkc_regs->ecp_ec, ecp_ec_reg);

  // Write microprogram for ECDSA 256
  memcpy((void *)&pkc_regs->ecp_mac, p256_ecdsa_mprog,
         sizeof(p256_ecdsa_mprog));

  // Set the hash, public key & signature in PKC module after reversing each
  reverse(pufcc_buffer, hash.val, PUFCC_SHA_256_LEN);
  memcpy((uint8_t *)&pkc_regs->ecp_data + PUFCC_DATA_ECDSA_HASH_OFFSET,
         pufcc_buffer, PUFCC_SHA_256_LEN);

  reverse(pufcc_buffer, pub_key->x, PUFCC_ECDSA_256_LEN);
  memcpy((uint8_t *)&pkc_regs->ecp_data + PUFCC_DATA_ECDSA_PUBX_OFFSET,
         pufcc_buffer, PUFCC_ECDSA_256_LEN);

  reverse(pufcc_buffer, pub_key->y, PUFCC_ECDSA_256_LEN);
  memcpy((uint8_t *)&pkc_regs->ecp_data + PUFCC_DATA_ECDSA_PUBY_OFFSET,
         pufcc_buffer, PUFCC_ECDSA_256_LEN);

  reverse(pufcc_buffer, sig->r, PUFCC_ECDSA_256_LEN);
  memcpy((uint8_t *)&pkc_regs->ecp_data + PUFCC_DATA_ECDSA_SIG_R_OFFSET,
         pufcc_buffer, PUFCC_ECDSA_256_LEN);

  reverse(pufcc_buffer, sig->s, PUFCC_ECDSA_256_LEN);
  memcpy((uint8_t *)&pkc_regs->ecp_data + PUFCC_DATA_ECDSA_SIG_S_OFFSET,
         pufcc_buffer, PUFCC_ECDSA_256_LEN);

  // Clear and disable PKC interrupt
  temp32 = 0;
  struct pufcc_intrpt_reg *intrpt_reg = (struct pufcc_intrpt_reg *)&temp32;
  intrpt_reg->intrpt_st = 1;
  REG_WRITE_32(&pkc_regs->interrupt, intrpt_reg);

  // Start PKC operation
  struct pufcc_start_reg *start_reg = (struct pufcc_start_reg *)&temp32;
  start_reg->start_p = 1;
  REG_WRITE_32(&pkc_regs->start, start_reg);

  // Poll on busy status
  status = busy_wait(&pkc_regs->status, PUFCC_PKC_ERROR_MASK);

  if (status != PUFCC_SUCCESS) {
    LOG_ERR("%s(%d) PUFs Verification Error:%d\n", __func__, __LINE__, (int)status);
    return -ECANCELED;
  }  

  return PUFCC_SUCCESS;
}

/**
 * @fn    rsa_p1v15_verify
 * @brief Verify input RSA2048 decrypted message according to PKCS#1 v1.5 RSA
 *        verification standard.
 * @param[in]  dec_msg  Input RSA decrypted message
 * @param[in]  msg_addr Address of the original data that was RSA signed
 * @return              PUFCC_SUCCESS on success, otherwise an error code.
 */
static enum pufcc_status rsa_p1v15_verify(const uint8_t *dec_msg,
                                          struct pufs_crypto_addr *msg_addr) {
  uint32_t i, prev_len = 0;
  struct pufs_crypto_hash hash;
  uint8_t pret[19] = {0x30, 0,    0x30, 0x0d, 0x06, 0x09, 0x60,
                      0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02,
                      0,    0x05, 0x00, 0x04, 0};

  if ((dec_msg[0] != 0x00) || (dec_msg[1] != 0x01)) {
    return PUFCC_E_VERFAIL;
  }

  for (i = 2; i < PUFCC_RSA_2048_LEN; i++) {
    if (dec_msg[i] != 0xff) {
      break;
    }
  }

  if (dec_msg[i++] != 0x00) {
    return PUFCC_E_VERFAIL;
  }

  // Verify that decrypted message has SHA256 hash
  if (dec_msg[i + 14] == 1) {
    pret[1] = 0x31;
    pret[14] = 0x01;
    pret[18] = 0x20;
  } else {
    return PUFCC_E_INVALID;
  }

  if ((memcmp(dec_msg + i, pret, 19) != 0) ||
      ((i + 19 + pret[18]) != PUFCC_RSA_2048_LEN)) {
    return PUFCC_E_VERFAIL;
  }

  // Calculate hash of the message

    struct hash_ctx lvHashCtx = {
    .device = NULL,
    .drv_sessn_state = NULL,
    .hash_hndlr = NULL,
    .started = false,
    .flags = (CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS)
  };

  struct hash_pkt lvHashPkt = {
    .in_buf = (uint8_t *)msg_addr->read_addr,
    .in_hash = NULL,
    .in_len = msg_addr->len,
    .prev_len = &prev_len,
    .out_buf = (uint8_t*)hash.val, /*hash.val is an array*/
    .out_len = 0,
    .next = NULL,
    .head = true,
    .tail = true,
    .ctx = &lvHashCtx
  };

  if (pufcc_calc_sha256_hash_sg(&lvHashCtx, &lvHashPkt) !=
      PUFCC_SUCCESS) {
    return PUFCC_E_ERROR;
  }

  if (memcmp(dec_msg + i + 19, hash.val, hash.len) != 0) {
    return PUFCC_E_VERFAIL;
  }

  return PUFCC_SUCCESS;
}

/**
 * @fn    reverse
 * @brief Reverse input byte array.
 *
 * @param[in]  dst  Destination address of the reversed byte array
 * @param[in]  src  Address of the input byte array
 * @param[in]  len  Length of the input byte array to be reversed
 */
static void reverse(uint8_t *dst, const uint8_t *src, size_t len) {
  for (size_t i = 0, j = len - 1; i < len; i++, j--) {
    dst[i] = src[j];
  }
}

/**
 * @fn    be2le
 * @brief Reverse endianness of the input 4 byte word
 *
 * @param[in]  var Input 4 byte variable
 * @return         4 byte word with reversed endianness
 */
static uint32_t be2le(uint32_t var) {
  return (((0xff000000 & var) >> 24) | ((0x00ff0000 & var) >> 8) |
          ((0x0000ff00 & var) << 8) | ((0x000000ff & var) << 24));
}

/**
 * @fn    busy_wait
 * @brief Waits on busy bit of the passed-in status register till it gets
 *        cleared or it times out.
 *
 * @param[in] status_reg_addr Status register's address
 * @param[in] error_mask      Bitmask of error bits in passed-in status register
 * @return                    PUFCC_SUCCESS if busy bit gets cleared without an
 *                            error, otherwise an error code.
 */
static enum pufcc_status busy_wait(volatile uint32_t *status_reg_addr,
                                   uint32_t error_mask) {
  uint32_t status;
  int32_t busy_count = PUFCC_MAX_BUSY_COUNT;

  do {
    status = sys_read32((mem_addr_t)status_reg_addr);
    busy_count--;
  } while ((status & PUFCC_BUSY_BIT_MASK) && (busy_count > 0));

  if (status & PUFCC_BUSY_BIT_MASK) {
    return PUFCC_E_TIMEOUT;
  } else if (status & error_mask) {
    return PUFCC_E_ERROR;
  }

  return PUFCC_SUCCESS;
}

static int crypto_pufs_init(const struct device *dev) {

  // Initialize base addresses of different PUFcc modules
  dma_regs = (struct pufcc_dma_regs *)((struct pufs_config*)dev->config)->base;
  hmac_regs = (struct pufcc_hmac_regs *)(((struct pufs_config*)dev->config)->base + PUFCC_HMAC_OFFSET);
  crypto_regs = (struct pufcc_crypto_regs *)(((struct pufs_config*)dev->config)->base + PUFCC_CRYPTO_OFFSET);
  sp38a_regs = (struct pufcc_sp38a_regs *)(((struct pufs_config*)dev->config)->base + PUFCC_SP38A_OFFSET);
  pkc_regs = (struct pufcc_pkc_regs *)(((struct pufs_config*)dev->config)->base + PUFCC_PKC_OFFSET);

  // Connect the IRQ
  ((struct pufs_config*)dev->config)->irq_init();

  return PUFCC_SUCCESS;
}

static void pufs_irq_handler(const struct device *dev) {

  int status = (dma_regs->status_0 & PUFCC_DMA_ERROR_MASK ? -ECANCELED : PUFCC_SUCCESS);

  struct pufcc_intrpt_reg *intrpt_reg_ptr =
      (struct pufcc_intrpt_reg *)&dma_regs->interrupt;

  struct pufs_data *lvPufsData = (struct pufs_data*)dev->data;

  switch(lvPufsData->pufs_session_type) {
    case(PUFS_SESSION_SIGN_VERIFICATION): {
      lvPufsData->pufs_session_callback.sign_cb(lvPufsData->pufs_pkt.sign_pkt, status);
    } break;
    case(PUFS_SESSION_HASH_CALCULATION): {
      lvPufsData->pufs_session_callback.hash_cb(lvPufsData->pufs_pkt.hash_pkt, status);
    } break;
    case(PUFS_SESSION_DECRYPTION): {
      lvPufsData->pufs_session_callback.cipher_cb(lvPufsData->pufs_pkt.cipher_pkt, status);
    } break;
    case(PUFS_SESSION_UNDEFINED): {
      LOG_ERR("%s(%d) Unsupported Session %d\n", __func__, __LINE__, lvPufsData->pufs_session_type);
    } break;
  }

  // Clear and disable interrupt
  intrpt_reg_ptr->intrpt_st = 1;  // Set to clear
  intrpt_reg_ptr->intrpt_en = 0;

  // After execution of a callback, the irq is disabled.
  irq_disable(((struct pufs_config *)dev->config)->irq_num);
}

static void pufs_irq_Init(void) {
    IRQ_CONNECT(
                  DT_INST_IRQN(0),
                  DT_INST_IRQ(0, priority),
                  pufs_irq_handler,
                  DEVICE_DT_INST_GET(0),
                  0
                );	

    /**
     * IRQ for PUFcc will be abled inside the
     * interfaces which enable asynch operation
     * callback registeration. When the IRQ is
     * received, then that will be disabled inside
     * the pufs_irq_hander.
     */
}

/********************************************************
 * Following set of APIs are in compliance with the
 * Zephyr user level Crypto API. These APIs will then
 * subsequently call local functions for performing
 * various crypto related operations.
*********************************************************/

/**
 * Query the driver capabilities
 * Not all the Modules of PUFs support all flags.
 * Please Check individual cipher/hash/sign_begin_session
 * interfaces to get the information of supported flags. 
 * */ 
static int pufs_query_hw_caps(const struct device *dev)
{
  return PUFS_HW_CAP;
}

static int pufs_ctr_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt, uint8_t *ctr)
{
  ((struct pufs_data*)ctx->device->data)->pufs_pkt.cipher_pkt = pkt;
  ((struct pufs_data*)ctx->device->data)->pufs_ctx.cipher_ctx = ctx;

  return pufcc_decrypt_aes(
                            (uint32_t)pkt->out_buf, (uint32_t)pkt->in_buf,
                            (uint32_t)pkt->in_len, pkt->prev_len, ctx->key_source,
                            (uint32_t)ctx->key.bit_stream, ctx->keylen, (uint32_t)ctr,
                            (uint32_t)ctx->mode_params.ctr_info.ctr_len, 
                            pkt->auto_increment, ctx->mode_params.ctr_info.readback_ctr
                          );
}

/* Following cipher operations (block, cbc, ccm and gcm) are not supported yet.
 */
__unused static int pufs_block_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
  return -ENOTSUP;
}

__unused static int pufs_cbc_op(struct cipher_ctx *ctx, struct cipher_pkt *pkt,
			uint8_t *iv)
{
  return -ENOTSUP;
}

__unused static int pufs_ccm_op(struct cipher_ctx *ctx, struct cipher_aead_pkt *pkt,
			 uint8_t *nonce)
{
  return -ENOTSUP;
}

__unused static int pufs_gcm_op(struct cipher_ctx *ctx, struct cipher_aead_pkt *pkt,
			 uint8_t *nonce)
{
  return -ENOTSUP;
}

static int pufs_cipher_begin_session(const struct device *dev, struct cipher_ctx *ctx,
                              enum cipher_algo algo, enum cipher_mode mode,
                              enum cipher_op op_type)
{
  ctx->device = dev;

  ((struct pufs_data *)dev->data)->pufs_ctx.cipher_ctx = ctx;

  struct pufs_data *lvPufsData = (struct pufs_data*)dev->data;

  uint16_t lvHashFlags = (
                            CAP_NO_ENCRYPTION | CAP_SYNC_OPS | 
                            CAP_ASYNC_OPS | CAP_NO_IV_PREFIX |
                            CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS
                         ), \
       lvHashFlagsMask = 0xFFFF;
  
  if(algo != CRYPTO_CIPHER_ALGO_AES) {
    LOG_ERR("%s(%d) UnSupported Algo. Only AES Supported\n", __func__, __LINE__);
    return -ENOTSUP;
  }

  if(mode != CRYPTO_CIPHER_MODE_CTR) {
    LOG_ERR("%s(%d) UnSupported Algo. Only CTR Mode Supported\n", __func__, __LINE__);
    return -ENOTSUP;
  } else {
    ctx->ops.ctr_crypt_hndlr = pufs_ctr_op;
    ctx->ops.block_crypt_hndlr = pufs_block_op;
    ctx->ops.cbc_crypt_hndlr = pufs_cbc_op;
    ctx->ops.ccm_crypt_hndlr = pufs_ccm_op;
    ctx->ops.gcm_crypt_hndlr = pufs_gcm_op;
  }

  if(op_type != CRYPTO_CIPHER_OP_DECRYPT) {
    LOG_ERR("%s(%d) UnSupported Operation. Only Decryption Supported\n", __func__, __LINE__);
    return -ENOTSUP;    
  }

  if((ctx->flags & lvHashFlagsMask) != (lvHashFlags)) {
    LOG_ERR("%s(%d) UnSupported Flags. Supported Flags_Mask:%d\n", __func__, __LINE__, lvHashFlags);
    return -ENOTSUP;
  }

  if(lvPufsData->pufs_session_type !=  PUFS_SESSION_UNDEFINED) {
    LOG_ERR("%s(%d) An Existing %s Session in Progress\n", __func__, __LINE__, \
            session_to_str(lvPufsData->pufs_session_type));
    return -ENOTSUP;
  } else {
    lvPufsData->pufs_session_type = PUFS_SESSION_DECRYPTION;
  }

  ctx->ops.cipher_mode = mode; 

  return PUFCC_SUCCESS;
}

/* Tear down an established session */
static int pufs_cipher_free_session(const struct device *dev, struct cipher_ctx *ctx)
{
    struct pufs_data *lvPufsData = (struct pufs_data*)dev->data;

  ctx->device = NULL;

  ctx->flags = 0x00;

  ctx->ops.block_crypt_hndlr = NULL;
  ctx->ops.cbc_crypt_hndlr = NULL;
  ctx->ops.ctr_crypt_hndlr = NULL;
  ctx->ops.ccm_crypt_hndlr = NULL;
  ctx->ops.gcm_crypt_hndlr = NULL;
 
  ctx->key.bit_stream = NULL;
  ctx->key.handle = NULL;

  ctx->drv_sessn_state = NULL;

  ctx->app_sessn_state = NULL;

  if(lvPufsData->pufs_session_type !=  PUFS_SESSION_DECRYPTION) {
    LOG_ERR("%s(%d) Cannot Free %s Session\n", __func__, __LINE__, \
            session_to_str(lvPufsData->pufs_session_type));
    return -ENOEXEC;
  } else {
    lvPufsData->pufs_session_type =  PUFS_SESSION_UNDEFINED;
    lvPufsData->pufs_session_callback.cipher_cb = NULL;
    lvPufsData->pufs_ctx.cipher_ctx = NULL;
    lvPufsData->pufs_pkt.cipher_pkt = NULL;
  }

  return PUFCC_SUCCESS;
}

/* Register async crypto op completion callback with the driver */
static int pufs_cipher_async_callback_set(const struct device *dev, cipher_completion_cb cb)
{
  if((pufs_query_hw_caps(dev) & CAP_ASYNC_OPS) != CAP_ASYNC_OPS) {
    LOG_ERR("%s(%d) Session:%s Does not Support Async Ops\n", __func__, __LINE__, \
    session_to_str(((struct pufs_data *)dev->data)->pufs_session_type));
    return -ENOTSUP;
  }

  ((struct pufs_data*)dev->data)->pufs_session_callback.cipher_cb = cb;

  irq_enable(((struct pufs_config *)dev->config)->irq_num);

  return PUFCC_SUCCESS;
}

static int pufs_hash_op(struct hash_ctx *ctx, struct hash_pkt *pkt, bool finish)
{
  enum pufcc_status lvStatus = PUFCC_SUCCESS;

  ((struct pufs_data*)ctx->device->data)->pufs_pkt.hash_pkt = pkt;
  ((struct pufs_data*)ctx->device->data)->pufs_ctx.hash_ctx = ctx;

  if(!ctx->started) {    // started flag indicates if chunkwise hash calculation was started
    lvStatus = pufcc_calc_sha256_hash(ctx, pkt);
  } else {
    lvStatus = pufcc_calc_sha256_hash_sg(ctx, pkt);
  }

  if(lvStatus != PUFCC_SUCCESS) {
    LOG_ERR("%s(%d) PUFs Error Code:%d\n", __func__, __LINE__, lvStatus);
    return -ECANCELED;
  }

  return PUFCC_SUCCESS;
}

/* Setup a hash session */
static int pufs_hash_begin_session(const struct device *dev, struct hash_ctx *ctx, enum hash_algo algo)
{
  ctx->device = dev;

  ((struct pufs_data *)dev->data)->pufs_ctx.hash_ctx = ctx;

  struct pufs_data *lvPufsData = (struct pufs_data*)dev->data;

  uint16_t lvHashFlags = (CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS | CAP_ASYNC_OPS), lvHashFlagsMask = 0xFFFF;
  
  if(algo != CRYPTO_HASH_ALGO_SHA256) {
    LOG_ERR("%s(%d) UnSupported Hash Algo. Only SHA256 Supported\n", __func__, __LINE__);
    return -ENOTSUP;
  }

  if((ctx->flags & lvHashFlagsMask) != (lvHashFlags)) {
    LOG_ERR("%s(%d) UnSupported Flags. Supported Flags_Mask:%d\n", __func__, __LINE__, lvHashFlags);
    return -ENOTSUP;
  }

  if(lvPufsData->pufs_session_type !=  PUFS_SESSION_UNDEFINED) {
    LOG_ERR("%s(%d) An Existing %s Session in Progress\n", __func__, __LINE__, \
            session_to_str(lvPufsData->pufs_session_type));
    return -ENOTSUP;
  } else {
    lvPufsData->pufs_session_type = PUFS_SESSION_HASH_CALCULATION;
  }

  ctx->hash_hndlr = pufs_hash_op;  

  return PUFCC_SUCCESS;
}

/* Tear down an established hash session */
static int pufs_hash_free_session(const struct device *dev, struct hash_ctx *ctx)
{
  struct pufs_data *lvPufsData = (struct pufs_data*)dev->data;

  ctx->device = NULL;

  ctx->started = false;

  ctx->flags = 0x00;
 
  ctx->hash_hndlr = NULL;  

  if(lvPufsData->pufs_session_type !=  PUFS_SESSION_HASH_CALCULATION) {
    LOG_ERR("%s(%d) Cannot Free %s Session\n", __func__, __LINE__, \
            session_to_str(lvPufsData->pufs_session_type));
    return -ENOEXEC;
  } else {
    lvPufsData->pufs_session_type =  PUFS_SESSION_UNDEFINED;
    lvPufsData->pufs_session_callback.hash_cb = NULL;
    lvPufsData->pufs_ctx.hash_ctx = NULL;
    lvPufsData->pufs_pkt.hash_pkt = NULL;
  }

  return PUFCC_SUCCESS;
}

/* Register async hash op completion callback with the driver */
static int pufs_hash_async_callback_set(const struct device *dev, hash_completion_cb cb)
{
  if((pufs_query_hw_caps(dev) & CAP_ASYNC_OPS) != CAP_ASYNC_OPS) {
    LOG_ERR("%s(%d) Session:%s Does not Support Async Ops\n", __func__, __LINE__, \
    session_to_str(((struct pufs_data *)dev->data)->pufs_session_type));
    return -ENOTSUP;
  }

  ((struct pufs_data*)dev->data)->pufs_session_callback.hash_cb = cb;

  irq_enable(((struct pufs_config *)dev->config)->irq_num);

  return PUFCC_SUCCESS;
}

/* Setup a signature session */
static int pufs_sign_begin_session(const struct device *dev, struct sign_ctx *ctx,
                                   enum sign_algo algo)
{
  ctx->device = dev;

  ((struct pufs_data *)dev->data)->pufs_ctx.sign_ctx = ctx;

  struct pufs_data *lvPufsData = (struct pufs_data*)dev->data;

  uint16_t lvHashFlags = (CAP_INPLACE_OPS | CAP_SYNC_OPS | CAP_ASYNC_OPS), lvHashFlagsMask = 0xFFFF;
  
  if((algo != CRYPTO_SIGN_ALGO_ECDSA256) || (algo != CRYPTO_SIGN_ALGO_RSA2048)) {
    LOG_ERR("%s(%d) Unupported Algo:%d. Supported Algo <ECDSA256, RSA2048>\n", __func__, __LINE__, algo);
    return -ENOTSUP;
  } else {
    if(algo == CRYPTO_SIGN_ALGO_ECDSA256) {
      ctx->ops.signing_algo = CRYPTO_SIGN_ALGO_ECDSA256;
      ctx->ops.ecdsa_crypt_hndlr = pufcc_ecdsa256_sign_verify;
    } else {
      ctx->ops.signing_algo = CRYPTO_SIGN_ALGO_RSA2048;
      ctx->ops.rsa_crypt_hndlr = pufcc_rsa2048_sign_verify;
    }
  }

  if((ctx->flags & lvHashFlagsMask) != (lvHashFlags)) {
    LOG_ERR("%s(%d) UnSupported Flags. Supported Flags_Mask:%d\n", __func__, __LINE__, lvHashFlags);
    return -ENOTSUP;
  }

  if(ctx->ops.signing_mode != CRYPTO_SIGN_VERIFY) {
    LOG_ERR("%s(%d) UnSupported Signing Action. Only Sign Verification Supported\n", __func__, __LINE__);
    return -ENOTSUP;
  }

  if(lvPufsData->pufs_session_type !=  PUFS_SESSION_UNDEFINED) {
    LOG_ERR("%s(%d) An Existing %s Session in Progress\n", __func__, __LINE__, \
            session_to_str(lvPufsData->pufs_session_type));
    return -ENOTSUP;
  } else {
    lvPufsData->pufs_session_type = PUFS_SESSION_SIGN_VERIFICATION;
  }

  return PUFCC_SUCCESS;
}

/* Tear down an established signature session */
static int pufs_sign_free_session(const struct device *dev, struct sign_ctx *ctx)
{
  struct pufs_data *lvPufsData = (struct pufs_data*)dev->data;

  ctx->device = NULL;

  ctx->flags = 0x00;

  ctx->ops.rsa_crypt_hndlr = NULL;

  ctx->ops.ecdsa_crypt_hndlr = NULL;
 
  ctx->pub_key = NULL;

  ctx->sig = NULL;

  ctx->drv_sessn_state = NULL;

  ctx->app_sessn_state = NULL;

  if(lvPufsData->pufs_session_type !=  PUFS_SESSION_SIGN_VERIFICATION) {
    LOG_ERR("%s(%d) Cannot Free %s Session\n", __func__, __LINE__, \
            session_to_str(lvPufsData->pufs_session_type));
    return -ENOEXEC;
  } else {
    lvPufsData->pufs_session_type =  PUFS_SESSION_UNDEFINED;
    lvPufsData->pufs_session_callback.sign_cb = NULL;
    lvPufsData->pufs_ctx.sign_ctx = NULL;
    lvPufsData->pufs_pkt.sign_pkt = NULL;
  }

  return PUFCC_SUCCESS;
}

/* Register async signature op completion callback with the driver */
static int pufs_sign_async_callback_set(const struct device *dev, sign_completion_cb cb)
{
  if((pufs_query_hw_caps(dev) & CAP_ASYNC_OPS) != CAP_ASYNC_OPS) {
    LOG_ERR("%s(%d) Session:%s Does not Support Async Ops\n", __func__, __LINE__, \
    session_to_str(((struct pufs_data *)dev->data)->pufs_session_type));
    return -ENOTSUP;
  }
    
  ((struct pufs_data*)dev->data)->pufs_session_callback.sign_cb = cb;

  irq_enable(((struct pufs_config *)dev->config)->irq_num);

  return PUFCC_SUCCESS;
}

static struct crypto_driver_api s_crypto_funcs = {
	.cipher_begin_session = pufs_cipher_begin_session,
  .cipher_free_session = pufs_cipher_free_session,
  .cipher_async_callback_set = pufs_cipher_async_callback_set,
  .hash_begin_session = pufs_hash_begin_session,
  .hash_free_session =  pufs_hash_free_session,
  .hash_async_callback_set =  pufs_hash_async_callback_set,
  .sign_begin_session = pufs_sign_begin_session,
  .sign_free_session =  pufs_sign_free_session,
  .sign_async_callback_set =  pufs_sign_async_callback_set,
  .query_hw_caps =  pufs_query_hw_caps
};

static struct pufs_data s_pufs_session_data = {
  .pufs_session_type = PUFS_SESSION_UNDEFINED,
  .pufs_session_callback = {NULL},
  .pufs_ctx = {NULL},
  .pufs_pkt = {NULL}
};

static const struct pufs_config s_pufs_configuration = {
  .base = DT_INST_REG_ADDR(0),
  .irq_init = pufs_irq_Init,
  .irq_num = DT_INST_IRQN(0),
  .dev = DEVICE_DT_INST_GET(0)
};

DEVICE_DT_INST_DEFINE(0, crypto_pufs_init, NULL,
		    &s_pufs_session_data,
		    &s_pufs_configuration, POST_KERNEL,
		    CONFIG_CRYPTO_INIT_PRIORITY, (void *)&s_crypto_funcs);
