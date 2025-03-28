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
#include <zephyr/zvm/vdev/vgic_common.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/shell/shell_uart.h>


LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

extern struct zvm_manage_info *zvm_overall_info;

static struct z_vserial_ctrl vserial_ctrl;
static struct k_thread tx_it_emulator_thread_data;

K_SEM_DEFINE(connect_vm_sem, 0, 1);
K_THREAD_STACK_DEFINE(tx_it_emulator_thread_stack, 1024);

uint32_t virt_serial_count(void)
{
	uint32_t retval = 0;
	struct virt_serial *vs;

	k_mutex_lock(&vserial_ctrl.virt_serial_list_lock, K_FOREVER);

	sys_dnode_t *vserial_node;

	SYS_DLIST_FOR_EACH_NODE(&vserial_ctrl.virt_serial_list, vserial_node) {
		vs = CONTAINER_OF(vserial_node, struct virt_serial, node);
		printk(
			"[%d]serial name:%s ,vmid:%d\n",
			retval, vs->name,
			((struct z_vm *)vs->vm)->vmid);
		retval++;
	}
	k_mutex_unlock(&vserial_ctrl.virt_serial_list_lock);

	return retval;
}


struct virt_serial *get_vserial(uint8_t vmid)
{
	struct virt_serial *serial = NULL;
	struct virt_serial *tmpserial;
	sys_dnode_t *vserial_node;

	SYS_DLIST_FOR_EACH_NODE(&vserial_ctrl.virt_serial_list, vserial_node) {
		tmpserial = CONTAINER_OF(vserial_node, struct virt_serial, node);
		if (((struct z_vm *)(tmpserial->vm))->vmid == vmid) {
			serial = tmpserial;
		}

	}
	if (serial == NULL) {
		printk("No virtual serial devices[vmid:%d]\n", vmid);
		return NULL;
	}

	return serial;
}

struct virt_serial *virt_serial_create(const char *name,
					   int (*send)(struct virt_serial *, unsigned char *, int),
					   void *priv)
{
	bool found = false;
	struct virt_serial *vserial = NULL;
	struct virt_pl011 *v_s;
	sys_dnode_t *vserial_node;

	if (!name) {
		return NULL;
	}

	k_mutex_lock(&vserial_ctrl.virt_serial_list_lock, K_FOREVER);
	SYS_DLIST_FOR_EACH_NODE(&vserial_ctrl.virt_serial_list, vserial_node) {
		vserial = CONTAINER_OF(vserial_node, struct virt_serial, node);
		if (strcmp(name, vserial->name) == 0) {
			found = true;
			break;
		}
	}

	if (found) {
		k_mutex_unlock(&vserial_ctrl.virt_serial_list_lock);
		v_s = (struct virt_pl011 *)priv;
		vserial->send = send;
		vserial->vm = v_s->vm;
		vserial->priv = priv;
		vserial->count = 0;
		return vserial;
	}

	vserial = k_calloc(1, (sizeof(struct virt_serial)));
	if (!vserial) {
		k_mutex_unlock(&vserial_ctrl.virt_serial_list_lock);
		return NULL;
	}
	vserial->count = 0;

	if (strlen(name) >= sizeof(vserial->name)) {
		k_free(vserial);
		k_mutex_unlock(&vserial_ctrl.virt_serial_list_lock);
		return NULL;
	}

	strncpy(vserial->name, name, sizeof(vserial->name));

	v_s = (struct virt_pl011 *)priv;
	vserial->send = send;
	vserial->vm = v_s->vm;
	vserial->priv = priv;
	sys_dnode_init(&vserial->node);
	sys_dlist_append(&vserial_ctrl.virt_serial_list, &vserial->node);
	k_mutex_unlock(&vserial_ctrl.virt_serial_list_lock);
	ZVM_LOG_INFO("Create virt_serial:%s for %s\n", name, v_s->vm->vm_name);

	return vserial;
}

int virt_serial_destroy(struct virt_serial *vserial)
{
	const struct shell *vs_shell = shell_backend_uart_get_ptr();

	vs_shell->ctx->bypass = NULL;
	sys_dlist_remove(&vserial->node);
	k_free(vserial);

	return 0;
}

static void vserial_it_emulator_thread(void *ctrl, void *arg2, void *arg3)
{
	struct virt_pl011 *vpl011;

	while (1) {
		k_sem_take(&connect_vm_sem, K_FOREVER);
		vpl011 = (struct virt_pl011 *)vserial_ctrl.connecting_vserial->priv;
		while (vserial_ctrl.connecting) {
			if (vpl011->enabled & vpl011->level) {
				set_virq_to_vm(
					vserial_ctrl.connecting_vserial->vm,
					vpl011->irq);
			}
			k_sleep(K_MSEC(1));
		}
	}
}

static void init_vserial_it_emulator_thread(void)
{
	k_tid_t tid;

	tid = k_thread_create(&tx_it_emulator_thread_data, tx_it_emulator_thread_stack,
					K_THREAD_STACK_SIZEOF(tx_it_emulator_thread_stack),
					vserial_it_emulator_thread, NULL, NULL, NULL,
					K_PRIO_COOP(7), 0, K_NO_WAIT);
	k_thread_name_set(tid, "vserial_it_emulator");
}

static int vserial_ctrl_init(void)
{
	memset(&vserial_ctrl, 0, sizeof(vserial_ctrl));

	k_mutex_init(&vserial_ctrl.virt_serial_list_lock);
	sys_dlist_init(&vserial_ctrl.virt_serial_list);
	init_vserial_it_emulator_thread();

	return 0;
}

SYS_INIT(vserial_ctrl_init, POST_KERNEL, CONFIG_vserial_ctrl_BLOCK_INIT_PRIORITY);



void uart_poll_out_to_host(unsigned char data)
{
	const struct shell *vs_shell = shell_backend_uart_get_ptr();
	const struct device *dev = ((struct shell_uart_common *)vs_shell->iface->ctx)->dev;

	uart_poll_out(dev, data);
}

void transfer(const struct shell *vs_shell, uint8_t *data, size_t len)
{
	uint8_t *rdata;

	if (data[0] == EXIT_VSERIAL_KEY) {
		shell_set_bypass(vs_shell, NULL);
		((struct virt_pl011 *)(vserial_ctrl.connecting_vserial->priv))->connecting = false;
		vserial_ctrl.connecting = false;
		vserial_ctrl.connecting_vm_id = 0;
		rdata = k_malloc(sizeof(uint8_t));
		*rdata = '\n';
		uart_poll_out_to_host('\n');
	} else {
		vserial_ctrl.connecting_vserial->send(vserial_ctrl.connecting_vserial, data, len);
	}
}

int switch_virtual_serial_handler(const struct shell *vs_shell, size_t argc, char **argv)
{
	uint8_t id;
	uint8_t *data;
	struct virt_serial *serial;
	struct shell_uart_int_driven *shell_uart;

	if (argc > 1) {
		if (argv[1][1] != '\0') {
			ZVM_LOG_WARN("Only supports VM ID with a length of 1.\n");
			return 0;
		}

		id = *argv[1];
		if (id > '9' || id < '0') {
			ZVM_LOG_WARN("Invalid VM ID %c\n", id);
			return 0;
		}
		id = id - 48;
		if (id > CONFIG_MAX_VM_NUM - 1) {
			ZVM_LOG_WARN("Max VM ID is %d\n", CONFIG_MAX_VM_NUM - 1);
			return 0;
		}
		if (!(BIT(id) & zvm_overall_info->alloced_vmid)) {
			ZVM_LOG_WARN("VM ID %d not alloced\n", id);
			return 0;
		}

		serial = get_vserial(id);
		((struct virt_pl011 *)(serial->priv))->connecting = true;
		vserial_ctrl.connecting = true;
		vserial_ctrl.connecting_vm_id = id;
		vserial_ctrl.connecting_vserial = serial;

		shell_set_bypass(vs_shell, transfer);
		k_sem_give(&connect_vm_sem);
		data = k_malloc(sizeof(uint8_t));
		*data = '\r';
		shell_uart = (struct shell_uart_int_driven *)vs_shell->iface->ctx;
		ring_buf_put(&shell_uart->rx_ringbuf, data, 1);
		shell_fprintf(vs_shell, SHELL_VT100_COLOR_YELLOW, "Connecting VM ID:%d\n", id);
	} else {
		ZVM_LOG_INFO("Reachable virtual serial:\n");
		virt_serial_count();
	}

	return 0;
}
