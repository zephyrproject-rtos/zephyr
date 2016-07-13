/** @file
 * @brief Network core definitions
 *
 * Definitions for networking support.
 */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __NET_CORE_H
#define __NET_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Network subsystem logging helpers */

#ifdef CONFIG_NETWORKING_WITH_LOGGING
#define NET_DBG(fmt, ...) printk("net: %s (%p): " fmt, __func__, \
				 sys_thread_self_get(), ##__VA_ARGS__)
#define NET_ERR(fmt, ...) printk("net: %s: " fmt, __func__, ##__VA_ARGS__)
#define NET_INFO(fmt, ...) printk("net: " fmt,  ##__VA_ARGS__)
#define NET_PRINT(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define NET_DBG(...)
#define NET_ERR(...)
#define NET_INFO(...)
#define NET_PRINT(...)
#endif /* CONFIG_NETWORKING_WITH_LOGGING */

enum net_tcp_type {
	NET_TCP_TYPE_UNKNOWN = 0,
	NET_TCP_TYPE_SERVER,
	NET_TCP_TYPE_CLIENT,
};

struct net_buf;
struct net_context;

#include <misc/printk.h>
#include <string.h>

#include <net/net_socket.h>

/**
 * @brief Initialize network stack. This is will be automatically called
 * by the OS.
 *
 * @details No harm will happen if application calls this multiple times.
 *
 * @return 0 if ok, < 0 in case of error.
 */
int net_init(void);

struct net_driver {
	/** How much headroom is needed for net transport headers */
	size_t head_reserve;

	/** Open the net transport */
	int (*open)(void);

	/** Send data to net. The send() function should return:
	 * 0 : If packet could not be sent. In this case buf should
	 *     not be released.
	 * 1 : If the packet was sent successfully. In this case the buf
	 *     should be released by either the send() or some other
	 *     lower layer function.
	 * <0: If there is an error, the buf should not be released by
	 *     send() function.
	 */
	int (*send)(struct net_buf *buf);
};

/**
 * @brief Register a new network driver to the network stack.
 *
 * @details Only one network device can be registered at a time.
 *
 * @param drv Network driver.
 *
 * @return 0 if ok, < 0 in case of error.
 */
int net_register_driver(struct net_driver *drv);

/**
 * @brief Unregister a previously registered network driver.
 *
 * @param drv Network driver.
 *
 * @details Networking will be disabled if there is no driver
 * available.
 *
 */
void net_unregister_driver(struct net_driver *drv);

/**
 * @brief Set the MAC/EUI-64 address of the device.
 *
 * @details Network device driver should call this function to
 * tell the used MAC or EUI-64 address to the IP stack.
 *
 * @param mac MAC or EUI-64 address
 * @param len If len == 6, then the first parameter is interpreted
 * to be the MAC address and the function sets the U/L bits etc.
 * If the len == 8, then the value is used as is to generate the
 * link local IPv6 address.
 *
 * @return 0 if ok, < 0 in case of error.
 */
int net_set_mac(uint8_t *mac, uint8_t len);

/**
 * @brief Send a reply packet to the message originator.
 *
 * @details Application can call this function if it has received
 * a network packet from peer. The application needs to write
 * reply data into net_buf. The app can use ip_buf_appdata(buf) and
 * ip_buf_appdatalen(buf) to set the application data and length.
 * This function will yield in thread and fiber contexts.
 *
 * @param context Network context
 * @param buf Network buffer containing the network data.
 *
 * @return 0 if ok, < 0 in case of error.
 */
int net_reply(struct net_context *context, struct net_buf *buf);

/* Called by driver when an IP packet has been received */
int net_recv(struct net_buf *buf);

void net_context_init(void);

/**
 * @brief Get current status of the TCP connection.
 *
 * @details Application can call this to get the current status
 * of TCP connection. The returned value maps to errno values.
 * Value 0 means ok. For UDP context 0 is always returned.
 *
 * @param context Network context
 *
 * @return 0 if ok, < 0 connection status
 */
int net_context_get_connection_status(struct net_context *context);

#ifdef __cplusplus
}
#endif

#endif /* __NET_CORE_H */
