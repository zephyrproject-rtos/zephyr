/*
 * Copyright (c) 2020 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_INTEL_VTD_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_INTEL_VTD_H_

#define VTD_INT_SHV	BIT(3)
#define VTD_INT_FORMAT	BIT(4)

/* We don't care about int_idx[15], since the size is fixed to 256,
 * it's always 0
 */
#define VTD_MSI_MAP(int_idx) \
	((0x0FEE << 20) | (int_idx << 5) | VTD_INT_SHV | VTD_INT_FORMAT)

/* Interrupt Remapping Table Entry (IRTE) for Remapped Interrupts */
struct vtd_irte {
	struct {
		uint64_t present		: 1;
		uint64_t fpd			: 1;
		uint64_t dst_mode		: 1;
		uint64_t redirection_hint	: 1;
		uint64_t trigger_mode		: 1;
		uint64_t delivery_mode		: 3;
		uint64_t available		: 4;
		uint64_t _reserved_0		: 3;
		uint64_t irte_mode		: 1;
		uint64_t vector			: 8;
		uint64_t _reserved_1		: 8;
		uint64_t dst_id			: 32;
	} l;

	struct {
		uint64_t src_id			: 16;
		uint64_t src_id_qualifier	: 2;
		uint64_t src_validation_type	: 2;
		uint64_t _reserved		: 44;
	} h;
} __packed;

/* The table must be 4KB aligned, which is exactly 256 entries.
 * And since we allow only 256 entries as a maximum: let's align to it.
 */
#define IRTE_NUM 256
#define IRTA_SIZE 7  /* size = 2^(X+1) where IRTA_SIZE is X 2^8 = 256 */

struct vtd_ictl_data {
	struct vtd_irte irte[IRTE_NUM];
	int irte_num_used;
};

struct vtd_ictl_cfg {
	DEVICE_MMIO_ROM;
};

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_INTEL_VTD_H_ */
