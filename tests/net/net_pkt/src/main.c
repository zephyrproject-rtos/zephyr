/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>

#include <tc_util.h>

#include <net/net_pkt.h>
#include <net/net_ip.h>

#define NET_LOG_ENABLED 1
#include "net_private.h"

#define LL_RESERVE 28
#define FRAG_COUNT 7

struct ipv6_hdr {
	u8_t vtc;
	u8_t tcflow;
	u16_t flow;
	u8_t len[2];
	u8_t nexthdr;
	u8_t hop_limit;
	struct in6_addr src;
	struct in6_addr dst;
} __packed;

struct udp_hdr {
	u16_t src_port;
	u16_t dst_port;
	u16_t len;
	u16_t chksum;
} __packed;

struct icmp_hdr {
	u8_t type;
	u8_t code;
	u16_t chksum;
} __packed;

static const char example_data[] =
	"0123456789abcdefghijklmnopqrstuvxyz!#¤%&/()=?"
	"0123456789abcdefghijklmnopqrstuvxyz!#¤%&/()=?"
	"0123456789abcdefghijklmnopqrstuvxyz!#¤%&/()=?"
	"0123456789abcdefghijklmnopqrstuvxyz!#¤%&/()=?"
	"0123456789abcdefghijklmnopqrstuvxyz!#¤%&/()=?"
	"0123456789abcdefghijklmnopqrstuvxyz!#¤%&/()=?";

static int test_ipv6_multi_frags(void)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct ipv6_hdr *ipv6;
	struct udp_hdr *udp;
	int bytes, remaining = strlen(example_data), pos = 0;

	/* Example of multi fragment scenario with IPv6 */
	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);
	frag = net_pkt_get_reserve_rx_data(LL_RESERVE, K_FOREVER);

	/* Place the IP + UDP header in the first fragment */
	if (!net_buf_tailroom(frag)) {
		ipv6 = (struct ipv6_hdr *)(frag->data);
		udp = (struct udp_hdr *)((void *)ipv6 + sizeof(*ipv6));
		if (net_buf_tailroom(frag) < sizeof(ipv6)) {
			printk("Not enough space for IPv6 header, "
			       "needed %zd bytes, has %zd bytes\n",
			       sizeof(ipv6), net_buf_tailroom(frag));
			return -EINVAL;
		}
		net_buf_add(frag, sizeof(ipv6));

		if (net_buf_tailroom(frag) < sizeof(udp)) {
			printk("Not enough space for UDP header, "
			       "needed %zd bytes, has %zd bytes\n",
			       sizeof(udp), net_buf_tailroom(frag));
			return -EINVAL;
		}

		net_pkt_set_appdata(pkt, (void *)udp + sizeof(*udp));
		net_pkt_set_appdatalen(pkt, 0);
	}

	net_pkt_frag_add(pkt, frag);

	/* Put some data to rest of the fragments */
	frag = net_pkt_get_reserve_rx_data(LL_RESERVE, K_FOREVER);
	if (net_buf_tailroom(frag) -
	      (CONFIG_NET_BUF_DATA_SIZE - LL_RESERVE)) {
		printk("Invalid number of bytes available in the buf, "
		       "should be 0 but was %zd - %d\n",
		       net_buf_tailroom(frag),
		       CONFIG_NET_BUF_DATA_SIZE - LL_RESERVE);
		return -EINVAL;
	}

	if (((int)net_buf_tailroom(frag) - remaining) > 0) {
		printk("We should have been out of space now, "
		       "tailroom %zd user data len %zd\n",
		       net_buf_tailroom(frag),
		       strlen(example_data));
		return -EINVAL;
	}

	while (remaining > 0) {
		int copy;

		bytes = net_buf_tailroom(frag);
		copy = remaining > bytes ? bytes : remaining;
		memcpy(net_buf_add(frag, copy), &example_data[pos], copy);

		printk("Remaining %d left %d copy %d\n", remaining, bytes,
		       copy);

		pos += bytes;
		remaining -= bytes;
		if (net_buf_tailroom(frag) - (bytes - copy)) {
			printk("There should have not been any tailroom left, "
			       "tailroom %zd\n",
			       net_buf_tailroom(frag) - (bytes - copy));
			return -EINVAL;
		}

		net_pkt_frag_add(pkt, frag);
		if (remaining > 0) {
			frag = net_pkt_get_reserve_rx_data(LL_RESERVE,
							   K_FOREVER);
		}
	}

	bytes = net_pkt_get_len(pkt);
	if (bytes != strlen(example_data)) {
		printk("Invalid number of bytes in message, %zd vs %d\n",
		       strlen(example_data), bytes);
		return -EINVAL;
	}

	/* Normally one should not unref the fragment list like this
	 * because it will leave the pkt->frags pointing to already
	 * freed fragment.
	 */
	net_pkt_frag_unref(pkt->frags);
	if (!pkt->frags) {
		printk("Fragment list should not be empty.\n");
		return -EINVAL;
	}
	pkt->frags = NULL; /* to prevent double free */

	net_pkt_unref(pkt);

	return 0;
}

static char buf_orig[200];
static char buf_copy[200];

static void linearize(struct net_pkt *pkt, char *buffer, int len)
{
	struct net_buf *frag;
	char *ptr = buffer;

	frag = pkt->frags;

	while (frag && len > 0) {

		memcpy(ptr, frag->data, frag->len);
		ptr += frag->len;
		len -= frag->len;

		frag = frag->frags;
	}
}

static int test_fragment_copy(void)
{
	struct net_pkt *pkt, *new_pkt;
	struct net_buf *frag, *new_frag;
	struct ipv6_hdr *ipv6;
	struct udp_hdr *udp;
	size_t orig_len, reserve;
	int pos;

	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);
	frag = net_pkt_get_reserve_rx_data(LL_RESERVE, K_FOREVER);

	/* Place the IP + UDP header in the first fragment */
	if (net_buf_tailroom(frag)) {
		ipv6 = (struct ipv6_hdr *)(frag->data);
		udp = (struct udp_hdr *)((void *)ipv6 + sizeof(*ipv6));
		if (net_buf_tailroom(frag) < sizeof(*ipv6)) {
			printk("Not enough space for IPv6 header, "
			       "needed %zd bytes, has %zd bytes\n",
			       sizeof(ipv6), net_buf_tailroom(frag));
			return -EINVAL;
		}
		net_buf_add(frag, sizeof(*ipv6));

		if (net_buf_tailroom(frag) < sizeof(*udp)) {
			printk("Not enough space for UDP header, "
			       "needed %zd bytes, has %zd bytes\n",
			       sizeof(udp), net_buf_tailroom(frag));
			return -EINVAL;
		}

		net_buf_add(frag, sizeof(*udp));

		memcpy(net_buf_add(frag, 15), example_data, 15);

		net_pkt_set_appdata(pkt, (void *)udp + sizeof(*udp) + 15);
		net_pkt_set_appdatalen(pkt, 0);
	}

	net_pkt_frag_add(pkt, frag);

	orig_len = net_pkt_get_len(pkt);

	printk("Total copy data len %zd\n", orig_len);

	linearize(pkt, buf_orig, orig_len);

	/* Then copy a fragment list to a new fragment list.
	 * Reserve some space in front of the buffers.
	 */
	reserve = sizeof(struct ipv6_hdr) + sizeof(struct icmp_hdr);
	new_frag = net_pkt_copy_all(pkt, reserve, K_FOREVER);
	if (!new_frag) {
		printk("Cannot copy fragment list.\n");
		return -EINVAL;
	}

	new_pkt = net_pkt_get_reserve_tx(0, K_FOREVER);
	new_pkt->frags = net_buf_frag_add(new_pkt->frags, new_frag);

	printk("Total new data len %zd\n", net_pkt_get_len(new_pkt));

	if ((net_pkt_get_len(pkt) + reserve) != net_pkt_get_len(new_pkt)) {
		int diff;

		diff = net_pkt_get_len(new_pkt) - reserve -
			net_pkt_get_len(pkt);

		printk("Fragment list missing data, %d bytes not copied "
		       "(%zd vs %zd)\n", diff,
		       net_pkt_get_len(pkt) + reserve,
		       net_pkt_get_len(new_pkt));
		return -EINVAL;
	}

	if (net_pkt_get_len(new_pkt) != (orig_len + sizeof(struct ipv6_hdr) +
					   sizeof(struct icmp_hdr))) {
		printk("Fragment list missing data, new pkt len %zd "
		       "should be %zd\n", net_pkt_get_len(new_pkt),
		       orig_len + sizeof(struct ipv6_hdr) +
		       sizeof(struct icmp_hdr));
		return -EINVAL;
	}

	linearize(new_pkt, buf_copy, sizeof(buf_copy));

	if (!memcmp(buf_orig, buf_copy, sizeof(buf_orig))) {
		printk("Buffer copy failed, buffers are same!\n");
		return -EINVAL;
	}

	pos = memcmp(buf_orig, buf_copy + sizeof(struct ipv6_hdr) +
		     sizeof(struct icmp_hdr), orig_len);
	if (pos) {
		printk("Buffer copy failed at pos %d\n", pos);
		return -EINVAL;
	}

	return 0;
}

/* Empty data and test data must be the same size in order the test to work */
static const char test_data[] = { '0', '1', '2', '3', '4',
				  '5', '6', '7' };

#if HEXDUMP
static void hexdump(const char *str, const u8_t *packet, size_t length)
{
	int n = 0;

	if (!length) {
		SYS_LOG_DBG("%s zero-length packet", str);
		return;
	}

	while (length--) {
		if (n % 16 == 0) {
			printk("%s %08X ", str, n);
		}

		printk("%02X ", *packet++);

		n++;
		if (n % 8 == 0) {
			if (n % 16 == 0) {
				printk("\n");
			} else {
				printk(" ");
			}
		}
	}

	if (n % 16) {
		printk("\n");
	}
}
#endif

static const char sample_data[] =
	"abcdefghijklmnopqrstuvxyz"
	"abcdefghijklmnopqrstuvxyz"
	"abcdefghijklmnopqrstuvxyz"
	"abcdefghijklmnopqrstuvxyz"
	"abcdefghijklmnopqrstuvxyz"
	"abcdefghijklmnopqrstuvxyz"
	"abcdefghijklmnopqrstuvxyz"
	"abcdefghijklmnopqrstuvxyz"
	"abcdefghijklmnopqrstuvxyz"
	"abcdefghijklmnopqrstuvxyz"
	"abcdefghijklmnopqrstuvxyz"
	"abcdefghijklmnopqrstuvxyz"
	"abcdefghijklmnopqrstuvxyz";

static char test_rw_short[] =
	"abcdefghijklmnopqrstuvwxyz";

static char test_rw_long[] =
	"abcdefghijklmnopqrstuvwxyz"
	"abcdefghijklmnopqrstuvwxyz"
	"abcdefghijklmnopqrstuvwxyz"
	"abcdefghijklmnopqrstuvwxyz"
	"abcdefghijklmnopqrstuvwxyz"
	"abcdefghijklmnopqrstuvwxyz"
	"abcdefghijklmnopqrstuvwxyz";

static int test_pkt_read_append(void)
{
	int remaining = strlen(sample_data);
	u8_t verify_rw_short[sizeof(test_rw_short)];
	u8_t verify_rw_long[sizeof(test_rw_long)];
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct net_buf *tfrag;
	struct ipv6_hdr *ipv6;
	struct udp_hdr *udp;
	u8_t data[10];
	int pos = 0;
	int bytes;
	u16_t off;
	u16_t tpos;
	u16_t fail_pos;

	/* Example of multi fragment read, append and skip APS's */
	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);
	frag = net_pkt_get_reserve_rx_data(LL_RESERVE, K_FOREVER);

	/* Place the IP + UDP header in the first fragment */
	if (!net_buf_tailroom(frag)) {
		ipv6 = (struct ipv6_hdr *)(frag->data);
		udp = (struct udp_hdr *)((void *)ipv6 + sizeof(*ipv6));
		if (net_buf_tailroom(frag) < sizeof(ipv6)) {
			printk("Not enough space for IPv6 header, "
			       "needed %zd bytes, has %zd bytes\n",
			       sizeof(ipv6), net_buf_tailroom(frag));
			return -EINVAL;
		}
		net_buf_add(frag, sizeof(ipv6));

		if (net_buf_tailroom(frag) < sizeof(udp)) {
			printk("Not enough space for UDP header, "
			       "needed %zd bytes, has %zd bytes\n",
			       sizeof(udp), net_buf_tailroom(frag));
			return -EINVAL;
		}

		net_pkt_set_appdata(pkt, (void *)udp + sizeof(*udp));
		net_pkt_set_appdatalen(pkt, 0);
	}

	net_pkt_frag_add(pkt, frag);

	/* Put some data to rest of the fragments */
	frag = net_pkt_get_reserve_rx_data(LL_RESERVE, K_FOREVER);
	if (net_buf_tailroom(frag) -
	      (CONFIG_NET_BUF_DATA_SIZE - LL_RESERVE)) {
		printk("Invalid number of bytes available in the buf, "
		       "should be 0 but was %zd - %d\n",
		       net_buf_tailroom(frag),
		       CONFIG_NET_BUF_DATA_SIZE - LL_RESERVE);
		return -EINVAL;
	}

	if (((int)net_buf_tailroom(frag) - remaining) > 0) {
		printk("We should have been out of space now, "
		       "tailroom %zd user data len %zd\n",
		       net_buf_tailroom(frag),
		       strlen(sample_data));
		return -EINVAL;
	}

	while (remaining > 0) {
		int copy;

		bytes = net_buf_tailroom(frag);
		copy = remaining > bytes ? bytes : remaining;
		memcpy(net_buf_add(frag, copy), &sample_data[pos], copy);

		printk("Remaining %d left %d copy %d\n", remaining, bytes,
		       copy);

		pos += bytes;
		remaining -= bytes;
		if (net_buf_tailroom(frag) - (bytes - copy)) {
			printk("There should have not been any tailroom left, "
			       "tailroom %zd\n",
			       net_buf_tailroom(frag) - (bytes - copy));
			return -EINVAL;
		}

		net_pkt_frag_add(pkt, frag);
		if (remaining > 0) {
			frag = net_pkt_get_reserve_rx_data(LL_RESERVE,
							   K_FOREVER);
		}
	}

	bytes = net_pkt_get_len(pkt);
	if (bytes != strlen(sample_data)) {
		printk("Invalid number of bytes in message, %zd vs %d\n",
		       strlen(sample_data), bytes);
		return -EINVAL;
	}

	/* Failure cases */
	/* Invalid buffer */
	tfrag = net_frag_skip(NULL, 10, &fail_pos, 10);
	if (!(!tfrag && fail_pos == 0xffff)) {
		printk("Invalid case NULL buffer\n");
		return -EINVAL;
	}

	/* Invalid: Skip more than a buffer length.*/
	tfrag = net_buf_frag_last(pkt->frags);
	tfrag = net_frag_skip(tfrag, tfrag->len - 1, &fail_pos, tfrag->len + 2);
	if (!(!tfrag && fail_pos == 0xffff)) {
		printk("Invalid case offset %d length to skip %d,"
		       "frag length %d\n",
		       tfrag->len - 1, tfrag->len + 2, tfrag->len);
		return -EINVAL;
	}

	/* Invalid offset */
	tfrag = net_buf_frag_last(pkt->frags);
	tfrag = net_frag_skip(tfrag, tfrag->len + 10, &fail_pos, 10);
	if (!(!tfrag && fail_pos == 0xffff)) {
		printk("Invalid case offset %d length to skip %d,"
		       "frag length %d\n",
		       tfrag->len + 10, 10, tfrag->len);
		return -EINVAL;
	}

	/* Valid cases */

	/* Offset is more than single fragment length */
	/* Get the first data fragment */
	tfrag = pkt->frags;
	tfrag = tfrag->frags;
	off = tfrag->len;
	tfrag = net_frag_read(tfrag, off + 10, &tpos, 10, data);
	if (!tfrag ||
	    memcmp(sample_data + off + 10, data, 10)) {
		printk("Failed to read from offset %d, frag length %d"
		       "read length %d\n",
		       tfrag->len + 10, tfrag->len, 10);
		return -EINVAL;
	}

	/* Skip till end of all fragments */
	/* Get the first data fragment */
	tfrag = pkt->frags;
	tfrag = tfrag->frags;
	tfrag = net_frag_skip(tfrag, 0, &tpos, strlen(sample_data));
	if (!(!tfrag && tpos == 0)) {
		printk("Invalid skip till end of all fragments");
		return -EINVAL;
	}

	/* Short data test case */
	/* Test case scenario:
	 * 1) Cache the current fragment and offset
	 * 2) Append short data
	 * 3) Append short data again
	 * 4) Skip first short data from cached frag or offset
	 * 5) Read short data and compare
	 */
	tfrag = net_buf_frag_last(pkt->frags);
	off = tfrag->len;

	if (!net_pkt_append_all(pkt, (u16_t)sizeof(test_rw_short),
			    test_rw_short, K_FOREVER)) {
		printk("net_pkt_append failed\n");
		return -EINVAL;
	}

	if (!net_pkt_append_all(pkt, (u16_t)sizeof(test_rw_short),
			    test_rw_short, K_FOREVER)) {
		printk("net_pkt_append failed\n");
		return -EINVAL;
	}

	tfrag = net_frag_skip(tfrag, off, &tpos,
			     (u16_t)sizeof(test_rw_short));
	if (!tfrag) {
		printk("net_frag_skip failed\n");
		return -EINVAL;
	}

	tfrag = net_frag_read(tfrag, tpos, &tpos,
			     (u16_t)sizeof(test_rw_short),
			     verify_rw_short);
	if (memcmp(test_rw_short, verify_rw_short, sizeof(test_rw_short))) {
		printk("net_frag_read failed with mismatch data");
		return -EINVAL;
	}

	/* Long data test case */
	/* Test case scenario:
	 * 1) Cache the current fragment and offset
	 * 2) Append long data
	 * 3) Append long data again
	 * 4) Skip first long data from cached frag or offset
	 * 5) Read long data and compare
	 */
	tfrag = net_buf_frag_last(pkt->frags);
	off = tfrag->len;

	if (!net_pkt_append_all(pkt, (u16_t)sizeof(test_rw_long),
				test_rw_long, K_FOREVER)) {
		printk("net_pkt_append failed\n");
		return -EINVAL;
	}

	if (!net_pkt_append_all(pkt, (u16_t)sizeof(test_rw_long),
				test_rw_long, K_FOREVER)) {
		printk("net_pkt_append failed\n");
		return -EINVAL;
	}

	tfrag = net_frag_skip(tfrag, off, &tpos,
			      (u16_t)sizeof(test_rw_long));
	if (!tfrag) {
		printk("net_frag_skip failed\n");
		return -EINVAL;
	}

	tfrag = net_frag_read(tfrag, tpos, &tpos,
			     (u16_t)sizeof(test_rw_long),
			     verify_rw_long);
	if (memcmp(test_rw_long, verify_rw_long, sizeof(test_rw_long))) {
		printk("net_frag_read failed with mismatch data");
		return -EINVAL;
	}

	net_pkt_unref(pkt);

	printk("test_pkt_read_append passed\n");

	return 0;
}

static int test_pkt_read_write_insert(void)
{
	struct net_buf *read_frag;
	struct net_buf *temp_frag;
	struct net_pkt *pkt;
	struct net_buf *frag;
	u8_t read_data[100];
	u16_t read_pos;
	u16_t len;
	u16_t pos;

	/* Example of multi fragment read, append and skip APS's */
	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);
	net_pkt_set_ll_reserve(pkt, LL_RESERVE);

	frag = net_pkt_get_reserve_rx_data(net_pkt_ll_reserve(pkt),
					   K_FOREVER);
	net_pkt_frag_add(pkt, frag);

	/* 1) Offset is with in input fragment.
	 * Write app data after IPv6 and UDP header. (If the offset is after
	 * IPv6 + UDP header size, api will create empty space till offset
	 * and write data).
	 */
	frag = net_pkt_write(pkt, frag, NET_IPV6UDPH_LEN, &pos, 10,
			     (u8_t *)sample_data, K_FOREVER);
	if (!frag || pos != 58) {
		printk("Usecase 1: Write failed\n");
		return -EINVAL;
	}

	read_frag = net_frag_read(frag, NET_IPV6UDPH_LEN, &read_pos, 10,
				 read_data);
	if (!read_frag && read_pos == 0xffff) {
		printk("Usecase 1: Read failed\n");
		return -EINVAL;
	}

	if (memcmp(read_data, sample_data, 10)) {
		printk("Usecase 1: Read data mismatch\n");
		return -EINVAL;
	}

	/* 2) Write IPv6 and UDP header at offset 0. (Empty space is created
	 * already in Usecase 1, just need to fill the header, at this point
	 * there shouldn't be any length change).
	 */
	frag = net_pkt_write(pkt, frag, 0, &pos, NET_IPV6UDPH_LEN,
			     (u8_t *)sample_data, K_FOREVER);
	if (!frag || pos != 48) {
		printk("Usecase 2: Write failed\n");
		return -EINVAL;
	}

	read_frag = net_frag_read(frag, 0, &read_pos, NET_IPV6UDPH_LEN,
				 read_data);
	if (!read_frag && read_pos == 0xffff) {
		printk("Usecase 2: Read failed\n");
		return -EINVAL;
	}

	if (memcmp(read_data, sample_data, NET_IPV6UDPH_LEN)) {
		printk("Usecase 2: Read data mismatch\n");
		return -EINVAL;
	}

	/* Unref */
	net_pkt_unref(pkt);

	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);
	net_pkt_set_ll_reserve(pkt, LL_RESERVE);

	/* 3) Offset is in next to next fragment.
	 * Write app data after 2 fragments. (If the offset far away, api will
	 * create empty fragments(space) till offset and write data).
	 */
	frag = net_pkt_write(pkt, pkt->frags, 200, &pos, 10,
			     (u8_t *)sample_data + 10, K_FOREVER);
	if (!frag) {
		printk("Usecase 3: Write failed");
	}

	read_frag = net_frag_read(frag, pos - 10, &read_pos, 10,
				 read_data);
	if (!read_frag && read_pos == 0xffff) {
		printk("Usecase 3: Read failed\n");
		return -EINVAL;
	}

	if (memcmp(read_data, sample_data + 10, 10)) {
		printk("Usecase 3: Read data mismatch\n");
		return -EINVAL;
	}

	/* 4) Offset is in next to next fragment (overwrite).
	 * Write app data after 2 fragments. (Space is already available from
	 * Usecase 3, this scenatio doesn't create any space, it just overwrites
	 * the existing data.
	 */
	frag = net_pkt_write(pkt, pkt->frags, 190, &pos, 10,
			     (u8_t *)sample_data, K_FOREVER);
	if (!frag) {
		printk("Usecase 4: Write failed\n");
		return -EINVAL;
	}

	read_frag = net_frag_read(frag, pos - 10, &read_pos, 20,
				 read_data);
	if (!read_frag && read_pos == 0xffff) {
		printk("Usecase 4: Read failed\n");
		return -EINVAL;
	}

	if (memcmp(read_data, sample_data, 20)) {
		printk("Usecase 4: Read data mismatch\n");
		return -EINVAL;
	}

	/* Unref */
	net_pkt_unref(pkt);

	/* 5) Write 20 bytes in fragment which has only 10 bytes space.
	 *    API should overwrite on first 10 bytes and create extra 10 bytes
	 *    and write there.
	 */
	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);
	net_pkt_set_ll_reserve(pkt, LL_RESERVE);

	frag = net_pkt_get_reserve_rx_data(net_pkt_ll_reserve(pkt),
					   K_FOREVER);
	net_pkt_frag_add(pkt, frag);

	/* Create 10 bytes space. */
	net_buf_add(frag, 10);

	frag = net_pkt_write(pkt, frag, 0, &pos, 20, (u8_t *)sample_data,
			     K_FOREVER);
	if (!frag && pos != 20) {
		printk("Usecase 5: Write failed\n");
		return -EINVAL;
	}

	read_frag = net_frag_read(frag, 0, &read_pos, 20, read_data);
	if (!read_frag && read_pos == 0xffff) {
		printk("Usecase 5: Read failed\n");
		return -EINVAL;
	}

	if (memcmp(read_data, sample_data, 20)) {
		printk("USecase 5: Read data mismatch\n");
		return -EINVAL;
	}

	/* Unref */
	net_pkt_unref(pkt);

	/* 6) First fragment is full, second fragment has 10 bytes tail room,
	 *    third fragment has 5 bytes.
	 *    Write data (30 bytes) in second fragment where offset is 10 bytes
	 *    before the tailroom.
	 *    So it should overwrite 10 bytes and create space for another 10
	 *    bytes and write data. Third fragment 5 bytes overwritten and space
	 *    for 5 bytes created.
	 */
	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);
	net_pkt_set_ll_reserve(pkt, LL_RESERVE);

	/* First fragment make it fully occupied. */
	frag = net_pkt_get_reserve_rx_data(net_pkt_ll_reserve(pkt),
					   K_FOREVER);
	net_pkt_frag_add(pkt, frag);

	len = net_buf_tailroom(frag);
	net_buf_add(frag, len);

	/* 2nd fragment last 10 bytes tailroom, rest occupied */
	frag = net_pkt_get_reserve_rx_data(net_pkt_ll_reserve(pkt),
					   K_FOREVER);
	net_pkt_frag_add(pkt, frag);

	len = net_buf_tailroom(frag);
	net_buf_add(frag, len - 10);

	read_frag = temp_frag = frag;
	read_pos = frag->len - 10;

	/* 3rd fragment, only 5 bytes occupied */
	frag = net_pkt_get_reserve_rx_data(net_pkt_ll_reserve(pkt),
					   K_FOREVER);
	net_pkt_frag_add(pkt, frag);
	net_buf_add(frag, 5);

	temp_frag = net_pkt_write(pkt, temp_frag, temp_frag->len - 10, &pos,
				  30, (u8_t *) sample_data, K_FOREVER);
	if (!temp_frag) {
		printk("Use case 6: Write failed\n");
		return -EINVAL;
	}

	read_frag = net_frag_read(read_frag, read_pos, &read_pos, 30,
				 read_data);
	if (!read_frag && read_pos == 0xffff) {
		printk("Usecase 6: Read failed\n");
		return -EINVAL;
	}

	if (memcmp(read_data, sample_data, 30)) {
		printk("Usecase 6: Read data mismatch\n");
		return -EINVAL;
	}

	/* Unref */
	net_pkt_unref(pkt);

	/* 7) Offset is with in input fragment.
	 * Write app data after IPv6 and UDP header. (If the offset is after
	 * IPv6 + UDP header size, api will create empty space till offset
	 * and write data). Insert some app data after IPv6 + UDP header
	 * before first set of app data.
	 */

	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);
	net_pkt_set_ll_reserve(pkt, LL_RESERVE);

	/* First fragment make it fully occupied. */
	frag = net_pkt_get_reserve_rx_data(net_pkt_ll_reserve(pkt),
					   K_FOREVER);
	net_pkt_frag_add(pkt, frag);

	frag = net_pkt_write(pkt, frag, NET_IPV6UDPH_LEN, &pos, 10,
			     (u8_t *)sample_data + 10, K_FOREVER);
	if (!frag || pos != 58) {
		printk("Usecase 7: Write failed\n");
		return -EINVAL;
	}

	read_frag = net_frag_read(frag, NET_IPV6UDPH_LEN, &read_pos, 10,
				 read_data);
	if (!read_frag && read_pos == 0xffff) {
		printk("Usecase 7: Read failed\n");
		return -EINVAL;
	}

	if (memcmp(read_data, sample_data + 10, 10)) {
		printk("Usecase 7: Read data mismatch\n");
		return -EINVAL;
	}

	if (!net_pkt_insert(pkt, frag, NET_IPV6UDPH_LEN, 10,
			    (u8_t *)sample_data, K_FOREVER)) {
		printk("Usecase 7: Insert failed\n");
		return -EINVAL;
	}

	read_frag = net_frag_read(frag, NET_IPV6UDPH_LEN, &read_pos, 20,
				 read_data);
	if (!read_frag && read_pos == 0xffff) {
		printk("Usecase 7: Read after failed\n");
		return -EINVAL;
	}

	if (memcmp(read_data, sample_data, 20)) {
		printk("Usecase 7: Read data mismatch after insertion\n");
		return -EINVAL;
	}

	/* Insert data outside input fragment length, error case. */
	if (net_pkt_insert(pkt, frag, 70, 10, (u8_t *)sample_data,
			   K_FOREVER)) {
		printk("Usecase 7: False insert failed\n");
		return -EINVAL;
	}

	/* Unref */
	net_pkt_unref(pkt);

	/* 8) Offset is with in input fragment.
	 * Write app data after IPv6 and UDP header. (If the offset is after
	 * IPv6 + UDP header size, api will create empty space till offset
	 * and write data). Insert some app data after IPv6 + UDP header
	 * before first set of app data. Insertion data is long which will
	 * take two fragments.
	 */
	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);
	net_pkt_set_ll_reserve(pkt, LL_RESERVE);

	/* First fragment make it fully occupied. */
	frag = net_pkt_get_reserve_rx_data(net_pkt_ll_reserve(pkt),
					   K_FOREVER);
	net_pkt_frag_add(pkt, frag);

	frag = net_pkt_write(pkt, frag, NET_IPV6UDPH_LEN, &pos, 10,
			     (u8_t *)sample_data + 60, K_FOREVER);
	if (!frag || pos != 58) {
		printk("Usecase 8: Write failed\n");
		return -EINVAL;
	}

	read_frag = net_frag_read(frag, NET_IPV6UDPH_LEN, &read_pos, 10,
				 read_data);
	if (!read_frag && read_pos == 0xffff) {
		printk("Usecase 8: Read failed\n");
		return -EINVAL;
	}

	if (memcmp(read_data, sample_data + 60, 10)) {
		printk("Usecase 8: Read data mismatch\n");
		return -EINVAL;
	}

	if (!net_pkt_insert(pkt, frag, NET_IPV6UDPH_LEN, 60,
			    (u8_t *)sample_data, K_FOREVER)) {
		printk("Usecase 8: Insert failed\n");
		return -EINVAL;
	}

	read_frag = net_frag_read(frag, NET_IPV6UDPH_LEN, &read_pos, 70,
				 read_data);
	if (!read_frag && read_pos == 0xffff) {
		printk("Usecase 8: Read after failed\n");
		return -EINVAL;
	}

	if (memcmp(read_data, sample_data, 70)) {
		printk("Usecase 8: Read data mismatch after insertion\n");
		return -EINVAL;
	}

	/* Unref */
	net_pkt_unref(pkt);

	printk("test_pkt_read_write_insert passed\n");

	return 0;
}

static int calc_fragments(struct net_pkt *pkt)
{
	struct net_buf *frag = pkt->frags;
	int count = 0;

	while (frag) {
		frag = frag->frags;
		count++;
	}

	return count;
}

static bool net_pkt_is_compact(struct net_pkt *pkt)
{
	struct net_buf *frag, *last;
	size_t total = 0, calc;
	int count = 0;

	last = NULL;
	frag = pkt->frags;

	while (frag) {
		total += frag->len;
		count++;

		last = frag;
		frag = frag->frags;
	}

	NET_ASSERT(last);

	if (!last) {
		return false;
	}

	calc = count * (last->size) - net_buf_tailroom(last) -
		count * (net_buf_headroom(last));

	if (total == calc) {
		return true;
	}

	NET_DBG("Not compacted total %zu real %zu", total, calc);

	return false;
}

static int test_fragment_compact(void)
{
	struct net_pkt *pkt;
	struct net_buf *frags[FRAG_COUNT], *frag;
	int i, bytes, total, count;

	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);
	frag = NULL;

	for (i = 0, total = 0; i < FRAG_COUNT; i++) {
		frags[i] = net_pkt_get_reserve_rx_data(12, K_FOREVER);

		if (frag) {
			net_buf_frag_add(frag, frags[i]);
		}

		frag = frags[i];

		/* Copy character test data in front of the fragment */
		memcpy(net_buf_add(frags[i], sizeof(test_data)),
		       test_data, sizeof(test_data));

		/* Followed by bytes of zeroes */
		memset(net_buf_add(frags[i], sizeof(test_data)), 0,
		       sizeof(test_data));

		total++;
	}

	if (total != FRAG_COUNT) {
		printk("There should be %d fragments but was %d\n",
		       FRAG_COUNT, total);
		return -1;
	}

	printk("step 1\n");

	pkt->frags = net_buf_frag_add(pkt->frags, frags[0]);

	bytes = net_pkt_get_len(pkt);
	if (bytes != FRAG_COUNT * sizeof(test_data) * 2) {
		printk("Compact test failed, fragments had %d bytes but "
		       "should have had %zd\n", bytes,
		       FRAG_COUNT * sizeof(test_data) * 2);
		return -1;
	}

	if (net_pkt_is_compact(pkt)) {
		printk("The pkt is definitely not compact. Test fails\n");
		return -1;
	}

	printk("step 2\n");

	net_pkt_compact(pkt);

	if (!net_pkt_is_compact(pkt)) {
		printk("The pkt should be in compact form. Test fails\n");
		return -1;
	}

	printk("step 3\n");

	/* Try compacting again, nothing should happen */
	net_pkt_compact(pkt);

	if (!net_pkt_is_compact(pkt)) {
		printk("The pkt should be compacted now. Test fails\n");
		return -1;
	}

	total = calc_fragments(pkt);

	/* Add empty fragment at the end and compact, the last fragment
	 * should be removed.
	 */
	frag = net_pkt_get_reserve_rx_data(0, K_FOREVER);

	net_pkt_frag_add(pkt, frag);

	count = calc_fragments(pkt);

	printk("step 4\n");

	net_pkt_compact(pkt);

	i = calc_fragments(pkt);

	if (count != (i + 1)) {
		printk("Last fragment removal failed, chain should have %d "
		       "fragments but had %d\n", i-1, i);
		return -1;
	}

	if (i != total) {
		printk("Fragments missing, expecting %d but got %d\n",
		       total, i);
		return -1;
	}

	/* Add two empty fragments at the end and compact, the last two
	 * fragment should be removed.
	 */
	frag = net_pkt_get_reserve_rx_data(0, K_FOREVER);

	net_pkt_frag_add(pkt, frag);

	frag = net_pkt_get_reserve_rx_data(0, K_FOREVER);

	net_pkt_frag_add(pkt, frag);

	count = calc_fragments(pkt);

	printk("step 5\n");

	net_pkt_compact(pkt);

	i = calc_fragments(pkt);

	if (count != (i + 2)) {
		printk("Last two fragment removal failed, chain should have "
		       "%d fragments but had %d\n", i-2, i);
		return -1;
	}

	if (i != total) {
		printk("Fragments missing, expecting %d but got %d\n",
		       total, i);
		return -1;
	}

	/* Add empty fragment at the beginning and at the end, and then
	 * compact, the two fragment should be removed.
	 */
	frag = net_pkt_get_reserve_rx_data(0, K_FOREVER);

	net_pkt_frag_insert(pkt, frag);

	frag = net_pkt_get_reserve_rx_data(0, K_FOREVER);

	net_pkt_frag_add(pkt, frag);

	count = calc_fragments(pkt);

	printk("step 6\n");

	net_pkt_compact(pkt);

	i = calc_fragments(pkt);

	if (count != (i + 2)) {
		printk("Two fragment removal failed, chain should have "
		       "%d fragments but had %d\n", i-2, i);
		return -1;
	}

	if (i != total) {
		printk("Fragments missing, expecting %d but got %d\n",
		       total, i);
		return -1;
	}

	printk("test_fragment_compact passed\n");

	return 0;
}

static const char frag_data[CONFIG_NET_BUF_DATA_SIZE] = { 42 };

static int test_fragment_split(void)
{
#define TEST_FRAG_COUNT (FRAG_COUNT - 2)
#define FRAGA (FRAG_COUNT - 2)
#define FRAGB (FRAG_COUNT - 1)
	struct net_pkt *pkt;
	struct net_buf *frags[FRAG_COUNT], *frag, *fragA, *fragB;
	int i, total, splitA, splitB;
	int ret;

	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);
	frag = NULL;

	for (i = 0, total = 0; i < TEST_FRAG_COUNT; i++) {
		frags[i] = net_pkt_get_reserve_rx_data(12, K_FOREVER);

		if (frag) {
			net_buf_frag_add(frag, frags[i]);
		}

		frag = frags[i];

		/* Copy some test data in front of the fragment */
		memcpy(net_buf_add(frags[i], sizeof(frag_data)),
		       frag_data, sizeof(frag_data));

		total++;
	}

	if (total != TEST_FRAG_COUNT) {
		printk("There should be %d fragments but was %d\n",
		       TEST_FRAG_COUNT, total);
		return -1;
	}

	net_pkt_frag_add(pkt, frags[0]);

	fragA = frags[FRAGA];
	fragB = frags[FRAGB];

	splitA = fragA->size * 2 / 3;
	splitB = fragB->size - splitA;

	/* Test some error cases first */
	ret = net_pkt_split(NULL, NULL, 1024, &fragA, &fragB, K_NO_WAIT);
	if (!ret) {
		printk("Invalid buf pointers\n");
		return -1;
	}

	ret = net_pkt_split(pkt, pkt->frags, 1024, &fragA, &fragB, K_NO_WAIT);
	if (!ret) {
		printk("Too long frag length %d\n", 1024);
		return -1;
	}

	ret = net_pkt_split(pkt, pkt->frags, CONFIG_NET_BUF_DATA_SIZE + 1,
			    &fragA, &fragB, K_NO_WAIT);
	if (!ret) {
		printk("Too long frag size %d vs %d\n",
		       CONFIG_NET_BUF_DATA_SIZE,
		       CONFIG_NET_BUF_DATA_SIZE + 1);
		return -1;
	}

	ret = net_pkt_split(pkt, pkt->frags, splitA,
			     &fragA, &fragB, K_NO_WAIT);
	if (ret < 0) {
		printk("Cannot split into %d and %d parts\n", splitA, splitB);
		return -1;
	}

	if (fragA->len != splitA) {
		printk("Frag A len %d not %d\n", fragA->len, splitA);
		return -1;
	}

	if (fragB->len != splitB) {
		printk("Frag B len %d not %d\n", fragB->len, splitB);
		return -1;
	}

	if (memcmp(pkt->frags->data, fragA->data, splitA)) {
		printk("Frag A data mismatch\n");
		return -1;
	}

	if (memcmp(pkt->frags->data + splitA, fragB->data, splitB)) {
		printk("Frag B data mismatch\n");
		return -1;
	}

	return 0;
}

void main(void)
{
	if (test_ipv6_multi_frags() < 0) {
		goto fail;
	}

	if (test_fragment_copy() < 0) {
		goto fail;
	}

	if (test_pkt_read_append() < 0) {
		goto fail;
	}

	if (test_pkt_read_write_insert() < 0) {
		goto fail;
	}

	if (test_fragment_compact() < 0) {
		goto fail;
	}

	if (test_fragment_split() < 0) {
		goto fail;
	}

	printk("net pkt tests passed\n");

	TC_END_REPORT(TC_PASS);
	return;

fail:
	TC_END_REPORT(TC_FAIL);
}
