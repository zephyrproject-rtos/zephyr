/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/iterable_sections.h>
#include <zephyr/spinlock.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/logging/log_frontend.h>
#include <zephyr/logging/log_ipc_service.h>
#include <zephyr/logging/log_internal.h>
#include <zephyr/logging/log_msg.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/cbprintf.h>
#include <stddef.h>
LOG_MODULE_REGISTER(log_frontend_ipc);

/* IPC service must support zero copy API. */
#if DT_HAS_CHOSEN(zephyr_log_ipc)
#define IPC_NODE DT_CHOSEN(zephyr_log_ipc)
#elif DT_NUM_INST_STATUS_OKAY(zephyr_ipc_icbmsg) == 1
#define IPC_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_ipc_icbmsg)
#elif DT_NUM_INST_STATUS_OKAY(zephyr_ipc_openamp_static_vrings) == 1
#define IPC_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_ipc_openamp_static_vrings)
#else
#error "No IPC node found"
#endif

#define EARLY_CHUNK_COUNT \
	(CONFIG_LOG_FRONTEND_IPC_EARLY_BUFFER_SIZE / CONFIG_LOG_FRONTEND_IPC_PACKET_SIZE)
#define MSG_HDR_SIZE 4
/* Maximum size of a log message that can be sent in a single IPC message. */
#define MSG_MAX_SIZE \
	(CONFIG_LOG_FRONTEND_IPC_PACKET_SIZE - MSG_HDR_SIZE)
BUILD_ASSERT(MSG_HDR_SIZE == offsetof(struct log_ipc_service_msg, data.log_msg.data));

#define IPC_BUFFER_SIZE(len) (offsetof(struct log_ipc_service_msg, data.log_msg.data) + (len))

#if MSG_MAX_SIZE < UINT8_MAX
typedef uint8_t early_chunk_size_t;
#else
typedef uint16_t early_chunk_size_t;
#endif

TYPE_SECTION_START_EXTERN(const char *, log_strings);

static uint8_t early_buffer[CONFIG_LOG_FRONTEND_IPC_EARLY_BUFFER_SIZE] __aligned(sizeof(uint32_t));
static early_chunk_size_t early_chunks[EARLY_CHUNK_COUNT];
static uint8_t early_chunk_status[EARLY_CHUNK_COUNT];
static size_t early_chunk_data_off;
static size_t early_chunk_idx;

static struct ipc_ept ept;
static volatile bool ipc_ready;
static atomic_t dropped_cnt;
static bool in_panic;

static struct log_ipc_service_msg *curr_msg;
static size_t curr_pkt_off;
static uint32_t curr_pkt_cap;
static struct k_spinlock lock;

static struct k_timer timer;
static struct k_timer dropped_timer;

static void cpy(void *dst, const void *src, size_t len)
{
	memcpy(dst, src, len);
}

static void msg_dropped(uint32_t cnt)
{
	if (atomic_add(&dropped_cnt, cnt) == 0 && ipc_ready) {
		k_timer_start(&dropped_timer,
			      K_MSEC(CONFIG_LOG_FRONTEND_IPC_DROPPED_MSG_TIMEOUT), K_NO_WAIT);
	}
}

static void dropped_notify(void)
{
	int err;
	uint32_t cnt = (uint32_t)atomic_set(&dropped_cnt, 0);

	if (cnt == 0) {
		return;
	}

	struct log_ipc_service_msg msg = {
		.id = Z_LOG_IPC_SERVICE_ID_DROPPED,
		.status = Z_LOG_IPC_SERVICE_STATUS_OK,
		.data = {
			.dropped = {
				.dropped = cnt
			}
		}
	};

	err = ipc_service_send(&ept, &msg, sizeof(msg));
	if (err < 0) {
		/* Failed to send dropped message count, try again later. */
		msg_dropped(cnt);
	}
}

static void dropped_timer_cb(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	dropped_notify();
}

static void send_early_chunks(void)
{
	uint8_t *data = early_buffer;
	struct log_ipc_service_msg *msg;
	size_t len;
	int rv;

	if (CONFIG_LOG_FRONTEND_IPC_EARLY_BUFFER_SIZE == 0) {
		dropped_notify();
		return;
	}

	if (early_chunk_data_off == 0) {
		return;
	}

	for (size_t i = 0; i <= early_chunk_idx; i++) {
		if (early_chunks[i] == 0) {
			break;
		}
		len = CONFIG_LOG_FRONTEND_IPC_PACKET_SIZE;
		do {
			rv = ipc_service_get_tx_buffer(&ept, (void **)&msg, &len, K_NO_WAIT);
		} while (rv < 0 && !in_panic);

		msg->id = Z_LOG_IPC_SERVICE_ID_MSG;
		msg->status = early_chunk_status[i];
		cpy(msg->data.log_msg.data, data, early_chunks[i]);
		rv = ipc_service_send_nocopy(&ept, msg, IPC_BUFFER_SIZE(early_chunks[i]));
		if (rv < 0) {
			return;
		}

		data += early_chunks[i];
	}
}

static bool early_write_prepare(size_t len)
{
	if (CONFIG_LOG_FRONTEND_IPC_EARLY_BUFFER_SIZE == 0) {
		return false;
	}

	if (early_chunk_idx >= EARLY_CHUNK_COUNT) {
		return false;
	}

	if (early_chunks[early_chunk_idx] + len > MSG_MAX_SIZE) {
		early_chunk_idx++;
		if (early_chunk_idx >= EARLY_CHUNK_COUNT) {
			return false;
		}
	}

	early_chunks[early_chunk_idx] += len;
	early_chunk_status[early_chunk_idx]++;
	return true;
}

static void early_write(const void *buf, size_t len)
{
	cpy(&early_buffer[early_chunk_data_off], buf, len);
	early_chunk_data_off += len;
}

/* Sends the current packet and resets the state. Must be called with the interrupt lock held. */
static void send_current_packet(void)
{
	int rv;
	size_t len;

	if (curr_msg == NULL) {
		return;
	}

	len = curr_pkt_off + offsetof(struct log_ipc_service_msg, data.log_msg.data);
	rv = ipc_service_send_nocopy(&ept, (void *)curr_msg, len);
	if (rv < 0) {
		/* Failed to send current packet, try again later. */
		msg_dropped(curr_msg->status);
		(void)ipc_service_drop_tx_buffer(&ept, (void *)curr_msg);
		return;
	}

	curr_msg = NULL;
	curr_pkt_off = 0;
	curr_pkt_cap = 0;
}


static void flush(void)
{
	K_SPINLOCK(&lock) {
		send_current_packet();
	}
}

static void msg_timeout(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	flush();
}

static bool ipc_write_prepare(size_t len)
{
	if (curr_msg != NULL && (curr_pkt_off + len > curr_pkt_cap)) {
		send_current_packet();
	}

	if (curr_msg == NULL) {
		int rv;
		uint32_t buf_len = MAX(CONFIG_LOG_FRONTEND_IPC_PACKET_SIZE, IPC_BUFFER_SIZE(len));

		rv = ipc_service_get_tx_buffer(&ept, (void **)&curr_msg, &buf_len, K_NO_WAIT);
		if (rv < 0) {
			return false;
		}

		curr_pkt_cap = buf_len - offsetof(struct log_ipc_service_msg, data.log_msg.data);
		curr_msg->id = Z_LOG_IPC_SERVICE_ID_MSG;
		curr_msg->status = 0;
		curr_pkt_off = 0;
		k_timer_start(&timer, K_MSEC(CONFIG_LOG_FRONTEND_IPC_MSG_TIMEOUT), K_NO_WAIT);
	}

	/* Status holds number of message in the current packet. */
	curr_msg->status++;

	return true;
}

static bool write_prepare(size_t len)
{
	bool rv;

	if (ipc_ready) {
		rv = ipc_write_prepare(len);
	} else {
		rv = early_write_prepare(len);
	}
	return rv;
}

static void ipc_write(const void *msg, size_t len)
{
	cpy(&curr_msg->data.log_msg.data[curr_pkt_off], (void *)msg, len);
	curr_pkt_off += len;
}

/* Callbacks ignore the empty call which cbprintf_package_convert() makes at the end of
 * the package because data (e.g. hexdump) must follow the package without padding.
 */
static int early_package_cb(const void *buf, size_t len, void *ctx)
{
	ARG_UNUSED(ctx);

	if ((CONFIG_LOG_FRONTEND_IPC_EARLY_BUFFER_SIZE > 0) && (buf != NULL)) {
		early_write(buf, len);
	}

	return 0;
}

static int package_cb(const void *buf, size_t len, void *ctx)
{
	ARG_UNUSED(ctx);

	if (buf != NULL) {
		ipc_write(buf, len);
	}

	return 0;
}

/* Next message in the packet must be aligned. */
static void msg_end(void)
{
	if (ipc_ready) {
		curr_pkt_off = ROUND_UP(curr_pkt_off, Z_LOG_MSG_ALIGNMENT);
	} else if (CONFIG_LOG_FRONTEND_IPC_EARLY_BUFFER_SIZE > 0) {
		early_chunk_data_off = ROUND_UP(early_chunk_data_off, Z_LOG_MSG_ALIGNMENT);
	}
}

static void send_source_details(void)
{
	bool str_access = !IS_ENABLED(CONFIG_LOG_FRONTEND_IPC_FSC);
	uintptr_t source_addr = (uintptr_t)TYPE_SECTION_START(log_const);
	uintptr_t log_str_ptr = (uintptr_t)TYPE_SECTION_START(log_strings);
	struct log_ipc_service_msg msg = {
		.id = Z_LOG_IPC_SERVICE_ID_READY,
		.status = Z_LOG_IPC_SERVICE_STATUS_OK,
		.data = {
			.ready = {
				.source_addr = str_access ? source_addr : (uintptr_t)0,
				.log_str_ptr = str_access ? log_str_ptr : (uintptr_t)0,
				.source_count = z_log_sources_count(),
			},
		},
	};

	int rv = ipc_service_send(&ept, &msg, sizeof(msg));

	(void)rv;
	__ASSERT_NO_MSG(rv >= 0);
}

static void bound_cb(void *priv)
{
	ARG_UNUSED(priv);

	ipc_ready = true;
	send_source_details();
	send_early_chunks();
}

static void get_name_response(uint16_t source_id)
{
	const char *name = log_source_name_get(0, source_id);
	size_t slen = strlen(name);
	size_t msg_offset = offsetof(struct log_ipc_service_msg, data.source_name.name);
	size_t msg_size = slen + 1 + msg_offset;
	size_t alloc_size = msg_size;
	struct log_ipc_service_msg *outmsg;
	int err;

	err = ipc_service_get_tx_buffer(&ept, (void **)&outmsg, &alloc_size, K_NO_WAIT);
	if (err < 0) {
		return;
	}

	char *dst = outmsg->data.source_name.name;

	outmsg->id = Z_LOG_IPC_SERVICE_ID_GET_SOURCE_NAME;
	outmsg->status = Z_LOG_IPC_SERVICE_STATUS_OK;
	memcpy(dst, name, slen);
	dst[slen] = '\0';

	outmsg->data.source_name.source_id = source_id;

	err = ipc_service_send_nocopy(&ept, outmsg, msg_size);
	__ASSERT_NO_MSG(err >= 0);
}

static void recv_cb(const void *data, size_t len, void *priv)
{
	ARG_UNUSED(priv);
	int err;

	if (!IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING) && !IS_ENABLED(CONFIG_LOG_FRONTEND_IPC_FSC)) {
		return;
	}

	struct log_ipc_service_msg *msg = (struct log_ipc_service_msg *)data;
	struct log_ipc_service_msg outmsg = {
		.id = msg->id,
		.status = Z_LOG_IPC_SERVICE_STATUS_OK
	};

	switch (msg->id) {
	case Z_LOG_IPC_SERVICE_ID_GET_SOURCE_NAME:
		if (IS_ENABLED(CONFIG_LOG_FRONTEND_IPC_FSC)) {
			get_name_response(msg->data.source_name.source_id);
			return;
		}
		break;
	case Z_LOG_IPC_SERVICE_ID_SET_RUNTIME_LEVEL:
	{
		uint16_t source_id = msg->data.set_rt_level.source_id;
		uint32_t level = msg->data.set_rt_level.runtime_level;

		outmsg.data.set_rt_level.runtime_level = log_frontend_filter_set(source_id, level);
		break;
	}
	case Z_LOG_IPC_SERVICE_ID_GET_LEVELS:
	{
		uint16_t source_id = msg->data.levels.source_id;

		outmsg.data.levels.level = log_frontend_filter_get(source_id, false);
		outmsg.data.levels.runtime_level = log_frontend_filter_get(source_id, true);
		break;
	}
	default:
		__ASSERT(0, "Unexpected message");
		break;
	}

	err = ipc_service_send(&ept, &outmsg, sizeof(outmsg));
	__ASSERT_NO_MSG(err >= 0);
}

void log_frontend_msg(const void *source,
		      const struct log_msg_desc desc,
		      uint8_t *package, const void *data)
{
	const void *source_id = desc.level == LOG_LEVEL_INTERNAL_RAW_STRING ?
			source : (void *)log_source_id(source);
	struct log_msg msg = {
		.hdr = {
			.desc = desc,
			.source = source_id,
			.timestamp = z_log_timestamp(),
		},
	};
	static const uint32_t flags = CBPRINTF_PACKAGE_CONVERT_RW_STR |
		(IS_ENABLED(CONFIG_LOG_FRONTEND_IPC_FSC) ?
			CBPRINTF_PACKAGE_CONVERT_RO_STR : 0);
	cbprintf_convert_cb write_cb;
	size_t in_pkg_len = desc.package_len;
	k_spinlock_key_t key;
	uint16_t strl[4];
	int package_len;
	size_t msg_len;

	package_len = cbprintf_package_convert(package, in_pkg_len, NULL, NULL, flags,
					       strl, ARRAY_SIZE(strl));
	if (package_len < 0) {
		return;
	}
	msg.hdr.desc.valid = 1;
	msg.hdr.desc.package_len = package_len;
	msg_len = log_msg_get_total_wlen(msg.hdr.desc) * sizeof(uint32_t);

	key = k_spin_lock(&lock);
	if (!write_prepare(msg_len)) {
		msg_dropped(1);
		goto out;
	}

	write_cb = ipc_ready ? package_cb : early_package_cb;
	write_cb(&msg, offsetof(struct log_msg, data), NULL);

	(void)cbprintf_package_convert(package, in_pkg_len, write_cb, NULL,
				       flags, strl, ARRAY_SIZE(strl));

	if (data != NULL && desc.data_len > 0) {
		write_cb(data, desc.data_len, NULL);
	}
	msg_end();
out:
	k_spin_unlock(&lock, key);
}

static void log_frontend_simple(const void *source, uint32_t level, const char *fmt,
				uint32_t *args, size_t args_len)
{
	/* If compressed message is not used, use the regular log message. */
	size_t pkg_len = (2 + args_len) * sizeof(uint32_t) +
			(IS_ENABLED(CONFIG_LOG_MSG_APPEND_RO_STRING_LOC) ? 1 : 0);
	struct log_msg_desc desc = {
		.valid = 1,
		.type = Z_LOG_MSG_LOG,
		.domain = 0,
		.level = level,
		.package_len = pkg_len,
	};
	uint8_t pkg[pkg_len] __aligned(sizeof(uint32_t));
	uint32_t *pkg32 = (uint32_t *)pkg;
	union cbprintf_package_hdr hdr = {
		.desc = {
			.len = args_len + 2 * sizeof(void *) / sizeof(uint32_t),
			.ro_str_cnt = IS_ENABLED(CONFIG_LOG_MSG_APPEND_RO_STRING_LOC) ? 1 : 0
		}
	};

	pkg32[0] = (uint32_t)(uintptr_t)hdr.raw;
	pkg32[1] = (uint32_t)(uintptr_t)fmt;

	for (size_t i = 0; i < args_len; i++) {
		pkg32[i + 2] = args[i];
	}
	if (IS_ENABLED(CONFIG_LOG_MSG_APPEND_RO_STRING_LOC)) {
		pkg[pkg_len - 1] = 1;
	}

	log_frontend_msg(source, desc, (uint8_t *)pkg, NULL);
}

void log_frontend_simple_0(const void *source, uint32_t level, const char *fmt)
{
	log_frontend_simple(source, level, fmt, NULL, 0);
}

void log_frontend_simple_1(const void *source, uint32_t level, const char *fmt, uint32_t arg)
{
	log_frontend_simple(source, level, fmt, &arg, 1);
}

void log_frontend_simple_2(const void *source, uint32_t level,
			   const char *fmt, uint32_t arg0, uint32_t arg1)
{
	uint32_t args[2] = { arg0, arg1 };

	log_frontend_simple(source, level, fmt, args, 2);
}

void log_frontend_panic(void)
{
	in_panic = true;
	flush();
}

void log_frontend_init(void)
{
#ifdef CONFIG_LOG_TIMESTAMP_64BIT
	log_set_timestamp_func(k_cycle_get_64, sys_clock_hw_cycles_per_sec());
#else
	log_set_timestamp_func(k_cycle_get_32, sys_clock_hw_cycles_per_sec());
#endif
}

static const struct ipc_ept_cfg ept_cfg = {
	.name = "logging",
	.prio = 0,
	.cb = {
		.bound = bound_cb,
		.received = recv_cb,
	},
};

static int ipc_init(void)
{
	const struct device *ipc_instance = DEVICE_DT_GET(IPC_NODE);
	int err;

	err = ipc_service_open_instance(ipc_instance);
	if (err < 0 && err != -EALREADY) {
		LOG_ERR("ipc_service_open_instance() failure (err:%d)\n", err);
		return err;
	}

	err = ipc_service_register_endpoint(ipc_instance, &ept, &ept_cfg);
	__ASSERT_NO_MSG(err >= 0);

	k_timer_init(&timer, msg_timeout, NULL);
	k_timer_init(&dropped_timer, dropped_timer_cb, NULL);

	return err;
}

SYS_INIT(ipc_init, POST_KERNEL, UTIL_INC(CONFIG_IPC_SERVICE_REG_BACKEND_PRIORITY));
