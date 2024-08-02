/*
 *  Benchmark demonstration program
 *
 *  Copyright (C) 2006-2016, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */

#if !defined(MBEDTLS_ALLOW_PRIVATE_ACCESS)
#define MBEDTLS_ALLOW_PRIVATE_ACCESS
#endif	/* MBEDTLS_ALLOW_PRIVATE_ACCESS */

#if !defined(CONFIG_MBEDTLS_CFG_FILE)
#include "mbedtls/config.h"
#else
#include CONFIG_MBEDTLS_CFG_FILE
#endif /* CONFIG_MBEDTLS_CFG_FILE */

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdio.h>
#define mbedtls_exit       exit
#define mbedtls_printf     printf
#define mbedtls_snprintf   snprintf
#define mbedtls_free       free
#endif

#include <string.h>
#include <stdlib.h>

#include "mbedtls/ssl.h"
#include "mbedtls/debug.h"
#include "mbedtls/timing.h"
#include "mbedtls/md5.h"
#include "mbedtls/ripemd160.h"
#include "mbedtls/sha1.h"
#include "mbedtls/sha256.h"
#include "mbedtls/sha512.h"
#include "mbedtls/des.h"
#include "mbedtls/aes.h"
#include "mbedtls/aria.h"
#include "mbedtls/camellia.h"
#include "mbedtls/chacha20.h"
#include "mbedtls/gcm.h"
#include "mbedtls/ccm.h"
#include "mbedtls/chachapoly.h"
#include "mbedtls/cmac.h"
#include "mbedtls/poly1305.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/hmac_drbg.h"
#include "mbedtls/rsa.h"
#include "mbedtls/dhm.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/ecdh.h"
#include "mbedtls/error.h"

#include <zephyr/types.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/random/random.h>

#include <zephyr/kernel.h>

#include <zephyr/sys/printk.h>
#define  MBEDTLS_PRINT ((int(*)(const char *, ...)) printk)

static void my_debug(void *ctx, int level,
		     const char *file, int line, const char *str)
{
	const char *p, *basename;
	int len;

	ARG_UNUSED(ctx);

	/* Extract basename from file */
	for (p = basename = file; *p != '\0'; p++) {
		if (*p == '/' || *p == '\\') {
			basename = p + 1;
		}
	}

	/* Avoid printing double newlines */
	len = strlen(str);
	if (str[len - 1] == '\n') {
		((char *)str)[len - 1] = '\0';
	}

	mbedtls_printf("%s:%04d: |%d| %s\n", basename, line, level, str);
}

/* mbedtls in Zephyr doesn't have timing.c implemented. So for sample
 * purpose implementing necessary functions.
 */

volatile int mbedtls_timing_alarmed;

static struct k_work_delayable mbedtls_alarm;
static void mbedtls_alarm_timeout(struct k_work *work);

/* Work synchronization objects must be in cache-coherent memory,
 * which excludes stacks on some architectures.
 */
static struct k_work_sync work_sync;

static void mbedtls_alarm_timeout(struct k_work *work)
{
	mbedtls_timing_alarmed = 1;
}

void mbedtls_set_alarm(int seconds)
{
	mbedtls_timing_alarmed = 0;

	k_work_schedule(&mbedtls_alarm, K_SECONDS(seconds));
}

/*
 * For heap usage estimates, we need an estimate of the overhead per allocated
 * block. ptmalloc2/3 (used in gnu libc for instance) uses 2 size_t per block,
 * so use that as our baseline.
 */
#define MEM_BLOCK_OVERHEAD  (2 * sizeof(size_t))

#define BUFSIZE         1024
#define HEADER_FORMAT   "  %-24s :  "
#define TITLE_LEN       25

#define OPTIONS                                                    \
	"md5, ripemd160, sha1, sha256, sha512,\n"             \
	"des3, des, camellia, chacha20,\n"         \
	"aes_cbc, aes_gcm, aes_ccm, aes_ctx, chachapoly,\n"        \
	"aes_cmac, des3_cmac, poly1305,\n"                         \
	"havege, ctr_drbg, hmac_drbg,\n"                           \
	"rsa, dhm, ecdsa, ecdh.\n"

#if defined(MBEDTLS_ERROR_C)
#define PRINT_ERROR {                                            \
	mbedtls_strerror(ret, (char *)tmp, sizeof(tmp));         \
	mbedtls_printf("FAILED: %s\n", tmp);                     \
	}
#else
#define PRINT_ERROR {                                            \
	mbedtls_printf("FAILED: -0x%04x\n", -ret);               \
	}
#endif

#define TIME_AND_TSC(TITLE, CODE)                                     \
do {                                                                  \
	unsigned long ii, jj;                                         \
	uint32_t tsc;                                                    \
	uint64_t delta;                                                  \
	int ret = 0;                                                  \
								      \
	mbedtls_printf(HEADER_FORMAT, TITLE);                         \
								      \
	mbedtls_set_alarm(1);                                         \
	for (ii = 1; ret == 0 && !mbedtls_timing_alarmed; ii++) {     \
		ret = CODE;                                           \
	}                                                             \
								      \
	tsc = k_cycle_get_32();                                       \
	for (jj = 0; ret == 0 && jj < 1024; jj++) {                   \
		ret = CODE;                                           \
	}                                                             \
								      \
	if (ret != 0) {                                               \
		PRINT_ERROR;                                          \
	}                                                             \
								      \
	delta = k_cycle_get_32() - tsc;                               \
	delta = k_cyc_to_ns_floor64(delta);                   \
								      \
	(void)k_work_cancel_delayable_sync(&mbedtls_alarm, &work_sync);\
								      \
	mbedtls_printf("%9lu KiB/s,  %9lu ns/byte\n",                 \
		       ii * BUFSIZE / 1024,                           \
		       (unsigned long)(delta / (jj * BUFSIZE)));      \
} while (0)

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C) && defined(MBEDTLS_MEMORY_DEBUG)

#define MEMORY_MEASURE_INIT {                                           \
	size_t max_used, max_blocks, max_bytes;                         \
	size_t prv_used, prv_blocks;                                    \
	mbedtls_memory_buffer_alloc_cur_get(&prv_used, &prv_blocks);    \
	mbedtls_memory_buffer_alloc_max_reset();                        \
}

#define MEMORY_MEASURE_PRINT(title_len) {                             \
	mbedtls_memory_buffer_alloc_max_get(&max_used, &max_blocks);  \
								      \
	for (ii = 12 - title_len; ii != 0; ii--) {                    \
		mbedtls_printf(" ");                                  \
	}                                                             \
								      \
	max_used -= prv_used;                                         \
	max_blocks -= prv_blocks;                                     \
	max_bytes = max_used + MEM_BLOCK_OVERHEAD * max_blocks;       \
	mbedtls_printf("%6u heap bytes", (unsigned int) max_bytes);   \
}

#else
#define MEMORY_MEASURE_INIT
#define MEMORY_MEASURE_PRINT(title_len)
#endif

#define TIME_PUBLIC(TITLE, TYPE, CODE)                                \
do {                                                                  \
	unsigned long ii;                                             \
	int ret;                                                      \
	MEMORY_MEASURE_INIT;                                          \
								      \
	mbedtls_printf(HEADER_FORMAT, TITLE);                         \
	mbedtls_set_alarm(3);                                         \
								      \
	ret = 0;                                                      \
	for (ii = 1; !mbedtls_timing_alarmed && !ret ; ii++) {        \
		CODE;                                                 \
	}                                                             \
								      \
	if (ret != 0) {                                               \
		PRINT_ERROR;                                          \
	} else {                                                      \
		mbedtls_printf("%6lu " TYPE "/s", ii / 3);            \
		MEMORY_MEASURE_PRINT(sizeof(TYPE) + 1);               \
		mbedtls_printf("\n");                                 \
	}                                                             \
								      \
	(void)k_work_cancel_delayable_sync(&mbedtls_alarm, &work_sync);\
} while (0)

static int myrand(void *rng_state, unsigned char *output, size_t len)
{
	if (rng_state != NULL) {
		rng_state  = NULL;
	}

	sys_rand_get(output, len);

	return(0);
}

/*
 * Clear some memory that was used to prepare the context
 */
#if defined(MBEDTLS_ECP_C) && defined(MBEDTLS_ALLOW_PRIVATE_ACCESS)
void ecp_clear_precomputed(mbedtls_ecp_group *grp)
{
	if (grp->T != NULL) {
		size_t i;

		for (i = 0; i < grp->T_size; i++) {
			mbedtls_ecp_point_free(&grp->T[i]);
		}

		mbedtls_free(grp->T);
	}

	grp->T = NULL;
	grp->T_size = 0;
}
#else
#define ecp_clear_precomputed(g)
#endif

unsigned char buf[BUFSIZE];

typedef struct {
	char md5, ripemd160, sha1, sha256, sha512, des3, des,
	     aes_cbc, aes_gcm, aes_ccm, aes_xts, chachapoly, aes_cmac,
	     des3_cmac, aria, camellia, chacha20, poly1305,
	     havege, ctr_drbg, hmac_drbg, rsa, dhm, ecdsa, ecdh;
} todo_list;

int main(void)
{
	mbedtls_ssl_config conf;
	unsigned char tmp[200];
	char title[TITLE_LEN];
	todo_list todo;
	int i;

	printk("\tMBEDTLS Benchmark sample\n");

#if defined(MBEDTLS_PLATFORM_PRINTF_ALT)
	mbedtls_platform_set_printf(MBEDTLS_PRINT);
#endif
	mbedtls_ssl_conf_dbg(&conf, my_debug, NULL);

	k_work_init_delayable(&mbedtls_alarm, mbedtls_alarm_timeout);
	memset(&todo, 1, sizeof(todo));

	memset(buf, 0xAA, sizeof(buf));
	memset(tmp, 0xBB, sizeof(tmp));


#if defined(MBEDTLS_MD5_C)
	if (todo.md5) {
		TIME_AND_TSC("MD5", mbedtls_md5(buf, BUFSIZE, tmp));
	}
#endif

#if defined(MBEDTLS_RIPEMD160_C)
	if (todo.ripemd160) {
		TIME_AND_TSC("RIPEMD160",
			     mbedtls_ripemd160(buf, BUFSIZE, tmp));
	}
#endif

#if defined(MBEDTLS_SHA1_C)
	if (todo.sha1) {
		TIME_AND_TSC("SHA-1", mbedtls_sha1(buf, BUFSIZE, tmp));
	}
#endif

#if defined(MBEDTLS_SHA256_C)
	if (todo.sha256) {
		TIME_AND_TSC("SHA-256", mbedtls_sha256(buf,
						   BUFSIZE, tmp, 0));
	}
#endif

#if defined(MBEDTLS_SHA512_C)
	if (todo.sha512) {
		TIME_AND_TSC("SHA-512", mbedtls_sha512(buf,
						   BUFSIZE, tmp, 0));
	}
#endif

#if defined(MBEDTLS_DES_C)
#if defined(MBEDTLS_CIPHER_MODE_CBC)
	if (todo.des3) {
		mbedtls_des3_context des3;

		mbedtls_des3_init(&des3);
		mbedtls_des3_set3key_enc(&des3, tmp);

		TIME_AND_TSC("3DES",
			     mbedtls_des3_crypt_cbc(
						&des3,
						MBEDTLS_DES_ENCRYPT,
						BUFSIZE, tmp, buf, buf));
		mbedtls_des3_free(&des3);
	}

	if (todo.des) {
		mbedtls_des_context des;

		mbedtls_des_init(&des);
		mbedtls_des_setkey_enc(&des, tmp);

		TIME_AND_TSC("DES",
			     mbedtls_des_crypt_cbc(&des,
						   MBEDTLS_DES_ENCRYPT,
						   BUFSIZE, tmp, buf, buf));
		mbedtls_des_free(&des);

	}
#endif /* MBEDTLS_CIPHER_MODE_CBC */
#if defined(MBEDTLS_CMAC_C)
	if (todo.des3_cmac) {
		unsigned char output[8];
		const mbedtls_cipher_info_t *cipher_info;

		memset(buf, 0, sizeof(buf));
		memset(tmp, 0, sizeof(tmp));

		cipher_info = mbedtls_cipher_info_from_type(
						MBEDTLS_CIPHER_DES_EDE3_ECB);

		TIME_AND_TSC("3DES-CMAC",
			     mbedtls_cipher_cmac(cipher_info, tmp, 192, buf,
						 BUFSIZE, output));
	}
#endif /* MBEDTLS_CMAC_C */
#endif /* MBEDTLS_DES_C */

#if defined(MBEDTLS_AES_C)
#if defined(MBEDTLS_CIPHER_MODE_CBC)
	if (todo.aes_cbc) {
		int keysize;
		mbedtls_aes_context aes;

		mbedtls_aes_init(&aes);

		for (keysize = 128; keysize <= 256; keysize += 64) {
			snprintk(title, sizeof(title),
				 "AES-CBC-%d", keysize);

			memset(buf, 0, sizeof(buf));
			memset(tmp, 0, sizeof(tmp));
			mbedtls_aes_setkey_enc(&aes, tmp, keysize);

			TIME_AND_TSC(title,
				     mbedtls_aes_crypt_cbc(&aes,
						   MBEDTLS_AES_ENCRYPT,
						   BUFSIZE, tmp, buf, buf));
		}

		mbedtls_aes_free(&aes);
	}
#endif
#if defined(MBEDTLS_CIPHER_MODE_XTS)
	if (todo.aes_xts) {
		int keysize;
		mbedtls_aes_xts_context ctx;

		mbedtls_aes_xts_init(&ctx);

		for (keysize = 128; keysize <= 256; keysize += 128) {
			snprintk(title, sizeof(title),
				 "AES-XTS-%d", keysize);

			memset(buf, 0, sizeof(buf));
			memset(tmp, 0, sizeof(tmp));

			mbedtls_aes_xts_setkey_enc(&ctx, tmp, keysize * 2);

			TIME_AND_TSC(title,
				     mbedtls_aes_crypt_xts(&ctx,
						MBEDTLS_AES_ENCRYPT, BUFSIZE,
						tmp, buf, buf));

			mbedtls_aes_xts_free(&ctx);
		}
	}
#endif
#if defined(MBEDTLS_GCM_C)
	if (todo.aes_gcm) {
		int keysize;
		mbedtls_gcm_context gcm;

		mbedtls_gcm_init(&gcm);

		for (keysize = 128; keysize <= 256; keysize += 64) {
			snprintk(title, sizeof(title), "AES-GCM-%d",
				 keysize);

			memset(buf, 0, sizeof(buf));
			memset(tmp, 0, sizeof(tmp));
			mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, tmp,
					   keysize);

			TIME_AND_TSC(title,
				     mbedtls_gcm_crypt_and_tag(&gcm,
							MBEDTLS_GCM_ENCRYPT,
							BUFSIZE, tmp,
							12, NULL, 0, buf, buf,
							16, tmp));
			mbedtls_gcm_free(&gcm);
		}
	}
#endif
#if defined(MBEDTLS_CCM_C)
	if (todo.aes_ccm) {
		int keysize;
		mbedtls_ccm_context ccm;

		mbedtls_ccm_init(&ccm);

		for (keysize = 128; keysize <= 256; keysize += 64) {
			snprintk(title, sizeof(title), "AES-CCM-%d",
				 keysize);

			memset(buf, 0, sizeof(buf));
			memset(tmp, 0, sizeof(tmp));
			mbedtls_ccm_setkey(&ccm, MBEDTLS_CIPHER_ID_AES, tmp,
					   keysize);

			TIME_AND_TSC(title,
				     mbedtls_ccm_encrypt_and_tag(&ccm, BUFSIZE,
							tmp, 12, NULL, 0, buf,
							buf, tmp, 16));

			mbedtls_ccm_free(&ccm);
		}
	}
#endif
#if defined(MBEDTLS_CHACHAPOLY_C)
	if (todo.chachapoly) {
		mbedtls_chachapoly_context chachapoly;

		mbedtls_chachapoly_init(&chachapoly);

		memset(buf, 0, sizeof(buf));
		memset(tmp, 0, sizeof(tmp));

		snprintk(title, sizeof(title), "ChaCha20-Poly1305");

		mbedtls_chachapoly_setkey(&chachapoly, tmp);

		TIME_AND_TSC(title,
			     mbedtls_chachapoly_encrypt_and_tag(&chachapoly,
				BUFSIZE, tmp, NULL, 0, buf, buf, tmp));

		mbedtls_chachapoly_free(&chachapoly);
	}
#endif
#if defined(MBEDTLS_CMAC_C)
	if (todo.aes_cmac) {
		unsigned char output[16];
		const mbedtls_cipher_info_t *cipher_info;
		mbedtls_cipher_type_t cipher_type;
		int keysize;

		for (keysize = 128, cipher_type = MBEDTLS_CIPHER_AES_128_ECB;
		     keysize <= 256; keysize += 64, cipher_type++) {
			snprintk(title, sizeof(title), "AES-CMAC-%d",
				 keysize);

			memset(buf, 0, sizeof(buf));
			memset(tmp, 0, sizeof(tmp));

			cipher_info = mbedtls_cipher_info_from_type(
							cipher_type);

			TIME_AND_TSC(title,
				     mbedtls_cipher_cmac(cipher_info,
							 tmp, keysize,
							 buf, BUFSIZE,
							 output));
		}

		memset(buf, 0, sizeof(buf));
		memset(tmp, 0, sizeof(tmp));

		TIME_AND_TSC("AES-CMAC-PRF-128",
			     mbedtls_aes_cmac_prf_128(tmp, 16, buf, BUFSIZE,
						      output));
	}
#endif /* MBEDTLS_CMAC_C */
#endif /* MBEDTLS_AES_C */

#if defined(MBEDTLS_ARIA_C) && defined(MBEDTLS_CIPHER_MODE_CBC)
	if (todo.aria) {
		int keysize;
		mbedtls_aria_context aria;

		mbedtls_aria_init(&aria);

		for (keysize = 128; keysize <= 256; keysize += 64) {
			snprintk(title, sizeof(title),
				 "ARIA-CBC-%d", keysize);

			memset(buf, 0, sizeof(buf));
			memset(tmp, 0, sizeof(tmp));

			mbedtls_aria_setkey_enc(&aria, tmp, keysize);

			TIME_AND_TSC(title,
				     mbedtls_aria_crypt_cbc(&aria,
						MBEDTLS_ARIA_ENCRYPT,
						BUFSIZE, tmp, buf, buf));
		}

		mbedtls_aria_free(&aria);
	}
#endif

#if defined(MBEDTLS_CAMELLIA_C) && defined(MBEDTLS_CIPHER_MODE_CBC)
	if (todo.camellia) {
		int keysize;
		mbedtls_camellia_context camellia;

		mbedtls_camellia_init(&camellia);

		for (keysize = 128; keysize <= 256; keysize += 64) {
			snprintk(title, sizeof(title),
				 "CAMELLIA-CBC-%d", keysize);

			memset(buf, 0, sizeof(buf));
			memset(tmp, 0, sizeof(tmp));

			mbedtls_camellia_setkey_enc(&camellia, tmp, keysize);

			TIME_AND_TSC(title,
				     mbedtls_camellia_crypt_cbc(&camellia,
						MBEDTLS_CAMELLIA_ENCRYPT,
						BUFSIZE, tmp, buf, buf));
		}

		mbedtls_camellia_free(&camellia);
	}
#endif

#if defined(MBEDTLS_CHACHA20_C)
	if (todo.chacha20) {
		TIME_AND_TSC("ChaCha20", mbedtls_chacha20_crypt(
						buf, buf, 0U, BUFSIZE,
						buf, buf));
	}
#endif

#if defined(MBEDTLS_POLY1305_C)
	if (todo.poly1305) {
		TIME_AND_TSC("Poly1305", mbedtls_poly1305_mac(
						buf, buf, BUFSIZE,
						buf));
	}
#endif

#if defined(MBEDTLS_HAVEGE_C)
	if (todo.havege) {
		mbedtls_havege_state hs;

		mbedtls_havege_init(&hs);

		TIME_AND_TSC("HAVEGE", mbedtls_havege_random(&hs,
							     buf, BUFSIZE));
		mbedtls_havege_free(&hs);
	}
#endif

#if defined(MBEDTLS_CTR_DRBG_C)
	if (todo.ctr_drbg) {
		mbedtls_ctr_drbg_context ctr_drbg;

		mbedtls_ctr_drbg_init(&ctr_drbg);

		if (mbedtls_ctr_drbg_seed(&ctr_drbg, myrand,
					  NULL, NULL, 0) != 0) {
			mbedtls_exit(1);
		}

		TIME_AND_TSC("CTR_DRBG (NOPR)",
			     mbedtls_ctr_drbg_random(&ctr_drbg, buf, BUFSIZE));

		if (mbedtls_ctr_drbg_seed(&ctr_drbg, myrand,
					  NULL, NULL, 0) != 0) {
			mbedtls_exit(1);
		}

		mbedtls_ctr_drbg_set_prediction_resistance(&ctr_drbg,
						MBEDTLS_CTR_DRBG_PR_ON);

		TIME_AND_TSC("CTR_DRBG (PR)",
			     mbedtls_ctr_drbg_random(&ctr_drbg, buf, BUFSIZE));

		mbedtls_ctr_drbg_free(&ctr_drbg);
	}
#endif

#if defined(MBEDTLS_HMAC_DRBG_C)
	if (todo.hmac_drbg) {
		mbedtls_hmac_drbg_context hmac_drbg;
		const mbedtls_md_info_t *md_info;

		mbedtls_hmac_drbg_init(&hmac_drbg);

#if defined(MBEDTLS_SHA1_C)
		md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
		if (md_info == NULL) {
			mbedtls_exit(1);
		}

		if (mbedtls_hmac_drbg_seed(&hmac_drbg, md_info,
					   myrand, NULL, NULL, 0) != 0) {
			mbedtls_exit(1);
		}

		TIME_AND_TSC("HMAC_DRBG SHA-1 (NOPR)",
			     mbedtls_hmac_drbg_random(&hmac_drbg, buf,
						      BUFSIZE));

		if (mbedtls_hmac_drbg_seed(&hmac_drbg, md_info, myrand,
					   NULL, NULL, 0) != 0) {
			mbedtls_exit(1);
		}

		mbedtls_hmac_drbg_set_prediction_resistance(&hmac_drbg,
						MBEDTLS_HMAC_DRBG_PR_ON);

		TIME_AND_TSC("HMAC_DRBG SHA-1 (PR)",
			     mbedtls_hmac_drbg_random(&hmac_drbg, buf,
						      BUFSIZE));
#endif

#if defined(MBEDTLS_SHA256_C)
		md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
		if (md_info == NULL) {
			mbedtls_exit(1);
		}

		if (mbedtls_hmac_drbg_seed(&hmac_drbg, md_info,
					   myrand, NULL, NULL, 0) != 0) {
			mbedtls_exit(1);
		}

		TIME_AND_TSC("HMAC_DRBG SHA-256 (NOPR)",
			     mbedtls_hmac_drbg_random(&hmac_drbg, buf,
						      BUFSIZE));

		if (mbedtls_hmac_drbg_seed(&hmac_drbg, md_info, myrand,
					   NULL, NULL, 0) != 0) {
			mbedtls_exit(1);
		}

		mbedtls_hmac_drbg_set_prediction_resistance(&hmac_drbg,
					MBEDTLS_HMAC_DRBG_PR_ON);

		TIME_AND_TSC("HMAC_DRBG SHA-256 (PR)",
			     mbedtls_hmac_drbg_random(&hmac_drbg, buf,
						      BUFSIZE));
#endif
		mbedtls_hmac_drbg_free(&hmac_drbg);
	}
#endif

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_GENPRIME)
	if (todo.rsa) {
		int keysize;
		mbedtls_rsa_context rsa;

		for (keysize = 2048; keysize <= 4096; keysize *= 2) {
			snprintk(title, sizeof(title), "RSA-%d",
				 keysize);

			mbedtls_rsa_init(&rsa);
			mbedtls_rsa_gen_key(&rsa, myrand, NULL, keysize,
					    65537);

			TIME_PUBLIC(title, " public",
				    buf[0] = 0;
				    ret = mbedtls_rsa_public(&rsa, buf, buf));

			TIME_PUBLIC(title, "private",
				    buf[0] = 0;
				    ret = mbedtls_rsa_private(&rsa, myrand,
							      NULL, buf, buf));

			mbedtls_rsa_free(&rsa);
		}
	}
#endif

#if defined(MBEDTLS_DHM_C) && defined(MBEDTLS_BIGNUM_C)
	if (todo.dhm) {
		int dhm_sizes[] = {2048, 3072};
		static const unsigned char dhm_P_2048[] =
					MBEDTLS_DHM_RFC3526_MODP_2048_P_BIN;
		static const unsigned char dhm_P_3072[] =
					MBEDTLS_DHM_RFC3526_MODP_3072_P_BIN;
		static const unsigned char dhm_G_2048[] =
					MBEDTLS_DHM_RFC3526_MODP_2048_G_BIN;
		static const unsigned char dhm_G_3072[] =
					MBEDTLS_DHM_RFC3526_MODP_3072_G_BIN;

		const unsigned char *dhm_P[] = { dhm_P_2048, dhm_P_3072 };
		const size_t dhm_P_size[] = { sizeof(dhm_P_2048),
					      sizeof(dhm_P_3072) };
		const unsigned char *dhm_G[] = { dhm_G_2048, dhm_G_3072 };
		const size_t dhm_G_size[] = { sizeof(dhm_G_2048),
					      sizeof(dhm_G_3072) };
		mbedtls_dhm_context dhm;
		size_t olen;
		size_t n;

		for (i = 0; i < ARRAY_SIZE(dhm_sizes); i++) {
			mbedtls_dhm_init(&dhm);

			if (mbedtls_mpi_read_binary(&dhm.P, dhm_P[i],
						    dhm_P_size[i]) != 0 ||
			    mbedtls_mpi_read_binary(&dhm.G, dhm_G[i],
						    dhm_G_size[i]) != 0) {
				mbedtls_exit(1);
			}

			n = mbedtls_mpi_size(&dhm.P);
			mbedtls_dhm_make_public(&dhm, (int)n, buf,
						n, myrand, NULL);
			if (mbedtls_mpi_copy(&dhm.GY, &dhm.GX) != 0) {
				mbedtls_exit(1);
			}

			snprintk(title, sizeof(title), "DHE-%d", dhm_sizes[i]);

			TIME_PUBLIC(title, "handshake",
				    ret |= mbedtls_dhm_make_public(&dhm,
						(int)n, buf, n,
						myrand, NULL);
				    ret |= mbedtls_dhm_calc_secret(&dhm, buf,
						sizeof(buf), &olen, myrand,
						NULL));

			snprintk(title, sizeof(title), "DH-%d", dhm_sizes[i]);

			TIME_PUBLIC(title, "handshake",
				    ret |= mbedtls_dhm_calc_secret(&dhm, buf,
						sizeof(buf), &olen, myrand,
						NULL));

			mbedtls_dhm_free(&dhm);
		}
	}
#endif

#if defined(MBEDTLS_ECDSA_C) && defined(MBEDTLS_SHA256_C)
	if (todo.ecdsa) {
		size_t sig_len;
		const mbedtls_ecp_curve_info *curve_info;
		mbedtls_ecdsa_context ecdsa;

		memset(buf, 0x2A, sizeof(buf));

		for (curve_info = mbedtls_ecp_curve_list();
		     curve_info->grp_id != MBEDTLS_ECP_DP_NONE;
		     curve_info++) {
			mbedtls_ecdsa_init(&ecdsa);

			if (mbedtls_ecdsa_genkey(&ecdsa, curve_info->grp_id,
						 myrand, NULL) != 0) {
				mbedtls_exit(1);
			}

			ecp_clear_precomputed(&ecdsa.grp);

			snprintk(title, sizeof(title), "ECDSA-%s",
				 curve_info->name);

			TIME_PUBLIC(title, "sign",
				    ret = mbedtls_ecdsa_write_signature(
						&ecdsa, MBEDTLS_MD_SHA256,
						buf, curve_info->bit_size,
						tmp, sizeof(tmp), &sig_len,
						myrand, NULL));

			mbedtls_ecdsa_free(&ecdsa);
		}

		for (curve_info = mbedtls_ecp_curve_list();
		     curve_info->grp_id != MBEDTLS_ECP_DP_NONE;
		     curve_info++) {
			mbedtls_ecdsa_init(&ecdsa);

			if (mbedtls_ecdsa_genkey(&ecdsa, curve_info->grp_id,
						 myrand, NULL) != 0 ||
			    mbedtls_ecdsa_write_signature(&ecdsa,
						MBEDTLS_MD_SHA256, buf,
						curve_info->bit_size,
						tmp, sizeof(tmp),
						&sig_len, myrand,
						NULL) != 0) {
				mbedtls_exit(1);
			}

			ecp_clear_precomputed(&ecdsa.grp);

			snprintk(title, sizeof(title), "ECDSA-%s",
				 curve_info->name);

			TIME_PUBLIC(title, "verify",
				    ret = mbedtls_ecdsa_read_signature(&ecdsa,
						buf, curve_info->bit_size,
						tmp, sig_len));

			mbedtls_ecdsa_free(&ecdsa);
		}
	}
#endif

#if defined(MBEDTLS_ECDH_C) && defined(MBEDTLS_ECDH_LEGACY_CONTEXT)
	if (todo.ecdh) {
		mbedtls_ecdh_context ecdh;
		mbedtls_mpi z;
		const mbedtls_ecp_curve_info montgomery_curve_list[] = {
#if defined(MBEDTLS_ECP_DP_CURVE25519_ENABLED)
			{ MBEDTLS_ECP_DP_CURVE25519, 0, 0, "Curve25519" },
#endif
#if defined(MBEDTLS_ECP_DP_CURVE448_ENABLED)
			{ MBEDTLS_ECP_DP_CURVE448, 0, 0, "Curve448" },
#endif
			{ MBEDTLS_ECP_DP_NONE, 0, 0, 0 }
			};
		const mbedtls_ecp_curve_info *curve_info;
		size_t olen;

		for (curve_info = mbedtls_ecp_curve_list();
		     curve_info->grp_id != MBEDTLS_ECP_DP_NONE;
		     curve_info++) {
			mbedtls_ecdh_init(&ecdh);

			if (mbedtls_ecp_group_load(&ecdh.grp,
						curve_info->grp_id) != 0 ||
			    mbedtls_ecdh_make_public(&ecdh, &olen, buf,
						     sizeof(buf),
						     myrand, NULL) != 0 ||
			    mbedtls_ecp_copy(&ecdh.Qp, &ecdh.Q) != 0) {
				mbedtls_exit(1);
			}

			ecp_clear_precomputed(&ecdh.grp);

			snprintk(title, sizeof(title), "ECDHE-%s",
				 curve_info->name);

			TIME_PUBLIC(title, "handshake",
				    ret |= mbedtls_ecdh_make_public(&ecdh,
						&olen, buf, sizeof(buf),
						myrand, NULL);
				    ret |= mbedtls_ecdh_calc_secret(&ecdh,
						&olen, buf, sizeof(buf),
						myrand, NULL));
			mbedtls_ecdh_free(&ecdh);
		}

		/* Montgomery curves need to be handled separately */
		for (curve_info = montgomery_curve_list;
		     curve_info->grp_id != MBEDTLS_ECP_DP_NONE;
		     curve_info++) {
			mbedtls_ecdh_init(&ecdh);
			mbedtls_mpi_init(&z);

			if (mbedtls_ecp_group_load(&ecdh.grp,
						   curve_info->grp_id) != 0 ||
			    mbedtls_ecdh_gen_public(&ecdh.grp, &ecdh.d,
						    &ecdh.Qp, myrand, NULL)
			    != 0) {
				mbedtls_exit(1);
			}

			snprintk(title, sizeof(title), "ECDHE-%s",
				 curve_info->name);

			TIME_PUBLIC(title, "handshake",
				    ret |= mbedtls_ecdh_gen_public(&ecdh.grp,
								   &ecdh.d,
								   &ecdh.Q,
								   myrand,
								   NULL);
				    ret |= mbedtls_ecdh_compute_shared(
								&ecdh.grp, &z,
								&ecdh.Qp,
								&ecdh.d,
								myrand,
								NULL));
			mbedtls_ecdh_free(&ecdh);
			mbedtls_mpi_free(&z);
		}

		for (curve_info = mbedtls_ecp_curve_list();
		     curve_info->grp_id != MBEDTLS_ECP_DP_NONE;
		     curve_info++) {
			mbedtls_ecdh_init(&ecdh);

			if (mbedtls_ecp_group_load(&ecdh.grp,
						   curve_info->grp_id) != 0 ||
			    mbedtls_ecdh_make_public(&ecdh, &olen, buf,
						     sizeof(buf), myrand,
						     NULL) != 0 ||
			    mbedtls_ecp_copy(&ecdh.Qp, &ecdh.Q) != 0 ||
			    mbedtls_ecdh_make_public(&ecdh, &olen, buf,
						     sizeof(buf), myrand,
						     NULL) != 0) {
				mbedtls_exit(1);
			}

			ecp_clear_precomputed(&ecdh.grp);

			snprintk(title, sizeof(title), "ECDH-%s",
				 curve_info->name);

			TIME_PUBLIC(title, "handshake",
				    ret |= mbedtls_ecdh_calc_secret(&ecdh,
							&olen,
							buf, sizeof(buf),
							myrand, NULL));
			mbedtls_ecdh_free(&ecdh);
		}

		/* Montgomery curves need to be handled separately */
		for (curve_info = montgomery_curve_list;
		     curve_info->grp_id != MBEDTLS_ECP_DP_NONE;
		     curve_info++) {
			mbedtls_ecdh_init(&ecdh);
			mbedtls_mpi_init(&z);

			if (mbedtls_ecp_group_load(&ecdh.grp,
						   curve_info->grp_id) != 0 ||
			    mbedtls_ecdh_gen_public(&ecdh.grp, &ecdh.d,
						    &ecdh.Qp, myrand,
						    NULL) != 0 ||
			    mbedtls_ecdh_gen_public(&ecdh.grp, &ecdh.d,
						    &ecdh.Q, myrand,
						    NULL) != 0) {
				mbedtls_exit(1);
			}

			snprintk(title, sizeof(title), "ECDH-%s",
				 curve_info->name);

			TIME_PUBLIC(title, "handshake",
				    ret |= mbedtls_ecdh_compute_shared(
								&ecdh.grp,
								&z, &ecdh.Qp,
								&ecdh.d,
								myrand,
								NULL));

			mbedtls_ecdh_free(&ecdh);
			mbedtls_mpi_free(&z);
		}
	}
#endif
	mbedtls_printf("\n       Done\n");
	return 0;
}
