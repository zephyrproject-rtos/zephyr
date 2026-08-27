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
#define VTD_MSI_MAP(int_idx, shv)					\
	((0x0FEE00000U) | (int_idx << 5) | shv | VTD_INT_FORMAT)

/* Interrupt Remapping Table Entry (IRTE) for Remapped Interrupts */
union vtd_irte {
	struct irte_parts {
		uint64_t low;
		uint64_t high;
	} parts;

	struct irte_bits {
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
		uint64_t src_id			: 16;
		uint64_t src_id_qualifier	: 2;
		uint64_t src_validation_type	: 2;
		uint64_t _reserved		: 44;
	} bits __packed;
};

/* The table must be 4KB aligned, which is exactly 256 entries.
 * And since we allow only 256 entries as a maximum: let's align to it.
 */
#define IRTE_NUM 256
#define IRTA_SIZE 7  /* size = 2^(X+1) where IRTA_SIZE is X 2^8 = 256 */

#define QI_NUM 256 /* Which is the minimal number we can set for the queue */
#define QI_SIZE 0 /* size = 2^(X+8) where QI_SIZE is X: 2^8 = 256 */
#define QI_WIDTH 128

struct qi_descriptor {
	uint64_t low;
	uint64_t high;
};

#define QI_TYPE_ICC 0x1UL

union qi_icc_descriptor {
	struct qi_descriptor desc;

	struct icc_bits {
		uint64_t type		: 4;
		uint64_t granularity	: 2;
		uint64_t _reserved_0	: 3;
		uint64_t zero		: 3;
		uint64_t _reserved_1	: 4;
		uint64_t domain_id	: 16;
		uint64_t source_id	: 16;
		uint64_t function_mask	: 2;
		uint64_t _reserved_2	: 14;
		uint64_t reserved;
	} icc __packed;
};

#define QI_TYPE_IEC 0x4UL

union qi_iec_descriptor {
	struct qi_descriptor desc;

	struct iec_bits {
		uint64_t type		: 4;
		uint64_t granularity	: 1;
		uint64_t _reserved_0	: 4;
		uint64_t zero		: 3;
		uint64_t _reserved_1	: 15;
		uint64_t index_mask	: 5;
		uint64_t interrupt_index: 16;
		uint64_t _reserved_2	: 16;
		uint64_t reserved;
	} iec __packed;
};

#define QI_TYPE_WAIT 0x5UL

union qi_wait_descriptor {
	struct qi_descriptor desc;

	struct wait_bits {
		uint64_t type		: 4;
		uint64_t interrupt_flag	: 1;
		uint64_t status_write	: 1;
		uint64_t fence_flag	: 1;
		uint64_t page_req_drain	: 1;
		uint64_t _reserved_0	: 1;
		uint64_t zero		: 3;
		uint64_t _reserved_1	: 20;
		uint64_t status_data	: 32;
		uint64_t reserved	: 2;
		uint64_t address	: 62;
	} wait __packed;
};

#define QI_WAIT_STATUS_INCOMPLETE 0x0UL
#define QI_WAIT_STATUS_COMPLETE 0x1UL

/* Arbitrary wait counter limit */
#define QI_WAIT_COUNT_LIMIT 100

struct vtd_ictl_data {
	DEVICE_MMIO_RAM;
	union vtd_irte irte[IRTE_NUM] __aligned(0x1000);
	struct qi_descriptor qi[QI_NUM] __aligned(0x1000);
	int irqs[IRTE_NUM];
	int vectors[IRTE_NUM];
	bool msi[IRTE_NUM];
	int irte_num_used;
	unsigned int fault_irq;
	uintptr_t fault_record_reg;
	uint16_t fault_record_num;
	uint16_t qi_tail;
	uint8_t fault_vector;
	bool pwc;
};

struct vtd_ictl_cfg {
	DEVICE_MMIO_ROM;
};

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_INTEL_VTD_H_ */
