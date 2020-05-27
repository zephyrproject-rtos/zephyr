/*
 * Copyright (c) 2018 Intel Corporation
 *
 * Author: Sathish Kuttan <sathish.k.kuttan@intel.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Intel GNA device driver
 *
 * Device driver implementation for Intel's
 * Gaussian Mixture Model and Neural Network Accelerator (GNA)
 */

#ifndef __INTEL_GNA__
#define __INTEL_GNA__

#ifdef __cplusplus
extern "C" {
#endif

/* number of requests that could be pending in driver */
#define GNA_REQUEST_QUEUE_LEN		CONFIG_INTEL_GNA_MAX_PENDING_REQUESTS
#define GNA_MAX_NUM_MODELS		CONFIG_INTEL_GNA_MAX_MODELS

/* values must match config values in Kconfig.intel_gna */
#define GNA_POWER_MODE_ALWAYS_ON	0
#define GNA_POWER_MODE_CLOCK_GATED	1
#define GNA_POWER_MODE_POWER_GATED	2
#define GNA_POWER_MODE_ALWAYS_OFF	3

#define INTEL_GNA_BASE_ADDR		0x0000E800

#define INTEL_GNA_IRQ_ID		0x00000506
#define INTEL_GNA_IRQ_PRIORITY		3

#define GNA_STS_INTR_PENDING		BIT(31)
#define GNA_STS_SATURATION_OCCURRED	BIT(17)
#define GNA_STS_BUFFER_FULL		BIT(16)
#define GNA_STS_ERROR			BIT(15)
#define GNA_STS_PARAM_OOR		BIT(8)
#define GNA_STS_VIRT_ADDR_OOR		BIT(7)
#define GNA_STS_STATS_VALID		BIT(3)
#define GNA_STS_SUSP_PAUSE		BIT(2)
#define GNA_STS_SUSP_BREAKP		BIT(1)
#define GNA_STS_SCORE_COMPL		BIT(0)

#define GNA_CTRL_INTR_DISABLE		BIT(31)
#define GNA_CTRL_PM_IDLE_DISABLE	BIT(18)
#define GNA_CTRL_PM_OVRIDE_CLK_ON	BIT(17)
#define GNA_CTRL_PM_OVRIDE_PWR_ON	BIT(16)
#define GNA_CTRL_STATS_ENABLE_STALL	(1 << 12)
#define GNA_CTRL_STATS_MASK		(BIT_MASK(4) << 12)
#define GNA_CTRL_ERR_INTR_ENABLE	(1 << 10)
#define GNA_CTRL_COMPL_INTR_ENABLE	(1 << 8)
#define GNA_CTRL_OPER_MODEL_XNN		(1 << 5)
#define GNA_CTRL_ABORT_CLEAR		(1 << 2)
#define GNA_CTRL_ACCEL_START		(1 << 0)
#define GNA_CTRL_ACCEL_BUSY		GNA_CTRL_ACCEL_START

#define GNA_CONFIG_DESC_PG_DIR_SIZE	64

#define GNA_LAYER_DESC_ALIGN		(128)

#define GNA_ADDRESSABLE_MEM_SIZE	L2_SRAM_SIZE
#define GNA_NUM_PG_TABLE_INDEX_BITS	10
#define GNA_NUM_PG_TABLE_ENTRIES	BIT(GNA_NUM_PG_TABLE_INDEX_BITS)
#define GNA_PG_SIZE_IN_BITSHIFT		12
#define GNA_PG_SIZE_IN_BYTES		BIT(GNA_PG_SIZE_IN_BITSHIFT)

#define GNA_SHIFT_RNDUP(value, shift)	(((value) + BIT_MASK(shift)) >> (shift))

#define GNA_NUM_PAGES(bytes)		\
	GNA_SHIFT_RNDUP((bytes), GNA_PG_SIZE_IN_BITSHIFT)

#define GNA_PAGES_TO_BYTES(pages)	((pages) << GNA_PG_SIZE_IN_BITSHIFT)

#define GNA_MAX_NUM_PAGES		GNA_NUM_PAGES(GNA_ADDRESSABLE_MEM_SIZE)

#define GNA_NUM_PG_TABLES_NEEDED	\
	GNA_SHIFT_RNDUP(GNA_MAX_NUM_PAGES, GNA_NUM_PG_TABLE_INDEX_BITS)

#if GNA_NUM_PG_TABLES_NEEDED > GNA_CONFIG_DESC_PG_DIR_SIZE
#error GNA_NUM_PG_TABLES_NEEDED exceeds GNA_CONFIG_DESC_PG_DIR_SIZE
#endif

#define GNA_GET_BITS(val, b_hi, b_lo)	((((uint32_t)(val)) << (31 - (b_hi))) >> \
		(31 - (b_hi) + (b_lo)))

#define GNA_VA_PG_DIR(virt_addr)	GNA_GET_BITS(virt_addr, 27, 22)
#define GNA_VA_PG_TABLE(virt_addr)	GNA_GET_BITS(virt_addr, 21, 12)

#define GNA_PHYS_ADDR_TO_PAGE(addr)	((uint32_t)(addr) >> \
		GNA_PG_SIZE_IN_BITSHIFT)
#define GNA_PG_DIR_ENTRY(phys_addr)	GNA_PHYS_ADDR_TO_PAGE(phys_addr)
#define GNA_PG_BASE(addr)		((uint32_t)(addr) & \
		~BIT_MASK(GNA_PG_SIZE_IN_BITSHIFT))
#define GNA_PG_OFFSET(addr)		((uint32_t)(addr) & \
		BIT_MASK(GNA_PG_SIZE_IN_BITSHIFT))
#define GNA_PG_TABLE_ENTRY(phys_addr)	GNA_PHYS_ADDR_TO_PAGE(phys_addr)

struct intel_gna_regs {
	uint32_t	gnasts;
	uint32_t	gnactrl;
	uint32_t	gnamctl;
	uint32_t	gnaptc;
	uint32_t	gnasc;
	uint32_t	gnaisi;
	uint32_t	gnais_low;
	uint32_t	gnais_high;
	uint32_t	gnabp_low;
	uint32_t	gnabp_high;
	uint32_t	reserved1[2];
	uint32_t	gnadesbase;
	uint32_t	gnaibuffs;
	uint32_t	reserved2[2];
	uint32_t	ovrcfgctl;
	uint32_t	reserved3[3];
	uint32_t	gnaversion;
};

struct intel_gna_config_desc {
	uint32_t	reserved1[64];
	uint32_t	labase;		/* layer array base */
	uint16_t	lacnt;		/* layer array count */
	uint16_t	reserved2;
	uint32_t	reserved3[62];
	uint32_t	vamaxaddr;	/* virtual address max address */
	uint32_t	reserved4[3];
	/* page directory entries */
	uint32_t	pagedir[GNA_CONFIG_DESC_PG_DIR_SIZE];
} __packed;

struct intel_gna_page_table {
	uint32_t	entry[GNA_NUM_PG_TABLE_ENTRIES];
} __aligned(GNA_PG_SIZE_IN_BYTES);

struct intel_gna_layer_desc {
	uint32_t	gna_words[8];
	uint32_t	inarrayptr;
	uint32_t	outarrayactptr;
	uint32_t	outarraysumptr;
	uint32_t	outfbarrayactptr;
	uint32_t	wtfltarrayptr;
	uint32_t	constarrayptr;
	uint32_t	actoutputslistptr;
	uint32_t	actfuncsectdefptr;
	uint32_t	reserved[16];
} __packed __aligned(GNA_LAYER_DESC_ALIGN);

struct intel_gna_config {
	struct gna_config config;
};

struct intel_gna_model {
	struct gna_model_info	model;
	void			*input;
	void			*output;
	void			*vabase;
	bool			registered;
};

struct intel_gna_pending_req {
	struct intel_gna_model	*model;
	void			*output;
	size_t			output_len;
	gna_callback		callback;
};

struct intel_gna_pending_resp {
	struct gna_inference_resp	response;
	gna_callback			callback;
};

enum gna_state {
	GNA_STATE_UNINITIALIZED,
	GNA_STATE_INITIALIZED,
	GNA_STATE_IDLE,
	GNA_STATE_ACTIVE,
};

struct intel_gna_data {
	/*
	 * gna_cb_work must be the first element in the structure
	 * since it will be typecast as intel_gna_data in the work handler
	 */
	struct k_work			gna_work;
	volatile struct intel_gna_regs	*regs;
	struct k_mem_slab		model_slab;
	struct intel_gna_model		models[GNA_MAX_NUM_MODELS];
	struct k_msgq			request_queue;
	struct intel_gna_pending_req	requests[GNA_REQUEST_QUEUE_LEN];
	struct k_msgq			response_queue;
	struct intel_gna_pending_resp	responses[GNA_REQUEST_QUEUE_LEN];
	enum gna_state			state;
};

#ifdef __cplusplus
}
#endif

#endif /* __INTEL_GNA__ */
