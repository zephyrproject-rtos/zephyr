/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Charlie, Xingyu Hu and etc.
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_VM_DEVICE_H_
#define ZEPHYR_INCLUDE_ZVM_VM_DEVICE_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/zvm/arm/cpu.h>

#define VIRT_DEV_NAME_LENGTH		(32)
#define VIRT_DEV_TYPE_LENGTH		(32)

#define DEV_TYPE_EMULATE_ALL_DEVICE (0x01)
#define DEV_TYPE_VIRTIO_DEVICE	  (0x02)
#define DEV_TYPE_PASSTHROUGH_DEVICE (0x03)

#define VM_DEVICE_INIT_RES	  (0xFF)
#define VM_DEVICE_INVALID_BASE  (0xFFFFFFFF)
#define VM_DEVICE_INVALID_VIRQ  (0xFF)

/**
 * @brief DEVICE_TYPE for each vm, which is used to
 * judge whether it is nesscary to init when vm creat.
 */
#define VM_DEVICE_PRE_KERNEL_1  (0x01)

#define TOSTRING(x) STRINGIFY(x)


typedef void (*virt_device_irq_callback_user_data_set_t)(const struct device *dev,
									void *cb, void *user_data);

struct z_virt_dev {
	/* name of virtual device */
	char name[VIRT_DEV_NAME_LENGTH];

	/* Is this dev pass-through device? */
	bool dev_pt_flag;
	/* Is this dev virtio device?*/
	bool shareable;

	uint32_t hirq;
	uint32_t virq;

	uint32_t vm_vdev_paddr;
	uint32_t vm_vdev_vaddr;
	uint32_t vm_vdev_size;

	struct _dnode vdev_node;
	struct z_vm *vm;

	/**
	 * Device private data may be useful,
	 * 1. For full virtual device, it store the emulated device's driver,
	 * for example: virt_device instance.
	 * 2. For passthrough device, it store the hardware instance data.
	 */
	const void *priv_data;

	/**
	 * Binding to full virtual device driver.
	 */
	void *priv_vdev;
};
typedef struct z_virt_dev virt_dev_t;

/**
 * @brief Get private date for vm.
 */
struct virt_device_data {
	/* virtual device types for vm, can be seen in macro 'DEVICE_TYPE'. */
	uint16_t vdevice_type;
	/* Get the virt device data port*/
	void *device_data;
#ifdef CONFIG_ENABLE_VIRT_DEVICE_INTERRUPT_DRIVEN
	virt_device_irq_callback_user_data_set_t irq_cb;
	void *irq_cb_data;
#endif
};

/**
 * @brief Get the device instance from dts, which include
 * the origin `device/config` from zephyr's device framework.
 */
struct virt_device_config {
	/* Regisiter base and size from dts*/
	uint32_t reg_base;
	uint32_t reg_size;
	uint32_t hirq_num;
	char device_type[VIRT_DEV_TYPE_LENGTH];
	/* Address of device instance config information */
	const void *device_config;
};

/**
 * @brief A virt device api for init/deinit or read/write device.
 */
struct virt_device_api {
	int (*init_fn)(const struct device *dev, struct z_vm *vm, struct z_virt_dev *vdev_desc);
	int (*deinit_fn)(const struct device *dev, struct z_vm *vm, struct z_virt_dev *vdev_desc);
	int (*virt_device_write)(
		struct z_virt_dev *vdev, uint64_t addr, uint64_t *value, uint16_t size);

	int (*virt_device_read)(
		struct z_virt_dev *vdev, uint64_t addr, uint64_t *value, uint16_t size);
#ifdef CONFIG_ENABLE_VIRT_DEVICE_INTERRUPT_DRIVEN
	void (*virt_irq_callback_set)(const struct device *dev, void *cb, void *user_data);
#endif
	/* Get the device driver api, if the device driver is initialed in host */
	const void *device_driver_api;
};

/**
 * @brief Virtual devices backend instance in zvm.
 */
struct virtual_device_instance {
	const char *name;
	struct virt_device_data *data;
	struct virt_device_config *cfg;
	const struct virt_device_api *api;
};

/**
 * @brief Macro for creating a virtual device instance.
 *
 * @param _init_fn	Init function for virtual device.
 * @param _level	Init level.
 * @param _prio	 Init priority.
 * @param _name		Name of the virtual device instance.
 * @param _data	 Date of the virtual device instance.
 * @param _cfg	  Configuration of virtual device.
 * @param _api		Virtual device backend API.
 * @param ...		Optional context.
 */
#define ZVM_VIRTUAL_DEVICE_DEFINE(	\
	_init_fn, _level, _prio, _name, _data, _cfg, _api, ...)	  \
	SYS_INIT_NAMED(	\
		_init_fn, _init_fn, _level, _prio);						\
	static const STRUCT_SECTION_ITERABLE(	\
	virtual_device_instance, _name) =	 \
	{									   \
		.name = STRINGIFY(_name),				\
		.data = &_data,						 \
		.cfg = &_cfg,						   \
		.api = &_api,							   \
	}

/* The overall virtual devices instances. */
extern const struct virtual_device_instance __virtual_device_instances_start[];
extern const struct virtual_device_instance __virtual_device_instances_end[];

/**
 * @brief Save the overall idle dev list info.
 * Smp condition must be considered here.
 */
struct zvm_dev_lists {
	uint16_t dev_count;
	sys_dlist_t dev_idle_list;
	sys_dlist_t dev_used_list;
	/*TODO: Add smp lock here*/
};

struct device_chosen {
	bool chosen_flag;
	struct k_spinlock lock;
};

/**
 * @brief Get virtual device.
 *
 * @return	Pointer to the virtual device instance.
 */
static inline const struct virtual_device_instance *zvm_virtual_device_get(uint32_t idx)
{
	return &__virtual_device_instances_start[idx];
}

/**
 * @brief Get number of virtual devices.
 *
 * @return Number of virtual devices.
 */
static inline int zvm_virtual_devices_count_get(void)
{
	return __virtual_device_instances_end - __virtual_device_instances_start;
}

/**
 * @brief Set the IRQ callback function pointer.
 *
 * This sets up the callback for IRQ. When an IRQ is triggered,
 * the specified function will be called with specified user data.
 *
 * @param dev virt device structure.
 * @param cb Pointer to the callback function.
 * @param user_data Data to pass to callback function.
 */
static inline void vdev_irq_callback_user_data_set(const struct device *dev,
						   void *cb, void *user_data)
{
#ifdef CONFIG_ENABLE_VIRT_DEVICE_INTERRUPT_DRIVEN
	const struct virt_device_api *api =
		(const struct virt_device_api *)dev->api;

	if ((api != NULL) && (api->virt_irq_callback_set != NULL)) {
		api->virt_irq_callback_set(dev, cb, user_data);
	}
#endif
}

/**
 * @brief Allocate device to vm, it will be called when device that will be
 * allocated to vm. Then, Set the device's irq for binding virt interrupt
 * with hardware interrupt.
 *
 * @return virt device instance.
 */
struct z_virt_dev *allocate_device_to_vm(
	const struct device *dev, struct z_vm *vm,
	struct z_virt_dev *vdev_desc,
	bool pt_flag, bool shareable);

/**
 * @brief vm virt device call back function, which will be called when the device
 * that allocated to vm is triggered.
 */
void vm_device_callback_func(const struct device *dev, void *cb, void *user_data);


struct z_virt_dev *vm_virt_dev_add(
	struct z_vm *vm, const char *dev_name, bool pt_flag,
	bool shareable, uint64_t dev_pbase, uint64_t dev_vbase,
	uint32_t dev_size, uint32_t dev_hirq, uint32_t dev_virq);

int vm_virt_dev_remove(struct z_vm *vm, struct z_virt_dev *vm_dev);

/**
 * @brief write or read vdev for VM operation....
 */
int vdev_mmio_abort(
	arch_commom_regs_t *regs, int write,
	uint64_t addr, uint64_t *value, uint16_t size);

/**
 * @brief unmap passthrough device.
 */
int vm_unmap_ptdev(
	struct z_virt_dev *vdev, uint64_t vm_dev_base,
	uint64_t vm_dev_size, struct z_vm *vm);

int vm_vdev_pause(struct z_vcpu *vcpu);

/**
 * @brief Handle VM's device memory access. When pa_addr is
 * located at a idle device, something need to do:
 * 1. Building a stage-2 translation table for this vm, which
 * can directly access this memory later.
 * 2. Rerun the fault code and access the physical device memory.
 */
int handle_vm_device_emulate(struct z_vm *vm, uint64_t pa_addr);

void virt_device_irq_callback_data_set(int irq, int priority, void *user_data);

int vm_device_init(struct z_vm *vm);
int vm_device_deinit(struct z_vm *vm);

#endif /* ZEPHYR_INCLUDE_ZVM_VM_DEVICE_H_ */
