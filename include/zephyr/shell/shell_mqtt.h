/*
 * Copyright (c) 2022 G-Technologies Sdn. Bhd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SHELL_MQTT_H_
#define ZEPHYR_INCLUDE_SHELL_MQTT_H_

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/sys/ring_buffer.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RX_RB_SIZE CONFIG_SHELL_MQTT_RX_BUF_SIZE
#define TX_BUF_SIZE CONFIG_SHELL_MQTT_TX_BUF_SIZE
#define SH_MQTT_BUFFER_SIZE 64
#define DEVICE_ID_BIN_MAX_SIZE 3
#define DEVICE_ID_HEX_MAX_SIZE ((DEVICE_ID_BIN_MAX_SIZE * 2) + 1)
#define SH_MQTT_TOPIC_RX_MAX_SIZE DEVICE_ID_HEX_MAX_SIZE + sizeof(CONFIG_SHELL_MQTT_TOPIC_RX_ID)
#define SH_MQTT_TOPIC_TX_MAX_SIZE DEVICE_ID_HEX_MAX_SIZE + sizeof(CONFIG_SHELL_MQTT_TOPIC_TX_ID)

extern const struct shell_transport_api shell_mqtt_transport_api;

struct shell_mqtt_tx_buf {
	/** tx buffer. */
	char buf[TX_BUF_SIZE];

	/** Current tx buf length. */
	uint16_t len;
};

/** MQTT-based shell transport. */
struct shell_mqtt {
	char device_id[DEVICE_ID_HEX_MAX_SIZE];
	char sub_topic[SH_MQTT_TOPIC_RX_MAX_SIZE];
	char pub_topic[SH_MQTT_TOPIC_TX_MAX_SIZE];

	/** Handler function registered by shell. */
	shell_transport_handler_t shell_handler;

	struct ring_buf rx_rb;
	uint8_t rx_rb_buf[RX_RB_SIZE];
	uint8_t *rx_rb_ptr;

	struct shell_mqtt_tx_buf tx_buf;

	/** Context registered by shell. */
	void *shell_context;

	/** The mqtt client struct */
	struct mqtt_client mqtt_cli;

	/* Buffers for MQTT client. */
	struct buffer {
		uint8_t rx[SH_MQTT_BUFFER_SIZE];
		uint8_t tx[SH_MQTT_BUFFER_SIZE];
	} buf;

	struct k_mutex lock;

	/** MQTT Broker details. */
	struct sockaddr_storage broker;

	struct zsock_addrinfo *haddr;
	struct zsock_pollfd fds[1];
	int nfds;

	struct mqtt_publish_param pub_data;

	struct net_mgmt_event_callback mgmt_cb;

	/** work */
	struct k_work_q workq;
	struct k_work net_disconnected_work;
	struct k_work_delayable connect_dwork;
	struct k_work_delayable subscribe_dwork;
	struct k_work_delayable process_dwork;
	struct k_work_delayable publish_dwork;

	/** MQTT connection states */
	enum sh_mqtt_transport_state {
		SHELL_MQTT_TRANSPORT_DISCONNECTED,
		SHELL_MQTT_TRANSPORT_CONNECTED,
	} transport_state;

	/** MQTT subscription states */
	enum sh_mqtt_subscribe_state {
		SHELL_MQTT_NOT_SUBSCRIBED,
		SHELL_MQTT_SUBSCRIBED,
	} subscribe_state;

	/** Network states */
	enum sh_mqtt_network_state {
		SHELL_MQTT_NETWORK_DISCONNECTED,
		SHELL_MQTT_NETWORK_CONNECTED,
	} network_state;
};

#define SHELL_MQTT_DEFINE(_name)                                                                   \
	static struct shell_mqtt _name##_shell_mqtt;                                               \
	struct shell_transport _name = { .api = &shell_mqtt_transport_api,                         \
					 .ctx = (struct shell_mqtt *)&_name##_shell_mqtt }

/**
 * @brief This function provides pointer to shell mqtt backend instance.
 *
 * Function returns pointer to the shell mqtt instance. This instance can be
 * next used with shell_execute_cmd function in order to test commands behavior.
 *
 * @returns Pointer to the shell instance.
 */
const struct shell *shell_backend_mqtt_get_ptr(void);

/**
 * @brief Function to define the device ID (devid) for which the shell mqtt backend uses as a
 * client ID when it connects to the broker. It will publish its output to devid_tx and subscribe
 * to devid_rx for input .
 *
 * @note This is a weak-linked function, and can be overridden if desired.
 *
 * @param id Pointer to the devid buffer
 * @param id_max_len Maximum size of the devid buffer defined by DEVICE_ID_HEX_MAX_SIZE
 *
 * @return true if length of devid > 0
 */
bool shell_mqtt_get_devid(char *id, int id_max_len);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SHELL_MQTT_H_ */
