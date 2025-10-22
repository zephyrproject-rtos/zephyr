/*
 * Copyright (c) 2022 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/modem/pipe.h>
#include <zephyr/modem/stats.h>

#ifndef ZEPHYR_MODEM_PPP_
#define ZEPHYR_MODEM_PPP_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Modem PPP
 * @defgroup modem_ppp Modem PPP
 * @since 3.5
 * @version 1.0.0
 * @ingroup modem
 * @{
 */

/** L2 network interface init callback */
typedef void (*modem_ppp_init_iface)(struct net_if *iface);

/**
 * @cond INTERNAL_HIDDEN
 */

enum modem_ppp_receive_state {
	/* Searching for start of frame and header */
	MODEM_PPP_RECEIVE_STATE_HDR_SOF = 0,
	MODEM_PPP_RECEIVE_STATE_HDR_FF,
	MODEM_PPP_RECEIVE_STATE_HDR_7D,
	MODEM_PPP_RECEIVE_STATE_HDR_23,
	/* Writing bytes to network packet */
	MODEM_PPP_RECEIVE_STATE_WRITING,
	/* Unescaping next byte before writing to network packet */
	MODEM_PPP_RECEIVE_STATE_UNESCAPING,
};

enum modem_ppp_transmit_state {
	MODEM_PPP_TRANSMIT_STATE_IDLE = 0,
	MODEM_PPP_TRANSMIT_STATE_SOF,
	MODEM_PPP_TRANSMIT_STATE_PROTOCOL,
	MODEM_PPP_TRANSMIT_STATE_DATA,
	MODEM_PPP_TRANSMIT_STATE_EOF,
};

struct modem_ppp {
	/* Network interface instance is bound to */
	struct net_if *iface;

	/* Hook for PPP L2 network interface initialization */
	modem_ppp_init_iface init_iface;

	atomic_t state;

	/* Buffers used for processing partial frames */
	uint8_t *receive_buf;
	uint8_t *transmit_buf;
	uint16_t buf_size;

	/* Wrapped PPP frames are sent and received through this pipe */
	struct modem_pipe *pipe;

	/* Receive PPP frame state */
	enum modem_ppp_receive_state receive_state;

	/* Allocated network packet being created */
	struct net_pkt *rx_pkt;

	/* Packet being sent */
	enum modem_ppp_transmit_state transmit_state;
	struct net_pkt *tx_pkt;
	uint16_t tx_pkt_fcs;

	/* Ring buffer used for transmitting partial PPP frame */
	struct ring_buf transmit_rb;

	struct k_fifo tx_pkt_fifo;

	/* Work */
	struct k_work send_work;
	struct k_work process_work;

#if defined(CONFIG_NET_STATISTICS_PPP)
	struct net_stats_ppp stats;
#endif

#if CONFIG_MODEM_STATS
	struct modem_stats_buffer receive_buf_stats;
	struct modem_stats_buffer transmit_buf_stats;
#endif
};

struct modem_ppp_config {
	const struct device *dev;
};

/**
 * @endcond
 */

/**
 * @brief Attach pipe to instance and connect
 *
 * @param ppp Modem PPP instance
 * @param pipe Pipe to attach to modem PPP instance
 */
int modem_ppp_attach(struct modem_ppp *ppp, struct modem_pipe *pipe);

/**
 * @brief Get network interface modem PPP instance is bound to
 *
 * @param ppp Modem PPP instance
 * @returns Pointer to network interface modem PPP instance is bound to
 */
struct net_if *modem_ppp_get_iface(struct modem_ppp *ppp);

/**
 * @brief Release pipe from instance
 *
 * @param ppp Modem PPP instance
 */
void modem_ppp_release(struct modem_ppp *ppp);

/**
 * @cond INTERNAL_HIDDEN
 */

/**
 * @brief Initialize modem PPP instance device
 * @param dev Device instance associated with network interface
 * @warning Should not be used directly
 */
int modem_ppp_init_internal(const struct device *dev);

/**
 * @endcond
 */

/**
 * @brief Define a modem PPP module and bind it to a network interface
 *
 * @details This macro defines the modem_ppp instance, initializes a PPP L2
 * network device instance, and binds the modem_ppp instance to the PPP L2
 * instance.
 *
 * If underlying cellular device is given, the PPP interface will manage the
 * power state of the cellular device when starting and stopping the PPP.
 *
 * @param _dev Cellular device instance for power management or NULL if not used
 * @param _name Name of the statically defined modem_ppp instance
 * @param _init_iface Hook for the PPP L2 network interface init function
 * @param _prio Initialization priority of the PPP L2 net iface
 * @param _mtu Max size of net_pkt data sent and received on PPP L2 net iface
 * @param _buf_size Size of partial PPP frame transmit and receive buffers
 */
#define MODEM_DEV_PPP_DEFINE(_dev, _name, _init_iface, _prio, _mtu, _buf_size)                     \
	extern const struct ppp_api modem_ppp_ppp_api;                                             \
                                                                                                   \
	static uint8_t _CONCAT(_name, _receive_buf)[_buf_size];                                    \
	static uint8_t _CONCAT(_name, _transmit_buf)[_buf_size];                                   \
                                                                                                   \
	static struct modem_ppp _name = {                                                          \
		.init_iface = _init_iface,                                                         \
		.receive_buf = _CONCAT(_name, _receive_buf),                                       \
		.transmit_buf = _CONCAT(_name, _transmit_buf),                                     \
		.buf_size = _buf_size,                                                             \
	};                                                                                         \
	static const struct modem_ppp_config _CONCAT(_name, _config) = {                           \
		.dev = _dev,                                                                       \
	};                                                                                         \
                                                                                                   \
	NET_DEVICE_INIT(_CONCAT(ppp_net_dev_, _name), "modem_ppp_" #_name,                         \
			modem_ppp_init_internal, NULL, &_name, &_CONCAT(_name, _config), _prio,    \
			&modem_ppp_ppp_api, PPP_L2, NET_L2_GET_CTX_TYPE(PPP_L2), _mtu)

/**
 * @brief Define a modem PPP module for cellular device tree instance.
 *
 * @see MODEM_DEV_PPP_DEFINE
 */
#define MODEM_DT_INST_PPP_DEFINE(inst, _name, _init_iface, _prio, _mtu, _buf_size)                 \
	MODEM_DEV_PPP_DEFINE(DEVICE_DT_INST_GET(inst), _name, _init_iface, _prio, _mtu, _buf_size)

/**
 * @brief Define a modem PPP module without a device and bind it to a network interface.
 *
 * @see MODEM_DEV_PPP_DEFINE
 */
#define MODEM_PPP_DEFINE(_name, _init_iface, _prio, _mtu, _buf_size)                               \
	MODEM_DEV_PPP_DEFINE(NULL, _name, _init_iface, _prio, _mtu, _buf_size)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_MODEM_PPP_ */
