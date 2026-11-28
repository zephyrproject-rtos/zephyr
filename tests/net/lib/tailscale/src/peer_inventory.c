/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/net/net_ip.h>
#include "tailscale_internal.h"
#include <zephyr/sys/printk.h>
#include <zephyr/ztest.h>

#define KEY1 "nodekey:0101010101010101010101010101010101010101010101010101010101010101"
#define KEY2 "nodekey:0202020202020202020202020202020202020202020202020202020202020202"
#define KEY3 "nodekey:0303030303030303030303030303030303030303030303030303030303030303"
#define DISCO1 "discokey:0101010101010101010101010101010101010101010101010101010101010101"
#define PEER1 "{\"ID\":1,\"Key\":\"" KEY1 "\"}"
#define PEER2 "{\"ID\":2,\"Key\":\"" KEY2 "\"}"

static struct tailscale_peer_inventory inventory;
static struct tailscale_peer peer;

static int apply(uint32_t generation, const char *json)
{
	return tailscale_peer_inventory_apply(&inventory, generation, (uint8_t *)json,
					      strlen(json), NULL);
}

static void expect_frame(uint32_t generation, const char *json,
			 enum tailscale_peer_inventory_frame expected)
{
	enum tailscale_peer_inventory_frame frame;

	zassert_ok(tailscale_peer_inventory_apply(&inventory, generation, (uint8_t *)json,
						 strlen(json), &frame));
	zassert_equal(frame, expected);
}

static struct tailscale_peer_inventory_summary summary(void)
{
	struct tailscale_peer_inventory_summary current;

	zassert_ok(tailscale_peer_inventory_get_summary(&inventory, &current));
	return current;
}

static void reset_inventory(void *fixture)
{
	ARG_UNUSED(fixture);
	tailscale_peer_inventory_init(&inventory);
}

ZTEST(tailscale_peer_inventory, test_authoritative_empty_first_map)
{
	zassert_ok(apply(1, "{\"KeepAlive\":true,\"Peers\":null}"));
	zassert_false(summary().valid);
	zassert_ok(apply(1, "{\"Node\":{\"ID\":9},\"Peers\":null}"));
	zassert_true(summary().valid);
	zassert_equal(summary().peer_count, 0);
	zassert_equal(tailscale_peer_inventory_get_peer(&inventory, 1, &peer), -ENOENT);

	tailscale_peer_inventory_session_lost(&inventory, 1);
	zassert_equal(summary().generation, 1);
	zassert_false(summary().valid);
	zassert_equal(apply(1, "{\"Peers\":[" PEER1 "]}"), -ESTALE);
	zassert_equal(apply(2, "{\"PeersChanged\":[" PEER1 "]}"), -EBADMSG);
	zassert_false(summary().valid);
	zassert_ok(apply(3, "{\"Peers\":[" PEER1 "]}"));
	tailscale_peer_inventory_session_lost(&inventory, 2);
	zassert_true(summary().valid);
	zassert_equal(summary().generation, 3);
}

ZTEST(tailscale_peer_inventory, test_full_and_deltas)
{
	zassert_ok(apply(1, "{\"Peers\":[" PEER1 "," PEER2 "]}"));
	zassert_equal(summary().peer_count, 2);
	zassert_ok(apply(1, "{\"Peers\":[],\"PeersRemoved\":[1],"
			   "\"PeersChanged\":[{\"ID\":3,\"Key\":\"" KEY3 "\"}]}"));
	zassert_equal(summary().peer_count, 2);
	zassert_equal(tailscale_peer_inventory_get_peer(&inventory, 1, &peer), -ENOENT);
	zassert_ok(tailscale_peer_inventory_get_peer(&inventory, 3, &peer));
	zassert_equal(peer.key[31], 3);

	zassert_ok(apply(1, "{\"Peers\":[" PEER1 "],\"PeersRemoved\":[1],"
			   "\"PeersChanged\":[" PEER2 "],\"OnlineChange\":{\"1\":true}}"));
	zassert_equal(summary().peer_count, 1);
	zassert_ok(tailscale_peer_inventory_get_peer(&inventory, 1, &peer));
	zassert_equal(peer.online, TAILSCALE_PEER_ONLINE_UNKNOWN);

	zassert_ok(apply(1, "{\"PeersRemoved\":[1],\"PeersChanged\":[" PEER1 "]}"));
	zassert_ok(tailscale_peer_inventory_get_peer(&inventory, 1, &peer));
	zassert_equal(summary().peer_count, 1);
}

ZTEST(tailscale_peer_inventory, test_first_node_and_deltas_with_empty_peers)
{
	expect_frame(1, "{\"KeepAlive\":true,\"Peers\":[" PEER1 "]}",
		     TAILSCALE_PEER_INVENTORY_KEEPALIVE);
	zassert_false(summary().valid);
	expect_frame(1, "{\"ControlTime\":\"2026-11-24T00:00:00Z\"}",
		     TAILSCALE_PEER_INVENTORY_OTHER);
	expect_frame(1, "{\"Node\":{\"ID\":9},\"Peers\":[],\"PeersChanged\":[" PEER1 "],"
		     "\"OnlineChange\":{\"1\":true}}", TAILSCALE_PEER_INVENTORY_FULL);
	zassert_true(summary().valid);
	zassert_equal(summary().peer_count, 1);
	zassert_ok(tailscale_peer_inventory_get_peer(&inventory, 1, &peer));
	zassert_equal(peer.online, TAILSCALE_PEER_ONLINE);
	expect_frame(1, "{\"Peers\":null,\"PeersRemoved\":[1]}", TAILSCALE_PEER_INVENTORY_UPDATE);
	zassert_equal(summary().peer_count, 0);
}

ZTEST(tailscale_peer_inventory, test_prefix_presence_and_narrowing)
{
	zassert_ok(apply(1, "{\"Peers\":[{\"ID\":1,\"StableID\":\"n123\",\"Key\":\"" KEY1 "\","
			   "\"Addresses\":[\"100.64.0.9/32\",\"fd7a:115c:a1e0::9/128\"],"
			   "\"AllowedIPs\":null},{\"ID\":2,\"Key\":\"" KEY2 "\","
			   "\"Addresses\":[\"100.64.0.2/32\"],\"AllowedIPs\":[]}]}"));

	zassert_ok(tailscale_peer_inventory_get_peer(&inventory, 1, &peer));
	zassert_equal(peer.allowed_ip_count, 2);
	zassert_equal(peer.allowed_ips[0].family, NET_AF_INET);
	zassert_equal(peer.allowed_ips[1].family, NET_AF_INET6);
	zassert_equal(peer.allowed_ips[1].length, 128);
	zassert_equal(peer.allowed_ips[1].address[15], 9);
	zassert_equal(strcmp(peer.stable_id, "n123"), 0);
	zassert_ok(tailscale_peer_inventory_get_peer(&inventory, 2, &peer));
	zassert_equal(peer.allowed_ip_count, 0);

	zassert_ok(apply(1, "{\"PeersChanged\":[{\"ID\":1,\"Key\":\"" KEY1 "\","
			   "\"Addresses\":[\"100.64.0.9/32\"],\"AllowedIPs\":[\"10.1.2.3/8\"]},"
			   "{\"ID\":2,\"Key\":\"" KEY2 "\",\"UnsignedPeerAPIOnly\":true,"
			   "\"Addresses\":[\"100.64.0.2/32\"],\"AllowedIPs\":[\"0.0.0.0/0\"]}]}"));
	zassert_ok(tailscale_peer_inventory_get_peer(&inventory, 1, &peer));
	zassert_equal(peer.allowed_ips[0].length, 8);
	zassert_equal(peer.allowed_ips[0].address[0], 10);
	zassert_equal(peer.allowed_ips[0].address[1], 0);
	zassert_ok(tailscale_peer_inventory_get_peer(&inventory, 2, &peer));
	zassert_equal(peer.allowed_ips[0].length, 32);
	zassert_equal(peer.allowed_ips[0].address[3], 2);
}

ZTEST(tailscale_peer_inventory, test_patch_null_empty_and_online_order)
{
	zassert_ok(apply(1, "{\"Peers\":[{\"ID\":1,\"Key\":\"" KEY1 "\",\"DiscoKey\":\""
			   DISCO1 "\",\"Endpoints\":[\"192.0.2.1:41641\","
			   "\"[2001:db8::1]:41642\"],\"Online\":true}]}"));
	zassert_ok(apply(1, "{\"OnlineChange\":{\"1\":true},\"PeersChangedPatch\":[{\"NodeID\":1,"
			   "\"Endpoints\":null,\"DiscoKey\":null,\"Online\":false,"
			   "\"Key\":\"" KEY3 "\",\"DERPRegion\":3}]}"));
	zassert_ok(tailscale_peer_inventory_get_peer(&inventory, 1, &peer));
	zassert_equal(peer.key[0], 3);
	zassert_equal(peer.disco_key[31], 1);
	zassert_equal(peer.online, TAILSCALE_PEER_OFFLINE);
	zassert_equal(peer.endpoint_count, 2);
	zassert_equal(peer.endpoints[0].family, NET_AF_INET);
	zassert_equal(peer.endpoints[0].port, 41641);
	zassert_equal(peer.endpoints[1].family, NET_AF_INET6);
	zassert_equal(peer.endpoints[1].port, 41642);

	zassert_ok(apply(1, "{\"PeersChangedPatch\":[{\"NodeID\":1,\"Endpoints\":[],"
			   "\"Key\":null,\"Online\":null,\"DERPRegion\":0},"
			   "{\"NodeID\":99,\"Online\":true}]}"));
	zassert_ok(tailscale_peer_inventory_get_peer(&inventory, 1, &peer));
	zassert_equal(peer.endpoint_count, 0);
	zassert_equal(peer.key[0], 3);
	zassert_equal(peer.online, TAILSCALE_PEER_OFFLINE);
	zassert_equal(peer.derp_region, 3);
	zassert_equal(summary().peer_count, 1);
}

ZTEST(tailscale_peer_inventory, test_sequence_and_publication_boundaries)
{
	zassert_ok(apply(20, "{\"Seq\":7,\"Peers\":[" PEER1 "]}"));
	uint64_t revision = summary().revision;

	zassert_ok(apply(20, "{\"KeepAlive\":true,\"Seq\":99}"));
	zassert_equal(summary().revision, revision);
	zassert_ok(tailscale_peer_inventory_get_peer_at(&inventory, revision, 0, &peer));
	zassert_ok(apply(20, "{\"Seq\":8,\"OnlineChange\":{\"1\":true}}"));
	zassert_equal(tailscale_peer_inventory_get_peer_at(&inventory, revision, 0, &peer),
		      -ESTALE);
	zassert_equal(apply(20, "{\"Seq\":10,\"PeersChanged\":[" PEER2 "]}"), -EILSEQ);
	zassert_false(summary().valid);
	zassert_equal(tailscale_peer_inventory_get_peer(&inventory, 1, &peer), -ENODATA);
	zassert_equal(apply(20, "{\"Seq\":9,\"Peers\":[" PEER1 "]}"), -ESTALE);
	zassert_ok(apply(21, "{\"Peers\":[" PEER2 "]}"));
	zassert_equal(tailscale_peer_inventory_get_peer(&inventory, 1, &peer), -ENOENT);
}

ZTEST(tailscale_peer_inventory, test_malformed_updates_invalidate_without_partial_peer)
{
	static const char *const bad[] = {
		"{\"PeersChanged\":[" PEER2 ",{\"ID\":3,\"key\":\"" KEY3 "\",\"Key\":null}]}",
		"{\"PeErs\":null,\"Peers\":[" PEER2 "]}",
		"{\"PeersChanged\":[{\"\\u0049D\":2,\"Key\":\"" KEY2 "\"}]}",
		"{\"Unknown\":[1,{\"bad\":2,}]}",
		"{\"PeersRemoved\":[1,1]}",
		"{\"PeersChanged\":[" PEER2 "," PEER2 "]}",
		"{\"PeersChangedPatch\":[{\"NodeID\":1,\"Endpoints\":[\"192.0.2.1\"]}]}",
		"{\"PeersChanged\":[{\"ID\":2,\"Key\":\"" KEY2 "\","
		"\"AllowedIPs\":[\"192.0.2.1/33\"]}]}",
		"{\"OnlineChange\":{\"1\":true,\"1\":false}}",
		"{\"PeersChanged\":[" PEER2 "]}{}",
	};

	for (size_t i = 0; i < ARRAY_SIZE(bad); i++) {
		uint32_t generation = i + 1;

		zassert_ok(apply(generation, "{\"Peers\":[" PEER1 "]}"));
		zassert_equal(apply(generation, bad[i]), -EBADMSG, "case %zu", i);
		zassert_false(summary().valid);
		zassert_equal(summary().peer_count, 0);
		zassert_equal(tailscale_peer_inventory_get_peer(&inventory, 1, &peer), -ENODATA);
		zassert_equal(tailscale_peer_inventory_get_peer(&inventory, 2, &peer), -ENODATA);
	}
}

ZTEST(tailscale_peer_inventory, test_peer_limit_invalidates_publication)
{
	char update[128];

	zassert_ok(apply(1, "{\"Node\":{\"ID\":99},\"Peers\":[]}"));
	for (size_t i = 1; i <= TAILSCALE_PEER_INVENTORY_CAPACITY + 1U; ++i) {
		int len = snprintk(update, sizeof(update),
				   "{\"PeersChanged\":[{\"ID\":%zu,\"Key\":\"" KEY2 "\"}]}", i);

		zassert_true(len > 0 && len < sizeof(update));
		if (i <= TAILSCALE_PEER_INVENTORY_CAPACITY) {
			zassert_ok(apply(1, update));
			zassert_equal(summary().peer_count, i);
		} else {
			zassert_equal(apply(1, update), -E2BIG);
		}
	}
	zassert_false(summary().valid);
}

ZTEST_SUITE(tailscale_peer_inventory, NULL, NULL, reset_inventory, NULL, NULL);
