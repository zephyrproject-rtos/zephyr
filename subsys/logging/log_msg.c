/*
 * Copyright (c) 2020 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/logging/log_internal.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_frontend.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log.h>
#include <zephyr/llext/symbol.h>
LOG_MODULE_DECLARE(log);

BUILD_ASSERT(sizeof(struct log_msg_desc) == sizeof(uint32_t),
	     "Descriptor must fit in 32 bits");

/* Returns true if any backend is in use. */
#define BACKENDS_IN_USE() \
	!(IS_ENABLED(CONFIG_LOG_FRONTEND) && \
	 (IS_ENABLED(CONFIG_LOG_FRONTEND_ONLY) || log_backend_count_get() == 0))

#define CBPRINTF_DESC_SIZE32 (sizeof(struct cbprintf_package_desc) / sizeof(uint32_t))

/* For simplified message handling cprintf package must have only 1 word. */
BUILD_ASSERT(!IS_ENABLED(CONFIG_LOG_SIMPLE_MSG_OPTIMIZE) ||
	     (IS_ENABLED(CONFIG_LOG_SIMPLE_MSG_OPTIMIZE) && (CBPRINTF_DESC_SIZE32 == 1)));


void z_log_msg_finalize(struct log_msg *msg, const void *source,
			 const struct log_msg_desc desc, const void *data)
{
	if (!msg) {
		z_log_dropped(false);

		return;
	}

	if (data) {
		uint8_t *d = msg->data + desc.package_len;

		memcpy(d, data, desc.data_len);
	}

	msg->hdr.desc = desc;
	msg->hdr.source = source;
#if CONFIG_LOG_THREAD_ID_PREFIX
	msg->hdr.tid = k_is_in_isr() ? NULL : k_current_get();
#endif
	z_log_msg_commit(msg);
}

static bool frontend_runtime_filtering(const void *source, uint32_t level)
{
	if (!IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING)) {
		return true;
	}

	/* If only frontend is used and log got here it means that it was accepted. */
	if (IS_ENABLED(CONFIG_LOG_FRONTEND_ONLY)) {
		return true;
	}

	if (level == LOG_LEVEL_NONE) {
		return true;
	}

	struct log_source_dynamic_data *dynamic = (struct log_source_dynamic_data *)source;
	uint32_t f_level = LOG_FILTER_SLOT_GET(&dynamic->filters, LOG_FRONTEND_SLOT_ID);

	return level <= f_level;
}

/** @brief Create a log message using simplified method.
 *
 * Simple log message has 0-2 32 bit word arguments so creating cbprintf package
 * is straightforward as there is no padding or alignment to concern about.
 * This function takes input data which is fmt pointer + 0-2 arguments, creates
 * package header which is very simple as it only contain non-zero length field.
 * Then space is allocated and message is committed. Such simple approach can
 * be applied because it is known that input string does not have any arguments
 * which complicate things (string pointers, floating numbers). Simple method is
 * also limited to 32 bit arch.
 *
 * @param source Source.
 * @param level  Severity level.
 * @param data   Package content (without header).
 * @param len    Package content length in words.
 */
static void z_log_msg_simple_create(const void *source, uint32_t level, uint32_t *data, size_t len)
{
	/* Package length (in words) is increased by the header. */
	size_t plen32 = len + CBPRINTF_DESC_SIZE32;
	/* Package length in bytes. */
	size_t plen8 = sizeof(uint32_t) * plen32 +
			(IS_ENABLED(CONFIG_LOG_MSG_APPEND_RO_STRING_LOC) ? 1 : 0);
	struct log_msg *msg = z_log_msg_alloc(Z_LOG_MSG_ALIGNED_WLEN(plen8, 0));
	union cbprintf_package_hdr package_hdr = {
		.desc = {
			.len = plen32,
			.ro_str_cnt = IS_ENABLED(CONFIG_LOG_MSG_APPEND_RO_STRING_LOC) ? 1 : 0
		}
	};

	if (msg) {
		uint32_t *package = (uint32_t *)msg->data;

		*package++ = (uint32_t)(uintptr_t)package_hdr.raw;
		for (size_t i = 0; i < len; i++) {
			*package++ = data[i];
		}
		if (IS_ENABLED(CONFIG_LOG_MSG_APPEND_RO_STRING_LOC)) {
			/* fmt string located at index 1 */
			*(uint8_t *)package = 1;
		}
	}

	struct log_msg_desc desc = {
		.level = level,
		.package_len = plen8,
		.data_len = 0,
	};

	z_log_msg_finalize(msg, source, desc, NULL);
}

void z_impl_z_log_msg_simple_create_0(const void *source, uint32_t level, const char *fmt)
{

	if (IS_ENABLED(CONFIG_LOG_FRONTEND) && frontend_runtime_filtering(source, level)) {
		if (IS_ENABLED(CONFIG_LOG_FRONTEND_OPT_API)) {
			log_frontend_simple_0(source, level, fmt);
		} else {
			/* If frontend does not support optimized API prepare data for
			 * the generic call.
			 */
			uint32_t plen32 = CBPRINTF_DESC_SIZE32 + 1;
			union cbprintf_package_hdr hdr = {
				.desc = {
					.len = plen32,
					.ro_str_cnt =
					   IS_ENABLED(CONFIG_LOG_MSG_APPEND_RO_STRING_LOC) ? 1 : 0
				}
			};
			uint8_t package[sizeof(uint32_t) * (CBPRINTF_DESC_SIZE32 + 1) +
				(IS_ENABLED(CONFIG_LOG_MSG_APPEND_RO_STRING_LOC) ? 1 : 0)]
				__aligned(sizeof(uint32_t));
			uint32_t *p32 = (uint32_t *)package;

			*p32++ = (uint32_t)(uintptr_t)hdr.raw;
			*p32++ = (uint32_t)(uintptr_t)fmt;
			if (IS_ENABLED(CONFIG_LOG_MSG_APPEND_RO_STRING_LOC)) {
				/* fmt string located at index 1 */
				*(uint8_t *)p32 = 1;
			}

			struct log_msg_desc desc = {
				.level = level,
				.package_len = sizeof(package),
				.data_len = 0,
			};

			log_frontend_msg(source, desc, package, NULL);
		}
	}

	if (!BACKENDS_IN_USE()) {
		return;
	}

	uint32_t data[] = {(uint32_t)(uintptr_t)fmt};

	z_log_msg_simple_create(source, level, data, ARRAY_SIZE(data));
}

void z_impl_z_log_msg_simple_create_1(const void *source, uint32_t level,
				      const char *fmt, uint32_t arg)
{
	if (IS_ENABLED(CONFIG_LOG_FRONTEND) && frontend_runtime_filtering(source, level)) {
		if (IS_ENABLED(CONFIG_LOG_FRONTEND_OPT_API)) {
			log_frontend_simple_1(source, level, fmt, arg);
		} else {
			/* If frontend does not support optimized API prepare data for
			 * the generic call.
			 */
			uint32_t plen32 = CBPRINTF_DESC_SIZE32 + 2;
			union cbprintf_package_hdr hdr = {
				.desc = {
					.len = plen32,
					.ro_str_cnt =
					   IS_ENABLED(CONFIG_LOG_MSG_APPEND_RO_STRING_LOC) ? 1 : 0
				}
			};
			uint8_t package[sizeof(uint32_t) * (CBPRINTF_DESC_SIZE32 + 2) +
				(IS_ENABLED(CONFIG_LOG_MSG_APPEND_RO_STRING_LOC) ? 1 : 0)]
				__aligned(sizeof(uint32_t));
			uint32_t *p32 = (uint32_t *)package;

			*p32++ = (uint32_t)(uintptr_t)hdr.raw;
			*p32++ = (uint32_t)(uintptr_t)fmt;
			*p32++ = arg;
			if (IS_ENABLED(CONFIG_LOG_MSG_APPEND_RO_STRING_LOC)) {
				/* fmt string located at index 1 */
				*(uint8_t *)p32 = 1;
			}

			struct log_msg_desc desc = {
				.level = level,
				.package_len = sizeof(package),
				.data_len = 0,
			};

			log_frontend_msg(source, desc, package, NULL);
		}
	}

	if (!BACKENDS_IN_USE()) {
		return;
	}

	uint32_t data[] = {(uint32_t)(uintptr_t)fmt, arg};

	z_log_msg_simple_create(source, level, data, ARRAY_SIZE(data));
}

void z_impl_z_log_msg_simple_create_2(const void *source, uint32_t level,
				      const char *fmt, uint32_t arg0, uint32_t arg1)
{
	if (IS_ENABLED(CONFIG_LOG_FRONTEND) && frontend_runtime_filtering(source, level)) {
		if (IS_ENABLED(CONFIG_LOG_FRONTEND_OPT_API)) {
			log_frontend_simple_2(source, level, fmt, arg0, arg1);
		} else {
			/* If frontend does not support optimized API prepare data for
			 * the generic call.
			 */
			uint32_t plen32 = CBPRINTF_DESC_SIZE32 + 3;
			union cbprintf_package_hdr hdr = {
				.desc = {
					.len = plen32,
					.ro_str_cnt =
					   IS_ENABLED(CONFIG_LOG_MSG_APPEND_RO_STRING_LOC) ? 1 : 0
				}
			};
			uint8_t package[sizeof(uint32_t) * (CBPRINTF_DESC_SIZE32 + 3) +
				(IS_ENABLED(CONFIG_LOG_MSG_APPEND_RO_STRING_LOC) ? 1 : 0)]
				__aligned(sizeof(uint32_t));
			uint32_t *p32 = (uint32_t *)package;

			*p32++ = (uint32_t)(uintptr_t)hdr.raw;
			*p32++ = (uint32_t)(uintptr_t)fmt;
			*p32++ = arg0;
			*p32++ = arg1;
			if (IS_ENABLED(CONFIG_LOG_MSG_APPEND_RO_STRING_LOC)) {
				/* fmt string located at index 1 */
				*(uint8_t *)p32 = 1;
			}

			struct log_msg_desc desc = {
				.level = level,
				.package_len = sizeof(package),
				.data_len = 0,
			};

			log_frontend_msg(source, desc, package, NULL);
		}
	}

	if (!BACKENDS_IN_USE()) {
		return;
	}

	uint32_t data[] = {(uint32_t)(uintptr_t)fmt, arg0, arg1};

	z_log_msg_simple_create(source, level, data, ARRAY_SIZE(data));
}

void z_impl_z_log_msg_static_create(const void *source,
			      const struct log_msg_desc desc,
			      uint8_t *package, const void *data)
{
	if (IS_ENABLED(CONFIG_LOG_FRONTEND) && frontend_runtime_filtering(source, desc.level)) {
		log_frontend_msg(source, desc, package, data);
	}

	if (!BACKENDS_IN_USE()) {
		return;
	}

	struct log_msg_desc out_desc = desc;
	int inlen = desc.package_len;
	struct log_msg *msg;

	if (inlen > 0) {
		uint32_t flags = CBPRINTF_PACKAGE_CONVERT_RW_STR |
				 (IS_ENABLED(CONFIG_LOG_MSG_APPEND_RO_STRING_LOC) ?
				 CBPRINTF_PACKAGE_CONVERT_KEEP_RO_STR : 0) |
				 (IS_ENABLED(CONFIG_LOG_FMT_SECTION_STRIP) ?
				 0 : CBPRINTF_PACKAGE_CONVERT_PTR_CHECK);
		uint16_t strl[4];
		int len;

		len = cbprintf_package_copy(package, inlen,
					    NULL, 0, flags,
					    strl, ARRAY_SIZE(strl));

		if (len > Z_LOG_MSG_MAX_PACKAGE) {
			struct cbprintf_package_hdr_ext *pkg =
				(struct cbprintf_package_hdr_ext *)package;

			LOG_WRN("Message (\"%s\") dropped because it exceeds size limitation (%u)",
				pkg->fmt, (uint32_t)Z_LOG_MSG_MAX_PACKAGE);
			return;
		}
		/* Update package length with calculated value (which may be extended
		 * when strings are copied into the package.
		 */
		out_desc.package_len = len;
		msg = z_log_msg_alloc(log_msg_get_total_wlen(out_desc));
		if (msg) {
			len = cbprintf_package_copy(package, inlen,
						    msg->data, out_desc.package_len,
						    flags, strl, ARRAY_SIZE(strl));
			__ASSERT_NO_MSG(len >= 0);
		}
	} else {
		msg = z_log_msg_alloc(log_msg_get_total_wlen(out_desc));
	}

	z_log_msg_finalize(msg, source, out_desc, data);
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_z_log_msg_static_create(const void *source,
			      const struct log_msg_desc desc,
			      uint8_t *package, const void *data)
{
	return z_impl_z_log_msg_static_create(source, desc, package, data);
}
#include <syscalls/z_log_msg_static_create_mrsh.c>
#endif

void z_log_msg_runtime_vcreate(uint8_t domain_id, const void *source,
				uint8_t level, const void *data, size_t dlen,
				uint32_t package_flags, const char *fmt, va_list ap)
{
	int plen;

	if (fmt) {
		va_list ap2;

		va_copy(ap2, ap);
		plen = cbvprintf_package(NULL, Z_LOG_MSG_ALIGN_OFFSET,
					 package_flags, fmt, ap2);
		__ASSERT_NO_MSG(plen >= 0);
		va_end(ap2);
	} else {
		plen = 0;
	}

	size_t msg_wlen = Z_LOG_MSG_ALIGNED_WLEN(plen, dlen);
	struct log_msg *msg;
	uint8_t *pkg;
	struct log_msg_desc desc =
		Z_LOG_MSG_DESC_INITIALIZER(domain_id, level, plen, dlen);

	if (IS_ENABLED(CONFIG_LOG_MODE_DEFERRED) && BACKENDS_IN_USE()) {
		msg = z_log_msg_alloc(msg_wlen);
		if (IS_ENABLED(CONFIG_LOG_FRONTEND) && msg == NULL) {
			pkg = alloca(plen);
		} else {
			pkg = msg ? msg->data : NULL;
		}
	} else {
		msg = alloca(msg_wlen * sizeof(int));
		pkg = msg->data;
	}

	if (pkg && fmt) {
		plen = cbvprintf_package(pkg, (size_t)plen, package_flags, fmt, ap);
		__ASSERT_NO_MSG(plen >= 0);
	}

	if (IS_ENABLED(CONFIG_LOG_FRONTEND) && frontend_runtime_filtering(source, desc.level)) {
		log_frontend_msg(source, desc, pkg, data);
	}

	if (BACKENDS_IN_USE()) {
		z_log_msg_finalize(msg, source, desc, data);
	}
}
