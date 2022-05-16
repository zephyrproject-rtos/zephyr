/*
 * Copyright (c) 2020 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/zephyr.h>
#include <zephyr/syscall_handler.h>
#include <zephyr/logging/log_internal.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_frontend.h>
#include <zephyr/logging/log_backend.h>

/* Returns true if any backend is in use. */
#define BACKENDS_IN_USE() \
	!(IS_ENABLED(CONFIG_LOG_FRONTEND) && \
	 (IS_ENABLED(CONFIG_LOG_FRONTEND_ONLY) || log_backend_count_get() == 0))

void z_log_msg2_finalize(struct log_msg2 *msg, const void *source,
			 const struct log_msg2_desc desc, const void *data)
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
	z_log_msg2_commit(msg);
}

void z_impl_z_log_msg2_static_create(const void *source,
			      const struct log_msg2_desc desc,
			      uint8_t *package, const void *data)
{
	if (IS_ENABLED(CONFIG_LOG_FRONTEND)) {
		log_frontend_msg(source, desc, package, data);
	}

	if (!BACKENDS_IN_USE()) {
		return;
	}

	struct log_msg2_desc out_desc = desc;
	int inlen = desc.package_len;
	struct log_msg2 *msg;

	if (inlen > 0) {
		uint32_t flags = CBPRINTF_PACKAGE_COPY_RW_STR;
		uint16_t strl[4];
		int len;

		len = cbprintf_package_copy(package, inlen,
					    NULL, 0, flags,
					    strl, ARRAY_SIZE(strl));

		/* Update package length with calculated value (which may be extended
		 * when strings are copied into the package.
		 */
		out_desc.package_len = len;
		msg = z_log_msg2_alloc(log_msg2_get_total_wlen(out_desc));
		if (msg) {
			len = cbprintf_package_copy(package, inlen,
						    msg->data, out_desc.package_len,
						    flags, strl, ARRAY_SIZE(strl));
			__ASSERT_NO_MSG(len >= 0);
		}
	} else {
		msg = z_log_msg2_alloc(log_msg2_get_total_wlen(out_desc));
	}

	z_log_msg2_finalize(msg, source, out_desc, data);
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_z_log_msg2_static_create(const void *source,
			      const struct log_msg2_desc desc,
			      uint8_t *package, const void *data)
{
	return z_impl_z_log_msg2_static_create(source, desc, package, data);
}
#include <syscalls/z_log_msg2_static_create_mrsh.c>
#endif

void z_impl_z_log_msg2_runtime_vcreate(uint8_t domain_id, const void *source,
				uint8_t level, const void *data, size_t dlen,
				uint32_t package_flags, const char *fmt, va_list ap)
{
	int plen;

	if (fmt) {
		va_list ap2;

		va_copy(ap2, ap);
		plen = cbvprintf_package(NULL, Z_LOG_MSG2_ALIGN_OFFSET,
					 package_flags, fmt, ap2);
		__ASSERT_NO_MSG(plen >= 0);
		va_end(ap2);
	} else {
		plen = 0;
	}

	size_t msg_wlen = Z_LOG_MSG2_ALIGNED_WLEN(plen, dlen);
	struct log_msg2 *msg;
	uint8_t *pkg;
	struct log_msg2_desc desc =
		Z_LOG_MSG_DESC_INITIALIZER(domain_id, level, plen, dlen);

	if (IS_ENABLED(CONFIG_LOG_MODE_DEFERRED) && BACKENDS_IN_USE()) {
		msg = z_log_msg2_alloc(msg_wlen);
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

	if (IS_ENABLED(CONFIG_LOG_FRONTEND)) {
		log_frontend_msg(source, desc, pkg, data);
	}

	if (BACKENDS_IN_USE()) {
		z_log_msg2_finalize(msg, source, desc, data);
	}
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_z_log_msg2_runtime_vcreate(uint8_t domain_id,
				const void *source,
				uint8_t level, const void *data, size_t dlen,
				uint32_t package_flags, const char *fmt, va_list ap)
{
	return z_impl_z_log_msg2_runtime_vcreate(domain_id, source, level, data,
						dlen, package_flags, fmt, ap);
}
#include <syscalls/z_log_msg2_runtime_vcreate_mrsh.c>
#endif
