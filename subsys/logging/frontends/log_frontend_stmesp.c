/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log_frontend.h>
#include <zephyr/logging/log_frontend_stmesp.h>
#include <zephyr/logging/log_frontend_stmesp_demux.h>
#include <zephyr/logging/log_output_dict.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_msg.h>
#include <zephyr/sys/cbprintf.h>
#ifdef CONFIG_NRF_ETR
#include <zephyr/drivers/misc/coresight/nrf_etr.h>
#endif

/* Only 32 bit platforms supported. */
BUILD_ASSERT(sizeof(void *) == sizeof(uint32_t));

#define LOG_FRONTEND_STM_NO_SOURCE 0xFFFF

#define EARLY_BUF_SIZE CONFIG_LOG_FRONTEND_STMESP_EARLY_BUF_SIZE

#define LEN_SZ sizeof(uint32_t)

#define STMESP_FLUSH_WORD 0xaabbccdd

#define STM_FLAG(reg) \
	stmesp_flag(reg, 1, false, IS_ENABLED(CONFIG_LOG_FRONTEND_STMESP_GUARANTEED_ACCESS))

#define STM_D8(reg, data, timestamp, marked)       \
	stmesp_data8(reg, data, timestamp, marked, \
		     IS_ENABLED(CONFIG_LOG_FRONTEND_STMESP_GUARANTEED_ACCESS))

#define STM_D32(reg, data, timestamp, marked)       \
	stmesp_data32(reg, data, timestamp, marked, \
		      IS_ENABLED(CONFIG_LOG_FRONTEND_STMESP_GUARANTEED_ACCESS))

/* Buffer for storing frontend data before STM/ETR is ready for usage.
 * When notification about ETR readiness is received content of this buffer is
 * written to the STM stimulus port.
 */
static atomic_t stmesp_chan_cnt;

static uint8_t early_buf[EARLY_BUF_SIZE] __aligned(sizeof(uint32_t));
static uint32_t early_buf_idx;
static struct k_spinlock lock;

/* Flag indicating that STM/ETR is ready for use. */
static bool etr_rdy;

/* Number of messages dropped due to too small early buffer. */
static uint32_t dropped;

/* Flag indicating that logging is in the panic mode. */
static bool in_panic;

static atomic_t new_data;

/* Enum used for type bit field in the message. */
enum stm_msg_type_log_dict {
	/* Dictionary-based log message */
	STM_MSG_TYPE_LOG_DICT = 0,
	/* Reserved for future use. */
	STM_MSG_TYPE_RESERVED = 1,
};

/* Descriptor of the dictionary based logging message. */
struct stm_log_dict_msg_desc {
	/* Structure version. Current version 0. */
	uint32_t ver: 2;

	/* Type. Set 0 as this is a logging message descriptor. */
	uint32_t type: 1;

	/* Severity level. */
	uint32_t level: 3;

	/* Data length, non-zero fox hexdump message. */
	uint32_t data_len: 12;

	/* Source ID. Source index as ordered in the memory section. */
	uint32_t source_id: 12;

	/* Reserved for future use. */
	uint32_t reserved: 2;
};

union stm_log_dict_hdr {
	struct stm_log_dict_msg_desc hdr;
	uint32_t raw;
};

/* Dictionary header initializer. */
#define DICT_HDR_INITIALIZER(_level, _source_id, _data_len)                                        \
	{                                                                                          \
		.hdr = {.ver = CONFIG_LOG_FRONTEND_STMESP_DICT_VER,                         \
			.type = STM_MSG_TYPE_LOG_DICT,                                             \
			.level = _level,                                                           \
			.data_len = _data_len,                                                     \
			.source_id = _source_id,                                                   \
			.reserved = 0 }                                                            \
	}

/* Align early buffer index to a 32 bit word. */
static inline void early_buf_align_idx(void)
{
	uint32_t rem = early_buf_idx & 0x3;

	if (rem) {
		early_buf_idx += (sizeof(uint32_t) - rem);
	}
}

/* Get address where length is written. Location is used in the dictionary mode
 * where length of the message is only known once whole message is written.
 * Calculated length is written to the length field location.
 */
static inline uint32_t *early_buf_len_loc(void)
{
	early_buf_align_idx();

	uint32_t *loc = (uint32_t *)&early_buf[early_buf_idx];

	early_buf_idx += LEN_SZ;

	return loc;
}

/* Check if there is space for requested amount of data. */
static inline bool early_buf_has_space(uint32_t len)
{
	return (EARLY_BUF_SIZE - early_buf_idx) >= len;
}

/* Calculate length of the message. It is calculated by taking current write
 * location and length location (which is at the beginning of the current message.
 * Used in dictionary mode.
 */
static inline uint32_t early_buf_get_len(uint32_t *len_loc)
{
	if (early_buf_idx == EARLY_BUF_SIZE) {
		return 0;
	}

	return (uint32_t)((uintptr_t)&early_buf[early_buf_idx] - (uintptr_t)len_loc - LEN_SZ);
}

/* Check if there is available space for the message. If yes, write length field
 * and return true. If allocation fails it sets next length field to 0 to indicate
 * that the buffer is full.
 */
static inline bool early_buf_alloc(uint32_t len)
{
	early_buf_align_idx();

	if (early_buf_has_space(len + LEN_SZ)) {
		*(uint32_t *)&early_buf[early_buf_idx] = len;
		early_buf_idx += LEN_SZ;
		return true;
	}

	if (early_buf_has_space(LEN_SZ)) {
		*(uint32_t *)&early_buf[early_buf_idx] = 0;
	}

	return false;
}

/* Switch to read mode. Start reading from the beginning. */
static inline void early_buf_read_mode(void)
{
	early_buf_idx = 0;
}

/* Get message. Returns 0 if no messages. */
static inline uint32_t early_buf_get_data(void **buf)
{
	early_buf_align_idx();

	if (early_buf_has_space(LEN_SZ)) {
		uint32_t len = *(uint32_t *)&early_buf[early_buf_idx];

		*buf = &early_buf[early_buf_idx + LEN_SZ];
		early_buf_idx += len + LEN_SZ;
		return len;
	}

	return 0;
}

static void early_buf_put_data(const void *buf, size_t len)
{
	if (early_buf_has_space(len)) {
		memcpy(&early_buf[early_buf_idx], buf, len);
		early_buf_idx += len;
	} else {
		early_buf_idx = EARLY_BUF_SIZE;
	}
}

static int early_package_cb(const void *buf, size_t len, void *ctx)
{
	ARG_UNUSED(ctx);

	early_buf_put_data(buf, len);

	return 0;
}

static inline void write_data(const void *data, size_t len, STMESP_Type *const stm_esp)
{
	uint32_t *p32 = (uint32_t *)data;

	while (len >= sizeof(uint32_t)) {
		STM_D32(stm_esp, *p32++, false, false);
		len -= sizeof(uint32_t);
	}

	uint8_t *p8 = (uint8_t *)p32;

	while (len > 0) {
		STM_D8(stm_esp, *p8++, false, false);
		len--;
	}
}

static int package_cb(const void *buf, size_t len, void *ctx)
{
	write_data(buf, len, (STMESP_Type *const)ctx);

	return len;
}

/* Get STM channel to use. Ensure that channel is unique for given priority level.
 * This way we know that writing to that channel will not be interrupted by
 * another log message writing to the same channel.
 */
static inline uint16_t get_channel(void)
{
	return (atomic_inc(&stmesp_chan_cnt) & 0x7F) + 1;
}

/* Convert pointer to the source structure to the source ID. */
static inline int16_t get_source_id(const void *source)
{
	if (source != NULL) {
		return IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING)
			       ? log_dynamic_source_id((void *)source)
			       : log_const_source_id(source);
	}

	return LOG_FRONTEND_STM_NO_SOURCE;
}

static void packet_end(STMESP_Type *stm_esp)
{
	STM_FLAG(stm_esp);
	atomic_set(&new_data, 1);
}

/* Common function to end the message when STMESP is not ready. */
static inline void early_msg_end(uint32_t *len_loc)
{
	*len_loc = early_buf_get_len(len_loc);
	if (*len_loc == 0) {
		dropped++;
	}
}

#if CONFIG_LOG_FRONTEND_STMESP_FSC
void log_frontend_msg(const void *source, const struct log_msg_desc desc, uint8_t *package,
		      const void *data)
{
	uint16_t strl[4];
	union log_frontend_stmesp_demux_header hdr = {.log = {.level = desc.level}};
	bool use_timestamp = desc.level != LOG_LEVEL_INTERNAL_RAW_STRING;
	const char *sname;
	static const char null_c = '\0';
	size_t sname_len;
	int package_len;
	int total_len;
	static const uint32_t flags = CBPRINTF_PACKAGE_CONVERT_RW_STR |
				      CBPRINTF_PACKAGE_CONVERT_RO_STR;

	sname = log_source_name_get(0, get_source_id(source));
	if (sname) {
		sname_len = strlen(sname) + 1;
	} else {
		sname = &null_c;
		sname_len = 1;
	}
	total_len = desc.data_len + sname_len /* null terminator */;

	package_len = cbprintf_package_convert(package, desc.package_len, NULL, NULL, flags,
					       strl, ARRAY_SIZE(strl));
	hdr.log.total_len = total_len + package_len;
	hdr.log.package_len = package_len;

	if ((EARLY_BUF_SIZE == 0) || etr_rdy) {
		STMESP_Type *stm_esp;
		int err;

		err = stmesp_get_port(get_channel(), &stm_esp);
		if (err < 0) {
			return;
		}

		STM_D32(stm_esp, hdr.raw, use_timestamp, true);
		(void)cbprintf_package_convert(package, desc.package_len,
					       package_cb, stm_esp, flags, strl, ARRAY_SIZE(strl));
		write_data(sname, sname_len, stm_esp);
		if (data) {
			write_data(data, desc.data_len, stm_esp);
		}
		packet_end(stm_esp);

	} else {
		k_spinlock_key_t key = k_spin_lock(&lock);

		if ((EARLY_BUF_SIZE == 0) ||
		    (early_buf_alloc(hdr.log.total_len + sizeof(hdr)) == false)) {
			dropped++;
			k_spin_unlock(&lock, key);
			return;
		}

		early_buf_put_data((const uint8_t *)&hdr, sizeof(hdr));
		(void)cbprintf_package_convert(package, desc.package_len, early_package_cb,
					       NULL, flags, strl, ARRAY_SIZE(strl));
		early_buf_put_data(sname, sname_len);
		if (data) {
			early_buf_put_data(data, desc.data_len);
		}

		k_spin_unlock(&lock, key);
	}
}

#else /* CONFIG_LOG_FRONTEND_STMESP_FSC  */

void log_frontend_msg(const void *source, const struct log_msg_desc desc, uint8_t *package,
		      const void *data)
{
	static const uint32_t flags = CBPRINTF_PACKAGE_CONVERT_RW_STR;
	union stm_log_dict_hdr dict_desc =
		DICT_HDR_INITIALIZER(desc.level, get_source_id(source), 0);

	if ((EARLY_BUF_SIZE == 0) || etr_rdy) {
		STMESP_Type *stm_esp;
		int err;

		err = stmesp_get_port(get_channel(), &stm_esp);
		if (err < 0) {
			return;
		}

		STM_D32(stm_esp, dict_desc.raw, true, true);
		(void)cbprintf_package_convert(package, desc.package_len, package_cb,
					       stm_esp, flags, NULL, 0);
		if (data) {
			package_cb(data, desc.data_len, stm_esp);
		}
		packet_end(stm_esp);
	} else {
		k_spinlock_key_t key;
		uint32_t *len_loc;

		key = k_spin_lock(&lock);
		len_loc = early_buf_len_loc();
		early_package_cb(&dict_desc.raw, sizeof(dict_desc.raw), NULL);
		(void)cbprintf_package_convert(package, desc.package_len, early_package_cb,
					       NULL, flags, NULL, 0);
		if (data) {
			early_package_cb(data, desc.data_len, NULL);
		}
		early_msg_end(len_loc);
		k_spin_unlock(&lock, key);
	}
}

/* Common function for optimized message (log with 0-2 arguments) which is used in
 * case when STMESP is not yet ready.
 */
static inline uint32_t *early_msg_start(uint32_t level, const void *source,
					uint32_t package_hdr, const char *fmt)
{
	union stm_log_dict_hdr dict_desc = DICT_HDR_INITIALIZER(level, get_source_id(source), 0);
	uint32_t fmt32 = (uint32_t)fmt;
	uint32_t *len_loc = early_buf_len_loc();

	early_package_cb(&dict_desc.raw, sizeof(dict_desc.raw), NULL);
	early_package_cb(&package_hdr, sizeof(package_hdr), NULL);
	early_package_cb(&fmt32, sizeof(fmt32), NULL);

	return len_loc;
}

/* Common function for optimized message (log with 0-2 arguments) which writes to STMESP */
static inline void msg_start(STMESP_Type *stm_esp, uint32_t level,
			     const void *source, uint32_t package_hdr, const char *fmt)

{
	union stm_log_dict_hdr dict_desc = DICT_HDR_INITIALIZER(level, get_source_id(source), 0);

	STM_D32(stm_esp, dict_desc.raw, true, true);
	STM_D32(stm_esp, package_hdr, false, false);
	STM_D32(stm_esp, (uint32_t)fmt, false, false);
}

void log_frontend_simple_0(const void *source, uint32_t level, const char *fmt)
{
	static const union cbprintf_package_hdr package_hdr = {.desc = {.len = 2}};

	if ((EARLY_BUF_SIZE == 0) || etr_rdy) {
		STMESP_Type *stm_esp;
		int err;

		err = stmesp_get_port(get_channel(), &stm_esp);
		if (err < 0) {
			return;
		}

		msg_start(stm_esp, level, source, (uint32_t)package_hdr.raw, fmt);
		packet_end(stm_esp);
		return;
	}

	uint32_t *len_loc;
	k_spinlock_key_t key;

	key = k_spin_lock(&lock);
	len_loc = early_msg_start(level, source, (uint32_t)package_hdr.raw, fmt);
	early_msg_end(len_loc);
	k_spin_unlock(&lock, key);
}

void log_frontend_simple_1(const void *source, uint32_t level, const char *fmt, uint32_t arg)
{
	static const union cbprintf_package_hdr package_hdr = {.desc = {.len = 2 + 1}};

	if ((EARLY_BUF_SIZE == 0) || etr_rdy) {
		STMESP_Type *stm_esp;
		int err;

		err = stmesp_get_port(get_channel(), &stm_esp);
		if (err < 0) {
			return;
		}

		msg_start(stm_esp, level, source, (uint32_t)package_hdr.raw, fmt);
		STM_D32(stm_esp, arg, false, false);
		packet_end(stm_esp);
		return;
	}

	uint32_t *len_loc;
	k_spinlock_key_t key;

	key = k_spin_lock(&lock);
	len_loc = early_msg_start(level, source, (uint32_t)package_hdr.raw, fmt);
	early_package_cb(&arg, sizeof(arg), NULL);
	early_msg_end(len_loc);
	k_spin_unlock(&lock, key);
}

void log_frontend_simple_2(const void *source, uint32_t level, const char *fmt, uint32_t arg0,
			   uint32_t arg1)
{
	static const union cbprintf_package_hdr package_hdr = {.desc = {.len = 2 + 2}};

	if ((EARLY_BUF_SIZE == 0) || etr_rdy) {
		STMESP_Type *stm_esp;
		int err;

		err = stmesp_get_port(get_channel(), &stm_esp);
		if (err < 0) {
			return;
		}

		msg_start(stm_esp, level, source, (uint32_t)package_hdr.raw, fmt);
		STM_D32(stm_esp, arg0, false, false);
		STM_D32(stm_esp, arg1, false, false);
		packet_end(stm_esp);
		return;
	}

	uint32_t *len_loc;
	k_spinlock_key_t key;

	key = k_spin_lock(&lock);
	len_loc = early_msg_start(level, source, (uint32_t)package_hdr.raw, fmt);
	early_package_cb(&arg0, sizeof(arg0), NULL);
	early_package_cb(&arg1, sizeof(arg1), NULL);
	early_msg_end(len_loc);
	k_spin_unlock(&lock, key);
}

#endif /* CONFIG_LOG_FRONTEND_STMESP_FSC */

void log_frontend_panic(void)
{
	in_panic = true;
#ifdef CONFIG_NRF_ETR
	nrf_etr_flush();
#endif
}

void log_frontend_init(void)
{
	/* empty */
}

void log_frontend_stmesp_dummy_write(void)
{
#define STMESP_DUMMY_WORD 0xaabbccdd

	STMESP_Type *stm_esp;

	(void)stmesp_get_port(CONFIG_LOG_FRONTEND_STMESP_FLUSH_PORT_ID, &stm_esp);
	STM_D32(stm_esp, STMESP_DUMMY_WORD, false, false);
}

void log_frontend_stmesp_pre_sleep(void)
{
	bool use_stm = etr_rdy || (EARLY_BUF_SIZE == 0);

	if (!use_stm || new_data == 0) {
		return;
	}

	for (uint32_t i = 0; i < CONFIG_LOG_FRONTEND_STMESP_FLUSH_COUNT; i++) {
		log_frontend_stmesp_dummy_write();
	}

	atomic_set(&new_data, 0);
}

#if EARLY_BUF_SIZE > 0
int log_frontend_stmesp_etr_ready(void)
{
	STMESP_Type *stm_esp;
	uint16_t len;
	uint32_t *buf = NULL;
	int err;

	err = stmesp_get_port(get_channel(), &stm_esp);
	if (err < 0) {
		return -EIO;
	}

	early_buf_read_mode();

	while ((len = early_buf_get_data((void **)&buf)) > 0) {
		/* Write first word with Marked and timestamp. */
		STM_D32(stm_esp, *buf, true, true);
		buf++;
		len -= sizeof(uint32_t);

		/* Write remaining data as raw data. */
		write_data(buf, len, stm_esp);

		/* Flag the end. */
		packet_end(stm_esp);
	}

	etr_rdy = true;

	return 0;
}
#endif /* EARLY_BUF_SIZE > 0 */
