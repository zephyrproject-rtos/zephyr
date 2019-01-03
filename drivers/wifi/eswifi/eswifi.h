/**
 * Copyright (c) 2018 Linaro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_WIFI_ESWIFI_ESWIFI_H_
#define ZEPHYR_DRIVERS_WIFI_ESWIFI_ESWIFI_H_

#include <zephyr.h>
#include <kernel.h>

#include <net/wifi_mgmt.h>

#include "eswifi_offload.h"

#define MAX_DATA_SIZE 1600

#define AT_OK_STR "\r\nOK\r\n> "
#define AT_OK_STR_LEN 8
#define AT_RSP_DELIMITER "\r\n"
#define AT_RSP_DELIMITER_LEN 2

struct eswifi_gpio {
	struct device *dev;
	unsigned int pin;
};

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

struct eswifi_dev {
	struct net_if *iface;
	struct eswifi_bus_ops *bus;
	struct eswifi_gpio resetn;
	struct eswifi_gpio wakeup;
	scan_result_cb_t scan_cb;
	struct k_work_q work_q;
	struct k_work request_work;
	struct eswifi_sta sta;
	enum eswifi_request req;
	enum eswifi_role role;
	u8_t mac[6];
	char buf[MAX_DATA_SIZE];
	struct k_mutex mutex;
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
	k_mutex_lock(&eswifi->mutex, K_FOREVER);
}

static inline void eswifi_unlock(struct eswifi_dev *eswifi)
{
	k_mutex_unlock(&eswifi->mutex);
}

extern struct eswifi_bus_ops eswifi_bus_ops_spi;
int eswifi_offload_init(struct eswifi_dev *eswifi);
struct eswifi_dev *eswifi_by_iface_idx(u8_t iface);
int eswifi_at_cmd_rsp(struct eswifi_dev *eswifi, char *cmd, char **rsp);
int eswifi_at_cmd(struct eswifi_dev *eswifi, char *cmd);

#endif
