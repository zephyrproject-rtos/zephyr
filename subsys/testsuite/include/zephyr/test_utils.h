/*  test_utils.h - TinyCrypt interface to common functions for tests */

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
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __TEST_UTILS_H__
#define __TEST_UTILS_H__

#include <zephyr/tc_util.h>
#include <tinycrypt/constants.h>

static inline void show_str(const char *label, const uint8_t *s, size_t len)
{
	uint32_t i;

	TC_PRINT("%s = ", label);
	for (i = 0U; i < (uint32_t)len; ++i) {
		TC_PRINT("%02x", s[i]);
	}
	TC_PRINT("\n");
}

static inline
void fatal(uint32_t testnum, const void *expected, size_t expectedlen,
	   const void *computed, size_t computedlen)
{
	TC_ERROR("\tTest #%d Failed!\n", testnum);
	show_str("\t\tExpected", expected, expectedlen);
	show_str("\t\tComputed  ", computed, computedlen);
	TC_PRINT("\n");
}

static inline
uint32_t check_result(uint32_t testnum, const void *expected,
		      size_t expectedlen, const void *computed,
		      size_t computedlen, uint32_t verbose)
{
	uint32_t result = TC_PASS;

	ARG_UNUSED(verbose);

        if (expectedlen != computedlen) {
		TC_ERROR("The length of the computed buffer (%zu)",
			 computedlen);
		TC_ERROR("does not match the expected length (%zu).",
			 expectedlen);
                result = TC_FAIL;
	} else {
		if (memcmp(computed, expected, computedlen) != 0) {
			fatal(testnum, expected, expectedlen,
			      computed, computedlen);
			result = TC_FAIL;
		}
	}

        return result;
}

#endif
