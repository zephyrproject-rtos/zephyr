/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
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

#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <net/ip_buf.h>
#include <net/net_core.h>
#include <net/net_socket.h>

#include <gatt/ipss.h>

#define UDP_PORT		4242

/* The 2001:db8::/32 is the private address space for documentation RFC 3849 */
#define MY_IPADDR { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
			0, 0x2 } } }

/* admin-local, dynamically allocated multicast address */
#define MCAST_IPADDR { { { 0xff, 0x84, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
			   0x2 } } }

static inline void reverse(unsigned char *buf, int len)
{
	int i, last = len - 1;

	for (i = 0; i < len / 2; i++) {
		unsigned char tmp = buf[i];

		buf[i] = buf[last - i];
		buf[last - i] = tmp;
	}
}

static inline struct net_buf *prepare_reply(const char *type,
					    struct net_buf *buf)
{
	printk("%s: received %d bytes\n", type, ip_buf_appdatalen(buf));

	/* In this test we reverse the received bytes.
	 * We could just pass the data back as is but
	 * this way it is possible to see how the app
	 * can manipulate the received data.
	 */
	reverse(ip_buf_appdata(buf), ip_buf_appdatalen(buf));

	return buf;
}

static inline bool get_context(struct net_context **recv,
			       struct net_context **mcast_recv)
{
	static struct net_addr mcast_addr;
	static struct net_addr any_addr;
	static struct net_addr my_addr;
	static const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
	static const struct in6_addr in6addr_mcast = MCAST_IPADDR;
	static struct in6_addr in6addr_my = MY_IPADDR;

	mcast_addr.in6_addr = in6addr_mcast;
	mcast_addr.family = AF_INET6;

	any_addr.in6_addr = in6addr_any;
	any_addr.family = AF_INET6;

	my_addr.in6_addr = in6addr_my;
	my_addr.family = AF_INET6;

	*recv = net_context_get(IPPROTO_UDP,
				&any_addr, 0,
				&my_addr, UDP_PORT);
	if (!*recv) {
		printk("%s: Cannot get network context\n", __func__);
		return NULL;
	}

	*mcast_recv = net_context_get(IPPROTO_UDP,
				      &any_addr, 0,
				      &mcast_addr, UDP_PORT);
	if (!*mcast_recv) {
		printk("%s: Cannot get receiving mcast network context\n",
		      __func__);
		return false;
	}

	return true;
}

static inline void receive_and_reply(struct net_context *recv,
				     struct net_context *mcast_recv)
{
	struct net_buf *buf;

	buf = net_receive(recv, K_FOREVER);
	if (buf) {
		prepare_reply("unicast ", buf);

		if (net_reply(recv, buf)) {
			ip_buf_unref(buf);
		}
		return;
	}

	buf = net_receive(mcast_recv, K_FOREVER);
	if (buf) {
		prepare_reply("multicast ", buf);

		if (net_reply(mcast_recv, buf)) {
			ip_buf_unref(buf);
		}
		return;
	}
}

static void listen(void)
{
	static struct net_context *recv;
	static struct net_context *mcast_recv;

	if (!get_context(&recv, &mcast_recv)) {
		printk("%s: Cannot get network contexts\n", __func__);
		return;
	}

	while (1) {
		receive_and_reply(recv, mcast_recv);
	}
}

void main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	net_init();

	ipss_init();

	err = ipss_advertise();
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");

	listen();
}
