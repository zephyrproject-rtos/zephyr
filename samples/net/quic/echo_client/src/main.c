/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_quic_echo_client_svc_sample, LOG_LEVEL_DBG);

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/zvfs/eventfd.h>
#include <zephyr/misc/lorem_ipsum.h>
#include <zephyr/shell/shell.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket_service.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/quic.h>

/* from samples/net/common/ */
#include "net_sample_common.h"
#include "quic_certificate.h"

/* Turn off the progress printing so that shell can be used.
 * Set to true if you want to see progress output.
 */
#define PRINT_PROGRESS true

#if defined(CONFIG_USERSPACE)
#include <zephyr/app_memory/app_memdomain.h>
extern struct k_mem_partition app_partition;
extern struct k_mem_domain app_domain;
#define APP_BMEM K_APP_BMEM(app_partition)
#define APP_DMEM K_APP_DMEM(app_partition)
#else
#define APP_BMEM
#define APP_DMEM
#endif

#if defined(CONFIG_NET_TC_THREAD_PREEMPTIVE)
#define THREAD_PRIORITY K_PRIO_PREEMPT(8)
#else
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
#endif

#define POLL_TIMEOUT_MS 1000
#define RECV_BUF_SIZE 1500
#define PEER_PORT 4422
#define APP_BANNER "Run Quic echo client"

#define EVENT_MASK (NET_EVENT_L4_CONNECTED | \
		    NET_EVENT_L4_DISCONNECTED)
#define IPV6_EVENT_MASK (NET_EVENT_IPV6_ADDR_ADD | \
			 NET_EVENT_IPV6_ADDR_DEPRECATED)

#define MAX_PACKET_LEN 1200

static const char lorem_ipsum[] = LOREM_IPSUM_LONG;
const int ipsum_len = sizeof(lorem_ipsum) - 1;

static APP_BMEM bool need_restart;
static bool want_to_quit;
static bool connected;

/*
 * Per-stream state. One instance per active QUIC stream.
 * Tracks the echo send/receive cycle independently for each stream.
 */
struct stream_ctx {
	int fd;              /* QUIC stream socket, -1 if slot is free */
	uint32_t expecting;  /* bytes expected back from server this round */
	uint32_t received;   /* bytes received so far this round */
	uint32_t counter;    /* total successful echo rounds */
	bool active;
};

static APP_BMEM struct config {
	/*
	 * fds[0]   : control eventfd (used to wake poll from shell commands)
	 * fds[1..N]: one entry per active stream socket
	 */
	struct zsock_pollfd fds[1 + CONFIG_QUIC_MAX_STREAMS_BIDI];
	int nfds;

	int quic_socket;  /* QUIC connection socket */
	struct stream_ctx streams[CONFIG_QUIC_MAX_STREAMS_BIDI];

	int mtu;
	char *proto;
} conf;

/* eventfd fd exposed so send_cmd can wake the main poll loop */
static int control_fd = -1;

static K_SEM_DEFINE(run_app, 0, 1);
static K_SEM_DEFINE(quit_lock, 0, 1);

static struct net_mgmt_event_callback mgmt_cb;
static struct net_mgmt_event_callback ipv6_mgmt_cb;

enum operation {
	OP_NONE,
	OP_START_STREAM,
	OP_STOP_STREAM,
};

struct cmd_msg {
	/*
	 * OP_START_STREAM : sock is unused (-1), worker picks a free slot.
	 * OP_STOP_STREAM  : sock is the slot index to stop.
	 */
	int sock;
	enum operation op;
};

K_MSGQ_DEFINE(cmd_msgq, sizeof(struct cmd_msg),
	      CONFIG_NET_MGMT_EVENT_QUEUE_SIZE, sizeof(intptr_t));

static void quit(void)
{
	k_sem_give(&quit_lock);
}

/* Rebuild the fds[] array from the set of currently active streams. */
static void prepare_fds(struct config *cfg)
{
	cfg->nfds = 0;

	cfg->fds[cfg->nfds].fd = control_fd;
	cfg->fds[cfg->nfds].events = ZSOCK_POLLIN;
	cfg->nfds++;

	ARRAY_FOR_EACH(cfg->streams, i) {
		if (cfg->streams[i].active) {
			cfg->fds[cfg->nfds].fd = cfg->streams[i].fd;
			cfg->fds[cfg->nfds].events = ZSOCK_POLLIN;
			cfg->nfds++;
		}
	}
}

static void wait(struct config *cfg)
{
	int ret;

	ret = zsock_poll(cfg->fds, cfg->nfds, POLL_TIMEOUT_MS);
	if (ret < 0) {
		static bool once;

		if (!once) {
			once = true;
			LOG_ERR("Error in poll:%d", errno);
		}
	}
}

static int compare_quic_data(struct stream_ctx *stream, const char *buf,
			     uint32_t received)
{
	if (stream->received + received > stream->expecting) {
		LOG_ERR("Too much data received on stream fd=%d", stream->fd);
		return -EIO;
	}

	if (memcmp(buf, lorem_ipsum + stream->received, received) != 0) {
		LOG_ERR("Data mismatch on stream fd=%d at offset %u",
			stream->fd, stream->received);
		LOG_HEXDUMP_ERR(buf, received, "received");
		LOG_HEXDUMP_ERR(lorem_ipsum+stream->received, received, "expecting");
		return -EIO;
	}

	return 0;
}

static int send_quic_data(struct config *cfg, struct stream_ctx *stream)
{
	int ret;

	do {
		stream->expecting = sys_rand32_get() % MIN(ipsum_len,
							   MAX_PACKET_LEN);
	} while (stream->expecting == 0U);

	stream->received = 0U;

	ret = zsock_send_all(stream->fd, lorem_ipsum, stream->expecting, 0,
			     K_FOREVER, NULL);
	if (ret < 0) {
		LOG_ERR("%s QUIC: stream fd=%d failed to send, errno %d",
			cfg->proto, stream->fd, errno);
	} else {
		if (PRINT_PROGRESS) {
			LOG_DBG("%s QUIC: stream fd=%d sent %u bytes",
				cfg->proto, stream->fd, stream->expecting);
		}
	}

	return ret;
}

/* Open a new QUIC stream on the connection socket, send the first batch. */
static int start_stream(struct config *cfg, int slot)
{
	struct stream_ctx *stream = &cfg->streams[slot];
	int ret;

	ret = quic_stream_open(cfg->quic_socket, QUIC_STREAM_CLIENT,
			       QUIC_STREAM_BIDIRECTIONAL, 0);
	if (ret < 0) {
		LOG_ERR("Failed to open QUIC stream (slot %d): %d", slot, ret);
		return ret;
	}

	stream->fd = ret;
	stream->expecting = 0;
	stream->received = 0;
	stream->counter = 0;
	stream->active = true;

	ret = send_quic_data(cfg, stream);
	if (ret < 0) {
		zsock_close(stream->fd);
		stream->fd = -1;
		stream->active = false;
		return ret;
	}

	LOG_INF("Stream started: slot=%d fd=%d", slot, stream->fd);
	return 0;
}

/* Close a single stream and mark the slot as free. */
static void stop_stream(struct config *cfg, int slot)
{
	struct stream_ctx *stream = &cfg->streams[slot];

	if (!stream->active) {
		LOG_WRN("Slot %d is not active", slot);
		return;
	}

	LOG_INF("Stopping stream: slot=%d fd=%d counter=%u",
		slot, stream->fd, stream->counter);

	zsock_close(stream->fd);
	stream->fd = -1;
	stream->active = false;
}

/*
 * Receive and process data on one stream.  Sends the next batch once a
 * complete echo response has been verified.
 */
static int process_stream(struct config *cfg, struct stream_ctx *stream)
{
	static char buf[RECV_BUF_SIZE];
	int received;
	int ret = 0;

	received = zsock_recv(stream->fd, buf, sizeof(buf), ZSOCK_MSG_DONTWAIT);
	if (received == 0) {
		/* Server closed the stream. */
		ret = -EIO;
		goto out;
	} else if (received < 0) {
		ret = (errno == EAGAIN || errno == EWOULDBLOCK) ? 0 : -errno;
		goto out;
	}

	ret = compare_quic_data(stream, buf, received);
	if (ret != 0) {
		goto out;
	}

	stream->received += received;

	if (stream->received < stream->expecting) {
		/* More data still expected for this round. */
		LOG_DBG("%s QUIC: stream fd=%d expecting %zd more data, missing %zd",
			cfg->proto, stream->fd, stream->expecting,
			stream->expecting - stream->received);
		goto out;
	}

	/* Full echo response received and verified. */
	if (PRINT_PROGRESS) {
		LOG_DBG("%s QUIC: stream fd=%d received+verified %u bytes",
			cfg->proto, stream->fd, stream->received);
	}

	if (++stream->counter % 1000 == 0U) {
		LOG_INF("%s QUIC: stream fd=%d exchanged %u rounds",
			cfg->proto, stream->fd, stream->counter);
	}

	/* Immediately send the next batch. */
	ret = send_quic_data(cfg, stream);

out:
	return ret;
}

/*
 * Drain cmd_msgq and act on every pending command.
 * Returns true if the fds array needs to be rebuilt.
 */
static bool handle_commands(struct config *cfg)
{
	struct cmd_msg msg;
	bool fds_dirty = false;

	while (k_msgq_get(&cmd_msgq, &msg, K_NO_WAIT) == 0) {
		if (msg.op == OP_START_STREAM) {
			/* Find a free slot. */
			int slot = -1;

			ARRAY_FOR_EACH(cfg->streams, i) {
				if (!cfg->streams[i].active) {
					slot = (int)i;
					break;
				}
			}

			if (slot < 0) {
				LOG_ERR("No free stream slots (max %d)",
					CONFIG_QUIC_MAX_STREAMS_BIDI);
				continue;
			}

			if (start_stream(cfg, slot) == 0) {
				fds_dirty = true;
			}

		} else if (msg.op == OP_STOP_STREAM) {
			int slot = msg.sock;

			if (slot < 0 ||
			    slot >= CONFIG_QUIC_MAX_STREAMS_BIDI) {
				LOG_ERR("Invalid slot %d", slot);
				continue;
			}

			stop_stream(cfg, slot);
			fds_dirty = true;
		}
	}

	return fds_dirty;
}

static void cleanup_all_streams(struct config *cfg)
{
	ARRAY_FOR_EACH(cfg->streams, i) {
		if (cfg->streams[i].active) {
			stop_stream(cfg, (int)i);
		}
	}

	if (cfg->quic_socket >= 0) {
		zsock_close(cfg->quic_socket);
		cfg->quic_socket = -1;
	}
}

static int check_our_ipv6_sockets(int sock,
				  struct net_in6_addr *deprecated_addr)
{
	struct net_sockaddr_in6 addr = { 0 };
	net_socklen_t addrlen = sizeof(addr);
	int ret;

	if (sock < 0) {
		return -EINVAL;
	}

	ret = zsock_getsockname(sock, (struct net_sockaddr *)&addr, &addrlen);
	if (ret != 0) {
		return -errno;
	}

	if (!net_ipv6_addr_cmp(deprecated_addr, &addr.sin6_addr)) {
		return -ENOENT;
	}

	need_restart = true;

	return 0;
}

static void ipv6_event_handler(struct net_mgmt_event_callback *cb,
			       uint64_t mgmt_event, struct net_if *iface)
{
	static char addr_str[INET6_ADDRSTRLEN];

	if (want_to_quit) {
		k_sem_give(&run_app);
		want_to_quit = false;
	}

	if (!IS_ENABLED(CONFIG_NET_IPV6_PE)) {
		return;
	}

	if ((mgmt_event & IPV6_EVENT_MASK) != mgmt_event) {
		return;
	}

	if (cb->info == NULL ||
	    cb->info_length != sizeof(struct in6_addr)) {
		return;
	}

	if (mgmt_event == NET_EVENT_IPV6_ADDR_ADD) {
		struct net_if_addr *ifaddr;
		struct in6_addr added_addr;

		memcpy(&added_addr, cb->info, sizeof(struct in6_addr));

		ifaddr = net_if_ipv6_addr_lookup(&added_addr, &iface);
		if (ifaddr == NULL) {
			return;
		}

		/* Wait until we get a temporary address before continuing
		 * after boot.
		 */
		if (ifaddr->is_temporary) {
			static bool once;

			LOG_INF("Temporary IPv6 address %s added",
				zsock_inet_ntop(NET_AF_INET6, &added_addr,
						addr_str,
						sizeof(addr_str) - 1));

			if (!once) {
				k_sem_give(&run_app);
				once = true;
			}
		}
	}

	if (mgmt_event == NET_EVENT_IPV6_ADDR_DEPRECATED) {
		struct in6_addr deprecated_addr;

		memcpy(&deprecated_addr, cb->info, sizeof(struct in6_addr));

		LOG_INF("IPv6 address %s deprecated",
			zsock_inet_ntop(NET_AF_INET6, &deprecated_addr,
					addr_str, sizeof(addr_str) - 1));

		(void)check_our_ipv6_sockets(conf.quic_socket,
					     &deprecated_addr);

		if (need_restart && control_fd >= 0) {
			zvfs_eventfd_write(control_fd, 1);
		}

		return;
	}
}

static void event_handler(struct net_mgmt_event_callback *cb,
			  uint64_t mgmt_event, struct net_if *iface)
{
	if ((mgmt_event & EVENT_MASK) != mgmt_event) {
		return;
	}

	if (mgmt_event == NET_EVENT_L4_CONNECTED) {
		LOG_INF("Network connected");

		connected = true;
		conf.mtu = net_if_get_mtu(iface);

		if (!IS_ENABLED(CONFIG_NET_IPV6_PE)) {
			k_sem_give(&run_app);
		}

		return;
	}

	if (mgmt_event == NET_EVENT_L4_DISCONNECTED) {
		LOG_INF("Network disconnected");

		connected = false;
		k_sem_reset(&run_app);

		return;
	}
}

int init_quic(void)
{
	sec_tag_t sec_tag_list[] = {
		QUIC_CA_CERTIFICATE_TAG,
	};
	static const char * const alpn_list[] = {
		"echo-quic",
		NULL
	};
	struct net_sockaddr_storage remote_addr;
	struct net_sockaddr_storage local_addr;
	int ret;
	int proto = AF_UNSPEC;

	ret = tls_credential_add(QUIC_CA_CERTIFICATE_TAG,
				 TLS_CREDENTIAL_CA_CERTIFICATE,
				 quic_ca_certificate,
				 sizeof(quic_ca_certificate));
	if (ret < 0) {
		LOG_ERR("Failed to register CA certificate: %d", ret);
	}

	memset(&remote_addr, 0, sizeof(remote_addr));

	/* Prefer IPv6 if both are enabled, but allow fallback to IPv4
	 * if IPv6 address is not valid.
	 */
	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		remote_addr.ss_family = AF_INET6;

		if (!net_ipaddr_parse(CONFIG_NET_CONFIG_PEER_IPV6_ADDR,
				      sizeof(CONFIG_NET_CONFIG_PEER_IPV6_ADDR) - 1,
				      net_sad(&remote_addr))) {
			LOG_DBG("Invalid %s IPv%d address", "remote", 6);
			goto skip_ipv6;
		}

		if (!net_ipaddr_parse(CONFIG_NET_CONFIG_MY_IPV6_ADDR,
				      sizeof(CONFIG_NET_CONFIG_MY_IPV6_ADDR) - 1,
				      net_sad(&local_addr))) {
			LOG_DBG("Invalid %s IPv%d address", "local", 6);
			goto skip_ipv6;
		}

		proto = AF_INET6;
	}

skip_ipv6:
	if (proto == NET_AF_UNSPEC && IS_ENABLED(CONFIG_NET_IPV4)) {
		remote_addr.ss_family = AF_INET;

		if (!net_ipaddr_parse(CONFIG_NET_CONFIG_PEER_IPV4_ADDR,
				      sizeof(CONFIG_NET_CONFIG_PEER_IPV4_ADDR) - 1,
				      net_sad(&remote_addr))) {
			LOG_ERR("Invalid %s IPv%d address", "remote", 4);
			return -EINVAL;
		}

		if (!net_ipaddr_parse(CONFIG_NET_CONFIG_MY_IPV4_ADDR,
				      sizeof(CONFIG_NET_CONFIG_MY_IPV4_ADDR) - 1,
				      net_sad(&local_addr))) {
			LOG_ERR("Invalid %s IPv%d address", "local", 4);
			return -EINVAL;
		}

		proto = AF_INET;
	}

	if (net_sin(net_sad(&remote_addr))->sin_port == 0) {
		net_sin(net_sad(&remote_addr))->sin_port = htons(PEER_PORT);
	}

	ret = setup_quic(net_sad(&remote_addr), net_sad(&local_addr),
			 QUIC_STREAM_CLIENT,
			 sec_tag_list, sizeof(sec_tag_list),
			 (const char **)alpn_list, sizeof(alpn_list));
	if (ret < 0) {
		LOG_ERR("Failed to setup QUIC over IPv%d (%d)",
			proto == AF_INET6 ? 6 : 4, ret);
		return ret;
	}

	LOG_INF("QUIC initialized successfully, sock=%d", ret);
	return ret;
}

static void init_app(struct config *cfg)
{
	LOG_INF(APP_BANNER);

#if defined(CONFIG_USERSPACE)
	struct k_mem_partition *parts[] = {
#if Z_LIBC_PARTITION_EXISTS
		&z_libc_partition,
#endif
		&app_partition
	};

	int ret = k_mem_domain_init(&app_domain, ARRAY_SIZE(parts), parts);

	__ASSERT(ret == 0, "k_mem_domain_init() failed %d", ret);
	ARG_UNUSED(ret);
#endif

	if (IS_ENABLED(CONFIG_NET_CONNECTION_MANAGER)) {
		net_mgmt_init_event_callback(&mgmt_cb,
					     event_handler, EVENT_MASK);
		net_mgmt_add_event_callback(&mgmt_cb);

		conn_mgr_mon_resend_status();
	}

	net_mgmt_init_event_callback(&ipv6_mgmt_cb,
				     ipv6_event_handler, IPV6_EVENT_MASK);
	net_mgmt_add_event_callback(&ipv6_mgmt_cb);

	init_vlan();

	cfg->quic_socket = init_quic();
	cfg->proto = IS_ENABLED(CONFIG_NET_IPV6) ? "IPv6" : "IPv4";

	ARRAY_FOR_EACH(cfg->streams, i) {
		cfg->streams[i].fd = -1;
		cfg->streams[i].active = false;
	}
}

/*
 * Main worker loop.
 *
 * Waits for network connectivity, then enters a poll loop that handles:
 *   - Control events from the shell (via cmd_msgq + control_fd eventfd)
 *   - Incoming data on any active stream socket
 *
 * When a need_restart is signalled (e.g. IPv6 address deprecation), all
 * streams are torn down and the connection is re-established.
 */
static void start_client(void *p1, void *p2, void *p3)
{
	struct config *cfg = p1;
	int ret;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* Block until the network is up. */
	k_sem_take(&run_app, K_FOREVER);

	if (IS_ENABLED(CONFIG_NET_IPV6_PE)) {
		/* Allow time for a temporary address to be assigned. */
		k_sleep(K_SECONDS(1));
	}

restart:
	if (need_restart) {
		LOG_INF("Restarting QUIC connection...");
		cleanup_all_streams(cfg);
		cfg->quic_socket = init_quic();
		need_restart = false;
	}

	/* Create the control eventfd that wakes the poll loop. */
	control_fd = zvfs_eventfd(0, 0);
	if (control_fd < 0) {
		LOG_ERR("Failed to create control eventfd: %d", errno);
		return;
	}

	prepare_fds(cfg);

	LOG_INF("Ready. Use 'sample start' to open a stream.");

	while (connected && !need_restart) {
		wait(cfg);

		/* Drain the control eventfd (non-blocking). */
		if (cfg->fds[0].revents & ZSOCK_POLLIN) {
			zvfs_eventfd_t val;

			zvfs_eventfd_read(control_fd, &val);
		}

		/* Process any pending shell commands first. */
		if (handle_commands(cfg)) {
			prepare_fds(cfg);
		}

		/*
		 * Process data on each active stream.  Iterate by slot, not
		 * by fds[] index, so that handle_commands() additions/removals
		 * during this iteration do not corrupt the loop.
		 */
		ARRAY_FOR_EACH(cfg->streams, i) {
			struct stream_ctx *stream = &cfg->streams[i];
			bool readable;

			if (!stream->active) {
				continue;
			}

			/* Check whether this stream's fd is in the poll result. */
			readable = false;

			for (int j = 1; j < cfg->nfds; j++) {
				if (cfg->fds[j].fd == stream->fd &&
				    (cfg->fds[j].revents & ZSOCK_POLLIN)) {
					readable = true;
					break;
				}
			}

			if (!readable) {
				continue;
			}

			ret = process_stream(cfg, stream);
			if (ret < 0) {
				LOG_ERR("Stream slot=%zu fd=%d error %d, closing",
					i, stream->fd, ret);
				stop_stream(cfg, (int)i);
				prepare_fds(cfg);
			}
		}
	}

	/* Close the control eventfd before a potential restart. */
	zsock_close(control_fd);
	control_fd = -1;

	if (need_restart) {
		goto restart;
	}

	cleanup_all_streams(cfg);
	LOG_INF("Client stopped.");
}

/*
 * Write a command to the msgq and poke the control eventfd so the worker
 * thread wakes up from poll() immediately.
 */
static void send_cmd(enum operation op, int sock)
{
	struct cmd_msg msg = {
		.sock = sock,
		.op = op,
	};
	int ret;

	ret = k_msgq_put(&cmd_msgq, &msg, K_MSEC(10));
	if (ret < 0) {
		LOG_ERR("Cannot write to cmd_msgq (%d)", ret);
		return;
	}

	/* Wake the poll loop. */
	if (control_fd >= 0) {
		zvfs_eventfd_write(control_fd, 1);
	}
}

static int cmd_sample_start(const struct shell *sh,
			    size_t argc, char *argv[])
{
	if (argv[1] != NULL && (strcmp(argv[1], "-h") == 0 ||
				strcmp(argv[1], "--help") == 0)) {
		shell_help(sh);
		return 0;
	}

	if (!connected) {
		shell_error(sh, "Network not connected");
		return -ENETDOWN;
	}

	send_cmd(OP_START_STREAM, -1);
	shell_print(sh, "Stream start requested. Check logs for slot number.");

	return 0;
}

static int cmd_sample_stop(const struct shell *sh,
			   size_t argc, char *argv[])
{
	int slot;
	int err = 0;

	if (argc >= 2 && argv[1] != NULL &&
	    (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
		shell_help(sh);
		return 0;
	}

	if (argc < 2) {
		/* No slot specified - stop all active streams */
		shell_print(sh, "Stopping all streams");
		for (int i = 0; i < CONFIG_QUIC_MAX_STREAMS_BIDI; i++) {
			send_cmd(OP_STOP_STREAM, i);
		}
		return 0;
	}

	slot = shell_strtol(argv[1], 10, &err);
	if (err < 0) {
		shell_error(sh, "Invalid slot number: %s", argv[1]);
		return -EINVAL;
	}

	send_cmd(OP_STOP_STREAM, slot);

	return 0;
}

static int cmd_sample_info(const struct shell *sh,
			   size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	bool any = false;

	shell_print(sh, "%-6s %-8s %-10s %-10s %-10s",
		    "Slot", "FD", "Expecting", "Received", "Rounds");

	ARRAY_FOR_EACH(conf.streams, i) {
		const struct stream_ctx *s = &conf.streams[i];

		if (!s->active) {
			continue;
		}

		shell_print(sh, "%-6zu %-8d %-10u %-10u %-10u",
			    i, s->fd, s->expecting, s->received, s->counter);
		any = true;
	}

	if (!any) {
		shell_print(sh, "(no active streams)");
	}

	return 0;
}

static int cmd_sample_quit(const struct shell *sh,
			   size_t argc, char *argv[])
{
	if (argv[1] != NULL && (strcmp(argv[1], "-h") == 0 ||
				strcmp(argv[1], "--help") == 0)) {
		shell_help(sh);
		return 0;
	}

	want_to_quit = true;
	quit();
	conn_mgr_mon_resend_status();

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sample_commands,
	SHELL_CMD(start, NULL,
		  "Start a new echo stream",
		  cmd_sample_start),
	SHELL_CMD_ARG(stop, NULL,
		      "Stop stream(s). Usage: stop [slot] (no slot = stop all)",
		      cmd_sample_stop, 1, 1),
	SHELL_CMD(info, NULL,
		  "List active streams and their statistics",
		  cmd_sample_info),
	SHELL_CMD(quit, NULL,
		  "Quit the sample application",
		  cmd_sample_quit),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(sample, &sample_commands,
		   "Sample application commands", NULL);

int main(void)
{
	init_app(&conf);

	if (!IS_ENABLED(CONFIG_NET_CONNECTION_MANAGER)) {
		/* Start immediately if connection management is not used. */
		connected = true;
		k_sem_give(&run_app);
	}

	k_thread_priority_set(k_current_get(), THREAD_PRIORITY);

#if defined(CONFIG_USERSPACE)
	k_thread_access_grant(k_current_get(), &run_app);
	k_mem_domain_add_thread(&app_domain, k_current_get());

	k_thread_user_mode_enter(start_client, &conf, NULL, NULL);
#else
	start_client(&conf, NULL, NULL);
#endif

	k_sleep(K_FOREVER);

	return 0;
}
