/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include <net/net_ip.h>
#include <net/ethernet.h>

#include <ztest.h>

static u8_t mac_addr[sizeof(struct net_eth_addr)];
static struct net_if *eth_if;
static u8_t small_buffer[512];

/************************\
 * FAKE ETHERNET DEVICE *
\************************/

static void fake_dev_iface_init(struct net_if *iface)
{
	if (mac_addr[2] == 0U) {
		/* 00-00-5E-00-53-xx Documentation RFC 7042 */
		mac_addr[0] = 0x00;
		mac_addr[1] = 0x00;
		mac_addr[2] = 0x5E;
		mac_addr[3] = 0x00;
		mac_addr[4] = 0x53;
		mac_addr[5] = sys_rand32_get();
	}

	net_if_set_link_addr(iface, mac_addr, 6, NET_LINK_ETHERNET);

	eth_if = iface;
}

static int fake_dev_send(struct device *dev, struct net_pkt *pkt)
{
	return 0;
}

int fake_dev_init(struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

#if defined(CONFIG_NET_L2_ETHERNET)
static const struct ethernet_api fake_dev_api = {
	.iface_api.init = fake_dev_iface_init,
	.send = fake_dev_send,
};

#define _ETH_L2_LAYER ETHERNET_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(ETHERNET_L2)
#define L2_HDR_SIZE sizeof(struct net_eth_hdr)
#else
static const struct dummy_api fake_dev_api = {
	.iface_api.init = fake_dev_iface_init,
	.send = fake_dev_send,
};

#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)
#define L2_HDR_SIZE 0
#endif

NET_DEVICE_INIT(fake_dev, "fake_dev",
		fake_dev_init, NULL, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&fake_dev_api, _ETH_L2_LAYER, _ETH_L2_CTX_TYPE,
		NET_ETH_MTU);

/*********************\
 * UTILITY FUNCTIONS *
\*********************/

static bool pkt_is_of_size(struct net_pkt *pkt, size_t size)
{
	return (net_pkt_available_buffer(pkt) == size);
}

static void pkt_print_cursor(struct net_pkt *pkt)
{
	if (!pkt || !pkt->cursor.buf || !pkt->cursor.pos) {
		printk("Unknown position\n");
	} else {
		printk("Position %zu (%p) in net_buf %p (data %p)\n",
		       pkt->cursor.pos - pkt->cursor.buf->data,
		       pkt->cursor.pos, pkt->cursor.buf,
		       pkt->cursor.buf->data);
	}
}


/*****************************\
 * HOW TO ALLOCATE - 2 TESTS *
\*****************************/

static void test_net_pkt_allocate_wo_buffer(void)
{
	struct net_pkt *pkt;

	/* How to allocate a packet, with no buffer */
	pkt = net_pkt_alloc(K_NO_WAIT);
	zassert_true(pkt != NULL, "Pkt not allocated");

	/* Freeing the packet */
	net_pkt_unref(pkt);
	zassert_true(atomic_get(&pkt->atomic_ref) == 0,
		     "Pkt not properly unreferenced");

	/* Note that, if you already know the iface to which the packet
	 * belongs to, you will be able to use net_pkt_alloc_on_iface().
	 */
	pkt = net_pkt_alloc_on_iface(eth_if, K_NO_WAIT);
	zassert_true(pkt != NULL, "Pkt not allocated");

	net_pkt_unref(pkt);
	zassert_true(atomic_get(&pkt->atomic_ref) == 0,
		     "Pkt not properly unreferenced");
}

static void test_net_pkt_allocate_with_buffer(void)
{
	struct net_pkt *pkt;

	/* How to allocate a packet, with buffer
	 * a) - with a size that will fit MTU, let's say 512 bytes
	 * Note: we don't care of the family/protocol for now
	 */
	pkt = net_pkt_alloc_with_buffer(eth_if, 512,
					AF_UNSPEC, 0, K_NO_WAIT);
	zassert_true(pkt != NULL, "Pkt not allocated");

	/* Did we get the requested size? */
	zassert_true(pkt_is_of_size(pkt, 512), "Pkt size is not right");

	/* Freeing the packet */
	net_pkt_unref(pkt);
	zassert_true(atomic_get(&pkt->atomic_ref) == 0,
		     "Pkt not properly unreferenced");

	/*
	 * b) - with a size that will not fit MTU, let's say 1800 bytes
	 * Note: again we don't care of family/protocol for now.
	 */
	pkt = net_pkt_alloc_with_buffer(eth_if, 1800,
					AF_UNSPEC, 0, K_NO_WAIT);
	zassert_true(pkt != NULL, "Pkt not allocated");

	zassert_false(pkt_is_of_size(pkt, 1800), "Pkt size is not right");
	zassert_true(pkt_is_of_size(pkt, net_if_get_mtu(eth_if) + L2_HDR_SIZE),
		     "Pkt size is not right");

	/* Freeing the packet */
	net_pkt_unref(pkt);
	zassert_true(atomic_get(&pkt->atomic_ref) == 0,
		     "Pkt not properly unreferenced");

	/*
	 * c) - Now with 512 bytes but on IPv4/UDP
	 */
	pkt = net_pkt_alloc_with_buffer(eth_if, 512, AF_INET,
					IPPROTO_UDP, K_NO_WAIT);
	zassert_true(pkt != NULL, "Pkt not allocated");

	/* Because 512 + NET_IPV4UDPH_LEN fits MTU, total must be that one */
	zassert_true(pkt_is_of_size(pkt, 512 + NET_IPV4UDPH_LEN),
		     "Pkt overall size does not match");

	/* Freeing the packet */
	net_pkt_unref(pkt);
	zassert_true(atomic_get(&pkt->atomic_ref) == 0,
		     "Pkt not properly unreferenced");

	/*
	 * c) - Now with 1800 bytes but on IPv4/UDP
	 */
	pkt = net_pkt_alloc_with_buffer(eth_if, 1800, AF_INET,
					IPPROTO_UDP, K_NO_WAIT);
	zassert_true(pkt != NULL, "Pkt not allocated");

	/* Because 1800 + NET_IPV4UDPH_LEN won't fit MTU, payload size
	 * should be MTU
	 */
	zassert_true(net_pkt_available_buffer(pkt) ==
		     net_if_get_mtu(eth_if),
		     "Payload buf size does not match for ipv4/udp");

	/* Freeing the packet */
	net_pkt_unref(pkt);
	zassert_true(atomic_get(&pkt->atomic_ref) == 0,
		     "Pkt not properly unreferenced");
}

/********************************\
 * HOW TO R/W A PACKET -  TESTS *
\********************************/

static void test_net_pkt_basics_of_rw(void)
{
	struct net_pkt_cursor backup;
	struct net_pkt *pkt;
	u16_t value16;
	int ret;

	pkt = net_pkt_alloc_with_buffer(eth_if, 512,
					AF_UNSPEC, 0, K_NO_WAIT);
	zassert_true(pkt != NULL, "Pkt not allocated");

	/* Once newly allocated with buffer,
	 * a packet has no data accounted for in its buffer
	 */
	zassert_true(net_pkt_get_len(pkt) == 0,
		     "Pkt initial length should be 0");

	/* This is done through net_buf which can distinguish
	 * the size of a buffer from the length of the data in it.
	 */

	/* Let's subsequently write 1 byte, then 2 bytes and 4 bytes
	 * We write values made of 0s
	 */
	ret = net_pkt_write_u8(pkt, 0);
	zassert_true(ret == 0, "Pkt write failed");

	/* Length should be 1 now */
	zassert_true(net_pkt_get_len(pkt) == 1, "Pkt length mismatch");

	ret = net_pkt_write_be16(pkt, 0);
	zassert_true(ret == 0, "Pkt write failed");

	/* Length should be 3 now */
	zassert_true(net_pkt_get_len(pkt) == 3, "Pkt length mismatch");

	/* Verify that the data is properly written to net_buf */
	net_pkt_cursor_backup(pkt, &backup);
	net_pkt_cursor_init(pkt);
	net_pkt_set_overwrite(pkt, true);
	net_pkt_skip(pkt, 1);
	net_pkt_read_be16(pkt, &value16);
	zassert_equal(value16, 0, "Invalid value %d read, expected %d",
		      value16, 0);

	/* Then write new value, overwriting the old one */
	net_pkt_cursor_init(pkt);
	net_pkt_skip(pkt, 1);
	ret = net_pkt_write_be16(pkt, 42);
	zassert_true(ret == 0, "Pkt write failed");

	/* And re-read the value again */
	net_pkt_cursor_init(pkt);
	net_pkt_skip(pkt, 1);
	ret = net_pkt_read_be16(pkt, &value16);
	zassert_true(ret == 0, "Pkt read failed");
	zassert_equal(value16, 42, "Invalid value %d read, expected %d",
		      value16, 42);

	net_pkt_set_overwrite(pkt, false);
	net_pkt_cursor_restore(pkt, &backup);

	ret = net_pkt_write_be32(pkt, 0);
	zassert_true(ret == 0, "Pkt write failed");

	/* Length should be 7 now */
	zassert_true(net_pkt_get_len(pkt) == 7, "Pkt length mismatch");

	/* All these writing functions use net_ptk_write(), which works
	 * this way:
	 */
	ret = net_pkt_write(pkt, small_buffer, 9);
	zassert_true(ret == 0, "Pkt write failed");

	/* Length should be 16 now */
	zassert_true(net_pkt_get_len(pkt) == 16, "Pkt length mismatch");

	/* Now let's say you want to memset some data */
	ret = net_pkt_memset(pkt, 0, 4);
	zassert_true(ret == 0, "Pkt memset failed");

	/* Length should be 20 now */
	zassert_true(net_pkt_get_len(pkt) == 20, "Pkt length mismatch");

	/* So memset affects the length exactly as write does */

	/* Sometimes you might want to advance in the buffer without caring
	 * what's written there since you'll eventually come back for that.
	 * net_pkt_skip() is used for it.
	 * Note: usually you will not have to use that function a lot yourself.
	 */
	ret = net_pkt_skip(pkt, 20);
	zassert_true(ret == 0, "Pkt skip failed");

	/* Length should be 40 now */
	zassert_true(net_pkt_get_len(pkt) == 40, "Pkt length mismatch");

	/* Again, skip affected the length also, like a write
	 * But wait a minute: how to get back then, in order to write at
	 * the position we just skipped?
	 *
	 * So let's introduce the concept of buffer cursor. (which could
	 * be named 'cursor' if such name has more relevancy. Basically, each
	 * net_pkt embeds such 'cursor': it's like a head of a tape
	 * recorder/reader, it holds the current position in the buffer where
	 * you can r/w. All operations use and update it below.
	 * There is, however, a catch: buffer is described through net_buf
	 * and these are like a simple linked-list.
	 * Which means that unlike a tape recorder/reader: you are not
	 * able to go backward. Only back from starting point and forward.
	 * Thus why there is a net_pkt_cursor_init(pkt) which will let you going
	 * back from the start. We could hold more info in order to avoid that,
	 * but that would mean growing each an every net_buf.
	 */
	net_pkt_cursor_init(pkt);

	/* But isn't it so that if I want to go at the previous position I
	 * skipped, I'll use skip again but then won't it affect again the
	 * length?
	 * Answer is yes. Hopefully there is a mean to avoid that. Basically
	 * for data that already "exists" in the buffer (aka: data accounted
	 * for in the buffer, through the length) you'll need to set the packet
	 * to overwrite: all subsequent operations will then work on existing
	 * data and will not affect the length (it won't add more data)
	 */
	net_pkt_set_overwrite(pkt, true);

	zassert_true(net_pkt_is_being_overwritten(pkt),
		     "Pkt is not set to overwrite");

	/* Ok so previous skipped position was at offset 20 */
	ret = net_pkt_skip(pkt, 20);
	zassert_true(ret == 0, "Pkt skip failed");

	/* Length should _still_ be 40 */
	zassert_true(net_pkt_get_len(pkt) == 40, "Pkt length mismatch");

	/* And you can write stuff */
	ret = net_pkt_write_le32(pkt, 0);
	zassert_true(ret == 0, "Pkt write failed");

	/* Again, length should _still_ be 40 */
	zassert_true(net_pkt_get_len(pkt) == 40, "Pkt length mismatch");

	/* Let's memset the rest */
	ret = net_pkt_memset(pkt, 0, 16);
	zassert_true(ret == 0, "Pkt memset failed");

	/* Again, length should _still_ be 40 */
	zassert_true(net_pkt_get_len(pkt) == 40, "Pkt length mismatch");

	/* We are now back at the end of the existing data in the buffer
	 * Since overwrite is still on, we should not be able to r/w
	 * anything.
	 * This is completely nominal, as being set, overwrite allows r/w only
	 * on existing data in the buffer:
	 */
	ret = net_pkt_write_be32(pkt, 0);
	zassert_true(ret != 0, "Pkt write succeeded where it shouldn't have");

	/* Logically, in order to be able to add new data in the buffer,
	 * overwrite should be disabled:
	 */
	net_pkt_set_overwrite(pkt, false);

	/* But it will fail: */
	ret = net_pkt_write_le32(pkt, 0);
	zassert_true(ret != 0, "Pkt write succeeded?");

	/* Why is that?
	 * This is because in case of r/w error: the iterator is invalidated.
	 * This a design choice, once you get a r/w error it means your code
	 * messed up requesting smaller buffer than you actually needed, or
	 * writing too much data than it should have been etc...).
	 * So you must drop your packet entirely.
	 */

	/* Freeing the packet */
	net_pkt_unref(pkt);
	zassert_true(atomic_get(&pkt->atomic_ref) == 0,
		     "Pkt not properly unreferenced");
}

void test_net_pkt_advanced_basics(void)
{
	struct net_pkt_cursor backup;
	struct net_pkt *pkt;
	int ret;

	pkt = net_pkt_alloc_with_buffer(eth_if, 512,
					AF_INET, IPPROTO_UDP, K_NO_WAIT);
	zassert_true(pkt != NULL, "Pkt not allocated");

	pkt_print_cursor(pkt);

	/* As stated earlier, initializing the cursor, is the way to go
	 * back from the start in the buffer (either header or payload then).
	 * We also showed that using net_pkt_skip() could be used to move
	 * forward in the buffer.
	 * But what if you are far in the buffer, you need to go backward,
	 * and back again to your previous position?
	 * You could certainly do:
	 */
	ret = net_pkt_write(pkt, small_buffer, 20);
	zassert_true(ret == 0, "Pkt write failed");

	pkt_print_cursor(pkt);

	net_pkt_cursor_init(pkt);

	pkt_print_cursor(pkt);

	/* ... do something here ... */

	/* And finally go back with overwrite/skip: */
	net_pkt_set_overwrite(pkt, true);
	ret = net_pkt_skip(pkt, 20);
	zassert_true(ret == 0, "Pkt skip failed");
	net_pkt_set_overwrite(pkt, false);

	pkt_print_cursor(pkt);

	/* In this example, do not focus on the 20 bytes. It is just for
	 * the sake of the example.
	 * The other method is backup/restore the packet cursor.
	 */
	net_pkt_cursor_backup(pkt, &backup);

	net_pkt_cursor_init(pkt);

	/* ... do something here ... */

	/* and restore: */
	net_pkt_cursor_restore(pkt, &backup);

	pkt_print_cursor(pkt);

	/* Another feature, is how you access your data. Earlier was
	 * presented basic r/w functions. But sometime you might want to
	 * access your data directly through a structure/type etc...
	 * Due to the "fragmented" possible nature of your buffer, you
	 * need to know if the data you are trying to access is in
	 * contiguous area.
	 * For this, you'll use:
	 */
	ret = (int) net_pkt_is_contiguous(pkt, 4);
	zassert_true(ret == 1, "Pkt contiguity check failed");

	/* If that's successful you should be able to get the actual
	 * position in the buffer and cast it to the type you want.
	 */
	{
		u32_t *val = (u32_t *)net_pkt_cursor_get_pos(pkt);

		*val = 0U;
		/* etc... */
	}

	/* However, to advance your cursor, since none of the usual r/w
	 * functions got used: net_pkt_skip() should be called relevantly:
	 */
	net_pkt_skip(pkt, 4);

	/* Freeing the packet */
	net_pkt_unref(pkt);
	zassert_true(atomic_get(&pkt->atomic_ref) == 0,
		     "Pkt not properly unreferenced");

	/* Obviously one will very rarely use these 2 last low level functions
	 * - net_pkt_is_contiguous()
	 * - net_pkt_cursor_update()
	 *
	 * Let's see why next.
	 */
}

void test_net_pkt_easier_rw_usage(void)
{
	struct net_pkt *pkt;
	int ret;

	pkt = net_pkt_alloc_with_buffer(eth_if, 512,
					AF_INET, IPPROTO_UDP, K_NO_WAIT);
	zassert_true(pkt != NULL, "Pkt not allocated");

	/* In net core, all goes down in fine to header manipulation.
	 * Either it's an IP header, UDP, ICMP, TCP one etc...
	 * One would then prefer to access those directly via there
	 * descriptors (struct net_udp_hdr, struct net_icmp_hdr, ...)
	 * rather than building it byte by bytes etc...
	 *
	 * As seen earlier, it is possible to cast on current position.
	 * However, due to the "fragmented" possible nature of the buffer,
	 * it should also be possible to handle the case the data being
	 * accessed is scattered on 1+ net_buf.
	 *
	 * To avoid redoing the contiguity check, cast or copy on failure,
	 * a complex type named struct net_pkt_header_access exists.
	 * It solves both cases (accessing data contiguous or not), without
	 * the need for runtime allocation (all is on stack)
	 */
	{
		NET_PKT_DATA_ACCESS_DEFINE(ip_access, struct net_ipv4_hdr);
		struct net_ipv4_hdr *ip_hdr;

		ip_hdr = (struct net_ipv4_hdr *)
			net_pkt_get_data(pkt, &ip_access);
		zassert_not_null(ip_hdr, "Accessor failed");

		ip_hdr->tos = 0x00;

		ret = net_pkt_set_data(pkt, &ip_access);
		zassert_true(ret == 0, "Accessor failed");

		zassert_true(net_pkt_get_len(pkt) == NET_IPV4H_LEN,
			     "Pkt length mismatch");
	}

	/* As you can notice: get/set take also care of handling the cursor
	 * and updating the packet length relevantly thus why packet length
	 * has properly grown.
	 */

	/* Freeing the packet */
	net_pkt_unref(pkt);
	zassert_true(atomic_get(&pkt->atomic_ref) == 0,
		     "Pkt not properly unreferenced");
}

u8_t b5_data[10] = "qrstuvwxyz";
struct net_buf b5 = {
	.ref   = 1,
	.data  = b5_data,
	.len   = 0,
	.size  = 0,
};

u8_t b4_data[4] = "mnop";
struct net_buf b4 = {
	.frags = &b5,
	.ref   = 1,
	.data  = b4_data,
	.len   = sizeof(b4_data) - 2,
	.size  = sizeof(b4_data),
};

struct net_buf b3 = {
	.frags = &b4,
	.ref   = 1,
};

u8_t b2_data[8] = "efghijkl";
struct net_buf b2 = {
	.frags = &b3,
	.ref   = 1,
	.data  = b2_data,
	.len   = 0,
	.size  = sizeof(b2_data),
};

u8_t b1_data[4] = "abcd";
struct net_buf b1 = {
	.frags = &b2,
	.ref   = 1,
	.data  = b1_data,
	.len   = sizeof(b1_data) - 2,
	.size  = sizeof(b1_data),
};

void test_net_pkt_copy(void)
{
	struct net_pkt *pkt_src;
	struct net_pkt *pkt_dst;

	pkt_src = net_pkt_alloc_on_iface(eth_if, K_NO_WAIT);
	zassert_true(pkt_src != NULL, "Pkt not allocated");

	pkt_print_cursor(pkt_src);

	/* Let's append the buffers */
	net_pkt_append_buffer(pkt_src, &b1);

	net_pkt_set_overwrite(pkt_src, true);

	/* There should be some space left */
	zassert_true(net_pkt_available_buffer(pkt_src) != 0, "No space left?");
	/* Length should be 4 */
	zassert_true(net_pkt_get_len(pkt_src) == 4, "Wrong length");

	/* Actual space left is 12 (in b1, b2 and b4) */
	zassert_true(net_pkt_available_buffer(pkt_src) == 12,
		     "Wrong space left?");

	pkt_print_cursor(pkt_src);

	/* Now let's clone the pkt
	 * This will test net_pkt_copy_new() as it uses it for the buffers
	 */
	pkt_dst = net_pkt_clone(pkt_src, K_NO_WAIT);
	zassert_true(pkt_dst != NULL, "Pkt not clone");

	/* Cloning does not take into account left space,
	 * but only occupied one
	 */
	zassert_true(net_pkt_available_buffer(pkt_dst) == 0, "Space left");
	zassert_true(net_pkt_get_len(pkt_src) == net_pkt_get_len(pkt_dst),
		     "Not same amount?");

	/* It also did not care to copy the net_buf itself, only the content
	 * so, knowing that the base buffer size is bigger than necessary,
	 * pkt_dst has only one net_buf
	 */
	zassert_true(pkt_dst->buffer->frags == NULL, "Not only one buffer?");

	/* Freeing the packet */
	pkt_src->buffer = NULL;
	net_pkt_unref(pkt_src);
	zassert_true(atomic_get(&pkt_src->atomic_ref) == 0,
		     "Pkt not properly unreferenced");
	net_pkt_unref(pkt_dst);
	zassert_true(atomic_get(&pkt_dst->atomic_ref) == 0,
		     "Pkt not properly unreferenced");
}

void test_main(void)
{
	eth_if = net_if_get_default();

	ztest_test_suite(net_pkt_tests,
			 ztest_unit_test(test_net_pkt_allocate_wo_buffer),
			 ztest_unit_test(test_net_pkt_allocate_with_buffer),
			 ztest_unit_test(test_net_pkt_basics_of_rw),
			 ztest_unit_test(test_net_pkt_advanced_basics),
			 ztest_unit_test(test_net_pkt_easier_rw_usage),
			 ztest_unit_test(test_net_pkt_copy)
		);

	ztest_run_test_suite(net_pkt_tests);
}
