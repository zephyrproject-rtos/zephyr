/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/cs.h>
#include <zephyr/fff.h>

DEFINE_FFF_GLOBALS;

ZTEST_SUITE(bt_le_cs_parse_pct, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test success case
 *
 *  Constraints:
 *   - Valid PCT is passed in
 *
 *  Expected behaviour:
 *   - IQ term matches expected values
 */
ZTEST(bt_le_cs_parse_pct, test_parsing_success)
{
	struct bt_le_cs_iq_sample iq;

	struct {
		uint8_t input[3];
		struct bt_le_cs_iq_sample output;
	} test_vector[] = {
		/* Edge cases */
		{.input = {0x00, 0x00, 0x00}, .output = {.i = 0, .q = 0}},
		{.input = {0xFF, 0xFF, 0xFF}, .output = {.i = -1, .q = -1}},
		{.input = {0xFF, 0x00, 0xFF}, .output = {.i = 255, .q = -16}},
		{.input = {0xFF, 0x00, 0x00}, .output = {.i = 255, .q = 0}},
		{.input = {0x00, 0xFF, 0x00}, .output = {.i = -256, .q = 15}},
		{.input = {0x00, 0x00, 0xFF}, .output = {.i = 0, .q = -16}},
		{.input = {0x00, 0x08, 0x80}, .output = {.i = -2048, .q = -2048}},
		{.input = {0xFF, 0xF7, 0x7F}, .output = {.i = 2047, .q = 2047}},

		/* Randomly generated using python */
		{.input = {0xEF, 0xCD, 0xAB}, .output = {.i = -529, .q = -1348}},
		{.input = {0x30, 0x75, 0x44}, .output = {.i = 1328, .q = 1095}},
		{.input = {0x46, 0x5D, 0xEB}, .output = {.i = -698, .q = -331}},
		{.input = {0xE8, 0x14, 0x45}, .output = {.i = 1256, .q = 1105}},
		{.input = {0x23, 0xCA, 0x5C}, .output = {.i = -1501, .q = 1484}},
		{.input = {0x68, 0xA0, 0x15}, .output = {.i = 104, .q = 346}},
		{.input = {0x39, 0x73, 0x1B}, .output = {.i = 825, .q = 439}},
		{.input = {0x23, 0x72, 0x3D}, .output = {.i = 547, .q = 983}},
		{.input = {0xF5, 0xF8, 0x3D}, .output = {.i = -1803, .q = 991}},
		{.input = {0xF7, 0xB4, 0xB9}, .output = {.i = 1271, .q = -1125}},
		{.input = {0x61, 0x9F, 0xD5}, .output = {.i = -159, .q = -679}},
		{.input = {0x9B, 0x21, 0xC6}, .output = {.i = 411, .q = -926}},
		{.input = {0x14, 0x86, 0x0F}, .output = {.i = 1556, .q = 248}},
		{.input = {0x8E, 0xBB, 0xC6}, .output = {.i = -1138, .q = -917}},
		{.input = {0x5B, 0xD1, 0xC2}, .output = {.i = 347, .q = -979}},
		{.input = {0x99, 0x4A, 0x28}, .output = {.i = -1383, .q = 644}},
		{.input = {0x32, 0x16, 0x2B}, .output = {.i = 1586, .q = 689}},
		{.input = {0x3E, 0x8C, 0xD4}, .output = {.i = -962, .q = -696}},
		{.input = {0x2B, 0x1F, 0x95}, .output = {.i = -213, .q = -1711}},
		{.input = {0x22, 0xE6, 0xD6}, .output = {.i = 1570, .q = -658}},
		{.input = {0x0B, 0x31, 0xD6}, .output = {.i = 267, .q = -669}},
		{.input = {0x1B, 0x98, 0x9D}, .output = {.i = -2021, .q = -1575}},
		{.input = {0x8E, 0x97, 0x63}, .output = {.i = 1934, .q = 1593}},
		{.input = {0x97, 0x91, 0x8D}, .output = {.i = 407, .q = -1831}},
		{.input = {0x67, 0xF7, 0x1F}, .output = {.i = 1895, .q = 511}},
		{.input = {0xD6, 0x5C, 0x23}, .output = {.i = -810, .q = 565}},
		{.input = {0x92, 0xD3, 0x0B}, .output = {.i = 914, .q = 189}},
		{.input = {0xE8, 0xF3, 0x23}, .output = {.i = 1000, .q = 575}},
		{.input = {0xE6, 0xE3, 0xAD}, .output = {.i = 998, .q = -1314}},
		{.input = {0x6E, 0x70, 0xA9}, .output = {.i = 110, .q = -1385}},
		{.input = {0x63, 0x65, 0x28}, .output = {.i = 1379, .q = 646}},
		{.input = {0x27, 0x0F, 0x32}, .output = {.i = -217, .q = 800}},
		{.input = {0x3F, 0x8C, 0xE1}, .output = {.i = -961, .q = -488}},
		{.input = {0x4E, 0x86, 0xAA}, .output = {.i = 1614, .q = -1368}},
		{.input = {0x9E, 0xD1, 0xF6}, .output = {.i = 414, .q = -147}},
		{.input = {0x86, 0x09, 0x56}, .output = {.i = -1658, .q = 1376}},
		{.input = {0xFF, 0x09, 0x41}, .output = {.i = -1537, .q = 1040}},
		{.input = {0x89, 0xC5, 0x1F}, .output = {.i = 1417, .q = 508}},
		{.input = {0x1A, 0xE2, 0x9A}, .output = {.i = 538, .q = -1618}},
		{.input = {0x7E, 0x03, 0xB8}, .output = {.i = 894, .q = -1152}},
		{.input = {0x5E, 0x28, 0xB3}, .output = {.i = -1954, .q = -1230}},
		{.input = {0xFF, 0x50, 0xF0}, .output = {.i = 255, .q = -251}},
		{.input = {0xB0, 0x07, 0x87}, .output = {.i = 1968, .q = -1936}},
		{.input = {0x7E, 0xD7, 0x0C}, .output = {.i = 1918, .q = 205}},
		{.input = {0x26, 0xA2, 0xC9}, .output = {.i = 550, .q = -870}},
		{.input = {0x97, 0x71, 0x72}, .output = {.i = 407, .q = 1831}},
		{.input = {0x73, 0x0E, 0xC1}, .output = {.i = -397, .q = -1008}},
		{.input = {0xAC, 0x20, 0x6B}, .output = {.i = 172, .q = 1714}},
		{.input = {0x85, 0x7D, 0xB4}, .output = {.i = -635, .q = -1209}},
		{.input = {0xCC, 0xE3, 0x1B}, .output = {.i = 972, .q = 446}},
		{.input = {0x88, 0x48, 0x65}, .output = {.i = -1912, .q = 1620}},
	};

	for (uint16_t k = 0; k < ARRAY_SIZE(test_vector); k++) {
		iq = bt_le_cs_parse_pct(test_vector[k].input);

		zassert_equal(iq.i, test_vector[k].output.i,
			      "Failed for k = %u, expected %d, not %d", k, test_vector[k].output.i,
			      iq.i);
		zassert_equal(iq.q, test_vector[k].output.q,
			      "Failed for k = %u, expected %d, not %d", k, test_vector[k].output.q,
			      iq.q);
	}
}
