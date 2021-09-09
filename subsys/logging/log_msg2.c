/*
 * Copyright (c) 2020 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <syscall_handler.h>
#include <logging/log_msg2.h>
#include <logging/log_core.h>
#include <logging/log_ctrl.h>

void z_log_msg2_finalize(struct log_msg2 *msg, const void *source,
			 const struct log_msg2_desc desc, const void *data)
{
	if (!msg) {
		z_log_dropped();

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
	uint32_t msg_wlen = log_msg2_get_total_wlen(desc);
	struct log_msg2 *msg = z_log_msg2_alloc(msg_wlen);

	if (msg) {
		memcpy(msg->data, package, desc.package_len);
	}

	z_log_msg2_finalize(msg, source, desc, data);
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
				const char *fmt, va_list ap)
{
	int plen;

	if (fmt) {
		va_list ap2;

		va_copy(ap2, ap);
		plen = cbvprintf_package(NULL, Z_LOG_MSG2_ALIGN_OFFSET, 0,
					 fmt, ap2);
		__ASSERT_NO_MSG(plen >= 0);
		va_end(ap2);
	} else {
		plen = 0;
	}

	size_t msg_wlen = Z_LOG_MSG2_ALIGNED_WLEN(plen, dlen);
	struct log_msg2 *msg;
	struct log_msg2_desc desc =
		Z_LOG_MSG_DESC_INITIALIZER(domain_id, level, plen, dlen);

	if (IS_ENABLED(CONFIG_LOG2_MODE_IMMEDIATE)) {
		msg = alloca(msg_wlen * sizeof(int));
	} else {
		msg = z_log_msg2_alloc(msg_wlen);
	}

	if (msg && fmt) {
		plen = cbvprintf_package(msg->data, plen, 0, fmt, ap);
		__ASSERT_NO_MSG(plen >= 0);
	}

	z_log_msg2_finalize(msg, source, desc, data);
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_z_log_msg2_runtime_vcreate(uint8_t domain_id,
				const void *source,
				uint8_t level, const void *data, size_t dlen,
				const char *fmt, va_list ap)
{
	return z_impl_z_log_msg2_runtime_vcreate(domain_id, source, level, data,
						dlen, fmt, ap);
}
#include <syscalls/z_log_msg2_runtime_vcreate_mrsh.c>
#endif
