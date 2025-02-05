/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Yuhao Hu, Qingqiao Wang and etc.
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_VIRT_SERIAL_H_
#define ZEPHYR_INCLUDE_ZVM_VIRT_SERIAL_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/zvm/vdev/vpl011.h>
#include <zephyr/shell/shell.h>

#define VM_FIELD_NAME_SIZE			32
#define EXIT_VSERIAL_KEY			0x18 /* CTRL+X */
#define SEND_BUFFER_SIZE			16
#define VIRT_SERIAL_NAME			vpl011

/**
 * @brief data strcut for k_fifo
 */
struct K_fifo_data {
	intptr_t _unused;
	uint8_t data[1];
};

/** Virtual serial port */
struct virt_serial {
	sys_dnode_t node;
	char name[VM_FIELD_NAME_SIZE];
	int (*send)(struct virt_serial *vserial, unsigned char *data, int len);
	struct K_fifo_data send_buffer[SEND_BUFFER_SIZE];
	uint32_t count;
	void *priv;
	void *vm;
};

struct z_vserial_ctrl {
	struct k_mutex virt_serial_list_lock;
	sys_dlist_t virt_serial_list;
	bool connecting;
	uint8_t connecting_vm_id;
	struct virt_serial *connecting_vserial;
};

static inline void *get_virt_serial_device(struct virt_serial *vserial)
{
	return (vserial) ? vserial->priv : NULL;
}


/**
 * @brief Create a virtual serial port
 */
struct virt_serial *virt_serial_create(const char *name,
				int (*send)(struct virt_serial *, unsigned char *, int),
				void *priv);
/**
 * @brief Destroy a virtual serial port
 */
int virt_serial_destroy(struct virt_serial *vserial);

/** Count of available virtual serial ports */
uint32_t virt_serial_count(void);

struct virt_serial *get_vserial(uint8_t vmid);

void transfer(const struct shell *shell, unsigned char *data, size_t len);

void uart_poll_out_to_host(unsigned char data);

int switch_virtual_serial_handler(const struct shell *shell, size_t argc, char **argv);

#endif	/* ZEPHYR_INCLUDE_ZVM_VIRT_SERIAL_H_ */
