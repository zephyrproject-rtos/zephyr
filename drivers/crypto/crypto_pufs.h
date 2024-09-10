/*
 * Copyright (c) 2024 Muhammad Junaid Aslam <junaid.aslam@rapidsilicon.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_DRIVERS_CRYPTO_PUFS_H_
#define ZEPHYR_DRIVERS_CRYPTO_PUFS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdint.h>

/*** Generic PUFcc defines ***/
#define PUFCC_WORD_SIZE 4
#define PUFCC_BUSY_BIT_MASK 0x00000001

/*** RT and OTP defines ***/
#define PUFCC_RT_OFFSET 0x3000
#define PUFCC_RT_OTP_OFFSET 0x400
#define PUFCC_RT_ERROR_MASK 0x0000001e
#define PUFCC_OTP_LEN 1024
#define PUFCC_OTP_KEY_LEN 32
#define PUFCC_OTP_ZEROIZE_BASE_CMD 0x80

// One read/write lock register controls 8 OTP words
#define PUFCC_OTP_WORDS_PER_RWLCK_REG 8

// 4 bits are reserved for lock value of one OTP word in read/write lock
// register
#define PUFCC_OTP_RWLCK_REG_BITS_PER_OTP_WORD 4
#define PUFCC_PIF_RWLCK_MASK 0xF
#define PUFCC_PIF_MAX_RWLOCK_REGS \
  (PUFCC_OTP_LEN / PUFCC_WORD_SIZE / PUFCC_OTP_WORDS_PER_RWLCK_REG)

// Start index of the RWLCK register in PIF registers group
#define PUFCC_PIF_RWLCK_START_INDEX 32

// Define all possible OTP lock values
#define PUFCC_OTP_RWLCK_RW_0 0x0  // Read Write access
#define PUFCC_OTP_RWLCK_RW_1 0x1  // Read Write access
#define PUFCC_OTP_RWLCK_RW_2 0x2  // Read Write access
#define PUFCC_OTP_RWLCK_RW_3 0x4  // Read Write access
#define PUFCC_OTP_RWLCK_RW_4 0x8  // Read Write access
#define PUFCC_OTP_RWLCK_RO_0 0x3  // Read Only access
#define PUFCC_OTP_RWLCK_RO_1 0x7  // Read Only access
#define PUFCC_OTP_RWLCK_RO_2 0xb  // Read Only access

/*** DMA defines ***/
#define PUFCC_DMA_KEY_DST_HASH 0x1
#define PUFCC_DMA_KEY_DST_SP38A 0x8
#define PUFCC_DMA_DSC_CFG2_SGDMA_VAL 0x20
#define PUFCC_DMA_ERROR_MASK 0xFFFFFFFE

/*** HMAC defines ***/
#define PUFCC_HMAC_OFFSET 0x0800
#define PUFCC_HMAC_SW_KEY_MAXLEN 64
#define PUFCC_SHA_256_LEN 32
#define PUFCC_HMAC_FUNCTION_HASH 0x0
#define PUFCC_HMAC_VARIANT_SHA256 0x03

/*** Crypto defines ***/
#define PUFCC_CRYPTO_OFFSET 0x0100
#define PUFCC_CRYPTO_SW_KEY_MAXLEN 64  // The byte length of CRYPTO_SW_KEY_ADDR
#define PUFCC_CRYPTO_DGST_LEN 64       // The byte length of CRYPTO_DGST field
#define PUFCC_CRYPTO_IV_MAXLEN 16
#define PUFCC_CRYPTO_AES128_KEY_LEN 16
#define PUFCC_CRYPTO_AES256_KEY_LEN 32

/*** SP38a defines ***/
#define PUFCC_SP38A_OFFSET 0x0200
#define PUFCC_SP38A_STATUS_ERROR_MASK 0xfffff0c0

/*** PKC defines ***/
#define PUFCC_PKC_OFFSET 0x1000
#define PUFCC_RSA_2048_LEN 256
#define PUFCC_ECDSA_256_LEN 32
#define PUFCC_DATA_RSA2048_MODULUS_OFFSET 256
#define PUFCC_DATA_RSA2048_SIGN_OFFSET 768
#define PUFCC_DATA_ECDSA_PRIME_OFFSET 256
#define PUFCC_PKC_ERROR_MASK 0xFFFFFFFE
#define PUFCC_DATA_ECDSA_EC_A_OFFSET \
  (PUFCC_DATA_ECDSA_PRIME_OFFSET + PUFCC_ECDSA_256_LEN)

#define PUFCC_DATA_ECDSA_EC_B_OFFSET \
  (PUFCC_DATA_ECDSA_EC_A_OFFSET + PUFCC_ECDSA_256_LEN)

#define PUFCC_DATA_ECDSA_PX_OFFSET \
  (PUFCC_DATA_ECDSA_EC_B_OFFSET + PUFCC_ECDSA_256_LEN)

#define PUFCC_DATA_ECDSA_PY_OFFSET \
  (PUFCC_DATA_ECDSA_PX_OFFSET + PUFCC_ECDSA_256_LEN)

#define PUFCC_DATA_ECDSA_ORDER_OFFSET \
  (PUFCC_DATA_ECDSA_PY_OFFSET + PUFCC_ECDSA_256_LEN)

#define PUFCC_DATA_ECDSA_HASH_OFFSET \
  (PUFCC_DATA_ECDSA_ORDER_OFFSET + PUFCC_ECDSA_256_LEN)

#define PUFCC_DATA_ECDSA_PUBX_OFFSET \
  (PUFCC_DATA_ECDSA_HASH_OFFSET + PUFCC_ECDSA_256_LEN)

#define PUFCC_DATA_ECDSA_PUBY_OFFSET \
  (PUFCC_DATA_ECDSA_PUBX_OFFSET + PUFCC_ECDSA_256_LEN)

#define PUFCC_DATA_ECDSA_SIG_R_OFFSET \
  (PUFCC_DATA_ECDSA_PUBY_OFFSET + PUFCC_ECDSA_256_LEN)

#define PUFCC_DATA_ECDSA_SIG_S_OFFSET \
  (PUFCC_DATA_ECDSA_SIG_R_OFFSET + PUFCC_ECDSA_256_LEN)

/**
 * @brief SHA lengths
 */
#define RS_SHA_MAX_LEN 64
#define RS_SHA256_LEN 32

/**
 * @brief ECDSA256 quadrant and key lengths
 */
#define RS_EC256_QLEN 32
#define RS_EC256_KEY_LEN (32 * 2)

/**
 * @brief RSA 2048 public key modulus length
 *
 */
#define RS_RSA_2048_LEN 256

/**
 * @brief RSA 2048 public key exponent length
 */
#define RS_RSA_E_LEN 4

/**
 * @brief RSA 2048 public key length
 */
#define RS_RSA_2048_KEY_LEN (RS_RSA_2048_LEN + RS_RSA_E_LEN)

/**
 * @brief IV length for AES-CTR128
 */
#define RS_AES_CTR128_IV_EN 16

/**
 * @brief Key length for AES128
 */
#define RS_AES16_KEY_LEN 16

/**
 * @brief Key length for AES256
 */
#define RS_AES32_KEY_LEN 32

/*****************************************************************************
 * Enumerations
 ****************************************************************************/
// PUFcc status codes
enum pufcc_status {
  PUFCC_SUCCESS,      // Success.
  PUFCC_E_ALIGN,      // Address alignment mismatch.
  PUFCC_E_OVERFLOW,   // Space overflow.
  PUFCC_E_UNDERFLOW,  // Size too small.
  PUFCC_E_INVALID,    // Invalid argument.
  PUFCC_E_BUSY,       // Resource is occupied.
  PUFCC_E_UNAVAIL,    // Resource is unavailable.
  PUFCC_E_FIRMWARE,   // Firmware error.
  PUFCC_E_VERFAIL,    // Invalid public key or digital signature.
  PUFCC_E_ECMPROG,    // Invalid ECC microprogram.
  PUFCC_E_DENY,       // Access denied.
  PUFCC_E_UNSUPPORT,  // Not support.
  PUFCC_E_INFINITY,   // Point at infinity.
  PUFCC_E_ERROR,      // Unspecific error.
  PUFCC_E_TIMEOUT,    // Operation timed out.
};

// PUFcc key slots; 32 slots of 256 bits each
enum pufcc_otp_slot {
  PUFCC_OTPKEY_0,   // OTP key slot 0, 256 bits
  PUFCC_OTPKEY_1,   // OTP key slot 1, 256 bits
  PUFCC_OTPKEY_2,   // OTP key slot 2, 256 bits
  PUFCC_OTPKEY_3,   // OTP key slot 3, 256 bits
  PUFCC_OTPKEY_4,   // OTP key slot 4, 256 bits
  PUFCC_OTPKEY_5,   // OTP key slot 5, 256 bits
  PUFCC_OTPKEY_6,   // OTP key slot 6, 256 bits
  PUFCC_OTPKEY_7,   // OTP key slot 7, 256 bits
  PUFCC_OTPKEY_8,   // OTP key slot 8, 256 bits
  PUFCC_OTPKEY_9,   // OTP key slot 9, 256 bits
  PUFCC_OTPKEY_10,  // OTP key slot 10, 256 bits
  PUFCC_OTPKEY_11,  // OTP key slot 11, 256 bits
  PUFCC_OTPKEY_12,  // OTP key slot 12, 256 bits
  PUFCC_OTPKEY_13,  // OTP key slot 13, 256 bits
  PUFCC_OTPKEY_14,  // OTP key slot 14, 256 bits
  PUFCC_OTPKEY_15,  // OTP key slot 15, 256 bits
  PUFCC_OTPKEY_16,  // OTP key slot 16, 256 bits
  PUFCC_OTPKEY_17,  // OTP key slot 17, 256 bits
  PUFCC_OTPKEY_18,  // OTP key slot 18, 256 bits
  PUFCC_OTPKEY_19,  // OTP key slot 19, 256 bits
  PUFCC_OTPKEY_20,  // OTP key slot 20, 256 bits
  PUFCC_OTPKEY_21,  // OTP key slot 21, 256 bits
  PUFCC_OTPKEY_22,  // OTP key slot 22, 256 bits
  PUFCC_OTPKEY_23,  // OTP key slot 23, 256 bits
  PUFCC_OTPKEY_24,  // OTP key slot 24, 256 bits
  PUFCC_OTPKEY_25,  // OTP key slot 25, 256 bits
  PUFCC_OTPKEY_26,  // OTP key slot 26, 256 bits
  PUFCC_OTPKEY_27,  // OTP key slot 27, 256 bits
  PUFCC_OTPKEY_28,  // OTP key slot 28, 256 bits
  PUFCC_OTPKEY_29,  // OTP key slot 29, 256 bits
  PUFCC_OTPKEY_30,  // OTP key slot 30, 256 bits
  PUFCC_OTPKEY_31,  // OTP key slot 31, 256 bits
};

// OTP lock types
enum pufcc_otp_lock {
  PUFCC_OTP_NA = 0xF,  // No-Access
  PUFCC_OTP_RO = 0x3,  // Read-Only
  PUFCC_OTP_RW = 0x0,  // Read-Write
};

// PUFcc read/write types
enum pufcc_dma_rw_type { AUTO_INCREMENT, FIXED_RW };

// PUFcc key types
enum pufcc_key_type { PUFCC_SW_KEY, PUFCC_OTP_KEY };

// PUFcc SP38a variants
enum pufcc_sp38a_variant {
  PUFCC_AES128,
  PUFCC_AES192,
  PUFCC_AES256,
  PUFCC_SM4
};

// PUFcc PKC schemes
enum pufcc_pkc_scheme { PUFCC_RSA_2048 = 0x86, PUFCC_ECDSA256 = 0x82 };

// PUFcc SP38a modes
enum pufcc_sp38a_mode {
  PUFCC_ECB_CLR,
  PUFCC_CFB,
  PUFCC_OFB,
  PUFCC_CBC_CLR,
  PUFCC_CBC_CTS1,
  PUFCC_CBC_CTS2,
  PUFCC_CBC_CTS3,
  PUFCC_CTR32,
  PUFCC_CTR64,
  PUFCC_CTR128
};

// Scatter gather DMA descriptor struct
struct pufcc_sg_dma_desc {
  uint32_t read_addr;
  uint32_t write_addr;
  uint32_t length;
  uint32_t next;
  uint32_t dsc_cfg_4;
  uint32_t key_cfg;
  uint32_t cypt_cfg[2];
};

/**
 * @enum  rs_crypto_status
 * @brief Enum for crypto specific status values
 *
 */
enum rs_crypto_status {
  CRYPTO_SUCCESS = 0,
  NULL_POINTER = 1,
  MEM_UNAVAILABLE = 2,
  CRYPTO_INIT_ERROR = 3,
  RSA_VERIFY_ERROR = 4,
  ECDSA_VERIFY_ERROR = 5,
  DECRYPTION_ERROR = 6,
  HASH_CALC_ERROR = 7,
  ECP_CONFIG_ERROR = 8,
  INVALID_RSA_TYPE = 9,
  INVALID_HASH_ALGO = 10,
  INVALID_ENCRYPTION_ALGO = 11,
  INVALID_SIGNING_ALGO = 12,
  INVALID_CIPHER_TYPE = 13,
  BOP_IMAGE_NOT_FOUND = 14,
  INVALID_IMAGE_TYPE = 15,
  KEY_WRITE_ERROR = 16,
  KEY_READ_ERROR = 17,
  KEY_LOCK_ERROR = 18,
  INVALID_PUBLIC_KEY = 19,
  INVALID_ENC_KEY = 20,
  NON_SECURE_IMAGE = 21,
  CRC_CHECK_FAILED = 22,
  INVALID_KEY_SLOT = 23,
  INVALID_KEY_SRC = 24,
  NO_ENC_KEY = 25,
  NO_ACCESS = 26,
  DATA_TRANSFER_ERROR = 27,
  KEY_NOT_IN_MANIFEST = 28,
  CRYPTO_xCB_ERROR = 29,
  INVALID_KEYS = 30,
  NO_ACTIONS = 31,
  INTEGRITY_CHECK_FAILED = 32,
  CBUFFER_READ_ERROR = 33,
  INVALID_TARGET_DEVICE = 34,
  DECOMPRESSION_ERROR = 35,
  INVALID_ACTION_CMD = 36,
  INVALID_CMPR_ALGO = 37,
  CBUFFER_DATA_CONSUMPTION_MISMATCH = 38,
  ALL_ACTIONS_NOT_CONSUMED = 39,
  MALFORMED_ACTION = 40,
  LIFECYCLE_PROGRAMMING_ERROR = 41,
  FLASH_PROGRAM_ERROR = 42,
  FLASH_ERASE_ERROR = 43
};

/**
 * @enum  rs_sign_algo
 * @brief Enum for signing algorithms
 *
 */
enum rs_crypto_sign_algo {
  SIGN_ALGO_NULL = 0,
  SIGN_ALGO_ECDSA256 = 0x10,
  SIGN_ALGO_ECDSA384 = 0x11,
  SIGN_ALGO_RSA2048 = 0x20,
  SIGN_ALGO_BRAINPOOL256 = 0x30,
  SIGN_ALGO_BRAINPOOL384 = 0x31,
  SIGN_ALGO_SM2 = 0x40
};

/**
 * @enum  rs_enc_algo
 * @brief Enum for encryption algorithms
 */
enum rs_crypto_enc_algo {
  ENC_ALGO_NULL = 0,
  ENC_ALGO_AES128_CTR = 0x10,
  ENC_ALGO_AES192_CTR = 0x11,
  ENC_ALGO_AES256_CTR = 0x12,
  ENC_ALGO_AES128_GCM = 0x20,
  ENC_ALGO_AES192_GCM = 0x21,
  ENC_ALGO_AES256_GCM = 0x22
};

/**
 * @enum  rs_crypto_cipher
 * @brief Symmetric cipher types
 */
enum rs_crypto_cipher { AES_CIPHER, SM4_CIPHER, CHACHA_CIPHER };

/**
 * @enum  rs_crypto_rsa_type
 * @brief RSA types
 */
enum rs_crypto_rsa_type { RSA_1024, RSA_2048, RSA_3072, RSA_4096 };

/**
 * @enum  rs_crypto_key_src
 * @brief Key source i.e. software or hardware
 */
enum rs_crypto_key_src {
  RS_SW_KEY,  // Software key in a buffer
  RS_HW_KEY   // Hardware/OTP based key
};

/**
 * @enum  rs_crypto_key_slots
 * @brief RS defined key slots
 */
enum rs_crypto_key_slot {
  RS_FSBL_RSA2048_PUBK_HASH_SLOT,
  RS_FSBL_ECDSA256_PUBK_HASH_SLOT,
  RS_FSBL_AES16_ENC_KEY_SLOT,
  RS_FSBL_AES32_ENC_KEY_SLOT,
  RS_ACPU_RSA2048_PUBK_HASH_SLOT,
  RS_ACPU_ECDSA256_PUBK_HASH_SLOT,
  RS_ACPU_AES16_ENC_KEY_SLOT,
  RS_ACPU_AES32_ENC_KEY_SLOT,
  RS_XCB_RSA2048_PUBK_HASH_SLOT,
  RS_XCB_ECDSA256_PUBK_HASH_SLOT,
  RS_XCB_AES16_ENC_KEY_SLOT,
  RS_XCB_AES32_ENC_KEY_SLOT,
  RS_LIFECYCLE_BITS_SLOT,
  RS_CHIP_ID_SLOT,
  RS_INVALID_KEY_SLOT,
  RS_ALL_KEY_SLOTS  // Used to indicate an operation on all valid key slots
                    // above
};

/**
 * @enum  rs_crypto_hash_type
 * @brief Types of cryptographic hash algorithms
 */
enum rs_crypto_hash_type { SHA256 = 0x10, SHA384 = 0x11, SHA512 = 0x12 };

/**
 * @enum  rs_crypto_tfr_type
 * @brief Types of secure transfer type in case of peripherals
 */
enum rs_crypto_tfr_type {
  RS_SECURE_TX,  // Write to peripheral
  RS_SECURE_RX   // Read from peripheral
};

enum rs_status { OK = 0, WOULD_BLOCK = 1, ERROR = 2 } __attribute__((packed));

typedef void (*rs_dma_callback_t)(void* app_data, int ch, int status);

/*****************************************************************************
 * Register structures
 ****************************************************************************/
// General register structs
struct pufcc_intrpt_reg {
  uint32_t intrpt_st : 1;
  uint32_t reserved1 : 15;
  uint32_t intrpt_en : 1;
  uint32_t reserved2 : 15;
};

struct pufcc_start_reg {
  uint32_t start_p : 1;
  uint32_t reserved : 31;
};

// DMA register structs
struct pufcc_dma_cfg_0_reg {
  uint32_t rng_en : 1;
  uint32_t sg_en : 1;
  uint32_t reserved : 30;
};

struct pufcc_dma_cfg_1_reg {
  uint8_t rbst_max;
  uint8_t wbst_max;
  uint8_t rbst_min;
  uint8_t wbst_min;
};

struct pufcc_dma_dsc_cfg_4_reg {
  uint32_t wprot : 8;
  uint32_t rprot : 8;
  uint32_t fw : 1;
  uint32_t fr : 1;
  uint32_t reserved : 5;
  uint32_t no_cypt : 1;
  uint32_t offset : 4;
  uint32_t dn_pause : 1;
  uint32_t dn_intrpt : 1;
  uint32_t tail : 1;
  uint32_t head : 1;
};

struct pufcc_dma_key_cfg0_reg {
  uint32_t key_src : 4;
  uint32_t key_dst : 4;
  uint32_t key_size : 11;
  uint32_t reserved1 : 5;
  uint32_t key_idx : 5;
  uint32_t reserved2 : 3;
};

// HMAC register structs
struct pufcc_hmac_config_reg {
  uint32_t variant : 4;
  uint32_t reserved : 4;
  uint32_t function : 1;
  uint32_t reserved2 : 23;
};

// PUFcc SP38a register structs
struct pufcc_sp38a_config_reg {
  uint32_t variant : 2;
  uint32_t reserved1 : 2;
  uint32_t mode : 4;
  uint32_t enc_dec : 1;
  uint32_t reserved2 : 23;
};

// PKC register register structs
struct pufcc_pkc_ecp_ec_reg {
  uint32_t reserved1 : 8;
  uint32_t field : 8;  // PKC scheme
  uint32_t h : 4;      // EC cofactor h
  uint32_t reserved2 : 12;
};

// ECC parameters structure
struct pufcc_ecc_param {
  const void *prime;  // Field modulus
  const void *a;      // EC parameter a
  const void *b;      // EC parameter b
  const void *px;     // x-coordinate of base point P
  const void *py;     // y-coordinate of base point P
  const void *order;  // Subgroup order
};

/*****************************************************************************
 * PUFcc register maps
 ****************************************************************************/
// OTP memory map
struct pufcc_otp_mem {
  uint32_t otp[256];
};

struct pufcc_rt_regs {
  volatile uint32_t pif[64];
  uint32_t _pad1[64];
  volatile uint32_t ptr[16];
  volatile uint32_t ptc[16];
  volatile uint32_t ptm[2];
  uint32_t _pad2[6];
  volatile uint32_t rn;
  volatile uint32_t rn_status;
  volatile uint32_t healthcfg;
  volatile uint32_t feature;
  volatile uint32_t interrupt;
  volatile uint32_t otp_psmsk[2];
  volatile uint32_t puf_psmsk;
  volatile uint32_t version;
  volatile uint32_t status;
  volatile uint32_t cfg;
  volatile uint32_t set_pin;
  volatile uint32_t auto_repair;
  volatile uint32_t ini_off_chk;
  volatile uint32_t repair_pgn;
  volatile uint32_t repair_reg;
  volatile uint32_t puf_qty_chk;
  volatile uint32_t puf_enroll;
  volatile uint32_t puf_zeroize;
  volatile uint32_t set_flag;
  volatile uint32_t otp_zeroize;
  uint32_t _pad3[3];
  volatile uint32_t puf[64];
  volatile uint32_t otp[256];
};

// DMA module register map
struct pufcc_dma_regs {
  volatile uint32_t version;
  volatile uint32_t interrupt;
  volatile uint32_t feature;
  uint32_t _pad1;
  volatile uint32_t status_0;
  volatile uint32_t status_1;
  uint32_t _pad2[2];
  volatile uint32_t start;
  volatile uint32_t cfg_0;
  volatile uint32_t cfg_1;
  uint32_t _pad3[2];
  volatile uint32_t dsc_cfg_0;
  volatile uint32_t dsc_cfg_1;
  volatile uint32_t dsc_cfg_2;
  volatile uint32_t dsc_cfg_3;
  volatile uint32_t dsc_cfg_4;
  uint32_t _pad4[2];
  volatile uint32_t dsc_cur_0;
  volatile uint32_t dsc_cur_1;
  volatile uint32_t dsc_cur_2;
  volatile uint32_t dsc_cur_3;
  volatile uint32_t dsc_cur_4;
  uint32_t _pad5[2];
  volatile uint32_t key_cfg_0;
  volatile uint32_t cl_cfg_0;
};

// HMAC module register map
struct pufcc_hmac_regs {
  volatile uint32_t version;
  volatile uint32_t interrupt;
  volatile uint32_t feature;
  uint32_t _pad1;
  volatile uint32_t status;
  uint32_t _pad2;
  volatile uint32_t cfg;
  uint32_t _pad3;
  volatile uint32_t plen;
  uint32_t _pad4[3];
  volatile uint32_t alen;
  uint32_t _pad5[3];
  volatile uint8_t sw_key[PUFCC_HMAC_SW_KEY_MAXLEN];
};

// Crypto module register map
struct pufcc_crypto_regs {
  volatile uint32_t version;
  volatile uint32_t interrupt;
  volatile uint32_t feature;
  uint32_t _pad1[5];
  volatile uint32_t iv_out[PUFCC_CRYPTO_IV_MAXLEN / PUFCC_WORD_SIZE];
  volatile uint32_t iv[PUFCC_CRYPTO_IV_MAXLEN / PUFCC_WORD_SIZE];
  volatile uint32_t sw_key[PUFCC_CRYPTO_SW_KEY_MAXLEN / PUFCC_WORD_SIZE];
  volatile uint32_t dgst_in[PUFCC_CRYPTO_DGST_LEN / PUFCC_WORD_SIZE];
  volatile uint32_t dgst_out[PUFCC_CRYPTO_DGST_LEN / PUFCC_WORD_SIZE];
};

// SP38a module register map
struct pufcc_sp38a_regs {
  volatile uint32_t version;
  volatile uint32_t interrupt;
  volatile uint32_t feature;
  uint32_t _pad1;
  volatile uint32_t status;
  uint32_t _pad2;
  volatile uint32_t cfg;
};

// PKC module register map
struct pufcc_pkc_regs {
  volatile uint32_t version;
  volatile uint32_t interrupt;
  volatile uint32_t start;
  volatile uint32_t status;
  volatile uint32_t ecp_err_code;
  volatile uint32_t ecp_err_pc;
  volatile uint32_t ecp_err_cmd;
  volatile uint32_t mp_version;
  uint32_t _pad1[56];
  volatile uint32_t ecp_ec;
  volatile uint32_t ecp_keysel;
  volatile uint32_t ecp_otpkba;
  volatile uint32_t ecp_key_usage;
  volatile uint32_t ecp_e_short;
  uint32_t _pad2[55];
  volatile uint32_t ecp_mac[4];
  volatile uint32_t ecp_data[512];
};

struct pufcc_dma_dev {
  struct pufcc_dma_regs *regs;
  bool is_dev_free;
  struct pufcc_sg_dma_desc *dma_descs;
  uint32_t num_descriptors;
  void *callback_args;
  rs_dma_callback_t callback;
};

/**
 * @struct rs_crypto_addr
 * @brief  Address info for cryptographic operations
 */
struct rs_crypto_addr {
  uint32_t read_addr;
  uint32_t write_addr;
  uint32_t len;
  enum rs_crypto_tfr_type tfr_type;  // Transfer type (read or write) in case of
                                     // peripherals, otherwise don't care
  bool periph_rw;  // Indicates if data transfer involves a peripheral
  struct rs_crypto_addr *next;  // In case data lies at multiple locations
};

/**
 * @struct rs_crypto_key
 * @brief  Generic cryptographic key structure
 */
struct rs_crypto_key {
  enum rs_crypto_key_src key_src;
  uint8_t *key_addr;                 // For software key in a buffer
  enum rs_crypto_key_slot key_slot;  // For HW/OTP/eFuse based key
  uint32_t key_len;
  uint8_t *iv;
  uint32_t iv_len;
  bool readback_iv;  // If true, read back updated IV value corresponding to
                     // number of processed data blocks during decryption into
                     // the iv buffer above. In this case iv buffer must be
                     // writable.
};

/**
 * @struct rs_crypto_rsa2048_puk
 * @brief  RSA 2048 public key structure
 */
struct rs_crypto_rsa2048_puk {
  uint8_t n[RS_RSA_2048_LEN];  // Modulus
  uint32_t e;                  // Exponent
};

/**
 * @struct rs_crypto_ec256_puk
 * @brief  ECDSA256 public key
 */
struct rs_crypto_ec256_puk {
  uint8_t x[RS_EC256_QLEN];
  uint8_t y[RS_EC256_QLEN];
};

/**
 * @struct rs_crypto_ec256_sig
 * @brief  ECDSA256 signature
 */
struct rs_crypto_ec256_sig {
  uint8_t r[RS_EC256_QLEN];
  uint8_t s[RS_EC256_QLEN];
};

/**
 * @struct rs_crypto_hash
 * @brief  Hash structure
 */
struct rs_crypto_hash {
  uint8_t val[RS_SHA_MAX_LEN];
  uint32_t len;
};

/**
 * @enum  rs_crypto_key_manifest
 * @brief Key manifest structure
 */
struct rs_crypto_key_manifest {
  uint32_t key_mask;  // Key availability bitmask according to
                      // 'rs_crypto_key_slot' enum above
  uint8_t fsbl_rsa2048_pubk_hash[32];
  uint8_t fsbl_ecdsa256_pubk_hash[32];
  uint8_t fsbl_aes128_enc_key[16];
  uint8_t fsbl_aes256_enc_key[32];
  uint8_t acpu_rsa2048_pubk_hash[32];
  uint8_t acpu_ecdsa256_pubk_hash[32];
  uint8_t acpu_aes128_enc_key[16];
  uint8_t acpu_aes256_enc_key[32];
  uint8_t xcb_rsa2048_pubk_hash[32];
  uint8_t xcb_ecdsa256_pubk_hash[32];
  uint8_t xcb_aes128_enc_key[16];
  uint8_t xcb_aes256_enc_key[32];
};

enum rs_dma_addr_adjust {
  RS_DMA_ADDR_ADJUST_INCREMENT = 0x0,
  RS_DMA_ADDR_ADJUST_DECREMENT = 0x1,
  RS_DMA_ADDR_ADJUST_FIXED = 0x2,
};

struct rs_dma_block_config {
  uintptr_t src_addr;
  uintptr_t dst_addr;
  uint32_t block_size;
  struct rs_dma_block_config* next_block;
  enum rs_dma_addr_adjust src_addr_adjust;
  enum rs_dma_addr_adjust dst_addr_adjust;
};

struct rs_dma_config {
  // peripheral combination. Platform dependent
  uint8_t dma_slot __attribute__((aligned(4)));
  // What type of DMA transfer this is
  uint8_t xfer_dir __attribute__((aligned(4)));
  // enable callback on transfer complete
  uint8_t complete_callback_en __attribute__((aligned(4)));
  // enable callback on transfer error
  uint8_t err_callback_en __attribute__((aligned(4)));
  // select software or hardware handshaking on the source
  uint8_t src_handshake_mode __attribute__((aligned(4)));
  // select software or hardware handshaking on the dest
  uint8_t dst_handshake_mode __attribute__((aligned(4)));
  // priority of the transfer
  uint8_t xfer_priority __attribute__((aligned(4)));
  // width of src data in bytes
  uint16_t src_data_size __attribute__((aligned(4)));
  // width of dst data in bytes
  uint16_t dst_data_size __attribute__((aligned(4)));
  // length of data to be retrieved in each src transfer step
  uint16_t src_burst_len __attribute__((aligned(4)));
  // length of data to be written in each dest transfer step
  uint16_t dst_burst_len __attribute__((aligned(4)));
  // total amount of transfer blocks to be used
  uint32_t block_count __attribute__((aligned(4)));
  // first transfer descriptor
  struct rs_dma_block_config* head_block __attribute__((aligned(4)));
  // arguments to be passed as app_data to the dma callback
  void* callback_args __attribute__((aligned(4)));
  rs_dma_callback_t callback __attribute__((aligned(4)));
} __attribute__((aligned(4)));

/*
The following macro can be used to write register values.
REG_WRITE_32(Destination_Addr, Source_Addr)
*/
#define REG_WRITE_32(Destination_Addr, Source_Addr) \
  *(uint32_t *)Destination_Addr = *(uint32_t *)Source_Addr;

#endif /* ZEPHYR_DRIVERS_CRYPTO_CRYPTO_STM32_PRIV_H_ */
