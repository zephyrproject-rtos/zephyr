/*
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 * Copyright (C) 2010-2020 ARM Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr.h>
#include <stdlib.h>
#include <arm_math.h>
#include "../../common/benchmark_common.h"

#define PATTERN_LENGTH	(256)

static const q31_t input1[256] = {
	0xC631366A, 0xFB13DDA9, 0xEED09227, 0xD28B3673,
	0xE2196135, 0xF374965D, 0x02ACCA0C, 0xCB7C49FF,
	0x07379279, 0x1447DB6D, 0xC6573A28, 0x9B58C226,
	0x28DFD755, 0x3D3B07A2, 0x0D68B78C, 0xD7DCA4EE,
	0xDB0C855B, 0x023602B6, 0xF916B096, 0xEBF0F01A,
	0xEF2088F4, 0x271B8868, 0x27D08994, 0xD6A88ADC,
	0x32EC53E8, 0xE81E138E, 0xCD458FEA, 0xB954E128,
	0x1EDE0E95, 0xF552AE24, 0xE50ED7CD, 0xEA8006A8,
	0xD15FDC5B, 0xE6664B86, 0xF2D00F35, 0x580806BE,
	0xDEABF04F, 0x0ED614B8, 0x02DD4DB0, 0x31168702,
	0xE8E1C9F2, 0x057B8340, 0x140401B2, 0xE9CD8CA6,
	0xF32B25C9, 0xD6E1A540, 0x1F2B6682, 0xB50095B3,
	0x9F6DCA67, 0xE4B450BB, 0x4EB1B95A, 0xDB7BFF93,
	0xCBEAEC9B, 0x21F416A1, 0xCDE2CCB1, 0xBD7E7949,
	0x1FA7DD36, 0xE053411E, 0x11C4257F, 0xED7C5D35,
	0xC19ADE19, 0xCE19D0C7, 0xF74B2CB9, 0x2A399C7B,
	0x0535E354, 0xFBF6A6C0, 0x20863CC0, 0xDB69DB37,
	0xB4F29C11, 0xD6B22F7B, 0x038B9816, 0xE3C682A0,
	0xEDCBB8B7, 0xDE7C72C9, 0x32F8DD8B, 0x3A95873C,
	0xF2111759, 0xF7DA7E6A, 0xED96D85D, 0xD2362CAF,
	0xF2E7EF09, 0xD4AC5EF7, 0xFBC85EF5, 0x0C1C43AB,
	0x0DF7FEB7, 0x3E65BFDA, 0xBBEFE59B, 0xE9684971,
	0xD0395A63, 0x00748F9A, 0x0F3489D3, 0x040CB837,
	0xFBF33C8F, 0xF3071033, 0x21D0FB32, 0x032EEE15,
	0x0DD08506, 0xE353BDD5, 0x1DF580B8, 0x29D7206E,
	0xBB37A59D, 0x1C10046B, 0xE45E09A1, 0xF9905A90,
	0xEB10CB71, 0x0B15EBA8, 0x085EF241, 0x38FE421D,
	0xBDA824C3, 0xF4F75651, 0xEC519026, 0x37A99EC2,
	0xEAD68528, 0xF4AAD39D, 0x163E8F6E, 0xEE264172,
	0x18138FDC, 0xEED062BC, 0x4B543E58, 0x1A6C1F71,
	0xEDC2E5D1, 0x45451847, 0xDD23449A, 0x23DAB3A8,
	0xFEF8783A, 0x5F9B1AB0, 0x19217DD4, 0x1EA54ED7,
	0xE0BDADFE, 0xEDA12547, 0xFCD4F5D8, 0xE9FE19F6,
	0x0442389E, 0xFE9C42A5, 0xF509E355, 0x2AE61FA0,
	0x1BC01C55, 0xDE523096, 0x1E3AADDC, 0x0C2E1F51,
	0xD86F78CD, 0x327333F7, 0x19F2138F, 0x2F9F42BC,
	0xE3E5BD9B, 0xEF39864E, 0xE856DC90, 0xEF27A130,
	0xEBC4DCE8, 0x0F708DD1, 0x2778EC5B, 0xFA3C037A,
	0xC065422F, 0xDDE79F2C, 0xE880E4F0, 0xB14CE586,
	0x091F7AC1, 0xD6929567, 0x24C4425C, 0x100FEC70,
	0x0D2B053C, 0x23FACA44, 0xF99AAE94, 0xC135C785,
	0xFC28C4F4, 0x10D76869, 0x0A7B1272, 0x10608353,
	0x2E9C08B0, 0x59A18ED8, 0xF0D49846, 0xC8D1A81B,
	0x1BBDF0B6, 0xF289F305, 0x05E74FEC, 0x27EBFC25,
	0xF4EA822B, 0x0CB43282, 0x19B782A9, 0x233C62FC,
	0xE8EDF38F, 0x025E93FD, 0xF7D7D282, 0xA675C383,
	0x0171EB58, 0xCB893E3C, 0xEFB60317, 0xFB72B6EF,
	0xF05A1137, 0x42ACFE0E, 0x25EB1D6B, 0x1C9D26B4,
	0x215B4D22, 0xE1C1E29B, 0x3B3E68FD, 0xBFE233EE,
	0x336C6C8E, 0x079D9442, 0x097E9C6B, 0xF3C69D03,
	0xDC026526, 0x0C6A4F89, 0x1063CA94, 0x093E62E9,
	0x21F1CD33, 0x08991A66, 0x03385438, 0xEE1A0BD8,
	0xDD01E7C7, 0x2223F95A, 0xDECC8D24, 0xEC2DEE81,
	0xE5CB797B, 0xD73940C9, 0x2A6D5443, 0x347F86DD,
	0xF3950FA3, 0xDC9AB3D4, 0xDBE1D805, 0xFB6B0E5C,
	0x207C019F, 0xF1F00F8F, 0xED3E7606, 0x470168BA,
	0xF3061229, 0xD3526760, 0x0F2D08F3, 0x97CDCF77,
	0xC2D5A7AA, 0xE7752C0B, 0xECCE8901, 0x0BFDE47E,
	0x4CACC995, 0x0221E381, 0xE43CD3B4, 0xF2E1262A,
	0x18D68649, 0x07C9883C, 0x07239928, 0xC62A1170,
	0x24F5B0E5, 0x02A9DF50, 0x03E2DA18, 0xF06623E6,
	0xED03EB89, 0x1DC68DE4, 0x225EF5AE, 0x48005603,
	0x4C0CEE5E, 0xFE56170E, 0x80000000, 0x057AA227,
	0x0E600876, 0xFD1D866A, 0xEA74C1DB, 0x22ED63F0
	};

static const q31_t input2[256] = {
	0x1C0A13BC, 0x1B873800, 0xE34CB773, 0xDA6DADAE,
	0xFF8618DF, 0xF79C1734, 0x087D8481, 0x21A431EA,
	0xF840AF7D, 0x0AE2BCEE, 0x2A582599, 0x19EC693D,
	0x091B03C7, 0x4E3E7131, 0xF462C8D3, 0xFEC29627,
	0x0824B403, 0xE5605B52, 0x0FD08240, 0x0CDE1B94,
	0xCE1148E2, 0x1160A638, 0xB583AAE1, 0x44B1F71F,
	0x265DF7F3, 0xD4F1E9B5, 0x2EE474D4, 0xCD383FED,
	0x36A03599, 0x0D833B71, 0xCAF5999D, 0xDA601039,
	0x3D1BA57B, 0x12CA8626, 0x12B5DB84, 0xE7E396C3,
	0xDE5B5D5C, 0x0DA9623A, 0x1E4CD13C, 0x2AE25F57,
	0xCE7D118B, 0x1D17F86A, 0x30F5A933, 0xEFCC13E2,
	0xDBC3AD5F, 0x2BB33845, 0x0FD0F0FD, 0xD643FCCC,
	0xF6476F7F, 0xF1F1F7BD, 0xBA683437, 0xE87FCB22,
	0x210DD0F6, 0xF0738E7F, 0xF61B2952, 0xF9A85CFD,
	0xEF980CC1, 0x1C78B775, 0x5D937EC5, 0xEA54C61D,
	0x0B8AF897, 0xC9C3B40E, 0x1DBCFF62, 0xF1A1866F,
	0xCFE278AE, 0x04844959, 0x1A821BBA, 0xEFEC5903,
	0xFE724C57, 0x1FBFA58E, 0xBDBA24C0, 0x1489DE32,
	0xF0B04CF9, 0x03F7C82E, 0xFA6DB38E, 0xDF0EF9CB,
	0x057E618A, 0x3ADA9765, 0xDC214567, 0xBEFAEC05,
	0x07C36015, 0xD506010D, 0x23FC80EA, 0x1019EB8A,
	0xFC3FA8E9, 0xEF70F6CF, 0xC2E534C0, 0x00CA86AB,
	0x359CB10A, 0xCEEED4CD, 0xF7A108E0, 0xEE58A199,
	0xE9DB5FCC, 0xEC75497C, 0xDECA4BD8, 0xE9B39B39,
	0x24DC6736, 0xF58219E5, 0x18F2A349, 0x2DB6B98D,
	0x32F3CB95, 0xF7B5D2EB, 0xF2D98779, 0x0738182A,
	0xE91D0A75, 0xF0A3271F, 0xDC4338E3, 0x320EF7F1,
	0xA5F51F14, 0x229D5EB5, 0xE340F852, 0x109E486D,
	0x265AABD0, 0x00D30A8B, 0xFE0E4A39, 0xF211B88A,
	0x2684E20F, 0xF05DD624, 0xFBB526FD, 0x33BBC360,
	0x16BDF629, 0xEE2B449E, 0x0DBE4FE2, 0x176744B2,
	0xBCAE90B1, 0xD7506581, 0x0E084745, 0xF48548B8,
	0x0F7B2E33, 0x1C048268, 0x1DA01712, 0x0BD9FFFF,
	0x09071057, 0x15C78815, 0xFEF31ACF, 0x20A46B7F,
	0x1A201E7D, 0xC99A8A86, 0x07EB6EFA, 0x0C51BB67,
	0xE19171BB, 0xED7FB395, 0x2139EDC4, 0xF7B8731A,
	0x3147704D, 0x00CDE1E8, 0x0BDCE7A3, 0xDC6E4D8E,
	0xE472432E, 0xFBA6F1C2, 0xFA6ADCE7, 0xED4397A2,
	0xD91373D0, 0xFF761BFB, 0x80000000, 0x13A8BF6C,
	0x0A435241, 0x050FC71E, 0xE9FBFD16, 0xD7551A22,
	0xEA4BBE90, 0xE04F3D13, 0xEB821D9A, 0x0E3A1F7D,
	0xE497D3E1, 0xBD1DEBEF, 0x20A89097, 0xD3FCF04F,
	0xE0CF2CE8, 0xEAD4AB1D, 0xD1FE455F, 0x3826A092,
	0x05F55C1A, 0x02460DAC, 0xF2605E45, 0xF0E2F7E9,
	0x055B6EFF, 0x15026E0E, 0x0CE37D91, 0x0A6608DF,
	0x0816BED3, 0xFDCA18C4, 0x11A9FA87, 0xE897E122,
	0x15F7DEC7, 0x00BCD0F5, 0x08A61E81, 0xED1B06BF,
	0xC660A3D6, 0xD96AACC4, 0x108D13EB, 0xE04D11F8,
	0x10D1ECA1, 0xF79C5AA0, 0xF4EEB4A7, 0xFDA8B0D4,
	0xD7E48BEC, 0xC34688EB, 0x0B8D245B, 0x252EB410,
	0x23D68826, 0xF6684A2F, 0xEACB4E33, 0xFDDA246D,
	0xDCD3CBD5, 0x014DACE7, 0xD34FCA72, 0x0768A475,
	0x121B47FE, 0xFABBB9E9, 0xE1B27B22, 0xD2C319EF,
	0x08BD722A, 0xF3586DAE, 0xF2CF2D0D, 0xCB98E626,
	0xFAB40975, 0xF74793E7, 0x0E4FAECF, 0xCBF853B2,
	0xBD58E24C, 0x165711EA, 0xDB490DB3, 0x0CEE2B58,
	0x1C2BDE43, 0xDF5E5585, 0xE0E245B1, 0x1D12CF16,
	0xFA6030C3, 0x00202A46, 0xE7D2A60B, 0xE92A0C14,
	0xF5CC7899, 0x273C7A64, 0x1F8B8D48, 0xEF1B951A,
	0xEE0B4B6C, 0xC08FA7FB, 0xF7625189, 0x2FBE33F9,
	0x25F96B97, 0xE4079AFF, 0x05B10472, 0x2743154D,
	0x1733D292, 0x0C21E583, 0x28EB1125, 0x2861780A,
	0xF297AE48, 0x311766BE, 0xEDF26EF4, 0xD4B0C893,
	0x293701E2, 0xC0D85C67, 0x06D39B8C, 0x0B7E6C0C
	};

void test_benchmark_vec_add_q31(void)
{
	uint32_t irq_key, timestamp, timespan;
	q31_t *output;

	/* Allocate output buffer */
	output = malloc(PATTERN_LENGTH * sizeof(q31_t));
	zassert_not_null(output, "output buffer allocation failed");

	/* Begin benchmark */
	benchmark_begin(&irq_key, &timestamp);

	/* Execute function */
	arm_add_q31(input1, input2, output, PATTERN_LENGTH);

	/* End benchmark */
	timespan = benchmark_end(irq_key, timestamp);

	/* Free output buffer */
	free(output);

	/* Print result */
	TC_PRINT(BENCHMARK_TYPE " = %u\n", timespan);
}

void test_benchmark_vec_sub_q31(void)
{
	uint32_t irq_key, timestamp, timespan;
	q31_t *output;

	/* Allocate output buffer */
	output = malloc(PATTERN_LENGTH * sizeof(q31_t));
	zassert_not_null(output, "output buffer allocation failed");

	/* Begin benchmark */
	benchmark_begin(&irq_key, &timestamp);

	/* Execute function */
	arm_sub_q31(input1, input2, output, PATTERN_LENGTH);

	/* End benchmark */
	timespan = benchmark_end(irq_key, timestamp);

	/* Free output buffer */
	free(output);

	/* Print result */
	TC_PRINT(BENCHMARK_TYPE " = %u\n", timespan);
}

void test_benchmark_vec_mult_q31(void)
{
	uint32_t irq_key, timestamp, timespan;
	q31_t *output;

	/* Allocate output buffer */
	output = malloc(PATTERN_LENGTH * sizeof(q31_t));
	zassert_not_null(output, "output buffer allocation failed");

	/* Begin benchmark */
	benchmark_begin(&irq_key, &timestamp);

	/* Execute function */
	arm_mult_q31(input1, input2, output, PATTERN_LENGTH);

	/* End benchmark */
	timespan = benchmark_end(irq_key, timestamp);

	/* Free output buffer */
	free(output);

	/* Print result */
	TC_PRINT(BENCHMARK_TYPE " = %u\n", timespan);
}

void test_benchmark_vec_abs_q31(void)
{
	uint32_t irq_key, timestamp, timespan;
	q31_t *output;

	/* Allocate output buffer */
	output = malloc(PATTERN_LENGTH * sizeof(q31_t));
	zassert_not_null(output, "output buffer allocation failed");

	/* Begin benchmark */
	benchmark_begin(&irq_key, &timestamp);

	/* Execute function */
	arm_abs_q31(input1, output, PATTERN_LENGTH);

	/* End benchmark */
	timespan = benchmark_end(irq_key, timestamp);

	/* Free output buffer */
	free(output);

	/* Print result */
	TC_PRINT(BENCHMARK_TYPE " = %u\n", timespan);
}

void test_benchmark_vec_negate_q31(void)
{
	uint32_t irq_key, timestamp, timespan;
	q31_t *output;

	/* Allocate output buffer */
	output = malloc(PATTERN_LENGTH * sizeof(q31_t));
	zassert_not_null(output, "output buffer allocation failed");

	/* Begin benchmark */
	benchmark_begin(&irq_key, &timestamp);

	/* Execute function */
	arm_negate_q31(input1, output, PATTERN_LENGTH);

	/* End benchmark */
	timespan = benchmark_end(irq_key, timestamp);

	/* Free output buffer */
	free(output);

	/* Print result */
	TC_PRINT(BENCHMARK_TYPE " = %u\n", timespan);
}

void test_benchmark_vec_offset_q31(void)
{
	uint32_t irq_key, timestamp, timespan;
	q31_t *output;

	/* Allocate output buffer */
	output = malloc(PATTERN_LENGTH * sizeof(q31_t));
	zassert_not_null(output, "output buffer allocation failed");

	/* Begin benchmark */
	benchmark_begin(&irq_key, &timestamp);

	/* Execute function */
	arm_offset_q31(input1, 1.0, output, PATTERN_LENGTH);

	/* End benchmark */
	timespan = benchmark_end(irq_key, timestamp);

	/* Free output buffer */
	free(output);

	/* Print result */
	TC_PRINT(BENCHMARK_TYPE " = %u\n", timespan);
}

void test_benchmark_vec_scale_q31(void)
{
	uint32_t irq_key, timestamp, timespan;
	q31_t *output;

	/* Allocate output buffer */
	output = malloc(PATTERN_LENGTH * sizeof(q31_t));
	zassert_not_null(output, "output buffer allocation failed");

	/* Begin benchmark */
	benchmark_begin(&irq_key, &timestamp);

	/* Execute function */
	arm_scale_q31(input1, 0x45, 1, output, PATTERN_LENGTH);

	/* End benchmark */
	timespan = benchmark_end(irq_key, timestamp);

	/* Free output buffer */
	free(output);

	/* Print result */
	TC_PRINT(BENCHMARK_TYPE " = %u\n", timespan);
}

void test_benchmark_vec_dot_prod_q31(void)
{
	uint32_t irq_key, timestamp, timespan;
	q63_t output;

	/* Begin benchmark */
	benchmark_begin(&irq_key, &timestamp);

	/* Execute function */
	arm_dot_prod_q31(input1, input2, PATTERN_LENGTH, &output);

	/* End benchmark */
	timespan = benchmark_end(irq_key, timestamp);

	/* Print result */
	TC_PRINT(BENCHMARK_TYPE " = %u\n", timespan);
}

void benchmark_basicmath_q31(void)
{
	ztest_test_suite(basicmath_q31_benchmark,
		ztest_unit_test(test_benchmark_vec_add_q31),
		ztest_unit_test(test_benchmark_vec_sub_q31),
		ztest_unit_test(test_benchmark_vec_mult_q31),
		ztest_unit_test(test_benchmark_vec_abs_q31),
		ztest_unit_test(test_benchmark_vec_negate_q31),
		ztest_unit_test(test_benchmark_vec_offset_q31),
		ztest_unit_test(test_benchmark_vec_scale_q31),
		ztest_unit_test(test_benchmark_vec_dot_prod_q31)
		);

	ztest_run_test_suite(basicmath_q31_benchmark);
}
