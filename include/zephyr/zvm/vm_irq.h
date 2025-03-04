/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Chenglai Xiong, Xingyu Hu and etc.
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_VM_IRQ_H_
#define ZEPHYR_INCLUDE_ZVM_VM_IRQ_H_

#include <stdint.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/arch/arm64/lib_helpers.h>
#include <zephyr/zvm/arm/switch.h>
#include <zephyr/zvm/vm_device.h>

/*TODO: HW_FLAG may not enbaled for each spi.*/
#define VIRQ_HW_FLAG				BIT(0)
#define VIRQ_PENDING_FLAG			BIT(1)
#define VIRQ_ACTIVATED_FLAG			BIT(2)
#define VIRQ_ENABLED_FLAG			BIT(3)
#define VIRQ_WAKEUP_FLAG			BIT(4)

/* Hardware irq states */
#define VIRQ_STATE_INVALID				(0b00)
#define VIRQ_STATE_PENDING				(0b01)
#define VIRQ_STATE_ACTIVE				(0b10)
#define VIRQ_STATE_ACTIVE_AND_PENDING	(0b11)

/* VM's injuct irq num, bind to the register */
#define VM_INVALID_DESC_ID				(0xFF)

/* VM's irq prio */
#define VM_DEFAULT_LOCAL_VIRQ_PRIO	  (0x20)

/* irq number for arm64 system. */
#define VM_LOCAL_VIRQ_NR	(VM_SGI_VIRQ_NR + VM_PPI_VIRQ_NR)
#define VM_GLOBAL_VIRQ_NR   (VM_LOCAL_VIRQ_NR + VM_SPI_VIRQ_NR)

struct z_vm;
struct z_virt_dev;

/**
 * @brief Description for each virt irq descripetor.
 */
struct virt_irq_desc {
	/* Id that describes the irq. */
	uint8_t id;
	uint8_t vcpu_id;
	uint8_t vm_id;
	uint8_t prio;
	/* software dev trigger level flag */
	uint8_t vdev_trigger;

	/* irq level type */
	uint8_t type;
	uint8_t src_cpu;

	/* hardware virq states */
	uint8_t virq_states;
	/* software virq flags */
	uint32_t virq_flags;

	/**
	 * If physical irq is existed, pirq_num has
	 * a value, otherwise, it is set to 0xFFFFFFFF
	 */
	uint32_t pirq_num;
	uint32_t virq_num;

	sys_dnode_t desc_node;
};

/* vcpu wfi struct */
struct vcpu_wfi {
	bool state;
	uint16_t yeild_count;
	struct k_spinlock wfi_lock;
	void *priv;
};

/**
 * @brief vm's irq block to describe this all the device interrupt
 * for vm. In this struct, it called `VM_LOCAL_VIRQ_NR`;
 */
struct vcpu_virt_irq_block {
	/**
	 * record the virt irq counts which is activated,
	 * when a virt irq is sent to vm, zvm should record
	 * it and it means there is a virt irq need to process.
	 */
	uint32_t virq_pending_counts;
	uint32_t pending_sgi_num;

	struct virt_irq_desc vcpu_virt_irq_desc[VM_LOCAL_VIRQ_NR];
	struct vcpu_wfi vwfi;

	struct k_spinlock spinlock;

	sys_dlist_t active_irqs;
	sys_dlist_t pending_irqs;
};

/**
 * @brief vm's irq block to describe this all the device interrupt
 * for vm. In this struct, it called `VM_GLOBAL_VIRQ_NR-VM_LOCAL_VIRQ_NR`;
 */
struct vm_virt_irq_block {

	bool enabled;
	bool irq_bitmap[VM_GLOBAL_VIRQ_NR];

	/* interrupt control block flag */
	uint32_t flags;
	uint32_t irq_num;
	uint32_t cpu_num;

	uint32_t irq_target[VM_GLOBAL_VIRQ_NR];
	uint32_t ipi_vcpu_source[CONFIG_MP_MAX_NUM_CPUS][VM_SGI_VIRQ_NR];

	/* virtual irq block. */
	struct k_spinlock vm_virq_lock;

	/* desc for this vm */
	struct virt_irq_desc vm_virt_irq_desc[VM_GLOBAL_VIRQ_NR-VM_LOCAL_VIRQ_NR];

	/* bind to interrupt controller */
	void *virt_priv_date;
};

bool vcpu_irq_exist(struct z_vcpu *vcpu);

int vcpu_wait_for_irq(struct z_vcpu *vcpu);

/**
 * @brief init the irq desc when add vm_dev.
 */
void vm_device_irq_init(struct z_vm *vm, struct z_virt_dev *vm_dev);

/**
 * @brief init irq block for vm.
 */
int vm_irq_block_init(struct z_vm *vm);

#endif /* ZEPHYR_INCLUDE_ZVM_VM_IRQ_H_ */
