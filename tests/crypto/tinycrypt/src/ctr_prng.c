/*  test_ctr_prng.c - TinyCrypt implementation of some CTR-PRNG tests */

/*
 * Copyright (c) 2016, Chris Morrison, All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * DESCRIPTION
 * This module tests the CTR-PRNG routines
 */

#include <tinycrypt/ctr_prng.h>
#include <tinycrypt/aes.h>
#include <tinycrypt/constants.h>
#include <test_utils.h>

#include <stdio.h>
#include <string.h>
#include <ztest.h>

#define MAX_EXPECTED_STRING	128
#define MAX_BIN_SIZE		(MAX_EXPECTED_STRING / 2)

uint8_t per[MAX_BIN_SIZE];
uint8_t ai1[MAX_BIN_SIZE];
uint8_t ai2[MAX_BIN_SIZE];

struct prng_vector {
	char *entropy;
	char *personal;	/* may be null */
	char *extra1;	/* may be null */
	char *extra2;	/* may be null */
	char *expected;
};

/* vectors taken from NIST CAVS 14.3 CTR_DRBG.rsp */
struct prng_vector vectors[] = {
	/*
	 * AES-128 no df, PredictionResistance = False, EntropyInputLen = 256,
	 * NonceLen = 0, PersonalizationStringLen = 0, AdditionalInputLen = 0,
	 * ReturnedBitsLen = 512
	 */
	{  /* Count 0 */
	"ce50f33da5d4c1d3d4004eb35244b7f2cd7f2e5076fbf6780a7ff634b249a5fc",
	0,
	0,
	0,
	"6545c0529d372443b392ceb3ae3a99a30f963eaf313280f1d1a1e87f9db373d361e75d"
	"18018266499cccd64d9bbb8de0185f213383080faddec46bae1f784e5a",
	},

	{ /* Count 1 */
	"a385f70a4d450321dfd18d8379ef8e7736fee5fbf0a0aea53b76696094e8aa93",
	0,
	0,
	0,
	"1a062553ab60457ed1f1c52f5aca5a3be564a27545358c112ed92c6eae2cb7597cfcc2"
	"e0a5dd81c5bfecc941da5e8152a9010d4845170734676c8c1b6b3073a5",
	},

	/*
	 * AES-128 no df, PredictionResistance = False, EntropyInputLen = 256,
	 * NonceLen = 0, PersonalizationStringLen = 0, AdditionalInputLen = 256,
	 * ReturnedBitsLen = 512
	 */
	{ /* Count 0 */
	"6bd4f2ae649fc99350951ff0c5d460c1a9214154e7384975ee54b34b7cae0704",
	0,
	"ecd4893b979ac92db1894ae3724518a2f78cf2dbe2f6bbc6fda596df87c7a4ae",
	"b23e9188687c88768b26738862c4791fa52f92502e1f94bf66af017c4228a0dc",
	"5b2bf7a5c60d8ab6591110cbd61cd387b02de19784f496d1a109123d8b3562a5de2dd6"
	"d5d1aef957a6c4f371cecd93c15799d82e34d6a0dba7e915a27d8e65f3",
	},

	{ /* Count 1 */
	"e2addbde2a76e769fc7aa3f45b31402f482b73bbe7067ad6254621f06d3ef68b",
	0,
	"ad11643b019e31245e4ea41f18f7680458310580fa6efad275c5833e7f800dae",
	"b5d849616b3123c9725d188cd0005003220768d1200f9e7cc29ef6d88afb7b9a",
	"132d0d50c8477a400bb8935be5928f916a85da9ffcf1a8f6e9f9a14cca861036cda14c"
	"f66d8953dab456b632cf687cd539b4b807926561d0b3562b9d3334fb61",
	},

	/*
	 * AES-128 no df, PredictionResistance = False, EntropyInputLen = 256,
	 * NonceLen = 0, PersonalizationStringLen = 256, AdditionalInputLen = 0,
	 * ReturnedBitsLen = 512
	 */
	{  /* Count 0 */
	"cee23de86a69c7ef57f6e1e12bd16e35e51624226fa19597bf93ec476a44b0f2",
	"a2ef16f226ea324f23abd59d5e3c660561c25e73638fe21c87566e86a9e04c3e",
	0,
	0,
	"2a76d71b329f449c98dc08fff1d205a2fbd9e4ade120c7611c225c984eac8531288dd3"
	"049f3dc3bb3671501ab8fbf9ad49c86cce307653bd8caf29cb0cf07764",
	},

	{ /* Count 1 */
	"b09eb4a82a39066ec945bb7c6aef6a0682a62c3e674bd900297d4271a5f25b49",
	"a3b768adcfe76d61c972d900da8dffeeb2a42e740247aa719ed1c924d2d10bd4",
	0,
	0,
	"5a1c26803f3ffd4daf32042fdcc32c3812bb5ef13bc208cef82ea047d2890a6f5dcecf"
	"32bcc32a2585775ac5e1ffaa8de00664c54fe00a7674b985619e953c3a",
	},

	/*
	 * AES-128 no df, PredictionResistance = False, EntropyInputLen = 256,
	 * NonceLen = 0, PersonalizationStringLen = 256,
	 * AdditionalInputLen = 256,
	 * ReturnedBitsLen = 512
	 */
	{  /* Count 0 */
	"50b96542a1f2b8b05074051fe8fb0e45adbbd5560e3594e12d485fe1bfcb741f",
	"820c3030f97b3ead81a93b88b871937278fd3d711d2085d9280cba394673b17e",
	"1f1632058806d6d8e231288f3b15a3c324e90ccef4891bd595f09c3e80e27469",
	"5cadc8bfd86d2a5d44f921f64c7d153001b9bdd7caa6618639b948ebfad5cb8a",
	"02b76a66f103e98d450e25e09c35337747d987471d2b3d81e03be24c7e985417a32acd"
	"72bc0a6eddd9871410dacb921c659249b4e2b368c4ac8580fb5db559bc",
	},

	{ /* Count 1 */
	"ff5f4b754e8b364f6df0c5effba5f1c036de49c4b38cd8d230ee1f14d7234ef5",
	"994eb339f64034005d2e18352899e77df446e285c3430631d557498aac4f4280",
	"e1824832d5fc2a6dea544cac2ab73306d6566bde98cc8f9425d064b860a9b218",
	"c08b42433a78fd393a34ffc24724d479af08c36882799c134165d98b2866dc0a",
	"1efa34aed07dd57bde9741b8d1907d28e8c1ac71601df37ef4295e6ffb67f6a1c4c13e"
	"5def65d505e2408aeb82948999ca1f9c9113b99a6b59ff7f0cc3dc6e92",
	},

	/*
	 * AES-128 no df, PredictionResistance = False, EntropyInputLen = 256,
	 * NonceLen = 0, PersonalizationStringLen = 0, AdditionalInputLen = 0,
	 * ReturnedBitsLen = 512
	 */
	{  /* Count 0 */
	"69a09f6bf5dda15cd4af29e14cf5e0cddd7d07ac39bba587f8bc331104f9c448",
	0,
	0,
	0,
	"f78a4919a6ec899f7b6c69381febbbe083315f3d289e70346db0e4ec4360473ae0b3d9"
	"16e9b6b964309f753ed66ae59de48da316cc1944bc8dfd0e2575d0ff6d",
	},

	{ /* Count 1 */
	"80bfbd340d79888f34f043ed6807a9f28b72b6644d9d9e9d777109482b80788a",
	0,
	0,
	0,
	"80db048d2f130d864b19bfc547c92503e580cb1a8e1f74f3d97fdda6501fb1aa81fced"
	"ac0dd18b6ccfdc183ca28a44fc9f3a08834ba8751a2f4495367c54a185",
	},

	/*
	 * AES-128 no df, PredictionResistance = False, EntropyInputLen = 256,
	 * NonceLen = 0, PersonalizationStringLen = 0, AdditionalInputLen = 256,
	 * ReturnedBitsLen = 512
	 */
	{ /* Count 0 */
	"7f40804693552e317523fda6935a5bc814353b1fbb7d334964ac4d1d12ddccce",
	0,
	"95c04259f64fcd1fe00c183aa3fb76b8a73b4d1243b800d770e38515bc41143c",
	"5523102dbd7fe1228436b91a765b165ae6405eb0236e237afad4759cf0888941",
	"1abf6bccb4c2d64e5187b1e2e34e493eca204ee4eef0d964267e38228f5f20efba3764"
	"30a266f3832916d0a45b2703f46401dfd145e447a0a1667ebd8b6ee748",
	},

	{ /* Count 1 */
	"350df677409a1dc297d01d3716a2abdfa6272cd030ab75f76839648582b47113",
	0,
	"ba5709a12ae6634a5436b7ea06838b48f7b847a237f6654a0e27c776ebee9511",
	"f1b2c717c5e3a934127e10471d67accc65f4a45010ca53b35f54c88833dbd8e7",
	"1ef1ea279812e8abe54f7ffd12d04c80ae40741f4ccfe232a5fba3a78dfd3e2ed419b8"
	"8ee9188df724160cbb3aea0f276e84a3c0ff01e3b89fe30ebcfa64cb86",
	},

	/*
	 * AES-128 no df, PredictionResistance = False, EntropyInputLen = 256,
	 * NonceLen = 0, PersonalizationStringLen = 256, AdditionalInputLen = 0,
	 * ReturnedBitsLen = 512
	 */
	{  /* Count 0 */
	"3fef762f0aa0677f61c65d749eeb10b013ff68ccc6314f150cfee752dcd8f987",
	"f56db099240c7590dac396372b8737404d418b2864a3df96a8a397967245735f",
	0,
	0,
	"af0afe0837442136fbb1959a1c91a9291c1d8188ede07c67d0e4dd6541303415e7a679"
	"99c302ba0df555324c26077514592a9b6db6be2f153fad2250161164e4",
	},

	{ /* Count 1 */
	"3eebe77db4631862e3eb7e39370515b8baa1cdd71a5b1b0cda79c14d0b5f48ea",
	"4be56a9b9c21242739c985ef12aa4d98e8c7da07c4c1dc6829f2e06833cfa148",
	0,
	0,
	"be9e18a753df261927473c8bb5fb7c3ea6e821df5ab49adc566a4ebf44f75fa825b1f9"
	"d8c154bcd469134c0bb688e07e3c3e45407ca350d540e1528cc2e64068",
	},

	/*
	 * AES-128 no df, PredictionResistance = False, EntropyInputLen = 256,
	 * NonceLen = 0, PersonalizationStringLen = 256,
	 * AdditionalInputLen = 256,
	 * ReturnedBitsLen = 512
	 */
	{  /* Count 0 */
	"c129c2732003bbf1d1dec244a933cd04cb47199bbce98fe080a1be880afb2155",
	"64e2b9ac5c20642e3e3ee454b7463861a7e93e0dd1bbf8c4a0c28a6cb3d811ba",
	"f94f0975760d52f47bd490d1623a9907e4df701f601cf2d573aba803a29d2b51",
	"6f99720b186e2028a5fcc586b3ea518458e437ff449c7c5a318e6d13f75b5db7",
	"7b8b3378b9031ab3101cec8af5b8ba5a9ca2a9af41432cd5f2e5e19716140bb219ed7f"
	"4ba88fc37b2d7e146037d2cac1128ffe14131c8691e581067a29cacf80",
	},

	{ /* Count 1 */
	"7667643670254b3530e80a17b16b22406e84efa6a4b5ceef3ebc877495fc6048",
	"40b92969953acde756747005117e46eff6893d7132a8311ffb1062280367326b",
	"797a02ffbe8ff2c94ed0e5d39ebdc7847adaa762a88238242ed8f71f5635b194",
	"d617f0f0e609e90d814192ba2e5214293d485402cdf9f789cc78b05e8c374f18",
	"e8d6f89dca9825aed8927b43187492a98ca8648db30f0ac709556d401a8ac2b959c813"
	"50fc64332c4c0deb559a286a72e65dbb462bd872f9b28c0728f353dc10",
	}
};

/* utility function to convert hex character representation
 * to their nibble (4 bit) values
 */
static uint8_t char_to_nibble(char c)
{
	if (c >= '0' && c <= '9') {
		return c - '0';
	}
	if (c >= 'a' && c <= 'f') {
		return c - 'a' + 10U;
	}
	if (c >= 'A' && c <= 'F') {
		return c - 'A' + 10U;
	}
	return 255U;
}

/*
 * Convert a string of characters representing a hex buffer into a series of
 * bytes of that real value
 */
static void hex_str_to_num(uint8_t *buf, char *in)
{
	int len;
	int i;

	len = strlen(in) / 2;
	for (i = 0; i < len; i++) {
		buf[i] = (char_to_nibble(*in) << 4) | char_to_nibble(*(in+1));
		in += 2;
	}
}

static int test_prng_vector(struct prng_vector *v)
{
	TCCtrPrng_t ctx;

	uint8_t entropy[MAX_BIN_SIZE];
	uint8_t expected[MAX_BIN_SIZE];
	uint8_t output[MAX_BIN_SIZE];
	uint8_t *personal   = NULL;
	uint8_t *extra1 = NULL;
	uint8_t *extra2 = NULL;

	int extra1_len = 0;
	int extra2_len = 0;
	int plen = 0;
	int ent_len;
	int exp_len;
	int rc;

	hex_str_to_num(entropy, v->entropy);
	hex_str_to_num(expected, v->expected);

	ent_len = strlen(v->entropy) / 2;
	exp_len = strlen(v->expected) / 2;

	if (v->personal != 0) {
		personal = per;
		hex_str_to_num(personal, v->personal);
		plen = strlen(v->personal) / 2;
	}

	if (v->extra1 != 0) {
		extra1 = ai1;
		hex_str_to_num(extra1, v->extra1);
		extra1_len = strlen(v->extra1) / 2;
	}

	if (v->extra2 != 0) {
		extra2 = ai2;
		hex_str_to_num(extra2, v->extra2);
		extra2_len = strlen(v->extra2) / 2;
	}

	rc = tc_ctr_prng_init(&ctx, entropy, ent_len, personal, plen);

	/**TESTPOINT: Check if init works*/
	zassert_equal(rc, TC_CRYPTO_SUCCESS, "CTR PRNG init failed");

	rc = tc_ctr_prng_generate(&ctx, extra1, extra1_len, output, exp_len);

	/**TESTPOINT: Check if generate works*/
	zassert_equal(rc, TC_CRYPTO_SUCCESS, "CTR PRNG generate failed");

	rc = tc_ctr_prng_generate(&ctx, extra2, extra2_len, output, exp_len);

	/**TESTPOINT: Check if generate works*/
	zassert_equal(rc, TC_CRYPTO_SUCCESS, "CTR PRNG generate failed");

	rc = memcmp(output, expected, exp_len);

	/**TESTPOINT: Check results*/
	zassert_false(rc, "expected value different - check failed");

	rc = TC_PASS;
	return rc;
}

void test_ctr_prng_reseed(void)
{
	uint8_t expectedV1[] = {0x7E, 0xE3, 0xA0, 0xCB, 0x6D, 0x5C, 0x4B, 0xC2,
				0x4B, 0x7E, 0x3C, 0x48, 0x88, 0xC3, 0x69, 0x70};
	uint8_t expectedV2[] = {0x5E, 0xC1, 0x84, 0xED, 0x45, 0x76, 0x67, 0xEC,
				0x7B, 0x4C, 0x08, 0x7E, 0xB0, 0xF9, 0x55, 0x4E};
	uint8_t extra_input[32] = {0};
	uint8_t entropy[32] = {0}; /* value not important */
	uint8_t output[32];
	TCCtrPrng_t ctx;
	uint32_t i;
	int rc;

	rc = tc_ctr_prng_init(&ctx, entropy, sizeof(entropy), 0, 0U);

	/**TESTPOINT: Check if init works*/
	zassert_equal(rc, TC_CRYPTO_SUCCESS, "CTR PRNG init failed");

	/* force internal state to max allowed count */
	ctx.reseedCount = 0x1000000000000ULL;

	rc = tc_ctr_prng_generate(&ctx, 0, 0, output, sizeof(output));

	/**TESTPOINT: Check if generate works*/
	zassert_equal(rc, TC_CRYPTO_SUCCESS, "CTR PRNG generate failed");

	/* expect further attempts to fail due to reaching reseed threshold */
	rc = tc_ctr_prng_generate(&ctx, 0, 0, output, sizeof(output));

	/**TESTPOINT: Check if generate works*/
	zassert_equal(rc, TC_CTR_PRNG_RESEED_REQ, "CTR PRNG generate failed");

	/* reseed and confirm generate works again
	 * make entropy different from original value - not really important
	 * for the purpose of this test
	 */
	(void)memset(entropy, 0xFF, sizeof(entropy));
	rc = tc_ctr_prng_reseed(&ctx, entropy, sizeof(entropy), extra_input,
				sizeof(extra_input));

	/**TESTPOINT: Recheck if the functions work*/
	zassert_equal(rc, TC_CRYPTO_SUCCESS, "CTR PRNG reseed failed");

	rc = tc_ctr_prng_generate(&ctx, 0, 0, output, sizeof(output));

	/**TESTPOINT: Check if generate works again*/
	zassert_equal(rc, TC_CRYPTO_SUCCESS, "CTR PRNG generate failed");

	/* confirm entropy and additional_input are being used correctly
	 * first, entropy only
	 */
	(void)memset(&ctx, 0x0, sizeof(ctx));
	for (i = 0U; i < sizeof(entropy); i++) {
		entropy[i] = i;
	}

	rc = tc_ctr_prng_reseed(&ctx, entropy, sizeof(entropy), 0, 0);

	/**TESTPOINT: Check if reseed works*/
	zassert_equal(rc, 1, "CTR PRNG reseed failed");

	/**TESTPOINT: Check results*/
	zassert_false(memcmp(ctx.V, expectedV1, sizeof(expectedV1)),
	"expected value different - check failed");

	/* now, entropy and additional_input */
	(void)memset(&ctx, 0x00, sizeof(ctx));
	for (i = 0U; i < sizeof(extra_input); i++) {
		extra_input[i] = i * 2U;
	}

	rc = tc_ctr_prng_reseed(&ctx, entropy, sizeof(entropy),
				 extra_input, sizeof(extra_input));

	/**TESTPOINT: Check if reseed works*/
	zassert_equal(rc, 1, "CTR PRNG reseed failed");

	/**TESTPOINT: Check results*/
	zassert_false(memcmp(ctx.V, expectedV2, sizeof(expectedV2)),
	"expected value different - check failed");

	TC_PRINT("CTR PRNG reseed test succeeded\n");
}

void test_ctr_prng_uninstantiate(void)
{
	uint8_t entropy[32] = {0}; /* value not important */
	TCCtrPrng_t ctx;
	size_t words;
	size_t i;
	int rc;

	rc = tc_ctr_prng_init(&ctx, entropy, sizeof(entropy), 0, 0);

	/**TESTPOINT: Check if init works*/
	zassert_equal(rc, TC_CRYPTO_SUCCESS, "CTR PRNG init failed");

	tc_ctr_prng_uninstantiate(&ctx);
	/* show that state has been zeroised */
	for (i = 0; i < sizeof(ctx.V); i++) {

		/**TESTPOINT: Check if states have been zeroised*/
		zassert_false(ctx.V[i], "some states have not been zeroised");
	}

	words = sizeof(ctx.key.words) / sizeof(ctx.key.words[0]);
	for (i = 0; i < words; i++) {

		/**TESTPOINT: Check words*/
		zassert_false(ctx.key.words[i],
		"expected value wrong - check failed");
	}

	/**TESTPOINT: Check if uninstantiation passed*/
	zassert_false(ctx.reseedCount, "CTR PRNG uninstantiate test failed");

	TC_PRINT("CTR PRNG uninstantiate test succeeded\n");
}

void test_ctr_prng_robustness(void)
{
	uint8_t entropy[32] = {0}; /* value not important */
	uint8_t output[32];
	TCCtrPrng_t ctx;
	int rc;

	/* show that the CTR PRNG is robust to invalid inputs */
	tc_ctr_prng_uninstantiate(0);

	rc = tc_ctr_prng_generate(&ctx, 0, 0, 0, 0);

	/**TESTPOINT: Check if invalid input test works*/
	zassert_equal(rc, TC_CRYPTO_FAIL, "CTR PRNG invalid input test failed");

	rc = tc_ctr_prng_generate(0, 0, 0, output, sizeof(output));

	/**TESTPOINT: Check if invalid input test works*/
	zassert_equal(rc, TC_CRYPTO_FAIL, "CTR PRNG invalid input test failed");

	rc = tc_ctr_prng_generate(0, 0, 0, 0, 0);

	/**TESTPOINT: Check if invalid input test works*/
	zassert_equal(rc, TC_CRYPTO_FAIL, "CTR PRNG invalid input test failed");

	rc = tc_ctr_prng_reseed(&ctx, 0, 0, 0, 0);

	/**TESTPOINT: Check if invalid input test works*/
	zassert_equal(rc, TC_CRYPTO_FAIL, "CTR PRNG invalid input test failed");

	/* too little entropy */
	rc = tc_ctr_prng_reseed(&ctx, entropy, sizeof(entropy) - 1, 0, 0);

	/**TESTPOINT: Check if invalid input test works*/
	zassert_equal(rc, TC_CRYPTO_FAIL, "CTR PRNG invalid input test failed");


	rc = tc_ctr_prng_reseed(0, entropy, sizeof(entropy), 0, 0);

	/**TESTPOINT: Check if invalid input test works*/
	zassert_equal(rc, TC_CRYPTO_FAIL, "CTR PRNG invalid input test failed");


	rc = tc_ctr_prng_reseed(0, 0, 0, 0, 0);

	/**TESTPOINT: Check if invalid input test works*/
	zassert_equal(rc, TC_CRYPTO_FAIL, "CTR PRNG invalid input test failed");


	rc = tc_ctr_prng_init(&ctx, 0, 0, 0, 0);

	/**TESTPOINT: Check if invalid input test works*/
	zassert_equal(rc, TC_CRYPTO_FAIL, "CTR PRNG invalid input test failed");

	/* too little entropy */
	rc = tc_ctr_prng_init(&ctx, entropy, sizeof(entropy) - 1, 0, 0);

	/**TESTPOINT: Check if invalid input test works*/
	zassert_equal(rc, TC_CRYPTO_FAIL, "CTR PRNG invalid input test failed");

	rc = tc_ctr_prng_init(0, entropy, sizeof(entropy), 0, 0);

	/**TESTPOINT: Check if invalid input test works*/
	zassert_equal(rc, TC_CRYPTO_FAIL, "CTR PRNG invalid input test failed");

	rc = tc_ctr_prng_init(0, 0, 0, 0, 0);

	/**TESTPOINT: Check if invalid input test works*/
	zassert_equal(rc, TC_CRYPTO_FAIL, "CTR PRNG invalid input test failed");

	TC_PRINT("CTR PRNG robustness test succeeded\n");
}

/*
 * Main task to test CTR PRNG
 */
void test_ctr_prng_vector(void)
{
	int elements;
	int rc;
	int i;

	TC_START("Performing CTR-PRNG tests:");

	elements = (int)sizeof(vectors) / sizeof(vectors[0]);
	for (i = 0; i < elements; i++) {
		rc = test_prng_vector(&vectors[i]);
		TC_PRINT("[%s] test_prng_vector #%d\n",
			 TC_RESULT_TO_STR(rc), i);

		/**TESTPOINT: Check if test passed*/
		zassert_false(rc, "CTR PRNG vector test failed");
	}
	TC_PRINT("CTR PRNG vector test succeeded\n");
}
