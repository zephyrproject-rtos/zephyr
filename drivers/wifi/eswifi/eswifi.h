/**
 * Copyright (c) 2018 Linaro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WIFI_ESWIFI_ESWIFI_H_
#define ZEPHYR_DRIVERS_WIFI_ESWIFI_ESWIFI_H_

#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/net/wifi_mgmt.h>

#include "eswifi_offload.h"

#define MAX_DATA_SIZE 1600

#define AT_OK_STR "\r\nOK\r\n> "
#define AT_OK_STR_LEN 8
#define AT_RSP_DELIMITER "\r\n"
#define AT_RSP_DELIMITER_LEN 2

enum eswifi_security_type {
	ESWIFI_SEC_OPEN,
	ESWIFI_SEC_WEP,
	ESWIFI_SEC_WPA,
	ESWIFI_SEC_WPA2_AES,
	ESWIFI_SEC_WPA2_MIXED,
	ESWIFI_SEC_MAX
};

enum eswifi_request {
	ESWIFI_REQ_SCAN,
	ESWIFI_REQ_CONNECT,
	ESWIFI_REQ_DISCONNECT,
	ESWIFI_REQ_NONE
};

enum eswifi_role {
	ESWIFI_ROLE_CLIENT,
	ESWIFI_ROLE_AP,
};

struct eswifi_sta {
	char ssid[WIFI_SSID_MAX_LEN + 1];
	enum eswifi_security_type security;
	char pass[65];
	bool connected;
	uint8_t channel;
};

struct eswifi_bus_ops;

struct eswifi_cfg {
	struct gpio_dt_spec resetn;
	struct gpio_dt_spec wakeup;
};

struct eswifi_dev {
	struct net_if *iface;
	struct eswifi_bus_ops *bus;
	scan_result_cb_t scan_cb;
	struct k_work_q work_q;
	struct k_work request_work;
	struct k_work_delayable status_work;
	struct eswifi_sta sta;
	enum eswifi_request req;
	enum eswifi_role role;
	uint8_t mac[6];
	char buf[MAX_DATA_SIZE];
	struct k_mutex mutex;
	atomic_val_t mutex_owner;
	unsigned int mutex_depth;
	void *bus_data;
	struct eswifi_off_socket socket[ESWIFI_OFFLOAD_MAX_SOCKETS];
};

struct eswifi_bus_ops {
	int (*init)(struct eswifi_dev *eswifi);
	int (*request)(struct eswifi_dev *eswifi, char *cmd, size_t clen,
		       char *rsp, size_t rlen);
};

static inline int eswifi_request(struct eswifi_dev *eswifi, char *cmd,
				 size_t clen, char *rsp, size_t rlen)
{
	return eswifi->bus->request(eswifi, cmd, clen, rsp, rlen);
}

static inline void eswifi_lock(struct eswifi_dev *eswifi)
{
	/* Nested locking */
	if (atomic_get(&eswifi->mutex_owner) != (atomic_t)(uintptr_t)_current) {
		k_mutex_lock(&eswifi->mutex, K_FOREVER);
		atomic_set(&eswifi->mutex_owner, (atomic_t)(uintptr_t)_current);
		eswifi->mutex_depth = 1;
	} else {
		eswifi->mutex_depth++;
	}
}

static inline void eswifi_unlock(struct eswifi_dev *eswifi)
{
	if (!--eswifi->mutex_depth) {
		atomic_set(&eswifi->mutex_owner, -1);
		k_mutex_unlock(&eswifi->mutex);
	}
}

int eswifi_at_cmd(struct eswifi_dev *eswifi, char *cmd);
static inline int __select_socket(struct eswifi_dev *eswifi, uint8_t idx)
{
	snprintk(eswifi->buf, sizeof(eswifi->buf), "P0=%d\r", idx);
	return eswifi_at_cmd(eswifi, eswifi->buf);
}

static inline
struct eswifi_dev *eswifi_socket_to_dev(struct eswifi_off_socket *socket)
{
	return CONTAINER_OF(socket - socket->index, struct eswifi_dev, socket);
}

struct eswifi_bus_ops *eswifi_get_bus(void);
int eswifi_offload_init(struct eswifi_dev *eswifi);
struct eswifi_dev *eswifi_by_iface_idx(uint8_t iface);
int eswifi_at_cmd_rsp(struct eswifi_dev *eswifi, char *cmd, char **rsp);
void eswifi_async_msg(struct eswifi_dev *eswifi, char *msg, size_t len);
void eswifi_offload_async_msg(struct eswifi_dev *eswifi, char *msg, size_t len);
int eswifi_socket_create(int family, int type, int proto);

int eswifi_socket_type_from_zephyr(int proto, enum eswifi_transport_type *type);

int __eswifi_socket_free(struct eswifi_dev *eswifi,
			 struct eswifi_off_socket *socket);
int __eswifi_socket_new(struct eswifi_dev *eswifi, int family, int type,
			int proto, void *context);
int __eswifi_off_start_client(struct eswifi_dev *eswifi,
			      struct eswifi_off_socket *socket);
int __eswifi_listen(struct eswifi_dev *eswifi, struct eswifi_off_socket *socket, int backlog);
int __eswifi_accept(struct eswifi_dev *eswifi, struct eswifi_off_socket *socket);
int __eswifi_bind(struct eswifi_dev *eswifi, struct eswifi_off_socket *socket,
		  const struct sockaddr *addr, socklen_t addrlen);
#if defined(CONFIG_NET_SOCKETS_OFFLOAD)
int eswifi_socket_offload_init(struct eswifi_dev *leswifi);
#endif

#if defined(CONFIG_WIFI_ESWIFI_SHELL)
void eswifi_shell_register(struct eswifi_dev *dev);
#else
#define eswifi_shell_register(dev)
#endif

#endif
