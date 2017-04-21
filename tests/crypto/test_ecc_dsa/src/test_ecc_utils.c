/* test_ecc_utils.c - TinyCrypt common functions for ECC tests */

/*
 *  Copyright (C) 2015 by Intel Corporation, All Rights Reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *    - Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *    - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 *    - Neither the name of Intel Corporation nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *  test_ecc_utils.c -- Implementation of some common functions for ECC tests.
 *
 */

#include <zephyr.h>
#include <drivers/rand32.h>

#include <tinycrypt/ecc.h>
#include <tinycrypt/ecc_dh.h>
#include <test_ecc_utils.h>
#include <test_utils.h>
#include <tinycrypt/constants.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>

int random_start(const char *fn)
{
	(void)fn;

	return 0;
}

int random_end(void)
{
	return 0;
}

int random_bytes(u32_t *out, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++) {
		out[i] = sys_rand32_get();
	}

	return i == len ? 0 : -EINVAL;
}

int hex_to_num(char hex)
{
	u8_t dec;

	if ('0' <= hex && hex <= '9') {
		dec = hex - '0';
	} else if ('a' <= hex && hex <= 'f') {
		dec = hex - 'a' + 10;
	} else if ('A' <= hex && hex <= 'F') {
		dec = hex - 'A' + 10;
	} else {
		return -1;
	}

	return dec;
}

/*
 * Convert hex string to byte string
 * Return number of bytes written to buf, or 0 on error
 */
int hex_to_num_str(u8_t *buf, const size_t buflen, const char *hex,
		   const size_t hexlen)
{
	int dec;

	if (buflen < hexlen / 2 + hexlen % 2) {
		return 0;
	}

	/* if hexlen is uneven, insert leading zero nibble */
	if (hexlen % 2) {
		dec = hex_to_num(hex[0]);
		if (dec == -1) {
			return 0;
		}
		buf[0] = dec;
		buf++;
		hex++;
	}

	/* regular hex conversion */
	for (size_t i = 0; i < hexlen / 2; i++) {
		dec = hex_to_num(hex[2 * i]);
		if (dec == -1) {
			return 0;
		}
		buf[i] = dec << 4;

		dec = hex_to_num(hex[2 * i + 1]);
		if (dec == -1) {
			return 0;
		}
		buf[i] += dec;
	}
	return hexlen / 2 + hexlen % 2;
}

/*
 * Convert hex string to zero-padded nanoECC scalar
 */
int str_to_scalar(u32_t *scalar, u32_t num_word32, char *str)
{
	u32_t num_bytes = 4 * num_word32;
	u8_t tmp[num_bytes];
	size_t hexlen = strlen(str);
	int padding;
	int rc;

	padding = 2 * num_bytes - strlen(str);
	if (padding < 0) {
		TC_PRINT("Error: 2*num_bytes(%d) < strlen(hex) (%zu)\n",
			 2 * num_bytes, strlen(str));
		return TC_FAIL;
	}

	memset(tmp, 0, padding / 2);

	rc = hex_to_num_str(tmp + padding / 2, num_bytes, str, hexlen);
	if (rc == 0) {
		return TC_FAIL;
	}

	ecc_bytes2native(scalar, tmp);

	return TC_PASS;
}

void vli_print(u32_t *p_vli, unsigned int p_size)
{
	while (p_size) {
		TC_PRINT("%08X ", (unsigned)p_vli[p_size - 1]);
		--p_size;
	}
}

int check_code(const int num, const char *name, const int expected,
	       const int computed, const int verbose)
{
	if (expected != computed) {
		TC_PRINT("\nVector #%02d check %s - FAILURE:\n", num, name);
		TC_PRINT("\nExpected: %d, computed: %d\n\n", expected,
			 computed);
		return TC_FAIL;
	}

	if (verbose) {
		TC_PRINT("Vector #%02d check %s - success (%d=%d)\n", num, name,
			 expected, computed);
	}

	return TC_PASS;
}

int check_ecc_result(const int num, const char *name, const u32_t *expected,
		     const u32_t *computed, const u32_t num_word32,
		     int verbose)
{
	u32_t num_bytes = 4 * num_word32;

	if (memcmp(computed, expected, num_bytes)) {
		TC_PRINT("\n  Vector #%02d check %s - FAILURE\n", num, name);
		return TC_FAIL;
	}
	if (verbose) {
		TC_PRINT("  Vector #%02d check %s - success\n", num, name);
	}

	return TC_PASS;
}

/* Test ecc_make_keys, and also as keygen part of other tests */
int keygen_vectors(EccPoint *pub, char **d_vec, char **qx_vec, char **qy_vec,
		   int tests, int verbose)
{
	u32_t seed[2 * NUM_ECC_DIGITS];
	u32_t prv[NUM_ECC_DIGITS];

	/* expected outputs (converted input vectors) */
	EccPoint exp_pub;
	u32_t exp_prv[NUM_ECC_DIGITS];

	int rc;

	for (int i = 0; i < tests; i++) {

		str_to_scalar(exp_prv, NUM_ECC_DIGITS, d_vec[i]);
		str_to_scalar(exp_pub.x, NUM_ECC_DIGITS, qx_vec[i]);
		str_to_scalar(exp_pub.y, NUM_ECC_DIGITS, qy_vec[i]);

		/*
		 * Feed prvkey vector as padded random seed into ecc_make_key.
		 * Internal mod-reduction will be zero-op and generate correct
		 * prv/pub
		 */
		memset(seed, 0, 2 * NUM_ECC_BYTES);
		str_to_scalar(seed, NUM_ECC_DIGITS, d_vec[i]);
		ecc_make_key(pub, prv, seed);

		/* validate correctness of vector conversion and make_key() */
		rc = check_ecc_result(i, "prv  ", exp_prv, prv,
				      NUM_ECC_DIGITS, verbose);
		if (rc != TC_PASS) {
			goto exit_test;
		}
		rc = check_ecc_result(i, "pub.x", exp_pub.x, pub->x,
				      NUM_ECC_DIGITS, verbose);
		if (rc != TC_PASS) {
			goto exit_test;
		}
		rc = check_ecc_result(i, "pub.y", exp_pub.y, pub->y,
				      NUM_ECC_DIGITS, verbose);
		if (rc != TC_PASS) {
			goto exit_test;
		}
	}

	rc = TC_PASS;
exit_test:
	return rc;
}
