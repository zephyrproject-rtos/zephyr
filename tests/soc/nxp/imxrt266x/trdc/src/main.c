/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/ztest.h>

#include <fsl_common.h>
#include <fsl_trdc_soc.h>

static void check_master(const volatile uint32_t *assignment, uint32_t expected,
			 const char *path)
{
	uint32_t actual = *assignment;

	zassert_equal(actual, expected, "Assignment mismatch for %s: actual=0x%08x expected=0x%08x",
		      path, actual, expected);
}

/* Readback includes the hardware's non-core master indicator. */
#define CHECK_MASTER(node, type, did, valid, non_core) \
	check_master(&((type *)DT_REG_ADDR(DT_PARENT(node))) \
			      ->MDA_DFMT1[DT_REG_ADDR_RAW(node)].MDA_W_DFMT1[0], \
		     did(DT_PROP(node, domain_id)) | (valid) | (non_core), DT_NODE_PATH(node));

#define CHECK_CONTROLLER(label) \
	DT_FOREACH_CHILD_STATUS_OKAY_VARGS(DT_NODELABEL(label), CHECK_MASTER, TRDC_Type, \
					  TRDC_MDA_W_DFMT1_DID, TRDC_MDA_W_DFMT1_VLD_MASK, \
					  TRDC_MDA_W_DFMT1_DFMT_MASK)

ZTEST(trdc, test_master_domains)
{
	CHECK_CONTROLLER(trdc_main)
	DT_FOREACH_CHILD_STATUS_OKAY_VARGS(DT_NODELABEL(trdc_cmpt), CHECK_MASTER, TRDC_CMPT_Type,
					  TRDC_CMPT_MDA_W_DFMT1_DID,
					  TRDC_CMPT_MDA_W_DFMT1_VLD_MASK,
					  TRDC_CMPT_MDA_W_DFMT1_DFMT_MASK)
	CHECK_CONTROLLER(trdc_wake)
	CHECK_CONTROLLER(trdc_audio)
	CHECK_CONTROLLER(trdc_comm)
	CHECK_CONTROLLER(trdc_media)
}

ZTEST(trdc, test_dma_pair_indices)
{
	zassert_equal(DT_REG_ADDR_RAW(DT_NODELABEL(trdc_main_edma5_ch0_1)),
		      kTRDC_MAIN_MasterEDMA5Ch0_1);
	zassert_equal(DT_REG_ADDR_RAW(DT_NODELABEL(trdc_main_edma5_ch2_3)),
		      kTRDC_MAIN_MasterEDMA5Ch2_3);
	zassert_equal(DT_REG_ADDR_RAW(DT_NODELABEL(trdc_main_medma3_ch0_1)),
		      kTRDC_MAIN_MasterMEDMA3Ch0_1);
}

ZTEST_SUITE(trdc, NULL, NULL, NULL, NULL, NULL);
