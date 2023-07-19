/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_PCIE_HOST_VC_H_
#define ZEPHYR_DRIVERS_PCIE_HOST_VC_H_

#define PCIE_VC_CAP_REG_1_OFFSET	0x04U
#define PCIE_VC_CAP_REG_2_OFFSET	0x08U
#define PCIE_VC_CTRL_STATUS_REG_OFFSET	0x0CU

/** Virtual Channel capability and control Registers */
struct pcie_vc_regs {
	union {
		struct {
			/** Virtual Channel Count */
			uint32_t vc_count              : 3;
			uint32_t _reserved1            : 1;
			/** Low Priority Virtual Channel Count */
			uint32_t lpvc_count            : 3;
			uint32_t _reserved2            : 1;
			/** Reference Clock */
			uint32_t reference_clock       : 2;
			/** Port Arbitration Table Entry Size */
			uint32_t pat_entry_size        : 3;
			uint32_t _reserved3            : 19;
		};
		uint32_t raw;
	} cap_reg_1;

	union {
		struct {
			/** Virtual Channel Arbitration Capability */
			uint32_t vca_cap               : 8;
			uint32_t _reserved1            : 16;
			/** Virtual Channel Arbitration Table Offset */
			uint32_t vca_table_offset      : 8;
		};
		uint32_t raw;
	} cap_reg_2;

	union {
		struct {
			/** Load Virtual Channel Arbitration Table */
			uint32_t load_vca_table        : 1;
			/** Virtual Channel Arbitration Select */
			uint32_t vca_select            : 3;
			uint32_t _reserved1            : 12;
			/** Virtual Channel Arbitration Table Status */
			uint32_t vca_table_status      : 1;
			uint32_t _reserved2            : 15;
		};
		uint32_t raw;
	} ctrl_reg;
};

#define PCIE_VC_RES_CAP_REG_OFFSET(_vc)		(0x10U + _vc * 0X0CU)
#define PCIE_VC_RES_CTRL_REG_OFFSET(_vc)	(0x14U + _vc * 0X0CU)
#define PCIE_VC_RES_STATUS_REG_OFFSET(_vc)	(0x18U + _vc * 0X0CU)

#define PCIE_VC_PA_RR		BIT(0)
#define PCIE_VC_PA_WRR		BIT(1)
#define PCIE_VC_PA_WRR64	BIT(2)
#define PCIE_VC_PA_WRR128	BIT(3)
#define PCIE_VC_PA_TMWRR128	BIT(4)
#define PCIE_VC_PA_WRR256	BIT(5)

/** Virtual Channel Resource Registers */
struct pcie_vc_resource_regs {
	union {
		struct {
			/** Port Arbitration Capability */
			uint32_t pa_cap                : 8;
			uint32_t _reserved1            : 6;
			uint32_t undefined             : 1;
			/** Reject Snoop Transactions */
			uint32_t rst                   : 1;
			/** Maximum Time Slots */
			uint32_t max_time_slots        : 7;
			uint32_t _reserved2            : 1;
			/** Port Arbitration Table Offset */
			uint32_t pa_table_offset       : 8;
		};
		uint32_t raw;
	} cap_reg;

	union {
		struct {
			/** Traffic Class to Virtual Channel Map */
			uint32_t tc_vc_map             : 8;
			uint32_t _reserved1            : 8;
			/** Load Port Arbitration Table */
			uint32_t load_pa_table         : 1;
			/** Port Arbitration Select */
			uint32_t pa_select             : 3;
			uint32_t _reserved2            : 4;
			/** Virtual Channel ID */
			uint32_t vc_id                 : 3;
			uint32_t _reserved3            : 4;
			/** Virtual Channel Enable */
			uint32_t vc_enable             : 1;
		};
		uint32_t raw;
	} ctrl_reg;

	union {
		struct {
			uint32_t _reserved1            : 16;
			/** Port Arbitration Table Status */
			uint32_t pa_table_status       : 1;
			/** Virtual Channel Negociation Pending */
			uint32_t vc_negocation_pending : 1;
			uint32_t _reserved2            : 14;
		};
		uint32_t raw;
	} status_reg;
};

uint32_t pcie_vc_cap_lookup(pcie_bdf_t bdf, struct pcie_vc_regs *regs);

void pcie_vc_load_resources_regs(pcie_bdf_t bdf,
				 uint32_t base,
				 struct pcie_vc_resource_regs *regs,
				 int nb_regs);

#endif /* ZEPHYR_DRIVERS_PCIE_HOST_VC_H_ */
