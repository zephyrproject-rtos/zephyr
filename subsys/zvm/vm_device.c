/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Chenglai Xiong, Xingyu Hu and etc.
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/spinlock.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/zvm/zvm.h>
#include <zephyr/zvm/vm_device.h>
#include <zephyr/zvm/vm_mm.h>
#include <zephyr/zvm/vdev/vgic_v3.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

#define DEV_CFG(dev) \
	((const struct virt_device_config * const)(dev)->config)
#define DEV_DATA(dev) \
	((struct virt_device_data *)(dev)->data)

static struct device_chosen vm_device_chosen;

int __weak vm_init_bdspecific_device(struct z_vm *vm)
{
	return 0;
}

static int vm_vdev_mem_add(struct z_vm *vm, struct z_virt_dev *vdev)
{
	uint32_t attrs = 0;

	/*If device is emulated, set access off attrs*/
	if (vdev->dev_pt_flag && !vdev->shareable) {
		attrs = MT_VM_DEVICE_MEM;
	} else {
		attrs = MT_VM_DEVICE_MEM | MT_S2_ACCESS_OFF;
	}

	return vm_vdev_mem_create(vm->vmem_domain, vdev->vm_vdev_paddr,
			vdev->vm_vdev_vaddr, vdev->vm_vdev_size, attrs);

}

struct z_virt_dev *vm_virt_dev_add_no_memmap(struct z_vm *vm, const char *dev_name, bool pt_flag,
				bool shareable, uint64_t dev_pbase, uint64_t dev_hva,
					uint32_t dev_size, uint32_t dev_hirq, uint32_t dev_virq)
{
	uint16_t name_len;
	struct z_virt_dev *vm_dev;

	vm_dev = (struct z_virt_dev *)k_malloc(sizeof(struct z_virt_dev));
	if (!vm_dev) {
		return NULL;
	}

	name_len = strlen(dev_name);
	name_len = name_len > VIRT_DEV_NAME_LENGTH ? VIRT_DEV_NAME_LENGTH : name_len;
	strncpy(vm_dev->name, dev_name, name_len);
	vm_dev->name[name_len] = '\0';

	vm_dev->dev_pt_flag = pt_flag;
	vm_dev->shareable = shareable;
	vm_dev->vm_vdev_paddr = dev_pbase;
	vm_dev->vm_vdev_vaddr = dev_hva;
	vm_dev->vm_vdev_size = dev_size;
	vm_dev->virq = dev_virq;
	vm_dev->hirq = dev_hirq;
	vm_dev->vm = vm;

	/* Init private data and vdev */
	vm_dev->priv_data = NULL;
	vm_dev->priv_vdev = NULL;

	sys_dlist_append(&vm->vdev_list, &vm_dev->vdev_node);

	return vm_dev;
}

struct z_virt_dev *vm_virt_dev_add(struct z_vm *vm, const char *dev_name, bool pt_flag,
				bool shareable, uint64_t dev_pbase, uint64_t dev_vbase,
					uint32_t dev_size, uint32_t dev_hirq, uint32_t dev_virq)
{
	uint16_t name_len;
	int ret;
	struct z_virt_dev *vm_dev;

	vm_dev = (struct z_virt_dev *)k_malloc(sizeof(struct z_virt_dev));
	if (!vm_dev) {
		return NULL;
	}

	name_len = strlen(dev_name);
	name_len = name_len > VIRT_DEV_NAME_LENGTH ? VIRT_DEV_NAME_LENGTH : name_len;
	strncpy(vm_dev->name, dev_name, name_len);
	vm_dev->name[name_len] = '\0';

	vm_dev->dev_pt_flag = pt_flag;
	vm_dev->shareable = shareable;
	vm_dev->vm_vdev_paddr = dev_pbase;
	vm_dev->vm_vdev_vaddr = dev_vbase;
	vm_dev->vm_vdev_size = dev_size;

	ret = vm_vdev_mem_add(vm, vm_dev);
	if (ret) {
		return NULL;
	}
	vm_dev->virq = dev_virq;
	vm_dev->hirq = dev_hirq;
	vm_dev->vm = vm;

	/*Init private data and vdev*/
	vm_dev->priv_data = NULL;
	vm_dev->priv_vdev = NULL;

	sys_dnode_init(&vm_dev->vdev_node);
	sys_dlist_append(&vm->vdev_list, &vm_dev->vdev_node);

	return vm_dev;
}

int vm_virt_dev_remove(struct z_vm *vm, struct z_virt_dev *vm_dev)
{
	struct zvm_dev_lists *vdev_list;
	struct z_virt_dev *chosen_dev = NULL;
	struct  _dnode *d_node, *ds_node;

	sys_dlist_remove(&vm_dev->vdev_node);

	vdev_list = get_zvm_dev_lists();
	SYS_DLIST_FOR_EACH_NODE_SAFE(&vdev_list->dev_used_list, d_node, ds_node) {
		chosen_dev = CONTAINER_OF(d_node, struct z_virt_dev, vdev_node);
		if (chosen_dev->vm_vdev_paddr == vm_dev->vm_vdev_paddr) {
			sys_dlist_remove(&chosen_dev->vdev_node);
			sys_dlist_append(&vdev_list->dev_idle_list, &chosen_dev->vdev_node);
			break;
		}
	}

	k_free(vm_dev);
	return 0;
}

int vdev_mmio_abort(arch_commom_regs_t *regs, int write, uint64_t addr,
				uint64_t *value, uint16_t size)
{
	uint64_t *reg_value = value;
	struct z_vm *vm;
	struct z_virt_dev *vdev;
	struct  _dnode *d_node, *ds_node;
	struct virtual_device_instance *vdevice_instance;

	vm = get_current_vm();
	SYS_DLIST_FOR_EACH_NODE_SAFE(&vm->vdev_list, d_node, ds_node) {
		vdev = CONTAINER_OF(d_node, struct z_virt_dev, vdev_node);
		vdevice_instance = (struct virtual_device_instance *)vdev->priv_data;
		if (vdevice_instance != NULL) {
			if (DEV_DATA(vdevice_instance)->vdevice_type & VM_DEVICE_PRE_KERNEL_1) {
				if ((addr >= vdev->vm_vdev_paddr) &&
				(addr < vdev->vm_vdev_paddr + vdev->vm_vdev_size)) {
					if (write) {
						return ((const struct virt_device_api *const)
						(vdevice_instance->api))->virt_device_write(
							vdev, addr, reg_value, size);
					} else {
						return ((const struct virt_device_api *const)
						(vdevice_instance->api))->virt_device_read(
							vdev, addr, reg_value, size);
					}
				}
			}
		}
	}
	/* Not found the vdev */
	ZVM_LOG_WARN("There are no virtual dev for this addr, addr : 0x%llx\n", addr);
	return -ENODEV;
}

int vm_unmap_ptdev(struct z_virt_dev *vdev, uint64_t vm_dev_base,
		 uint64_t vm_dev_size, struct z_vm *vm)
{
	uint64_t p_base, v_base, p_size, v_size;

	p_base = vdev->vm_vdev_paddr;
	p_size = vdev->vm_vdev_size;
	v_base = vm_dev_base;
	v_size = vm_dev_size;

	if (p_size != v_size || p_size == 0) {
		ZVM_LOG_WARN("The device is not matching, can not allocat this dev to the vm!");
		return -ENODEV;
	}

	return arch_vm_dev_domain_unmap(
		p_size, v_base, v_size, vdev->name, vm->vmid,
		&vm->vmem_domain->vm_mm_domain->arch.ptables);

}

int vm_vdev_pause(struct z_vcpu *vcpu)
{
	ARG_UNUSED(vcpu);
	return 0;
}

int handle_vm_device_emulate(struct z_vm *vm, uint64_t pa_addr)
{
	int ret;
	struct z_virt_dev *vm_dev, *chosen_dev = NULL;
	struct zvm_dev_lists *vdev_list;
	struct  _dnode *d_node, *ds_node;
	struct device *dev;
	k_spinlock_key_t key;

	key = k_spin_lock(&vm_device_chosen.lock);

	vdev_list = get_zvm_dev_lists();
	SYS_DLIST_FOR_EACH_NODE_SAFE(&vdev_list->dev_idle_list, d_node, ds_node) {
		vm_dev = CONTAINER_OF(d_node, struct z_virt_dev, vdev_node);
		/* Match the memory address ? */
		if (pa_addr >= vm_dev->vm_vdev_vaddr &&
		pa_addr < (vm_dev->vm_vdev_vaddr + vm_dev->vm_vdev_size)) {
			vm_device_chosen.chosen_flag = true;

			chosen_dev = vm_virt_dev_add(
				vm, vm_dev->name, vm_dev->dev_pt_flag, vm_dev->shareable,
				vm_dev->vm_vdev_paddr, vm_dev->vm_vdev_vaddr,
				vm_dev->vm_vdev_size, vm_dev->hirq, vm_dev->virq);
			if (!chosen_dev) {
				ZVM_LOG_WARN("there are no idle device %s for vm!", vm_dev->name);
				vm_device_chosen.chosen_flag = false;
				k_spin_unlock(&vm_device_chosen.lock, key);
				return -ENODEV;
			}
			/* move device to used node! */
			sys_dlist_remove(&vm_dev->vdev_node);
			sys_dlist_append(&vdev_list->dev_used_list, &vm_dev->vdev_node);
			vm_device_irq_init(vm, chosen_dev);

			dev = (struct device *)vm_dev->priv_data;
			DEV_DATA(dev)->device_data = chosen_dev;

			if (chosen_dev->shareable) {
				chosen_dev->priv_data = dev;
				ret =
				((struct virt_device_api *)dev->api)->init_fn(dev, vm, chosen_dev);
				if (ret) {
					ZVM_LOG_WARN(" Init device %s error!\n", dev->name);
					return -EFAULT;
				}
			}

			ZVM_LOG_INFO("** Adding %s device to %s.\n", chosen_dev->name, vm->vm_name);
			k_spin_unlock(&vm_device_chosen.lock, key);
			return 0;
		}
	}
	k_spin_unlock(&vm_device_chosen.lock, key);
	return -ENODEV;
}

static void virt_device_isr(const void *user_data)
{
	uint32_t virq;
	int err = 0;
	const struct z_virt_dev *vdev = (const struct z_virt_dev *)user_data;

	virq = vdev->virq;
	if (virq == VM_DEVICE_INVALID_VIRQ) {
		ZVM_LOG_WARN("Invalid interrupt occur!\n");
		return;
	}
	if (!vdev->vm) {
		ZVM_LOG_WARN("VM struct not exit here!");
		return;
	}

	err = set_virq_to_vm(vdev->vm, virq);
	if (err < 0) {
		ZVM_LOG_WARN("Send virq to vm error!");
	}
}


void virt_device_irq_callback_data_set(int irq, int priority, void *user_data)
{
	static int vector_num;

	vector_num = irq_connect_dynamic(irq, 1, virt_device_isr, user_data, 0);
	if (vector_num < 0) {
		ZVM_LOG_WARN("Connect dynamic irq error!\n");
		ZVM_LOG_WARN("irq: %d, priority: %d.\n", irq, priority);
		return;
	}
	irq_enable(irq);
}

struct z_virt_dev *allocate_device_to_vm(
	const struct device *dev, struct z_vm *vm,
	struct z_virt_dev *vdev_desc, bool pt_flag, bool shareable)
{
	struct z_virt_dev *vdev;

	vdev = vm_virt_dev_add(vm, dev->name, pt_flag, shareable, DEV_CFG(dev)->reg_base,
					vdev_desc->vm_vdev_paddr, DEV_CFG(dev)->reg_size,
					DEV_CFG(dev)->hirq_num, vdev_desc->virq);
	if (!vdev) {
		return NULL;
	}

	vm_device_irq_init(vm, vdev);
	return vdev;
}

void vm_device_callback_func(const struct device *dev, void *cb,
				void *user_data)
{
	uint32_t virq;

	ARG_UNUSED(cb);
	int err = 0;
	const struct z_virt_dev *vdev = (const struct z_virt_dev *)user_data;

	virq = vdev->virq;
	if (virq == VM_DEVICE_INVALID_VIRQ) {
		ZVM_LOG_WARN("Invalid interrupt occur!\n");
		return;
	}
	if (!vdev->vm) {
		ZVM_LOG_WARN("VM struct not exit here!");
		return;
	}

	err = set_virq_to_vm(vdev->vm, virq);
	if (err < 0) {
		ZVM_LOG_WARN("Send virq to vm error!");
	}

}

int vm_device_init(struct z_vm *vm)
{
	int ret = 0, i;

	sys_dlist_init(&vm->vdev_list);

	/* Assign ids to virtual devices. */
	for (i = 0; i < zvm_virtual_devices_count_get(); i++) {
		const struct virtual_device_instance *virtual_device = zvm_virtual_device_get(i);

		ZVM_LOG_INFO("Device name: %s.\n", virtual_device->name);
		/*If the virtual device is nessenary for vm*/
		if (virtual_device->data->vdevice_type & VM_DEVICE_PRE_KERNEL_1) {
			virtual_device->api->init_fn(NULL, vm, NULL);
			ZVM_LOG_INFO("Init %s for VM successful.\n", virtual_device->name);
		}
	}

	/* TODO: scan the dtb and get the device's node. */
	/* Board specific device init, for example fig debugger. */
	switch (vm->os->info.os_type) {
	case OS_TYPE_ZEPHYR:
		break;
	case OS_TYPE_LINUX:
		ret = vm_init_bdspecific_device(vm);
		break;
	default:
		break;
	}

	return ret;
}

int vm_device_deinit(struct z_vm *vm)
{
	int ret = 0;
	struct _dnode *dev_list = &vm->vdev_list;
	struct  _dnode *d_node, *ds_node;
	struct z_virt_dev *vdev;
	const struct virtual_device_instance *vdevice_instance;

	SYS_DLIST_FOR_EACH_NODE_SAFE(dev_list, d_node, ds_node) {
		vdev = CONTAINER_OF(d_node, struct z_virt_dev, vdev_node);
		vdevice_instance = (const struct virtual_device_instance *)vdev->priv_data;
		if (vdevice_instance != NULL) {
			if (vdevice_instance->api->deinit_fn) {
				ret = vdevice_instance->api->deinit_fn(NULL, vm, vdev);
				ZVM_LOG_INFO("Remove virt_serial: %s.\n", vdev->name);
			}
		}
	}

	return ret;
}
