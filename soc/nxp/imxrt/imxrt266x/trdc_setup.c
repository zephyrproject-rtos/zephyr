/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Access control for the i.MX RT266x: hand the Resource Domain Controller over
 * from the EdgeLock enclave, then apply the devicetree master assignments.
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

struct trdc_master_config {
	uint32_t master;
	uint32_t domain;
};

/* Convert one devicetree child into data; no register accesses occur here. */
#define TRDC_MASTER_ENTRY(node) \
	{ .master = DT_REG_ADDR_RAW(node), .domain = DT_PROP(node, domain_id) },

/* OR one child's MDAC bit into the controller bitmap. */
#define TRDC_MASTER_BIT(node) | BIT64(DT_REG_ADDR_RAW(node))

/* One bit per MDAC, used to detect repeated indices in an assignment table. */
#define TRDC_MASTER_MASK(node) (0ULL DT_FOREACH_CHILD_STATUS_OKAY(node, TRDC_MASTER_BIT))

/* valid is the bitmask of this controller's implemented non-core master MDACs. */
#define TRDC_CHECK_MASTER(node, valid)							\
	BUILD_ASSERT(DT_PROP(node, domain_id) < FSL_FEATURE_TRDC_DOMAIN_COUNT,		\
		     "TRDC domain ID is not supported by this SoC");			\
	BUILD_ASSERT(DT_NUM_REGS(node) == 1, "A TRDC master must have one MDAC index");	\
	BUILD_ASSERT(DT_REG_ADDR_RAW(node) < 64, "TRDC master index out of range");	\
	BUILD_ASSERT((BIT64(DT_REG_ADDR_RAW(node)) & (valid)) != 0U,			\
		     "TRDC master index is reserved or belongs to a core master");

/*
 * Build a constant table from an enabled controller's enabled children and
 * validate their indices. Repeated indices reduce the bitmap's set-bit count,
 * so comparing that count to the table size detects duplicate assignments.
 * A disabled controller produces an empty table and no validation checks.
 */
#define TRDC_DEFINE_MASTERS(label, valid)							\
	static const struct trdc_master_config label##_masters[] = {				\
		COND_CODE_1(DT_NODE_HAS_STATUS(DT_NODELABEL(label), okay),			\
			(DT_FOREACH_CHILD_STATUS_OKAY(DT_NODELABEL(label),			\
							TRDC_MASTER_ENTRY)), ())		\
	};											\
	COND_CODE_1(DT_NODE_HAS_STATUS(DT_NODELABEL(label), okay),				\
		(DT_FOREACH_CHILD_STATUS_OKAY_VARGS(DT_NODELABEL(label),			\
							TRDC_CHECK_MASTER, valid)		\
		BUILD_ASSERT(ARRAY_SIZE(label##_masters) ==					\
			POPCOUNT((uint32_t)TRDC_MASTER_MASK(DT_NODELABEL(label))) +		\
			POPCOUNT((uint32_t)(TRDC_MASTER_MASK(DT_NODELABEL(label)) >> 32)),	\
			"Duplicate TRDC master index");), ())

TRDC_DEFINE_MASTERS(trdc_main,
		GENMASK64(kTRDC_MAIN_MasterMEDMA3Ch30_31, kTRDC_MAIN_MasterEDMA5Ch0_1) |
		BIT64(kTRDC_MAIN_MasterDAP_AHBP) | BIT64(kTRDC_MAIN_MasterETR) |
		BIT64(kTRDC_MAIN_MasterWAKE_SS) | BIT64(kTRDC_MAIN_MasterMTR) |
		BIT64(kTRDC_MAIN_MasterTESTPORT))
TRDC_DEFINE_MASTERS(trdc_cmpt,
		BIT64(kTRDC_CMPT_MasterLLC_RD) | BIT64(kTRDC_CMPT_MasterLLC_WR) |
		BIT64(kTRDC_CMPT_MasterNPU))
TRDC_DEFINE_MASTERS(trdc_wake, GENMASK64(kTRDC_WAKE_MasterWEDMA3Ch0_4, 0))
TRDC_DEFINE_MASTERS(trdc_audio, GENMASK64(kTRDC_AUDIO_MasterAEDMA3Ch0_4, 0))
TRDC_DEFINE_MASTERS(trdc_comm, GENMASK64(kTRDC_COMM_MasterXSPI_RESP, 0))
TRDC_DEFINE_MASTERS(trdc_media, GENMASK64(kTRDC_MEDIA_MasterJPEG, 0))

static void soc_trdc_assign_controller(uintptr_t address,
					const struct trdc_master_config *masters, size_t count)
{
	TRDC_Type *const base = (TRDC_Type *)address;

	for (size_t i = 0; i < count; i++) {
		const struct trdc_master_config *master = &masters[i];

		base->MDA_DFMT1[master->master].MDA_W_DFMT1[0] =
			TRDC_MDA_W_DFMT1_DID(master->domain) | TRDC_MDA_W_DFMT1_VLD_MASK;
	}
}

/*
 * Hand the Resource Domain Controller over from the EdgeLock enclave. Must run
 * before any peripheral access; soc_trdc_assign_masters() is the rest of TRDC
 * setup and runs later, once clocks are up (see its own comment).
 */
void soc_ele_release_rdc(void)
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

	/* Apply LLC assignments before cache enable, leaving NPU for the late phase. */
	for (size_t i = 0; i < ARRAY_SIZE(trdc_cmpt_masters); i++) {
		const struct trdc_master_config *master = &trdc_cmpt_masters[i];

		if (master->master == kTRDC_CMPT_MasterLLC_RD ||
		    master->master == kTRDC_CMPT_MasterLLC_WR) {
			soc_trdc_assign_controller(DT_REG_ADDR(DT_NODELABEL(trdc_cmpt)),
						   master, 1U);
		}
	}
}

/*
 * Apply the remaining board/application assignments after clocks are ready.
 * CPU0's core master AXIM/AHBP assignments are left unchanged.
 */
void soc_trdc_assign_masters(void)
{
	soc_trdc_assign_controller(DT_REG_ADDR(DT_NODELABEL(trdc_main)),
				   trdc_main_masters, ARRAY_SIZE(trdc_main_masters));
	soc_trdc_assign_controller(DT_REG_ADDR(DT_NODELABEL(trdc_wake)),
				   trdc_wake_masters, ARRAY_SIZE(trdc_wake_masters));
	soc_trdc_assign_controller(DT_REG_ADDR(DT_NODELABEL(trdc_audio)),
				   trdc_audio_masters, ARRAY_SIZE(trdc_audio_masters));
	soc_trdc_assign_controller(DT_REG_ADDR(DT_NODELABEL(trdc_comm)),
				   trdc_comm_masters, ARRAY_SIZE(trdc_comm_masters));
	soc_trdc_assign_controller(DT_REG_ADDR(DT_NODELABEL(trdc_media)),
				   trdc_media_masters, ARRAY_SIZE(trdc_media_masters));

	for (size_t i = 0; i < ARRAY_SIZE(trdc_cmpt_masters); i++) {
		const struct trdc_master_config *master = &trdc_cmpt_masters[i];

		if (master->master == kTRDC_CMPT_MasterNPU) {
			soc_trdc_assign_controller(DT_REG_ADDR(DT_NODELABEL(trdc_cmpt)),
						   master, 1U);
		}
	}
}
