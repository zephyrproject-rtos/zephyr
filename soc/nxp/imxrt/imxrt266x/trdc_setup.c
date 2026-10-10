/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Access control for the i.MX RT266x: hand the Resource Domain Controller over
 * from the EdgeLock enclave, then put every bus-master initiator in the same
 * domain as the CPU. Both have to happen before any driver touches a peripheral.
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
 * Give every TRDC master (MAIN / CMPT / WAKE / AUDIO / COMM / MEDIA) the CPU's
 * access-control domain, so every initiator's transfers see the same view of
 * the RDC-released peripherals and memories that the CPU does. CPU0's own
 * AXIM/AHBP ports (CMPT masters 0, 1) are already in domain 0.
 *
 * Mirrors BOARD_ConfigTRDC()'s exhaustive branch in the SDK board support,
 * which is what actually runs there: board.h defines
 * BOARD_TRDC_ALL_MASTER_TO_PREVELEGE_DOMAIN to 1 unless a build overrides it.
 * BOARD_CommonSetting() calls BOARD_InitBootClocks() before BOARD_ConfigTRDC()
 * for the same reason this has to run after soc_clock_init(): MEDIA__TRDC sits
 * on the MEDIA domain bus, which mediabus_rootclk (CGU slice 32) has to be
 * clocking before anything on that bus is accessible -- writing it any earlier
 * stalls the bus transaction indefinitely and hangs the core.
 *
 * The TRDC has one master per channel PAIR on the eDMA controllers, and an
 * unassigned master is denied silently -- the transfer completes and moves
 * nothing -- so every master is assigned rather than just the ones a given
 * Zephyr driver happens to use today. Upper bounds are each block's last
 * trdc_master_t enumerator (fsl_trdc_soc.h); ranges are contiguous except for
 * MAIN's five reserved slots.
 */
void soc_trdc_assign_masters(void)
{
	for (uint32_t master = 0U; master <= (uint32_t)kTRDC_MAIN_MasterTESTPORT; master++) {
		if (master == (uint32_t)kTRDC_MAIN_MasterReserved0 ||
		    master == (uint32_t)kTRDC_MAIN_MasterReserved1 ||
		    master == (uint32_t)kTRDC_MAIN_MasterReserved2 ||
		    master == (uint32_t)kTRDC_MAIN_MasterReserved3 ||
		    master == (uint32_t)kTRDC_MAIN_MasterReserved4) {
			continue;
		}
		MAIN__TRDC->MDA_DFMT1[master].MDA_W_DFMT1[0] =
			TRDC_MDA_W_DFMT1_DID(0) | TRDC_MDA_W_DFMT1_VLD_MASK;
	}

	/* CPU0_AXIM/CPU0_AHBP (0, 1) are the CPU's own ports; Reserved0/1 (2, 3)
	 * do not exist.
	 */
	for (uint32_t master = (uint32_t)kTRDC_CMPT_MasterLLC_RD;
	     master <= (uint32_t)kTRDC_CMPT_MasterNPU; master++) {
		CMPT__TRDC->MDA_DFMT1[master].MDA_W_DFMT1[0] =
			TRDC_CMPT_MDA_W_DFMT1_DID(0) | TRDC_CMPT_MDA_W_DFMT1_VLD_MASK;
	}

	for (uint32_t master = 0U; master <= (uint32_t)kTRDC_WAKE_MasterWEDMA3Ch0_4; master++) {
		WAKE__TRDC->MDA_DFMT1[master].MDA_W_DFMT1[0] =
			TRDC_MDA_W_DFMT1_DID(0) | TRDC_MDA_W_DFMT1_VLD_MASK;
	}

	for (uint32_t master = 0U; master <= (uint32_t)kTRDC_AUDIO_MasterAEDMA3Ch0_4; master++) {
		AUDIO__TRDC->MDA_DFMT1[master].MDA_W_DFMT1[0] =
			TRDC_MDA_W_DFMT1_DID(0) | TRDC_MDA_W_DFMT1_VLD_MASK;
	}

	for (uint32_t master = 0U; master <= (uint32_t)kTRDC_COMM_MasterXSPI_RESP; master++) {
		COMM__TRDC->MDA_DFMT1[master].MDA_W_DFMT1[0] =
			TRDC_MDA_W_DFMT1_DID(0) | TRDC_MDA_W_DFMT1_VLD_MASK;
	}

	for (uint32_t master = 0U; master <= (uint32_t)kTRDC_MEDIA_MasterJPEG; master++) {
		MEDIA__TRDC->MDA_DFMT1[master].MDA_W_DFMT1[0] =
			TRDC_MDA_W_DFMT1_DID(0) | TRDC_MDA_W_DFMT1_VLD_MASK;
	}
}

/*
 * Hand the Resource Domain Controller over from the EdgeLock enclave. Must run
 * before any peripheral access; soc_trdc_assign_masters() is the rest of TRDC
 * setup and runs later, once clocks are up (see its own comment).
 */
void soc_trdc_setup(void)
{
	soc_ele_release_rdc();
}
