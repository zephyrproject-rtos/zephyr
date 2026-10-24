/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Telnet COM port control (RFC 2217)
 */

#ifndef ZEPHYR_INCLUDE_NET_RFC2217_H_
#define ZEPHYR_INCLUDE_NET_RFC2217_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/util.h>

#if defined(CONFIG_NET_RFC2217_SERVER)
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_service.h>
#include <zephyr/sys/ring_buffer.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Telnet COM port control (RFC 2217)
 * @defgroup rfc2217 RFC 2217
 * @since 4.5
 * @version 0.1.0
 * @ingroup networking
 * @{
 */

/** SET-PARITY values */
enum rfc2217_parity {
	RFC2217_PARITY_REQUEST = 0, /**< Current value requested */
	RFC2217_PARITY_NONE = 1,    /**< No parity */
	RFC2217_PARITY_ODD = 2,     /**< Odd parity */
	RFC2217_PARITY_EVEN = 3,    /**< Even parity */
	RFC2217_PARITY_MARK = 4,    /**< Mark parity */
	RFC2217_PARITY_SPACE = 5,   /**< Space parity */
};

/** SET-STOPSIZE values */
enum rfc2217_stop_size {
	RFC2217_STOP_SIZE_REQUEST = 0, /**< Current value requested */
	RFC2217_STOP_SIZE_1 = 1,       /**< 1 stop bit */
	RFC2217_STOP_SIZE_2 = 2,       /**< 2 stop bits */
	RFC2217_STOP_SIZE_1_5 = 3,     /**< 1.5 stop bits */
};

/** SET-CONTROL values */
enum rfc2217_control {
	RFC2217_CONTROL_FLOW_REQUEST = 0,           /**< Outbound flow control requested */
	RFC2217_CONTROL_FLOW_NONE = 1,              /**< No outbound flow control */
	RFC2217_CONTROL_FLOW_XON_XOFF = 2,          /**< XON/XOFF outbound flow control */
	RFC2217_CONTROL_FLOW_HARDWARE = 3,          /**< Hardware outbound flow control */
	RFC2217_CONTROL_BREAK_REQUEST = 4,          /**< BREAK state requested */
	RFC2217_CONTROL_BREAK_ON = 5,               /**< BREAK on */
	RFC2217_CONTROL_BREAK_OFF = 6,              /**< BREAK off */
	RFC2217_CONTROL_DTR_REQUEST = 7,            /**< DTR state requested */
	RFC2217_CONTROL_DTR_ON = 8,                 /**< DTR on */
	RFC2217_CONTROL_DTR_OFF = 9,                /**< DTR off */
	RFC2217_CONTROL_RTS_REQUEST = 10,           /**< RTS state requested */
	RFC2217_CONTROL_RTS_ON = 11,                /**< RTS on */
	RFC2217_CONTROL_RTS_OFF = 12,               /**< RTS off */
	RFC2217_CONTROL_INBOUND_FLOW_REQUEST = 13,  /**< Inbound flow control requested */
	RFC2217_CONTROL_INBOUND_FLOW_NONE = 14,     /**< No inbound flow control */
	RFC2217_CONTROL_INBOUND_FLOW_XON_XOFF = 15, /**< XON/XOFF inbound flow control */
	RFC2217_CONTROL_INBOUND_FLOW_HARDWARE = 16, /**< Hardware inbound flow control */
	RFC2217_CONTROL_FLOW_DCD = 17,              /**< DCD outbound flow control */
	RFC2217_CONTROL_INBOUND_FLOW_DTR = 18,      /**< DTR inbound flow control */
	RFC2217_CONTROL_FLOW_DSR = 19,              /**< DSR outbound flow control */
};

/** PURGE-DATA values */
enum rfc2217_purge {
	RFC2217_PURGE_RX = 1,   /**< Data received from the COM port */
	RFC2217_PURGE_TX = 2,   /**< Data to transmit on the COM port */
	RFC2217_PURGE_BOTH = 3, /**< Both */
};

/**
 * @name NOTIFY-MODEMSTATE bits
 * @{
 */
#define RFC2217_MODEM_CD    BIT(7) /**< Receive line signal detect (CD) */
#define RFC2217_MODEM_RI    BIT(6) /**< Ring indicator */
#define RFC2217_MODEM_DSR   BIT(5) /**< Data set ready */
#define RFC2217_MODEM_CTS   BIT(4) /**< Clear to send */
#define RFC2217_MODEM_DCD_D BIT(3) /**< Delta receive line signal detect */
#define RFC2217_MODEM_RI_D  BIT(2) /**< Trailing edge ring detector */
#define RFC2217_MODEM_DSR_D BIT(1) /**< Delta data set ready */
#define RFC2217_MODEM_CTS_D BIT(0) /**< Delta clear to send */
/** @} */

struct rfc2217_session;

/**
 * @brief Session callbacks
 *
 * The setters get the value the client asked for, or 0 when the client asks
 * for the current value, and return the value now in effect: the client gets
 * that value back, and takes a different one as a refusal.
 */
struct rfc2217_session_ops {
	/** Serial data from the client, Telnet escapes removed (required). */
	void (*data)(struct rfc2217_session *session, const uint8_t *buf, size_t len);
	/** Telnet bytes for the client (required). */
	void (*send)(struct rfc2217_session *session, const uint8_t *buf, size_t len);
	/** SET-BAUDRATE (required). */
	uint32_t (*baud_rate)(struct rfc2217_session *session, uint32_t baud_rate);
	/** SET-DATASIZE, 5 to 8 bits (required). */
	uint8_t (*data_size)(struct rfc2217_session *session, uint8_t data_size);
	/** SET-PARITY, see @ref rfc2217_parity (required). */
	uint8_t (*parity)(struct rfc2217_session *session, uint8_t parity);
	/** SET-STOPSIZE, see @ref rfc2217_stop_size (required). */
	uint8_t (*stop_size)(struct rfc2217_session *session, uint8_t stop_size);
	/** SET-CONTROL, see @ref rfc2217_control (required). */
	uint8_t (*control)(struct rfc2217_session *session, uint8_t control);
	/** PURGE-DATA, see @ref rfc2217_purge (optional). */
	void (*purge)(struct rfc2217_session *session, uint8_t purge);
	/** FLOWCONTROL-SUSPEND (true) and FLOWCONTROL-RESUME (false) (optional). */
	void (*suspend)(struct rfc2217_session *session, bool suspend);
	/** Modem lines for a client polling them, see NOTIFY-MODEMSTATE bits (optional). */
	uint8_t (*modem_state)(struct rfc2217_session *session);
};

/**
 * @brief The server side of one Telnet connection
 *
 * Transport agnostic: the application feeds it the bytes received from the
 * client and gets serial data, replies and port settings through
 * @ref rfc2217_session_ops.
 */
struct rfc2217_session {
	/** Callbacks */
	const struct rfc2217_session_ops *ops;
	/** Application data */
	void *user_data;
	/** @cond INTERNAL_HIDDEN */
	uint8_t state;
	uint8_t verb;
	uint8_t sb[16];
	uint8_t sb_len;
	bool sb_overflow;
	bool cr;
	uint8_t we;
	uint8_t they;
	uint8_t we_asked;
	uint8_t they_asked;
	/** @endcond */
};

/**
 * @brief Start a session
 *
 * Resets @p session and sends the server's opening negotiation.
 *
 * @param session Session
 * @param ops Callbacks
 * @param user_data Application data
 */
void rfc2217_session_init(struct rfc2217_session *session, const struct rfc2217_session_ops *ops,
			  void *user_data);

/**
 * @brief Feed bytes received from the client
 *
 * @param session Session
 * @param buf Bytes
 * @param len Number of bytes
 */
void rfc2217_session_input(struct rfc2217_session *session, const uint8_t *buf, size_t len);

/**
 * @brief Whether the client agreed to the COM port control option
 *
 * @param session Session
 *
 * @return true once either side has the option enabled
 */
bool rfc2217_session_is_active(const struct rfc2217_session *session);

/**
 * @brief Escape serial data for the client
 *
 * Copies @p in to @p out, doubling each 0xFF (IAC) byte, until @p out is full.
 *
 * @param in Serial data
 * @param in_len Number of bytes in @p in
 * @param out Output buffer
 * @param out_size Size of @p out
 * @param consumed Number of bytes of @p in copied
 *
 * @return Number of bytes written to @p out
 */
size_t rfc2217_escape(const uint8_t *in, size_t in_len, uint8_t *out, size_t out_size,
		      size_t *consumed);

#if defined(CONFIG_NET_RFC2217_SERVER) || defined(__DOXYGEN__)

struct rfc2217_server;

/**
 * @brief Line controls the UART API does not cover
 *
 * Both are optional, and run in a thread.
 */
struct rfc2217_server_callbacks {
	/**
	 * Assert (@p on true) or release a BREAK on the line.  Without it, the
	 * server refuses BREAK requests.
	 */
	int (*set_break)(struct rfc2217_server *server, bool on);
	/**
	 * Set the DTR (UART_LINE_CTRL_DTR) or RTS (UART_LINE_CTRL_RTS) line.
	 * Without it, the server uses uart_line_ctrl_set().
	 */
	int (*set_line)(struct rfc2217_server *server, uint32_t line, bool on);
};

/** Server configuration */
struct rfc2217_server_config {
	/** UART the clients get, with interrupt-driven API */
	const struct device *uart;
	/** TCP port */
	uint16_t port;
};

/**
 * @brief RFC 2217 server
 *
 * One UART over TCP, one client at a time: a new connection replaces the
 * current one.  Define it with @ref RFC2217_SERVER_DEFINE, or get one
 * defined in the devicetree with @ref RFC2217_SERVER_DT_GET.
 */
struct rfc2217_server {
	/** Application data, for the callbacks */
	void *user_data;
	/** @cond INTERNAL_HIDDEN */
	const struct net_socket_service_desc *service;
	struct ring_buf *rx;
	struct ring_buf *tx;
	const struct device *uart;
	const struct rfc2217_server_callbacks *cb;
	struct rfc2217_session session;
	struct k_mutex lock;
	struct k_work_delayable work;
	struct uart_config line;
	struct zsock_pollfd fds[3];
	struct zsock_pollfd registered[3];
	size_t out_len;
	size_t out_off;
	uint16_t port;
	bool running;
	bool suspended;
	bool brk;
	bool dtr;
	bool rts;
	uint8_t out[CONFIG_NET_RFC2217_SERVER_OUT_SIZE];
	/** @endcond */
};

/** @cond INTERNAL_HIDDEN */
void z_rfc2217_server_service(struct net_socket_service_event *pev);

#define Z_RFC2217_SERVER_DEFINE(_name)                                                             \
	NET_SOCKET_SERVICE_SYNC_DEFINE_STATIC(_name##_service, z_rfc2217_server_service, 3);       \
	static uint8_t _name##_rx_data[CONFIG_NET_RFC2217_SERVER_RX_SIZE];                         \
	static uint8_t _name##_tx_data[CONFIG_NET_RFC2217_SERVER_TX_SIZE];                         \
	static struct ring_buf _name##_rx =                                                        \
		RING_BUF_INIT(_name##_rx_data, CONFIG_NET_RFC2217_SERVER_RX_SIZE);                 \
	static struct ring_buf _name##_tx =                                                        \
		RING_BUF_INIT(_name##_tx_data, CONFIG_NET_RFC2217_SERVER_TX_SIZE);                 \
	struct rfc2217_server _name = {                                                            \
		.service = &_name##_service,                                                       \
		.rx = &_name##_rx,                                                                 \
		.tx = &_name##_tx,                                                                 \
	}

#define Z_RFC2217_SERVER_DT_NAME(node_id) _CONCAT(__rfc2217_server_dt_, DT_DEP_ORD(node_id))

#define Z_RFC2217_SERVER_DT_DECLARE(node_id)                                                       \
	extern struct rfc2217_server Z_RFC2217_SERVER_DT_NAME(node_id);

DT_FOREACH_STATUS_OKAY(zephyr_rfc2217_server, Z_RFC2217_SERVER_DT_DECLARE)
/** @endcond */

/**
 * @brief Define a server
 *
 * @param _name Name of the @ref rfc2217_server variable
 */
#define RFC2217_SERVER_DEFINE(_name) Z_RFC2217_SERVER_DEFINE(_name)

/**
 * @brief Get the server of a devicetree node
 *
 * A @c zephyr,rfc2217-server node is a server started at boot.
 *
 * @param node_id Node identifier of a @c zephyr,rfc2217-server node
 */
#define RFC2217_SERVER_DT_GET(node_id) (&Z_RFC2217_SERVER_DT_NAME(node_id))

/**
 * @brief Start a server
 *
 * Takes over the UART's interrupt callback and listens for clients.
 *
 * @param server Server
 * @param config Configuration
 *
 * @retval 0 on success
 * @retval -EALREADY if the server is running
 * @retval -ENODEV if the UART is not ready
 * @retval -errno other negative errno code on failure
 */
int rfc2217_server_start(struct rfc2217_server *server, const struct rfc2217_server_config *config);

/**
 * @brief Stop a server
 *
 * Closes the client's connection and the listening sockets, and releases the
 * UART.
 *
 * @param server Server
 *
 * @retval 0 on success
 * @retval -EALREADY if the server is not running
 */
int rfc2217_server_stop(struct rfc2217_server *server);

/**
 * @brief Set the callbacks of a server
 *
 * @param server Server
 * @param cb Callbacks, NULL for none
 * @param user_data Application data
 */
void rfc2217_server_set_callbacks(struct rfc2217_server *server,
				  const struct rfc2217_server_callbacks *cb, void *user_data);

#endif /* CONFIG_NET_RFC2217_SERVER */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_RFC2217_H_ */
