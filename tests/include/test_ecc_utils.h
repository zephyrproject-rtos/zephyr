/*  test_ecc_utils.h - TinyCrypt interface to common functions for ECC tests */

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
 *  test_ecc_utils.h -- Interface to common functions for ECC tests.
 */

#ifndef __TEST_ECC_UTILS_H__
#define __TEST_ECC_UTILS_H__

#include <stdlib.h>
#include <tinycrypt/constants.h>

int keygen_vectors(EccPoint  *point, char **d_vec, char **qx_vec, char **qy_vec,
		   int tests, int verbose);

int random_start(const char *fn);
int random_end(void);
int random_bytes(uint32_t *out, size_t len);

void string2host(uint32_t *native, const uint8_t *bytes, size_t len);

int hex_to_num(char hex);

int hex_to_num_str(uint8_t *buf, const size_t buflen, const char *hex,
		   const size_t hexlen);

int str_to_scalar(uint32_t *scalar, uint32_t num_word32, char *str);

void vli_print(uint32_t *p_vli, unsigned int p_size);

int check_code(const int num, const char *name, const int expected,
	       const int computed, const int verbose);

int check_ecc_result(const int num, const char *name, const uint32_t *expected,
		     const uint32_t *computed, const uint32_t num_word32,
		     int verbose);

#endif
