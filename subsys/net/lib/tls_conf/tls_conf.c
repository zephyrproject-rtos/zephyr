/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <sys/types.h>
#include <stdbool.h>
#include <errno.h>

#if !defined(CONFIG_MBEDTLS_CFG_FILE)
#include "mbedtls/config.h"
#else
#include CONFIG_MBEDTLS_CFG_FILE
#endif /* CONFIG_MBEDTLS_CFG_FILE */
#include "mbedtls/platform.h"

#include <mbedtls/net.h>
#include <mbedtls/ssl.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/pk.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/debug.h>

#include <net/tls_conf.h>

#ifdef __ZEPHYR__
#include <kernel.h>
#include <device.h>
#include <entropy.h>

#define SYS_LOG_LEVEL 4
#include <logging/sys_log.h>

#else /* __ZEPHYR__ */

#define SYS_LOG_ERR(msg, ...) printf(msg "\n", ##__VA_ARGS__)
#define printk printf

#endif /* __ZEPHYR__ */

mbedtls_ssl_config ztls_default_tls_client_conf;
mbedtls_ssl_config ztls_default_tls_server_conf;

/*static mbedtls_entropy_context ztls_entropy;*/
static mbedtls_ctr_drbg_context ztls_ctr_drbg;

#ifdef __ZEPHYR__

#ifdef CONFIG_ENTROPY_HAS_DRIVER
static int ztls_entropy_func(void *data, unsigned char *output, size_t len)
{
	int res = entropy_get_entropy(data, output, len);
	return res;
}
#else
/* No real entropy driver, use pseudo-random number generator (potentially
 * insecure).
 */
static int ztls_entropy_func(void *data, unsigned char *output, size_t len)
{
	size_t i = len / 4;
	u32_t v;

	while (i--) {
		v = sys_rand32_get();
		UNALIGNED_PUT(v, (u32_t *)output);
		output += 4;
	}

	i = len & 3;
	v = sys_rand32_get();
	while (i--) {
		*output++ = v;
		v >>= 8;
	}

	return 0;
}
#endif

static int ztls_mbedtls_ctr_drbg_random(void *p_rng, unsigned char *output,
					size_t output_len)
{
	static K_MUTEX_DEFINE(mutex);
	int res;

	/* Avoid connection lockups due to no entropy, error out instead */
	res = k_mutex_lock(&mutex, K_SECONDS(1));
	if (res < 0) {
		return res;
	}

	res = mbedtls_ctr_drbg_random(p_rng, output, output_len);
	k_mutex_unlock(&mutex);

	return res;
}

#else /* __ZEPHYR__ */

static int null_entropy_func(void *data, unsigned char *output, size_t len)
{
	/* Warning: this is test-only implementation for non-Zephyr
	 * environments.
	 */
	(void)data;
	(void)output;
	(void)len;
	return 0;
}

#define ztls_mbedtls_ctr_drbg_random mbedtls_ctr_drbg_random

#endif

#ifdef MBEDTLS_DEBUG_C
static void ztls_mbedtls_debug(void *ctx, int level, const char *file,
			       int line, const char *str)
{
	(void)ctx;
	printk("MBEDTLS%d:%s:%04d: %s\n", level, file, line, str);
}
#endif

#define ztls_system_is_inited() (ztls_ctr_drbg.f_entropy != NULL)


static int ztls_system_init(void)
{
	int ret;
	/* TODO: Should use something device-specific, like MAC address */
	static const unsigned char drbg_seed[] = "zephyr";
	struct device *dev = NULL;

#if defined(__ZEPHYR__) && defined(CONFIG_ENTROPY_HAS_DRIVER)
	dev = device_get_binding(CONFIG_ENTROPY_NAME);

	if (!dev) {
		SYS_LOG_ERR("can't get entropy device");
		return -ENODEV;
	}
#else
	printk("*** WARNING: This system lacks entropy driver, "
	       "TLS communication may be INSECURE! ***\n\n");
#endif

	/* We don't use mbedTLS entropy pool as of now. */
	/* mbedtls_entropy_init(&p->entropy); */

	mbedtls_ctr_drbg_init(&ztls_ctr_drbg);
#ifdef __ZEPHYR__
	ret = mbedtls_ctr_drbg_seed(&ztls_ctr_drbg, ztls_entropy_func, dev,
				    drbg_seed, sizeof(drbg_seed));
#else
	ret = mbedtls_ctr_drbg_seed(&ztls_ctr_drbg, null_entropy_func, NULL,
				    drbg_seed, sizeof(drbg_seed));
#endif
	if (ret != 0) {
		mbedtls_ctr_drbg_free(&ztls_ctr_drbg);
		return ret;
	}

	return 0;
}

static int ztls_init_tls_conf(mbedtls_ssl_config *conf, int client_or_serv)
{
	int ret;

	if (!ztls_system_is_inited()) {
		ret = ztls_system_init();
		if (ret < 0) {
			return ret;
		}
	}

	mbedtls_ssl_config_init(conf);

	ret = mbedtls_ssl_config_defaults(conf, client_or_serv,
					  MBEDTLS_SSL_TRANSPORT_STREAM,
					  MBEDTLS_SSL_PRESET_DEFAULT);
	if (ret != 0) {
		SYS_LOG_ERR("mbedtls_ssl_config_defaults returned -0x%x",
			    -ret);
		goto cleanup_conf;
	}

	mbedtls_ssl_conf_rng(conf, ztls_mbedtls_ctr_drbg_random,
			     &ztls_ctr_drbg);

	#if 0
	/* These are defaults in mbedTLS */
	mbedtls_ssl_conf_authmode(&ztls_default_tls_client_conf,
				  MBEDTLS_SSL_VERIFY_REQUIRED);
	mbedtls_ssl_conf_authmode(&ztls_default_tls_server_conf,
				  MBEDTLS_SSL_VERIFY_NONE);
	#endif

#ifdef MBEDTLS_DEBUG_C
	mbedtls_ssl_conf_dbg(conf, ztls_mbedtls_debug, NULL);
#ifdef CONFIG_MBEDTLS_DEBUG_LEVEL
	/* 3 - info, 4 - debug */
	mbedtls_debug_set_threshold(CONFIG_MBEDTLS_DEBUG_LEVEL);
#endif
#endif
	return 0;

cleanup_conf:
	mbedtls_ssl_config_free(conf);

	return ret;

}

static int ztls_get_tls_conf_helper(mbedtls_ssl_config **out_conf,
				    mbedtls_ssl_config *in_conf,
				    int client_or_serv)
{
	int ret;

	*out_conf = NULL;
	ret = ztls_init_tls_conf(in_conf, client_or_serv);
	if (ret < 0) {
		return ret;
	}

	*out_conf = in_conf;
	return 0;
}

int ztls_get_tls_client_conf(mbedtls_ssl_config **out_conf)
{
	return ztls_get_tls_conf_helper(out_conf,
					&ztls_default_tls_client_conf,
					MBEDTLS_SSL_IS_CLIENT);
}

int ztls_get_tls_server_conf(mbedtls_ssl_config **out_conf)
{
	return ztls_get_tls_conf_helper(out_conf,
					&ztls_default_tls_server_conf,
					MBEDTLS_SSL_IS_SERVER);
}

#ifdef MBEDTLS_X509_CRT_PARSE_C
int ztls_parse_cert_key_pair(struct ztls_cert_key_pair *pair,
			     const unsigned char *cert, size_t cert_len,
			     const unsigned char *priv_key,
			     size_t priv_key_len)
{
	int ret;

	mbedtls_x509_crt_init(&pair->cert);
	mbedtls_pk_init(&pair->priv_key);

	ret = mbedtls_x509_crt_parse(&pair->cert, cert, cert_len);
	if (ret != 0) {
		SYS_LOG_ERR("mbedtls_x509_crt_parse returned -0x%x", -ret);
		goto error;
	}

	ret = mbedtls_pk_parse_key(&pair->priv_key, priv_key,
				   priv_key_len, NULL, 0);
	if (ret != 0) {
		SYS_LOG_ERR("mbedtls_pk_parse_key returned -0x%x", -ret);
		goto error;
	}

	return 0;

error:
	mbedtls_x509_crt_free(&pair->cert);
	mbedtls_pk_free(&pair->priv_key);

	return ret;
}

int ztls_conf_add_own_cert_key_pair(mbedtls_ssl_config *conf,
				    struct ztls_cert_key_pair *pair)
{
	int ret = mbedtls_ssl_conf_own_cert(conf, &pair->cert,
					    &pair->priv_key);
	if (ret != 0) {
		SYS_LOG_ERR("mbedtls_ssl_conf_own_cert returned -0x%x", -ret);
	}

	return ret;
}
#endif /* MBEDTLS_X509_CRT_PARSE_C */
