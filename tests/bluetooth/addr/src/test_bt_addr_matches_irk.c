/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/crypto.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

/* Sample data for the random address hash function ah, from the Bluetooth Core
 * Specification, Vol 3, Part H, Appendix D.7:
 *
 *   IRK   = ec0234a3 57c8ad05 341010a6 0a397d9b
 *   prand = 708194
 *   ah    = 0dfbaa
 *
 * The IRK and the address are stored least significant octet first, so the RPA
 * is 70:81:94:0D:FB:AA, with prand in the upper three octets and the hash in the
 * lower three.
 */
static const uint8_t spec_irk[BT_IRK_SIZE] = {
	0x9b, 0x7d, 0x39, 0x0a, 0xa6, 0x10, 0x10, 0x34,
	0x05, 0xad, 0xc8, 0x57, 0xa3, 0x34, 0x02, 0xec,
};

static const bt_addr_t spec_rpa = {{0xaa, 0xfb, 0x0d, 0x94, 0x81, 0x70}};

ZTEST_SUITE(bt_addr_matches_irk, NULL, NULL, NULL, NULL, NULL);

static ZTEST(bt_addr_matches_irk, test_spec_sample_data)
{
	zassert_true(BT_ADDR_IS_RPA(&spec_rpa), "Sample address is not an RPA");
	zassert_true(bt_addr_matches_irk(&spec_rpa, spec_irk),
		     "Sample RPA does not match the sample IRK");
}

static ZTEST(bt_addr_matches_irk, test_irk_mismatch)
{
	uint8_t irk[BT_IRK_SIZE];

	(void)memcpy(irk, spec_irk, sizeof(irk));
	irk[BT_IRK_SIZE - 1U] ^= 0x01;

	zassert_false(bt_addr_matches_irk(&spec_rpa, irk), "RPA matches a different IRK");
}

static ZTEST(bt_addr_matches_irk, test_irk_byte_order)
{
	uint8_t irk[BT_IRK_SIZE];

	for (size_t i = 0U; i < BT_IRK_SIZE; i++) {
		irk[i] = spec_irk[BT_IRK_SIZE - 1U - i];
	}

	zassert_false(bt_addr_matches_irk(&spec_rpa, irk),
		      "RPA matches the IRK in most significant octet first order");
}

static ZTEST(bt_addr_matches_irk, test_hash_mismatch)
{
	bt_addr_t rpa = spec_rpa;

	rpa.val[0] ^= 0x01;

	zassert_false(bt_addr_matches_irk(&rpa, spec_irk), "RPA with a modified hash matches");
}

static ZTEST(bt_addr_matches_irk, test_prand_mismatch)
{
	bt_addr_t rpa = spec_rpa;

	rpa.val[3] ^= 0x01;

	zassert_false(bt_addr_matches_irk(&rpa, spec_irk), "RPA with a modified prand matches");
}

static ZTEST(bt_addr_matches_irk, test_non_rpa_does_not_match)
{
	/* The sample prand with each value of the two most significant bits: 0b01 (RPA),
	 * 0b00 (non-resolvable), 0b10 (reserved) and 0b11 (static)
	 */
	const uint8_t prands[][3] = {
		{0x94, 0x81, 0x70},
		{0x94, 0x81, 0x30},
		{0x94, 0x81, 0xb0},
		{0x94, 0x81, 0xf0},
	};
	bt_addr_t addr[ARRAY_SIZE(prands)];

	for (size_t i = 0U; i < ARRAY_SIZE(prands); i++) {
		uint8_t r[16] = {0};
		uint8_t enc[16];

		/* ah(k, r) = e(k, r') mod 2^24, with r' = padding || r */
		(void)memcpy(r, prands[i], sizeof(prands[i]));
		zassert_ok(bt_encrypt_le(spec_irk, r, enc), "AES encryption failed");
		(void)memcpy(&addr[i].val[0], enc, 3U);
		(void)memcpy(&addr[i].val[3], prands[i], sizeof(prands[i]));
	}

	zassert_true(bt_addr_matches_irk(&addr[0], spec_irk),
		     "RPA with a valid hash does not match");

	for (size_t i = 1U; i < ARRAY_SIZE(prands); i++) {
		zassert_false(BT_ADDR_IS_RPA(&addr[i]), "Address %zu is an RPA", i);
		zassert_false(bt_addr_matches_irk(&addr[i], spec_irk),
			      "Address %zu, which is not an RPA, matches", i);
	}
}
