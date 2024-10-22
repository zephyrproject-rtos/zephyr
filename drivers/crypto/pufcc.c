#include "pufcc.h"

#include <string.h>

#if CONFIG_RS_RTOS_PORT
  #include <zephyr/sys/sys_io.h>
  #include <zephyr/kernel.h>
  #include "crypto_pufs.h"
  #include <stdio.h>
  static volatile bool s_Asynch_Operation = false;
  void pufcc_set_asynch_ops_flag(bool Val) {s_Asynch_Operation = Val;}
  bool pufcc_get_asynch_ops_flag(void) {return s_Asynch_Operation;}
#else
  #include "profiling_util.h"
  #include "rs_crypto.h"
  #include "rs_util.h"
#endif

/*****************************************************************************
 * Macros
 ****************************************************************************/
#define SG_DMA_MAX_DSCS_SIZE (512 - 8)  // Enough for 15 descriptors
#if !CONFIG_RS_RTOS_PORT
  #define BUFFER_SIZE 512
#endif
#define PUFCC_MAX_BUSY_COUNT 8000000  // Max busy count for processing 10MB data
#define CTR_MODE_BLOCK_SIZE 16

/*****************************************************************************
 * Local variable declarations
 ****************************************************************************/
#if CONFIG_RS_RTOS_PORT
  static struct pufcc_sg_dma_desc *sg_dma_descs =
    (struct pufcc_sg_dma_desc *)__pufcc_descriptors;
#else
  extern uint32_t __pufcc_descriptors;
  static struct pufcc_sg_dma_desc *sg_dma_descs =
    (struct pufcc_sg_dma_desc *)&__pufcc_descriptors;
#endif


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
static volatile struct pufcc_dma_regs *dma_regs;
static volatile struct pufcc_rt_regs *rt_regs;
static volatile struct pufcc_otp_mem *otp_mem;
static volatile struct pufcc_hmac_regs *hmac_regs;
static volatile struct pufcc_crypto_regs *crypto_regs;
static volatile struct pufcc_sp38a_regs *sp38a_regs;
static volatile struct pufcc_pkc_regs *pkc_regs;

/*****************************************************************************
 * Local function declarations
 ****************************************************************************/
static enum pufcc_status rsa_p1v15_verify(const uint8_t *dec_msg,
                                          struct rs_crypto_addr *msg_addr);
static enum pufcc_status otp_range_check(uint32_t addr, uint32_t len);
static int rwlck_index_get(uint32_t idx);
static void reverse(uint8_t *dst, const uint8_t *src, size_t len);
static uint32_t be2le(uint32_t var);
static enum pufcc_status busy_wait(volatile uint32_t *status_reg_addr,
                                   uint32_t error_mask);

/*****************************************************************************
 * API functions
 ****************************************************************************/
/**
 * @fn    pufcc_calc_sha256_hash
 * @brief Calculates SHA256 hash
 *
 * @param[in]  data_addr  Data address info
 * @param[in]  hash       Pointer to hash strcut to return hash value in
 * @return                PUFCC_SUCCESS on success, otherwise an error code.
 */
enum pufcc_status pufcc_calc_sha256_hash(struct rs_crypto_addr *data_addr,
                                         struct rs_crypto_hash *hash) {
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
  dma_regs->dsc_cfg_0 = data_addr->read_addr;

  // Set data length in dsc_cfg_2 register
  dma_regs->dsc_cfg_2 = data_addr->len;

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
    ((uint32_t *)hash->val)[i] = be2le(crypto_regs->dgst_out[i]);
  }

  hash->len = PUFCC_SHA_256_LEN;

  return PUFCC_SUCCESS;
}

/**
 * @fn    pufcc_calc_sha256_hash_sg
 * @brief Calculates SHA256 hash of non-contiguous data.
 *        All non contiguous data addresses can be passed in as a single linked
 *        list in the form of 'rs_crypto_addr' struct as 'data_addr' parameter
 *        or this function can be invoked multiple times with partial data
 *        address info in the linked list by setting the 'first' and 'last'
 *        params accordingly as detailed below against each param. In case of
 *        partial data address info and hence multiple invocations, we also need
 *        to pass in previously calculated hash values to every invocation
 *        after the first one and the accumulated length of all previous
 *        messages/data.
 *
 *        Note: In case of multiple data chunks either in a single linked list
 *        or as partial linked lists using multiple invocations the sizes of all
 *        chunks must be multiples of 64 bytes except the last chunk.
 *
 * @param[in]  data_addr  A linked list of data address info
 * @param[in]  first      Boolean to indicate if 'data_addr' contains the first
 *                        data block
 * @param[in]  last       Boolean to indicate if 'data_addr' contains the last
 *                        data block
 *                        Note: 'first' and 'last' can both be true if
 *                        'data_addr' linked list contains both first and last
 *                        data blocks or if it contains a single data block that
 *                        is both first as well last.
 * @param[in]  prev_len   Pointer to accumulated length of previously processed
 *                        data; 0 for first invocation. Currently processed data
 *                        length will be added to this value.
 * @param[in]  hash_in    Hash of the data bocks processed already; can be NULL
 *                        for first invocation.
 * @param[out] hash_out   Pointer to hash strcut to return hash value in
 * @return                PUFCC_SUCCESS on success, otherwise an error code.
 */
enum pufcc_status pufcc_calc_sha256_hash_sg(struct rs_crypto_addr *data_addr,
                                            bool first, bool last,
                                            uint32_t *prev_len,
                                            struct rs_crypto_hash *hash_in,
                                            struct rs_crypto_hash *hash_out) {
  enum pufcc_status status;
  uint32_t plen = 0;
  uint8_t desc_count = 0;
  struct rs_crypto_addr *curr_addr = data_addr;
  struct pufcc_dma_cfg_0_reg dma_cfg_0_reg = {0};
  struct pufcc_dma_dsc_cfg_4_reg dma_dsc_cfg_4_reg = {0};
  struct pufcc_dma_key_cfg0_reg dma_key_cfg0_reg = {0};
  struct pufcc_intrpt_reg intrpt_reg = {0};
  struct pufcc_start_reg start_reg = {0};
  struct pufcc_hmac_config_reg hmac_config_reg = {0};

  if (!first) plen = *prev_len;

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
  if (!first) {
    for (uint8_t i = 0; i < (PUFCC_SHA_256_LEN / PUFCC_WORD_SIZE); i++) {
      crypto_regs->dgst_in[i] = be2le(((uint32_t *)hash_in->val)[i]);
    }
  }

  // Set SGDMA descriptors
  do {
    sg_dma_descs[desc_count].read_addr = be2le(curr_addr->read_addr);
    sg_dma_descs[desc_count].length = be2le(curr_addr->len);
    sg_dma_descs[desc_count].next =
        be2le((uint32_t)&sg_dma_descs[desc_count + 1]);

    sg_dma_descs[desc_count].key_cfg = be2le(*(uint32_t *)&dma_key_cfg0_reg);
    sg_dma_descs[desc_count].cypt_cfg[0] = be2le(*(uint32_t *)&hmac_config_reg);
    sg_dma_descs[desc_count].cypt_cfg[1] = be2le(plen);

    *(uint32_t *)&dma_dsc_cfg_4_reg = 0;
    dma_dsc_cfg_4_reg.offset = plen % 16;

    plen += curr_addr->len;
    curr_addr = curr_addr->next;

    if (!desc_count && first) {
      dma_dsc_cfg_4_reg.head = 1;
    }

    // Mark this descriptor as last if there is no more data
    if (!curr_addr) {
      dma_dsc_cfg_4_reg.dn_pause = 1;
      if (last) {
        dma_dsc_cfg_4_reg.tail = 1;
      }
    }

    sg_dma_descs[desc_count].dsc_cfg_4 = be2le(*(uint32_t *)&dma_dsc_cfg_4_reg);
    desc_count++;
  } while (curr_addr && ((desc_count * sizeof(struct pufcc_sg_dma_desc)) <
                         SG_DMA_MAX_DSCS_SIZE));

  if (curr_addr) {
    // No enough descriptors available
    return PUFCC_E_OVERFLOW;
  }

  // Update accumulated data length
  *prev_len = plen;

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
    ((uint32_t *)hash_out->val)[i] = be2le(crypto_regs->dgst_out[i]);
  }

  hash_out->len = PUFCC_SHA_256_LEN;
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
enum pufcc_status pufcc_decrypt_aes(uint32_t out_addr, uint32_t in_addr,
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
 * @param[in]  sig       Address of the message signature
 * @param[in]  msg_addr  Address of the message data
 * @param[in]  pub_key   RSA2048 public key
 * @return               PUFCC_SUCCESS on success, otherwise an error code.
 */
enum pufcc_status pufcc_rsa2048_sign_verify(
    uint8_t *sig, struct rs_crypto_addr *msg_addr,
    struct rs_crypto_rsa2048_puk *pub_key) {
  enum pufcc_status status = PUFCC_SUCCESS;
  uint32_t temp32;
  uint8_t dec_msg[PUFCC_RSA_2048_LEN];
  // Configure signature scheme
  temp32 = 0;

  struct pufcc_pkc_ecp_ec_reg *ecp_ec_reg =
      (struct pufcc_pkc_ecp_ec_reg *)&temp32;
  ecp_ec_reg->field = PUFCC_RSA_2048;
  REG_WRITE_32(&pkc_regs->ecp_ec, ecp_ec_reg);

  // Reverse public key modulus
  reverse(pufcc_buffer, pub_key->n, PUFCC_RSA_2048_LEN);

  #if !CONFIG_RS_RTOS_PORT
    memcpy((uint8_t *)&pkc_regs->ecp_data + PUFCC_DATA_RSA2048_MODULUS_OFFSET,
          pufcc_buffer, PUFCC_RSA_2048_LEN);
  #else
    uint32_t *ptr = (uint32_t*)&pkc_regs->ecp_data + PUFCC_DATA_RSA2048_MODULUS_OFFSET/4;
    uint32_t *buf_ptr = (uint32_t*)pufcc_buffer;
    for(int i = 0; i < PUFCC_RSA_2048_LEN/4; i++) {
      *ptr++ = *buf_ptr++;
    }
  #endif

  // Write public key exponent to ecp_e_short register
  REG_WRITE_32(&pkc_regs->ecp_e_short, &pub_key->e);

  // Reverse signature
  reverse(pufcc_buffer, sig, PUFCC_RSA_2048_LEN);

  // Write reversed signature to ECP data field at proper offset
  #if !CONFIG_RS_RTOS_PORT
    memcpy((uint8_t *)&pkc_regs->ecp_data + PUFCC_DATA_RSA2048_SIGN_OFFSET,
          pufcc_buffer, PUFCC_RSA_2048_LEN);
  #else
    ptr = (uint32_t*)&pkc_regs->ecp_data + PUFCC_DATA_RSA2048_SIGN_OFFSET/4;
    buf_ptr = (uint32_t*)pufcc_buffer;
    for(int i = 0; i < PUFCC_RSA_2048_LEN/4; i++) {
      *ptr++ = *buf_ptr++;
    }
  #endif

  // Write microprogram for RSA2048
  #if !CONFIG_RS_RTOS_PORT
    memcpy((void *)&pkc_regs->ecp_mac, rsa_2048_mprog, sizeof(rsa_2048_mprog));
  #else
    ptr = (uint32_t*)&pkc_regs->ecp_mac;
    buf_ptr = (uint32_t*)rsa_2048_mprog;
    for(int i = 0; i < sizeof(rsa_2048_mprog)/4; i++) {
      *ptr++ = *buf_ptr++;
    }
  #endif
  
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
    return status;
  }

  // Read decrypted message from proper offset in ECP data field and reverse it
  memcpy(pufcc_buffer,
         (uint8_t *)&pkc_regs->ecp_data + PUFCC_DATA_RSA2048_SIGN_OFFSET,
         PUFCC_RSA_2048_LEN);
  reverse(dec_msg, pufcc_buffer, PUFCC_RSA_2048_LEN);
  
  status = rsa_p1v15_verify(dec_msg, msg_addr);

  return status;
}

/**
 * @fn    pufcc_ecdsa256_sign_verify
 * @brief Verify ECDSA256 signature of the input message data
 *
 * @param[in]  sig       Address of the message signature
 * @param[in]  msg_addr  Address of the message data
 * @param[in]  pub_key   ECDSA256 public key
 * @return               PUFCC_SUCCESS on success, otherwise an error code.
 */
enum pufcc_status pufcc_ecdsa256_sign_verify(
    struct rs_crypto_ec256_sig *sig, struct rs_crypto_addr *msg_addr,
    struct rs_crypto_ec256_puk *pub_key) {
  uint32_t temp32, prev_len = 0;
  enum pufcc_status status;
  struct rs_crypto_hash hash;   

  // Calculate hash of the message
  if (pufcc_calc_sha256_hash_sg(msg_addr, true, true, &prev_len, NULL, &hash) !=
      PUFCC_SUCCESS) {
    return PUFCC_E_ERROR;
  }

#if !CONFIG_RS_RTOS_PORT
  RS_PROFILE_CHECKPOINT("msg hash calc");
#endif

  // Set the EC NIST P256 parameters after reversing them
  reverse(pufcc_buffer, ecc_param_nistp256.prime, PUFCC_ECDSA_256_LEN);
  #if CONFIG_RS_RTOS_PORT
    uint32_t *ptr = (uint32_t*)&pkc_regs->ecp_data + PUFCC_DATA_ECDSA_PRIME_OFFSET/4;
    uint32_t *buf_ptr = (uint32_t*)pufcc_buffer;
    for(int i = 0; i < PUFCC_ECDSA_256_LEN/4; i++) {
      *ptr++ = *buf_ptr++;
    }
  #else
    memcpy((uint8_t *)&pkc_regs->ecp_data + PUFCC_DATA_ECDSA_PRIME_OFFSET,
          pufcc_buffer, PUFCC_ECDSA_256_LEN);
  #endif

  reverse(pufcc_buffer, ecc_param_nistp256.a, PUFCC_ECDSA_256_LEN);
  #if CONFIG_RS_RTOS_PORT
    ptr = (uint32_t*)&pkc_regs->ecp_data + PUFCC_DATA_ECDSA_EC_A_OFFSET/4;
    buf_ptr = (uint32_t*)pufcc_buffer;
    for(int i = 0; i < PUFCC_ECDSA_256_LEN/4; i++) {
      *ptr++ = *buf_ptr++;
    }
  #else
    memcpy((uint8_t *)&pkc_regs->ecp_data + PUFCC_DATA_ECDSA_EC_A_OFFSET,
          pufcc_buffer, PUFCC_ECDSA_256_LEN);
  #endif

  reverse(pufcc_buffer, ecc_param_nistp256.b, PUFCC_ECDSA_256_LEN);
  #if CONFIG_RS_RTOS_PORT
    ptr = (uint32_t*)&pkc_regs->ecp_data + PUFCC_DATA_ECDSA_EC_B_OFFSET/4;
    buf_ptr = (uint32_t*)pufcc_buffer;
    for(int i = 0; i < PUFCC_ECDSA_256_LEN/4; i++) {
      *ptr++ = *buf_ptr++;
    }
  #else  
    memcpy((uint8_t *)&pkc_regs->ecp_data + PUFCC_DATA_ECDSA_EC_B_OFFSET,
          pufcc_buffer, PUFCC_ECDSA_256_LEN);
  #endif

  reverse(pufcc_buffer, ecc_param_nistp256.px, PUFCC_ECDSA_256_LEN);
  #if CONFIG_RS_RTOS_PORT
    ptr = (uint32_t*)&pkc_regs->ecp_data + PUFCC_DATA_ECDSA_PX_OFFSET/4;
    buf_ptr = (uint32_t*)pufcc_buffer;
    for(int i = 0; i < PUFCC_ECDSA_256_LEN/4; i++) {
      *ptr++ = *buf_ptr++;
    }
  #else
    memcpy((uint8_t *)&pkc_regs->ecp_data + PUFCC_DATA_ECDSA_PX_OFFSET,
          pufcc_buffer, PUFCC_ECDSA_256_LEN);
  #endif

  reverse(pufcc_buffer, ecc_param_nistp256.py, PUFCC_ECDSA_256_LEN);
  #if CONFIG_RS_RTOS_PORT
    ptr = (uint32_t*)&pkc_regs->ecp_data + PUFCC_DATA_ECDSA_PY_OFFSET/4;
    buf_ptr = (uint32_t*)pufcc_buffer;
    for(int i = 0; i < PUFCC_ECDSA_256_LEN/4; i++) {
      *ptr++ = *buf_ptr++;
    }
  #else  
    memcpy((uint8_t *)&pkc_regs->ecp_data + PUFCC_DATA_ECDSA_PY_OFFSET,
          pufcc_buffer, PUFCC_ECDSA_256_LEN);
  #endif          

  reverse(pufcc_buffer, ecc_param_nistp256.order, PUFCC_ECDSA_256_LEN);
  #if CONFIG_RS_RTOS_PORT
    ptr = (uint32_t*)&pkc_regs->ecp_data + PUFCC_DATA_ECDSA_ORDER_OFFSET/4;
    buf_ptr = (uint32_t*)pufcc_buffer;
    for(int i = 0; i < PUFCC_ECDSA_256_LEN/4; i++) {
      *ptr++ = *buf_ptr++;
    }
  #else    
    memcpy((uint8_t *)&pkc_regs->ecp_data + PUFCC_DATA_ECDSA_ORDER_OFFSET,
          pufcc_buffer, PUFCC_ECDSA_256_LEN);
  #endif

  // Configure signature scheme
  temp32 = 0;
  struct pufcc_pkc_ecp_ec_reg *ecp_ec_reg =
      (struct pufcc_pkc_ecp_ec_reg *)&temp32;
  ecp_ec_reg->field = PUFCC_ECDSA256;
  ecp_ec_reg->h = 1;  // EC cofactor h
  REG_WRITE_32(&pkc_regs->ecp_ec, ecp_ec_reg);

  // Write microprogram for ECDSA 256
  #if CONFIG_RS_RTOS_PORT
    ptr = (uint32_t*)&pkc_regs->ecp_mac;
    buf_ptr = (uint32_t*)p256_ecdsa_mprog;
    for(int i = 0; i < sizeof(p256_ecdsa_mprog)/4; i++) {
      *ptr++ = *buf_ptr++;
    }
  #else    
    memcpy((void *)&pkc_regs->ecp_mac, p256_ecdsa_mprog,
          sizeof(p256_ecdsa_mprog));
  #endif

  // Set the hash, public key & signature in PKC module after reversing each
  reverse(pufcc_buffer, hash.val, PUFCC_SHA_256_LEN);
  #if CONFIG_RS_RTOS_PORT
    ptr = (uint32_t*)&pkc_regs->ecp_data + PUFCC_DATA_ECDSA_HASH_OFFSET/4;
    buf_ptr = (uint32_t*)pufcc_buffer;
    for(int i = 0; i < PUFCC_ECDSA_256_LEN/4; i++) {
      *ptr++ = *buf_ptr++;
    }
  #else  
    memcpy((uint8_t *)&pkc_regs->ecp_data + PUFCC_DATA_ECDSA_HASH_OFFSET,
          pufcc_buffer, PUFCC_SHA_256_LEN);
  #endif

  reverse(pufcc_buffer, pub_key->x, PUFCC_ECDSA_256_LEN);
  #if CONFIG_RS_RTOS_PORT
    ptr = (uint32_t*)&pkc_regs->ecp_data + PUFCC_DATA_ECDSA_PUBX_OFFSET/4;
    buf_ptr = (uint32_t*)pufcc_buffer;
    for(int i = 0; i < PUFCC_ECDSA_256_LEN/4; i++) {
      *ptr++ = *buf_ptr++;
    }
  #else    
    memcpy((uint8_t *)&pkc_regs->ecp_data + PUFCC_DATA_ECDSA_PUBX_OFFSET,
          pufcc_buffer, PUFCC_ECDSA_256_LEN);
  #endif

  reverse(pufcc_buffer, pub_key->y, PUFCC_ECDSA_256_LEN);
  #if CONFIG_RS_RTOS_PORT
    ptr = (uint32_t*)&pkc_regs->ecp_data + PUFCC_DATA_ECDSA_PUBY_OFFSET/4;
    buf_ptr = (uint32_t*)pufcc_buffer;
    for(int i = 0; i < PUFCC_ECDSA_256_LEN/4; i++) {
      *ptr++ = *buf_ptr++;
    }
  #else   
    memcpy((uint8_t *)&pkc_regs->ecp_data + PUFCC_DATA_ECDSA_PUBY_OFFSET,
          pufcc_buffer, PUFCC_ECDSA_256_LEN);
  #endif          

  reverse(pufcc_buffer, sig->r, PUFCC_ECDSA_256_LEN);
  #if CONFIG_RS_RTOS_PORT
    ptr = (uint32_t*)&pkc_regs->ecp_data + PUFCC_DATA_ECDSA_SIG_R_OFFSET/4;
    buf_ptr = (uint32_t*)pufcc_buffer;
    for(int i = 0; i < PUFCC_ECDSA_256_LEN/4; i++) {
      *ptr++ = *buf_ptr++;
    }
  #else   
    memcpy((uint8_t *)&pkc_regs->ecp_data + PUFCC_DATA_ECDSA_SIG_R_OFFSET,
          pufcc_buffer, PUFCC_ECDSA_256_LEN);
  #endif          

  reverse(pufcc_buffer, sig->s, PUFCC_ECDSA_256_LEN);
  #if CONFIG_RS_RTOS_PORT
    ptr = (uint32_t*)&pkc_regs->ecp_data + PUFCC_DATA_ECDSA_SIG_S_OFFSET/4;
    buf_ptr = (uint32_t*)pufcc_buffer;
    for(int i = 0; i < PUFCC_ECDSA_256_LEN/4; i++) {
      *ptr++ = *buf_ptr++;
    }
  #else     
    memcpy((uint8_t *)&pkc_regs->ecp_data + PUFCC_DATA_ECDSA_SIG_S_OFFSET,
          pufcc_buffer, PUFCC_ECDSA_256_LEN);
  #endif          

#if !CONFIG_RS_RTOS_PORT
  RS_PROFILE_CHECKPOINT("misc verif ops");
#endif

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

#if !CONFIG_RS_RTOS_PORT  
  RS_PROFILE_CHECKPOINT("PKC op");
#endif

  return status;
}

#if !CONFIG_RS_RTOS_PORT

  /**
   * @fn    pufcc_dma_transfer
   * @brief Transfer data using PUFcc DMA
   *
   * @param[in]  src_addr    Source data address
   * @param[in]  dest_addr   Destination data address
   * @param[in]  len         Length of the data to be transferred
   * @param[in]  fixed_read  Read data from a fixed address
   * @param[in]  fixed_write Write data to a fixed address
   * @return                 PUFCC_SUCCESS on success, otherwise an error code
   */
  enum pufcc_status pufcc_dma_transfer(uint32_t src_addr, uint32_t dest_addr,
                                      uint32_t len, bool fixed_read,
                                      bool fixed_write) {
    enum pufcc_status status;
    uint32_t temp32 = 0;

    // Configure DMA intrpt register
    struct pufcc_intrpt_reg *intrpt_reg = (struct pufcc_intrpt_reg *)&temp32;
    intrpt_reg->intrpt_st = 1;  // Write 1 to clear interrupt
    REG_WRITE_32(&dma_regs->interrupt, intrpt_reg);

    // Set dma_cfg_0 register
    temp32 = 0;  // rng_en = 0, sg_en = 0
    REG_WRITE_32(&dma_regs->cfg_0, &temp32);

    struct pufcc_dma_cfg_1_reg *cfg_1_reg = (struct pufcc_dma_cfg_1_reg *)&temp32;
    cfg_1_reg->rbst_max = 0xF;
    cfg_1_reg->rbst_min = 0xF;
    cfg_1_reg->wbst_max = 0xF;
    cfg_1_reg->wbst_min = 0xF;
    REG_WRITE_32(&dma_regs->cfg_1, cfg_1_reg);

    // Set data source address in dsc_cfg_0 register
    REG_WRITE_32(&dma_regs->dsc_cfg_0, &src_addr);

    // Set decrypted data destination address in dsc_cfg_1 register
    REG_WRITE_32(&dma_regs->dsc_cfg_1, &dest_addr);

    // Set data length in dsc_cfg_2 register
    REG_WRITE_32(&dma_regs->dsc_cfg_2, &len);

    // Configure dma_dsc_cfg_4 register
    temp32 = 0;
    struct pufcc_dma_dsc_cfg_4_reg *dsc_cfg_4_reg =
        (struct pufcc_dma_dsc_cfg_4_reg *)&temp32;
    dsc_cfg_4_reg->fw = fixed_write;
    dsc_cfg_4_reg->fr = fixed_read;
    dsc_cfg_4_reg->no_cypt = true;  // Bypass crypto modules
    REG_WRITE_32(&dma_regs->dsc_cfg_4, dsc_cfg_4_reg);

    // Start DMA operation
    struct pufcc_start_reg *start_reg = (struct pufcc_start_reg *)&temp32;
    start_reg->start_p = 1;
    REG_WRITE_32(&dma_regs->start, start_reg);

    // Poll on busy status
    status = busy_wait(&dma_regs->status_0, PUFCC_DMA_ERROR_MASK);

    return status;
  }

#endif

/**
 * @fn    pufcc_otp_setup_wait
 * @brief Wait for the PUFrt module setup during power on
 *
 * @return PUFCC_SUCCESS if OTP is setup successfully, otherwise an error code
 */
enum pufcc_status pufcc_otp_setup_wait(void) {
  enum pufcc_status status = busy_wait(&rt_regs->status, PUFCC_RT_ERROR_MASK);

  return status;
}

/**
 * @fn    pufcc_program_otp
 * @brief Write data to an OTP slot.
 *        PUFcc OTP memory contains 1024 bytes and is divided into 32
 *        individual slots of 32 bytes each.
 *
 * @param[in]  in_buf   Buffer containing data to be written to OTP
 * @param[in]  len      Length of the input buffer
 * @param[in]  otp_slot OTP slot; PUFCC_OTPKEY_0 ~ PUFCC_OTPKEY_31
 * @return              PUFCC_SUCCESS on success, otherwise an error code.
 */
enum pufcc_status pufcc_program_otp(const uint8_t *in_buf, uint32_t len,
                                    enum pufcc_otp_slot otp_slot) {
  enum pufcc_status check;
  uint16_t addr = otp_slot * PUFCC_OTP_KEY_LEN;
  uint32_t start_index = addr / PUFCC_WORD_SIZE;
  enum pufcc_otp_lock lock;

  if ((check = otp_range_check(addr, len)) != PUFCC_SUCCESS) return check;

  // Return error if write access is locked
  pufcc_get_otp_rwlck(otp_slot, &lock);
  if (lock != PUFCC_OTP_RW) return PUFCC_E_DENY;

  // Program the OTP slot
  for (uint32_t i = 0; i < len; i += 4) {
    union {
      uint32_t word;
      uint8_t byte[4];
    } otp_word;
    for (int8_t j = 3; j >= 0; j--)  // reserve, default 0xff
      otp_word.byte[j] = ((i + 3 - j) < len) ? in_buf[i + 3 - j] : 0xff;

    otp_mem->otp[start_index + (i / 4)] = otp_word.word;
  }

  return PUFCC_SUCCESS;
}

/**
 * @fn    pufcc_read_otp
 * @brief Read data from an OTP slot.
 *        PUFcc OTP memory contains 1024 bytes and is divided into 32
 *        individual slots of 32 bytes each.
 *
 * @param[in]  out_buf  Buffer to read data into
 * @param[in]  len      Length of the data to read
 * @param[in]  otp_slot OTP slot; PUFCC_OTPKEY_0 ~ PUFCC_OTPKEY_31
 * @return              PUFCC_SUCCESS on success, otherwise an error code.
 */
enum pufcc_status pufcc_read_otp(uint8_t *out_buf, uint32_t len,
                                 enum pufcc_otp_slot otp_slot) {
  enum pufcc_status check;
  uint16_t addr = otp_slot * PUFCC_OTP_KEY_LEN;
  uint32_t word, start_index, wlen;
  enum pufcc_otp_lock lock = PUFCC_OTP_RW;  // default value

  if ((check = otp_range_check(addr, len)) != PUFCC_SUCCESS) return check;

  // Return error if read access is locked
  pufcc_get_otp_rwlck(otp_slot, &lock);
  if (lock == PUFCC_OTP_NA) return PUFCC_E_DENY;

  wlen = len / PUFCC_WORD_SIZE;
  start_index = addr / PUFCC_WORD_SIZE;

  if (wlen > 0) {
    memcpy(out_buf, (void *)(otp_mem->otp + start_index),
           wlen * PUFCC_WORD_SIZE);

    uint32_t *out32 = (uint32_t *)out_buf;
    for (size_t i = 0; i < wlen; ++i) *(out32 + i) = be2le(*(out32 + i));
  }

  if (len % PUFCC_WORD_SIZE != 0) {
    out_buf += wlen * PUFCC_WORD_SIZE;
    word = be2le(*(otp_mem->otp + start_index + wlen));
    memcpy(out_buf, &word, len % PUFCC_WORD_SIZE);
  }

  return PUFCC_SUCCESS;
}

/**
 * @fn    pufcc_lock_otp
 * @brief Lock an OTP key slot according to passed-in lock value.
 *
 * @param[in]  otp_slot OTP slot; PUFCC_OTPKEY_0 ~ PUFCC_OTPKEY_31
 * @param[in]  len      Length of the OTP slot to be locked
 * @param[in]  lock     Value of the lock to be placed
 * @return              PUFCC_SUCCESS on success, otherwise an error code.
 */
enum pufcc_status pufcc_lock_otp(enum pufcc_otp_slot otp_slot, uint32_t len,
                                 enum pufcc_otp_lock lock) {
  enum pufcc_status check;
  uint32_t lock_val = lock, shift = 0, start = 0, end = 0, mask = 0, val32 = 0;
  int rwlock_index;
  uint16_t addr = otp_slot * PUFCC_OTP_KEY_LEN;

  if ((check = otp_range_check(addr, len)) != PUFCC_SUCCESS) return check;

  // Set end and start indices of OTP words
  end = (len + 3) / 4;
  start = addr / PUFCC_WORD_SIZE;

  for (uint32_t i = 0; i < end; i++) {
    int idx = start + i;

    // Get the index of RWLCK register corresponding to current OTP word index
    if ((rwlock_index = rwlck_index_get(idx)) == -1) {
      return PUFCC_E_ERROR;
    }

    // Get shift size for lock value to be written to RWLCK register according
    // to OTP word index
    shift = (idx % PUFCC_OTP_WORDS_PER_RWLCK_REG) *
            PUFCC_OTP_RWLCK_REG_BITS_PER_OTP_WORD;
    // Update lock value for current 'rwlock_index'
    val32 |= lock_val << shift;
    // Update mask value for current 'rwlock_index'
    mask |= 0xF << shift;

    // If we have fully utilized RWLCK register at 'rwlock_index' or this is
    // the end of OTP range that we are locking then write the lock value in
    // RWLCK register at 'rwlock_index'
    if (shift == 28 || i == end - 1) {
      // Read modify write
      val32 |= (rt_regs->pif[rwlock_index] & (~mask));
      rt_regs->pif[rwlock_index] = val32;

      // Clear values for next iteration
      val32 = 0;
      mask = 0;
    }
  }

  return PUFCC_SUCCESS;
}

/**
 * @fn    pufcc_zeroize_otp
 * @brief Zeroize an OTP key slot (32 bytes) permanently.
 *
 * @param[in] otp_slot OTP slot; PUFCC_OTPKEY_0 ~ PUFCC_OTPKEY_31
 * @return             PUFCC_SUCCESS on success, otherwise an error code.
 */
enum pufcc_status pufcc_zeroize_otp(enum pufcc_otp_slot otp_slot) {
  enum pufcc_status status;

  if (otp_slot < PUFCC_OTPKEY_0 || otp_slot > PUFCC_OTPKEY_31) {
    status = PUFCC_E_INVALID;
  } else {
    uint32_t zeroize_cmd =
        (otp_slot - PUFCC_OTPKEY_0) + PUFCC_OTP_ZEROIZE_BASE_CMD;

    rt_regs->otp_zeroize = zeroize_cmd;

    // Wait on busy status
    status = busy_wait(&rt_regs->status, PUFCC_RT_ERROR_MASK);
  }

  return status;
}

/**
 * @fn    pufcc_get_otp_rwlck
 * @brief Get the read write lock value of passed-in OTP slot. This function
 *        assumes that the lock value of all the words of an OTP is same as that
 *        of the first word of that slot
 *
 * @param[in]  otp_slot OTP slot; PUFCC_OTPKEY_0 ~ PUFCC_OTPKEY_31
 * @param[out] lock_val Lock address to return the lock value in
 * @return              PUFCC_SUCCESS on success, otherwise and error code
 */
enum pufcc_status pufcc_get_otp_rwlck(enum pufcc_otp_slot otp_slot,
                                      enum pufcc_otp_lock *lock_val) {
  enum pufcc_status check;
  uint16_t addr = otp_slot * PUFCC_OTP_KEY_LEN;

  if ((check = otp_range_check(addr, 4)) != PUFCC_SUCCESS) return check;

  // Get OTP word index
  int index = addr / PUFCC_WORD_SIZE;

  // Get offset of the lock value within RWLCK register corresponding to this
  // OTP word
  int rwlck_offset = (index % PUFCC_OTP_WORDS_PER_RWLCK_REG) *
                     PUFCC_OTP_RWLCK_REG_BITS_PER_OTP_WORD;

  // Get the index of RWLCK register corresponding to current OTP word index
  // read lock value at that index
  index = rwlck_index_get(index);
  uint32_t lck = (rt_regs->pif[index] >> rwlck_offset) & PUFCC_PIF_RWLCK_MASK;

  switch (lck) {
    // Read write access
    case PUFCC_OTP_RWLCK_RW_0:
    case PUFCC_OTP_RWLCK_RW_1:
    case PUFCC_OTP_RWLCK_RW_2:
    case PUFCC_OTP_RWLCK_RW_3:
    case PUFCC_OTP_RWLCK_RW_4:
      *lock_val = PUFCC_OTP_RW;
      break;

      // Read only access
    case PUFCC_OTP_RWLCK_RO_0:
    case PUFCC_OTP_RWLCK_RO_1:
    case PUFCC_OTP_RWLCK_RO_2:
      *lock_val = PUFCC_OTP_RO;
      break;

      // All other values indicate no access
    default:
      *lock_val = PUFCC_OTP_NA;
      break;
  }

  return PUFCC_SUCCESS;
}

#if CONFIG_RS_RTOS_PORT

  int pufcc_clear_and_disable_intr(void) {
    int status = (dma_regs->status_0 & PUFCC_DMA_ERROR_MASK ? -1 : 0);
    struct pufcc_intrpt_reg *intrpt_reg_ptr =
        (struct pufcc_intrpt_reg *)&dma_regs->interrupt;

    // Clear and disable interrupt
    intrpt_reg_ptr->intrpt_st = 1;  // Set to clear
    intrpt_reg_ptr->intrpt_en = 0;

    return status;
  }

#else

  int pufcc_dma_request_channel(struct pufcc_dma_dev *dev) {
    if (dev->is_dev_free) {
      dev->is_dev_free = false;
      return 0;
    }

    return -1;
  }

  void pufcc_dma_release_channel(struct pufcc_dma_dev *dev, int channel) {
    (void)channel;
    dev->is_dev_free = true;
  }

  enum rs_status pufcc_dma_config_descriptor_memory(struct pufcc_dma_dev *dev,
                                                    int channel, uintptr_t addr,
                                                    size_t max_descriptors) {
    // Check that channel is valid and is in use
    if ((channel != 0) || (dev->is_dev_free)) {
      return ERROR;
    }

    dev->dma_descs = (struct pufcc_sg_dma_desc *)addr;
    dev->num_descriptors = max_descriptors;
    return OK;
  }

  enum rs_status pufcc_dma_config_xfer(struct pufcc_dma_dev *dev, int channel,
                                      struct rs_dma_config *config) {
    uint8_t desc_count = 0;
    struct rs_dma_block_config *current_block = config->head_block;
    struct pufcc_dma_cfg_0_reg dma_cfg_0_reg = {0};
    struct pufcc_dma_cfg_1_reg cfg_1_reg = {0};
    struct pufcc_dma_dsc_cfg_4_reg dma_dsc_cfg_4_reg = {0};
    struct pufcc_intrpt_reg intrpt_reg = {0};

    // Check that channel is valid and is in use and required descriptors for this
    // transactions are available
    if ((channel != 0) || (dev->is_dev_free) ||
        (config->block_count > dev->num_descriptors)) {
      return ERROR;
    }

    // Write 1 to clear interrupt
    intrpt_reg.intrpt_st = 1;

    // Setup interrupt
    if (config->complete_callback_en) {
      intrpt_reg.intrpt_en = 1;
    } else {
      intrpt_reg.intrpt_en = 0;
    }

    // Set SGDMA descriptors
    do {
      dev->dma_descs[desc_count].read_addr =
          be2le((uint32_t)current_block->src_addr);
      dev->dma_descs[desc_count].write_addr =
          be2le((uint32_t)current_block->dst_addr);
      dev->dma_descs[desc_count].length = be2le(current_block->block_size);
      dev->dma_descs[desc_count].next =
          be2le((uint32_t)&dev->dma_descs[desc_count + 1]);

      dev->dma_descs[desc_count].key_cfg = 0;
      dev->dma_descs[desc_count].cypt_cfg[0] = 0;
      dev->dma_descs[desc_count].cypt_cfg[1] = 0;

      *(uint32_t *)&dma_dsc_cfg_4_reg = 0;
      if (current_block->src_addr_adjust == RS_DMA_ADDR_ADJUST_FIXED) {
        dma_dsc_cfg_4_reg.fr = 1;
      } else if (current_block->src_addr_adjust != RS_DMA_ADDR_ADJUST_INCREMENT) {
        return ERROR;
      }

      if (current_block->dst_addr_adjust == RS_DMA_ADDR_ADJUST_FIXED) {
        dma_dsc_cfg_4_reg.fw = 1;
      } else if (current_block->dst_addr_adjust != RS_DMA_ADDR_ADJUST_INCREMENT) {
        return ERROR;
      }

      // Bypass crypto modules
      dma_dsc_cfg_4_reg.no_cypt = true;

      if (!desc_count) {
        dma_dsc_cfg_4_reg.head = true;
      }

      current_block = current_block->next_block;

      // Mark this descriptor as last if there is no more data
      if (!current_block) {
        dma_dsc_cfg_4_reg.dn_pause = true;
        dma_dsc_cfg_4_reg.tail = true;

        if (config->complete_callback_en) {
          dma_dsc_cfg_4_reg.dn_intrpt = true;
        }
      }

      dev->dma_descs[desc_count].dsc_cfg_4 =
          be2le(*(uint32_t *)&dma_dsc_cfg_4_reg);

      desc_count++;
    } while (current_block && (desc_count < dev->num_descriptors));

    if (current_block) {
      // No enough descriptors available
      return ERROR;
    }

    /*** Configure DMA registers ***/
    // Enable SGDMA in dma_cfg_0 register
    dma_cfg_0_reg.sg_en = 1;
    REG_WRITE_32(&dev->regs->cfg_0, &dma_cfg_0_reg);

    cfg_1_reg.rbst_max = 0xF;
    cfg_1_reg.rbst_min = 0xF;
    cfg_1_reg.wbst_max = 0xF;
    cfg_1_reg.wbst_min = 0xF;
    REG_WRITE_32(&dev->regs->cfg_1, &cfg_1_reg);

    // Set dsc_cfg_2 register to indicate it's an SGDMA operation
    dev->regs->dsc_cfg_2 = PUFCC_DMA_DSC_CFG2_SGDMA_VAL;

    // Set starting address of SGDMA descriptors in dma_dsc_cfg3 register
    dev->regs->dsc_cfg_3 = (uint32_t)dev->dma_descs;

    // Write interrupt register with values populated above
    REG_WRITE_32(&dev->regs->interrupt, &intrpt_reg);

    // Set up call backs
    dev->callback = config->callback;
    dev->callback_args = config->callback_args;

    return OK;
  }

  enum rs_status pufcc_dma_start_xfer(struct pufcc_dma_dev *dev, int channel) {
    struct pufcc_start_reg start_reg = {0};

    // Check that channel number is valid and the channel has been requested
    if ((channel != 0) || (dev->is_dev_free)) {
      return ERROR;
    }

    // Start the DMA operation by writing to its start register
    start_reg.start_p = 1;
    REG_WRITE_32(&dev->regs->start, &start_reg);

    return OK;
  }

  enum rs_status pufcc_dma_stop_xfer(struct pufcc_dma_dev *dev, int channel) {
    struct pufcc_sg_dma_desc *next_desc_ptr;
    struct pufcc_dma_dsc_cfg_4_reg dma_dsc_cfg_4_reg = {0};

    // check the channel number is valid and the channel has been requested
    if ((channel != 0) || dev->is_dev_free) {
      return ERROR;
    }

    // Get pointer to next descriptor in queue
    next_desc_ptr = (struct pufcc_sg_dma_desc *)dev->regs->dsc_cur_3;

    // Make sure that this descriptor lies within descriptor memory; it will lie
    // outside the memory if last descriptor is already being processed
    if (((uint32_t)next_desc_ptr > (uint32_t)dev->dma_descs) &&
        ((uint32_t)next_desc_ptr <
        ((uint32_t)dev->dma_descs +
          (dev->num_descriptors * sizeof(struct pufcc_sg_dma_desc))))) {
      // Set up descriptor to stop DMA operation
      *(uint32_t *)&dma_dsc_cfg_4_reg = next_desc_ptr->dsc_cfg_4;
      dma_dsc_cfg_4_reg.dn_pause = true;
      dma_dsc_cfg_4_reg.tail = true;
    }

    return OK;
  }

  void pufcc_dma_irq_handler(struct pufcc_dma_dev *dev) {
    int status = (dev->regs->status_0 & PUFCC_DMA_ERROR_MASK ? -1 : 0);
    struct pufcc_intrpt_reg *intrpt_reg_ptr =
        (struct pufcc_intrpt_reg *)&dev->regs->interrupt;

    // Clear and disable interrupt
    intrpt_reg_ptr->intrpt_st = 1;  // Set to clear
    intrpt_reg_ptr->intrpt_en = 0;

    dev->callback(dev->callback_args, 0, status);
  }

#endif

/**
 * @fn    rsa_p1v15_verify
 * @brief Verify input RSA2048 decrypted message according to PKCS#1 v1.5 RSA
 *        verification standard.
 * @param[in]  dec_msg  Input RSA decrypted message
 * @param[in]  msg_addr Address of the original data that was RSA signed
 * @return              PUFCC_SUCCESS on success, otherwise an error code.
 */
static enum pufcc_status rsa_p1v15_verify(const uint8_t *dec_msg,
                                          struct rs_crypto_addr *msg_addr) {
  uint32_t i, prev_len = 0;
  struct rs_crypto_hash hash;
  uint8_t pret[19] = {0x30, 0,    0x30, 0x0d, 0x06, 0x09, 0x60,
                      0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02,
                      0,    0x05, 0x00, 0x04, 0};

  if ((dec_msg[0] != 0x00) || (dec_msg[1] != 0x01)) {
    printf("%s(%d)\r\n", __func__,__LINE__);
    return PUFCC_E_VERFAIL;
  }

  for (i = 2; i < PUFCC_RSA_2048_LEN; i++) {
    if (dec_msg[i] != 0xff) {
      break;
    }
  }

  if (dec_msg[i++] != 0x00) {
    printf("%s(%d)\r\n", __func__,__LINE__);
    return PUFCC_E_VERFAIL;
  }

  // Verify that decrypted message has SHA256 hash
  if (dec_msg[i + 14] == 1) {
    pret[1] = 0x31;
    pret[14] = 0x01;
    pret[18] = 0x20;
  } else {
    printf("%s(%d)\r\n", __func__,__LINE__);
    return PUFCC_E_INVALID;
  }

  if ((memcmp(dec_msg + i, pret, 19) != 0) ||
      ((i + 19 + pret[18]) != PUFCC_RSA_2048_LEN)) {
        printf("%s(%d)\r\n", __func__,__LINE__);
    return PUFCC_E_VERFAIL;
  }

  // Calculate hash of the message
  if (pufcc_calc_sha256_hash_sg(msg_addr, true, true, &prev_len, NULL, &hash) !=
      PUFCC_SUCCESS) {
    printf("%s(%d)\r\n", __func__,__LINE__);
    return PUFCC_E_ERROR;
  }

  if (memcmp(dec_msg + i + 19, hash.val, hash.len) != 0) {
    printf("%s(%d)\r\n", __func__,__LINE__);
    return PUFCC_E_VERFAIL;
  }

  return PUFCC_SUCCESS;
}

/**
 * @fn    rwlck_index_get
 * @brief Get the index of RWLCK register corresponding to passed-in OTP word
 *        index.
 *
 * @param[in] idx OTP word index for which to get the RWLCK register index for
 * @return        Index of the RWLCK register
 */
static int rwlck_index_get(uint32_t idx) {
  uint32_t rwlck_idx = idx / PUFCC_OTP_WORDS_PER_RWLCK_REG;

  if (rwlck_idx >= PUFCC_PIF_MAX_RWLOCK_REGS) return -1;

  // Return the actual index of RWCLK register in PIF registers group
  return PUFCC_PIF_RWLCK_START_INDEX + rwlck_idx;
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
 * @fn    otp_range_check
 * @brief Check range validity of the input OTP address
 *
 * @param[in]  addr OTP address
 * @param[in]  len  Length of OTP after OTP address
 * @return          4 byte word with reversed endianness
 * @return          PUFCC_SUCCESS if OTP address and range is valid, otherwise
 *                  an error code.
 */
static enum pufcc_status otp_range_check(uint32_t addr, uint32_t len) {
  // word-aligned OTP address check
  if ((addr % PUFCC_WORD_SIZE) != 0) return PUFCC_E_ALIGN;
  // OTP boundary check
  if ((len > PUFCC_OTP_LEN) || (addr > (PUFCC_OTP_LEN - len)))
    return PUFCC_E_OVERFLOW;
  return PUFCC_SUCCESS;
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
    #if CONFIG_RS_RTOS_PORT
      status = sys_read32((mem_addr_t)status_reg_addr);
    #else
      status = read_reg(status_reg_addr);
    #endif
    busy_count--;
  } while ((status & PUFCC_BUSY_BIT_MASK) && (busy_count > 0));

  if (status & PUFCC_BUSY_BIT_MASK) {
    printf("%s(%d)\r\n", __func__,__LINE__);
    return PUFCC_E_TIMEOUT;
  } else if (status & error_mask) {
    printf("%s(%d) status 0x%08x ecp_err_code:0x%x\r\n", \
    __func__,__LINE__, status, pkc_regs->ecp_err_code);
    return PUFCC_E_ERROR;
  }

  return PUFCC_SUCCESS;
}

enum pufcc_status pufcc_init(uint32_t base_addr) {
  enum pufcc_status status;

  // Initialize base addresses of different PUFcc modules
  dma_regs = (struct pufcc_dma_regs *)base_addr;
  rt_regs = (struct pufcc_rt_regs *)(base_addr + PUFCC_RT_OFFSET);
  otp_mem = (struct pufcc_otp_mem *)(base_addr + PUFCC_RT_OFFSET +
                                     PUFCC_RT_OTP_OFFSET);
  hmac_regs = (struct pufcc_hmac_regs *)(base_addr + PUFCC_HMAC_OFFSET);
  crypto_regs = (struct pufcc_crypto_regs *)(base_addr + PUFCC_CRYPTO_OFFSET);
  sp38a_regs = (struct pufcc_sp38a_regs *)(base_addr + PUFCC_SP38A_OFFSET);
  pkc_regs = (struct pufcc_pkc_regs *)(base_addr + PUFCC_PKC_OFFSET);

  // Wait for OTP setup
  status = pufcc_otp_setup_wait();

  return status;
}
