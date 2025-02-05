/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Yuhao Hu, Qingqiao Wang and etc.
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/zvm/zvm.h>
#include <zephyr/zvm/vm.h>
#include <zephyr/zvm/vdev/vpl011.h>
#include <zephyr/zvm/vm_device.h>
#include <zephyr/zvm/vdev/vserial.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/shell/shell_uart.h>


LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

#define DEV_DATA(dev) \
	((struct virt_device_data *)(dev)->data)

static const struct virtual_device_instance *serial_virtual_device_instance;

static int vpl011_regs_init(
	struct z_vm *vm, struct virt_pl011 *pl011,
	uint32_t serial_base, uint32_t serial_size)
{

	struct virt_pl011 *vpl011 = pl011;
	uint8_t pl011_id[8] = ARM_PL011_ID;
	int i;

	vpl011->vm = vm;
	vpl011->vserial_base = serial_base;
	vpl011->vserial_size = serial_size;
	vpl011->vserial_reg_base = (uint32_t *)k_malloc(vpl011->vserial_size);

	/* check serial device */
	if (!vpl011->vserial_reg_base) {
		ZVM_LOG_ERR("vpl011 device has init error!");
		return -ENODEV;
	}
	memset(vpl011->vserial_reg_base, 0, serial_size);

	/* Init spinlock */
	ZVM_SPINLOCK_INIT(&vpl011->vserial_lock);
	VPL011_REGS(vpl011)->fr =	0x90;
	VPL011_REGS(vpl011)->cr =	0x30;
	VPL011_REGS(vpl011)->ifls = 0x12;
	for (i = 0; i < 8; i++) {
		VPL011_REGS(vpl011)->id[i] = pl011_id[i];
	}

	return 0;
}

static int vserial_vdev_mem_read(
	struct z_virt_dev *vdev, uint64_t addr,
	uint64_t *value, uint16_t size)
{
	uint32_t offset;
	uint32_t *v;
	struct virt_pl011 *vs = (struct virt_pl011 *)vdev->priv_vdev;

	*value = 0;
	vs->set_irq = false;
	v = (uint32_t *)value;
	offset = addr - vs->vserial_base;
	if (offset == 0) {
		VDEV_REGS(vdev)->fr  &= ~VPL011_FR_RXFF;
		VDEV_REGS(vdev)->ris &= ~VPL011_INT_RX;
		vs->level	= VDEV_REGS(vdev)->ris;
		vs->enabled = VDEV_REGS(vdev)->imsc;

		struct K_fifo_data *rdata = k_fifo_get(&vs->rx_fifo, K_NO_WAIT);

		vs->count--;
		if (vs->count > 0) {
			VDEV_REGS(vdev)->ris |= VPL011_INT_RX;
			vs->level = VDEV_REGS(vdev)->ris;
		} else {
			VDEV_REGS(vdev)->fr |= VPL011_FR_RXFE;
		}
		*v = rdata->data[0];
	} else if (offset == 0x40) {
		*v = VDEV_REGS(vdev)->ris & VDEV_REGS(vdev)->imsc;
	} else if (offset >= 0xfe0 && offset < 0x1000) {
		*v = VDEV_REGS(vdev)->id[(offset - 0xfe0)>>2];
	} else {
		*v = vserial_sysreg_read32(vs->vserial_reg_base, offset);
	}

	return 0;
}

static int vserial_vdev_mem_write(
	struct z_virt_dev *vdev, uint64_t addr,
	uint64_t *value, uint16_t size)
{
	uint32_t offset;
	struct virt_pl011 *vs = (struct virt_pl011 *)vdev->priv_vdev;
	uint32_t *v = (uint32_t *)value;

	vs->set_irq = false;
	offset = addr - vs->vserial_base;
	vserial_sysreg_write32(*v, vs->vserial_reg_base, offset);

	if (offset == 0) {
		VDEV_REGS(vdev)->ris |= VPL011_INT_TX;
		vs->level = VDEV_REGS(vdev)->ris;
		vs->enabled = VDEV_REGS(vdev)->imsc;

		VDEV_REGS(vdev)->dr = 0x00;
		if (vs->connecting) {
			uart_poll_out_to_host((unsigned char)*v);
		}
	} else if (offset == 0x38) {
		vs->level = VDEV_REGS(vdev)->ris;
		vs->enabled = VDEV_REGS(vdev)->imsc;
	} else if (offset == 0x44) {
		VDEV_REGS(vdev)->imsc &= ~VDEV_REGS(vdev)->icr;
		VDEV_REGS(vdev)->ris &= ~VDEV_REGS(vdev)->icr;
	}

	return 0;
}

static int pl011_virt_serial_send(
	struct virt_serial *serial, unsigned char *data, int len)
{
	struct virt_pl011 *vpl011 =
	(struct virt_pl011 *)get_virt_serial_device(serial);
	int using_buffer_index = vpl011->count;

	for (size_t i = 0; i < len; i++) {
		serial->send_buffer[i+using_buffer_index].data[0] = data[i];
		k_fifo_put(
			&vpl011->rx_fifo,
			&serial->send_buffer[i+using_buffer_index]);
		vpl011->count++;
	}
	VPL011_REGS(vpl011)->fr &= ~PL011_FLAG_RXFE;
	if (VPL011_REGS(vpl011)->cr & 0x10 || vpl011->count == FIFO_SIZE) {
		VPL011_REGS(vpl011)->fr |= PL011_FLAG_RXFF;
	}
	if (vpl011->count >= 0) {
		VPL011_REGS(vpl011)->ris |= PL011_INT_RX;
		vpl011->set_irq = true;
		vpl011->level = VPL011_REGS(vpl011)->ris;
		vpl011->enabled = VPL011_REGS(vpl011)->imsc;
	}
	return 0;
}

static int vm_virt_serial_init(
	const struct device *dev, struct z_vm *vm,
	struct z_virt_dev *vdev_desc)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(vdev_desc);
	char name[64];
	int ret;
	uint32_t serial_base, serial_size, virq;
	struct z_virt_dev *virt_dev;
	struct virt_pl011 *vpl011;

	serial_base = VSERIAL_REG_BASE;
	serial_size = VSERIAL_REG_SIZE;

	/* check serial device */
	if (!serial_base || !serial_size) {
		ZVM_LOG_ERR("vSERIAL device has init error!\n");
		return -ENODEV;
	}

	/* Init virtual device for vm. */
	virq = VSERIAL_HIRQ_NUM;
	virt_dev = vm_virt_dev_add(
		vm, TOSTRING(VIRT_SERIAL_NAME), false, false,
		serial_base, serial_base, serial_size, virq, virq);
	if (!virt_dev) {
		ZVM_LOG_ERR("Allocat memory for vserial error\n");
		return -ENODEV;
	}
	/* Init virtual serial device for virtual device. */
	vpl011 = (struct virt_pl011 *)k_malloc(sizeof(struct virt_pl011));
	if (!vpl011) {
		ZVM_LOG_ERR("Allocat memory for vserial error\n");
		return -ENODEV;
	}

	ret = vpl011_regs_init(vm, vpl011, serial_base, serial_size);
	if (ret) {
		ZVM_LOG_ERR("Init virt serial error\n");
		return -ENODEV;
	}
	vpl011->irq = virq;
	vpl011->count = 0;
	k_fifo_init(&vpl011->rx_fifo);

	/* init virt_serial abstract instance*/
	strncpy(name, vm->vm_name, sizeof(name));
	strncat(name, "/vpl011", sizeof(name) - strlen(name) - 1);

	vpl011->vserial = virt_serial_create(
		name, &pl011_virt_serial_send, vpl011);
	vm_device_irq_init(vm, virt_dev);

	virt_dev->priv_data = serial_virtual_device_instance;
	virt_dev->priv_vdev = vpl011;
	return 0;
}

static int vm_virt_serial_deinit(
	const struct device *dev, struct z_vm *vm,
	struct z_virt_dev *vdev_desc)
{
	ARG_UNUSED(dev);
	int ret = 0;
	struct virt_pl011 *vpl011;

	vpl011 = (struct virt_pl011 *)vdev_desc->priv_vdev;
	if (vpl011->vserial) {
		virt_serial_destroy(vpl011->vserial);
	}
	k_free(vpl011);

	vdev_desc->priv_data = NULL;
	vdev_desc->priv_vdev = NULL;
	ret = vm_virt_dev_remove(vm, vdev_desc);
	return ret;
}

static int virt_pl011_init(void)
{
	int i;

	for (i = 0; i < zvm_virtual_devices_count_get(); i++) {
		const struct virtual_device_instance *virtual_device =
		zvm_virtual_device_get(i);

		if (strcmp(virtual_device->name, TOSTRING(VIRT_SERIAL_NAME))) {
			continue;
		}
		DEV_DATA(virtual_device)->vdevice_type |= VM_DEVICE_PRE_KERNEL_1;
		serial_virtual_device_instance = virtual_device;
		break;
	}

	return 0;
}

static struct virt_device_config virt_pl011_cfg = {
	.hirq_num = 0,
	.device_config = NULL,
};

static struct virt_device_data virt_pl011_data_port = {
	.device_data = NULL,
};

/**
 * @brief vserial device operations api.
 */
static const struct virt_device_api virt_pl011_api = {
	.init_fn = vm_virt_serial_init,
	.deinit_fn = vm_virt_serial_deinit,
	.virt_device_read = vserial_vdev_mem_read,
	.virt_device_write = vserial_vdev_mem_write,
};

ZVM_VIRTUAL_DEVICE_DEFINE(virt_pl011_init,
			POST_KERNEL, CONFIG_VM_VSERIAL_INIT_PRIORITY,
			VIRT_SERIAL_NAME,
			virt_pl011_data_port,
			virt_pl011_cfg,
			virt_pl011_api);
