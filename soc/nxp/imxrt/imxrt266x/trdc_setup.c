/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Access control for the i.MX RT266x: hand the Resource Domain Controller over
 * from the EdgeLock enclave, then put the DMA initiators in the same domain as
 * the CPU. Both have to happen before any driver touches a peripheral.
 */

#include <zephyr/kernel.h>

#include "soc.h"

#include <fsl_common.h>
#include <fsl_trdc_soc.h>

/*
 * EdgeLock Release-RDC message (from the SDK board support). The enclave owns
 * the Resource Domain Controller after reset; until it is released, CPU accesses
 * to most peripherals bus-fault.
 */
#define ELE_RELEASE_RDC              0x17C40206U
#define ELE_RELEASE_RDC_RESPONSE_HDR 0xE1C40206U
#define ELE_RELEASE_RDC_ALL          0xAAU
#define ELE_RESPONSE_SUCCESS         0xD6U
#define ELE_RESPONSE_ALREADY_GRANTED 0xD329U

static void soc_ele_release_rdc(void)
{
	S3MU_Type *const mu = MAIN__SENTMU0_SENTMUA;
	uint32_t response[2];

	SOC_POLL_UNTIL((mu->TSR & S3MU_TSR_TEn(1U << 0)) != 0U, SOC_STEP_ELE_SEND);
	mu->TR[0] = ELE_RELEASE_RDC;
	SOC_POLL_UNTIL((mu->TSR & S3MU_TSR_TEn(1U << 1)) != 0U, SOC_STEP_ELE_SEND);
	mu->TR[1] = ((uint32_t)ELE_RELEASE_RDC_ALL << 8) | 0x1U;

	SOC_POLL_UNTIL((mu->RSR & S3MU_RSR_RFn(1U << 0)) != 0U, SOC_STEP_ELE_RECEIVE);
	response[0] = mu->RR[0];
	SOC_POLL_UNTIL((mu->RSR & S3MU_RSR_RFn(1U << 1)) != 0U, SOC_STEP_ELE_RECEIVE);
	response[1] = mu->RR[1];

	if ((response[0] != ELE_RELEASE_RDC_RESPONSE_HDR) ||
	    ((response[1] != ELE_RESPONSE_SUCCESS) &&
	     (response[1] != ELE_RESPONSE_ALREADY_GRANTED))) {
		/*
		 * Continuing would turn one identifiable failure into a bus
		 * fault in whichever driver happens to touch a peripheral
		 * first.
		 */
		soc_early_init_failed(SOC_STEP_ELE_REFUSED);
	}
}

/*
 * Give the MAIN eDMA masters the CPU's access-control domain, so a DMA initiator
 * sees the same view of the RDC-released peripherals and memories that the CPU
 * does. CPU0's own AXIM/AHBP ports are already in domain 0.
 *
 * The TRDC has one master per channel PAIR, and an unassigned pair is denied
 * silently -- the transfer completes and moves nothing -- so every pair of both
 * controllers is assigned rather than just the ones a given driver happens to
 * use. Masters 0..7 are eDMA5's 16 channels, 8..23 are eDMA3's 32.
 */
static void soc_trdc_assign_dma_masters(void)
{
	for (uint8_t master = kTRDC_MAIN_MasterEDMA5Ch0_1; master <= kTRDC_MAIN_MasterMEDMA3Ch30_31;
	     master++) {
		MAIN__TRDC->MDA_DFMT1[master].MDA_W_DFMT1[0] =
			TRDC_MDA_W_DFMT1_DID(0) | TRDC_MDA_W_DFMT1_VLD_MASK;
	}
}

void soc_trdc_setup(void)
{
	/* Release first: every step after it is itself a peripheral access. */
	soc_ele_release_rdc();
	soc_trdc_assign_dma_masters();
}
