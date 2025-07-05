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
                     CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS | \
                     CAP_NO_IV_PREFIX | CAP_NO_ENCRYPTION | CAP_NO_SIGNING)

static char *session_to_str(enum pufs_session_type inSession) {
  return ((inSession ==  PUFS_SESSION_HASH_CALCULATION)?"Hash":\
            ((inSession ==  PUFS_SESSION_DECRYPTION)?"Decryption":\
            ((inSession ==  PUFS_SESSION_SIGN_VERIFICATION)?"Sign_Verification":"Unknown")));
}                                      

uint8_t __pufcc_descriptors[BUFFER_SIZE];

static int pufs_query_hw_caps(const struct device *dev);

static int fill_rs_crypto_hash_addr(struct hash_pkt *pkt, struct rs_crypto_addr *data_addr) {
  uint8_t desc_count = 0;

  if((pkt == NULL) || (data_addr == NULL)) {
    LOG_ERR("%s(%d) NULL pointer(s)\n", __func__, __LINE__);
    return -EBADFD;
  }

  // Set SGDMA descriptors input data
  while(true) {

    data_addr[desc_count].read_addr = (uint32_t)pkt->in_buf;
    data_addr[desc_count].len = pkt->in_len;
    data_addr[desc_count].periph_rw = false;
    
    if((desc_count * sizeof(struct pufcc_sg_dma_desc)) < BUFFER_SIZE){            
      pkt = pkt->next;
    } else {
      break;
    }    
    if(pkt != NULL) {
      data_addr[desc_count].next = &data_addr[desc_count+1];
    } else {
      data_addr[desc_count].next = NULL;
      break;
    }
    desc_count++;
  }

  if (pkt) {
    // No enough descriptors available
    LOG_ERR("%s(%d) Not More Than %d Elements Allowed. Count:%d\n", \
    __func__, __LINE__, (BUFFER_SIZE/sizeof(struct pufcc_sg_dma_desc)), desc_count+1);
    return -ENOTEMPTY;
  }

  return PUFCC_SUCCESS;
}

static int fill_rs_crypto_sign_addr(struct sign_pkt *pkt, struct rs_crypto_addr *data_addr) {

  uint8_t desc_count = 0;

  if((pkt == NULL) || (data_addr == NULL)) {
    LOG_ERR("%s(%d) NULL pointer(s)\n", __func__, __LINE__);
    return -EBADFD;
  }

  while(true) {

    data_addr[desc_count].read_addr = (uint32_t)pkt->in_buf;
    data_addr[desc_count].len = pkt->in_len;
    
    if((desc_count * sizeof(struct pufcc_sg_dma_desc)) < BUFFER_SIZE){            
      pkt = pkt->next;
    } else {
      break;
    }    
    if(pkt != NULL) {
      data_addr[desc_count].next = &data_addr[desc_count+1];
    } else {
      data_addr[desc_count].next = NULL;
      break;
    }
    desc_count++;
  }

  if (pkt) {
    // No enough descriptors available
    LOG_ERR("%s(%d) Not More Than %d Elements Allowed. Count:%d\n", \
    __func__, __LINE__, (BUFFER_SIZE/sizeof(struct pufcc_sg_dma_desc)), desc_count+1);
    return -ENOTEMPTY;
  }

  return PUFCC_SUCCESS;
}

static int crypto_pufs_init(const struct device *dev) {

  // Initialize base addresses of different PUFcc modules
  enum pufcc_status lvStatus = pufcc_init(((struct pufs_config*)dev->config)->base);

  if(lvStatus == PUFCC_SUCCESS) {
    // Connect the IRQ
    ((struct pufs_config*)dev->config)->irq_init();
  } else {
    return -ENODEV;
  }

  return PUFCC_SUCCESS;
}

static void pufs_irq_handler(const struct device *dev) {

  // Clear and disable the interrupts from the pufcc register map
  int status = pufcc_clear_and_disable_intr();

  struct pufs_data *lvPufsData = (struct pufs_data*)dev->data;

  if((pufs_query_hw_caps(dev) & CAP_ASYNC_OPS) == CAP_ASYNC_OPS)
  {
    switch(lvPufsData->pufs_session_type) {
      case(PUFS_SESSION_SIGN_VERIFICATION): {
        if(lvPufsData->pufs_session_callback.sign_cb != NULL) {
          lvPufsData->pufs_session_callback.sign_cb(lvPufsData->pufs_pkt.sign_pkt, status);
        }
      } break;
      case(PUFS_SESSION_HASH_CALCULATION): {
        if(lvPufsData->pufs_session_callback.hash_cb != NULL) {
          lvPufsData->pufs_session_callback.hash_cb(lvPufsData->pufs_pkt.hash_pkt, status);
        }
      } break;
      case(PUFS_SESSION_DECRYPTION): {
        if(lvPufsData->pufs_session_callback.cipher_cb != NULL) {
          lvPufsData->pufs_session_callback.cipher_cb(lvPufsData->pufs_pkt.cipher_pkt, status);
        }
      } break;
      case(PUFS_SESSION_UNDEFINED): {
        LOG_ERR("%s(%d) Unsupported Session %d\n", __func__, __LINE__, lvPufsData->pufs_session_type);
      } break;
    }
  }

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
  enum pufcc_status lvStatus = PUFCC_SUCCESS;
  enum pufcc_dma_rw_type write_type;

  ((struct pufs_data*)ctx->device->data)->pufs_pkt.cipher_pkt = pkt;
  ((struct pufs_data*)ctx->device->data)->pufs_ctx.cipher_ctx = ctx;

  if(pkt->auto_increment) { 
    write_type = AUTO_INCREMENT;
  } else {
    write_type = FIXED_RW;
  }
  
  lvStatus = pufcc_decrypt_aes(
                                (uint32_t)pkt->out_buf, (uint32_t)pkt->in_buf,
                                (uint32_t)pkt->in_len, pkt->prev_len, ctx->key_source,
                                (uint32_t)ctx->key.bit_stream, ctx->keylen, (uint32_t)ctr,
                                (uint32_t)ctx->mode_params.ctr_info.ctr_len, 
                                write_type, ctx->mode_params.ctr_info.readback_ctr
                              );
  
  if(lvStatus != PUFCC_SUCCESS) {
    LOG_ERR("%s(%d) PUFcc Error Code:%d\n", __func__, __LINE__, lvStatus);
    return -ECANCELED;
  }

  return 0;
}

static int pufs_cipher_begin_session(const struct device *dev, struct cipher_ctx *ctx,
                              enum cipher_algo algo, enum cipher_mode mode,
                              enum cipher_op op_type)
{
  ctx->device = dev;

  ((struct pufs_data *)dev->data)->pufs_ctx.cipher_ctx = ctx;

  struct pufs_data *lvPufsData = (struct pufs_data*)dev->data;

  uint16_t lvHashFlagsMask = (
                            CAP_NO_ENCRYPTION | CAP_SYNC_OPS | 
                            CAP_NO_IV_PREFIX |
                            CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS
                         );
  
  if(algo != CRYPTO_CIPHER_ALGO_AES) {
    LOG_ERR("%s(%d) UnSupported Algo. Only AES Supported\n", __func__, __LINE__);
    return -ENOTSUP;
  }

  if(mode != CRYPTO_CIPHER_MODE_CTR) {
    LOG_ERR("%s(%d) UnSupported Algo. Only CTR Mode Supported\n", __func__, __LINE__);
    return -ENOTSUP;
  } else {
    ctx->ops.ctr_crypt_hndlr = pufs_ctr_op;
  }

  if(op_type != CRYPTO_CIPHER_OP_DECRYPT) {
    LOG_ERR("%s(%d) UnSupported Operation. Only Decryption Supported\n", __func__, __LINE__);
    return -ENOTSUP;    
  }

  if(ctx->flags & (~lvHashFlagsMask)) {
    LOG_ERR("%s(%d) UnSupported Flags. Supported Flags_Mask:%d\n", __func__, __LINE__, lvHashFlagsMask);
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
    
    struct rs_crypto_addr data_addr = {
      .read_addr = (uint32_t)pkt->in_buf,
      .write_addr = (uint32_t)pkt->out_buf,
      .len = pkt->in_len      
    };
    
    struct rs_crypto_hash hash_out = {0};
        
    lvStatus = pufcc_calc_sha256_hash(&data_addr, &hash_out);

    /* Copy output hash values calculated by PUFcc */
    memcpy((void*)pkt->out_buf, (void*)hash_out.val, hash_out.len);
    pkt->out_len = hash_out.len;

  } else {

    struct rs_crypto_addr lvHash_Data_Addr[(BUFFER_SIZE/sizeof(struct rs_crypto_addr))] = {0};
    struct hash_pkt *lvPkt = &pkt[0];
    int error = fill_rs_crypto_hash_addr(pkt, lvHash_Data_Addr);
    
    if(error != PUFCC_SUCCESS) {      
      return error;
    }

    struct rs_crypto_hash hash_in = {0};

    hash_in.len = lvPkt->in_hash_len;

    if(lvPkt->in_hash != NULL) {
      /* Copy input hash values for operating upon */
      memcpy((void*)hash_in.val, (void*)lvPkt->in_hash, hash_in.len);
    }

    struct rs_crypto_hash hash_out = {0};

    lvStatus = pufcc_calc_sha256_hash_sg(
                                          (struct rs_crypto_addr*)lvHash_Data_Addr, 
                                          lvPkt->head, 
                                          lvPkt->tail, 
                                          lvPkt->prev_len,
                                          &hash_in,
                                          &hash_out
                                        );

    if(lvStatus == PUFCC_SUCCESS) {
      /* Copy output hash values calculated by PUFcc */
      memcpy((void*)lvPkt->out_buf, (void*)hash_out.val, (size_t)hash_out.len);
      lvPkt->out_len = hash_out.len; 
    }                                       
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

  uint16_t lvHashFlagsMask = (CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS);
  
  if(algo != CRYPTO_HASH_ALGO_SHA256) {
    LOG_ERR("%s(%d) UnSupported Hash Algo. Only SHA256 Supported\n", __func__, __LINE__);
    return -ENOTSUP;
  }

  if((ctx->flags & (~lvHashFlagsMask))) {
    LOG_ERR("%s(%d) UnSupported Flags. Supported Flags_Mask:%d\n", __func__, __LINE__, lvHashFlagsMask);
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

///////////////////////////////////////////////////////////////
int pufs_sign_rsa_op(struct sign_ctx *ctx, struct sign_pkt *pkt)
{
  enum pufcc_status lvStatus = PUFCC_SUCCESS;

  int error = 0;

  ((struct pufs_data*)ctx->device->data)->pufs_pkt.sign_pkt = pkt;
  ((struct pufs_data*)ctx->device->data)->pufs_ctx.sign_ctx = ctx; 

  struct rs_crypto_addr msg_addr[(BUFFER_SIZE/sizeof(struct rs_crypto_addr))] = {0};
  
  error = fill_rs_crypto_sign_addr(pkt, msg_addr);

  if(error != PUFCC_SUCCESS) {
    return error;
  }

  lvStatus = pufcc_rsa2048_sign_verify(
                                        (uint8_t *)ctx->sig, 
                                        msg_addr,
                                        (struct rs_crypto_rsa2048_puk *)ctx->pub_key
                                      );

  if(lvStatus != PUFCC_SUCCESS) {
    LOG_ERR("%s(%d) PUFs Error Code:%d\n", __func__, __LINE__, lvStatus);
    return -ECANCELED;
  }

  return PUFCC_SUCCESS;
}

int pufs_sign_ecdsa_op(struct sign_ctx *ctx, struct sign_pkt *pkt)
{
  int error = 0;
  enum pufcc_status lvStatus = PUFCC_SUCCESS;

  ((struct pufs_data*)ctx->device->data)->pufs_pkt.sign_pkt = pkt;
  ((struct pufs_data*)ctx->device->data)->pufs_ctx.sign_ctx = ctx;

  struct rs_crypto_addr msg_addr[(BUFFER_SIZE/sizeof(struct rs_crypto_addr))] = {0};
  
  error = fill_rs_crypto_sign_addr(pkt, msg_addr);

  if(error != PUFCC_SUCCESS) {
    return error;
  }
  
  lvStatus = pufcc_ecdsa256_sign_verify(
                                         (struct rs_crypto_ec256_sig *)ctx->sig, 
                                         msg_addr, 
                                         (struct rs_crypto_ec256_puk *)ctx->pub_key
                                       );

  if(lvStatus != PUFCC_SUCCESS) {
    LOG_ERR("%s(%d) PUFs Error Code:%d\n", __func__, __LINE__, lvStatus);
    return -ECANCELED;
  }

  return PUFCC_SUCCESS;
}

/* Setup a signature session */
static int pufs_sign_begin_session(const struct device *dev, struct sign_ctx *ctx,
                                   enum sign_algo algo)
{
  ctx->device = dev;

  ((struct pufs_data *)dev->data)->pufs_ctx.sign_ctx = ctx;

  struct pufs_data *lvPufsData = (struct pufs_data*)dev->data;

  uint16_t lvHashFlagsMask = (CAP_INPLACE_OPS | CAP_SYNC_OPS);
  
  if((algo != CRYPTO_SIGN_ALGO_ECDSA256) && (algo != CRYPTO_SIGN_ALGO_RSA2048)) {
    LOG_ERR("%s(%d) Unupported Algo:%d. Supported Algo <ECDSA256, RSA2048>\n", __func__, __LINE__, algo);
    return -ENOTSUP;
  } else {
    if(algo == CRYPTO_SIGN_ALGO_ECDSA256) {
      ctx->ops.signing_algo = CRYPTO_SIGN_ALGO_ECDSA256;
      ctx->ops.ecdsa_crypt_hndlr = pufs_sign_ecdsa_op;
    } else {
      ctx->ops.signing_algo = CRYPTO_SIGN_ALGO_RSA2048;
      ctx->ops.rsa_crypt_hndlr = pufs_sign_rsa_op;
    }
  }

  if((ctx->flags & (~lvHashFlagsMask))) {
    LOG_ERR("%s(%d) UnSupported Flags. Supported Flags_Mask:%d\n", __func__, __LINE__, lvHashFlagsMask);
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
  .irq_num = DT_INST_IRQN(0)
};

DEVICE_DT_INST_DEFINE(0, crypto_pufs_init, NULL,
		    &s_pufs_session_data,
		    &s_pufs_configuration, POST_KERNEL,
		    CONFIG_CRYPTO_INIT_PRIORITY, (void *)&s_crypto_funcs);
