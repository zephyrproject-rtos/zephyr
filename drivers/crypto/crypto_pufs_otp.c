/*
 * Copyright (c) 2024 Rapid Silicon
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "pufcc.h"
#include "crypto_otp_mem.h"

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/types.h>

#define LOG_LEVEL CONFIG_CRYPTO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(crypto_puf_security_otp);

#if DT_HAS_COMPAT_STATUS_OKAY(pufsecurity_otp)
  #define DT_DRV_COMPAT pufsecurity_otp
#else
  #error No PUF Security HW Crypto OTP in device tree
#endif

#define OTP_HW_CAP (CAP_READ_OTP | CAP_WRITE_OTP | \
                     CAP_LOCK_OTP | CAP_ZEROIZ_OTP | CAP_SYNC_OPS)

/* Device constant configuration parameters */
struct pufs_otp_config {
	uint32_t base;
};

/**
 * Query the driver capabilities
 * Not all the Modules of PUFs support all flags.
 * Please Check individual cipher/hash/sign_begin_session
 * interfaces to get the information of supported flags. 
 * */ 
static int crypto_pufs_otp_query_hw_caps(const struct device *dev)
{
  return OTP_HW_CAP;
}

static int crypto_pufs_otp_init(const struct device *dev) {

  // Initialize base addresses of different PUFcc modules
  enum pufcc_status lvStatus = pufcc_init(((struct pufs_otp_config*)dev->config)->base);

  return PUFCC_SUCCESS;
}

static struct otp_driver_api s_crypto_otp_funcs = {
  .otp_hw_caps = crypto_pufs_otp_query_hw_caps,
  .otp_get_lock = NULL,
  .otp_info = NULL,
  .otp_read = NULL,
  .otp_set_lock = NULL,
  .otp_write = NULL,
  .otp_zeroize = NULL
};

static const struct pufs_otp_config s_pufs_otp_configuration = {
  .base = DT_REG_ADDR(DT_PARENT(DT_DRV_INST(0))),
};

DEVICE_DT_INST_DEFINE(0, crypto_pufs_otp_init, NULL,
		    NULL,
		    &s_pufs_otp_configuration, POST_KERNEL,
		    CONFIG_CRYPTO_INIT_PRIORITY, (void *)&s_crypto_otp_funcs);
