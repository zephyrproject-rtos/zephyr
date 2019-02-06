/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RESOURCE_TABLE_H__
#define RESOURCE_TABLE_H__

#ifndef OPENAMP_PACKED_BEGIN
#define OPENAMP_PACKED_BEGIN
#endif

#ifndef OPENAMP_PACKED_END
#define OPENAMP_PACKED_END __attribute__((__packed__))
#endif

/* beginning of section: copied from OpenAMP */
/**
 * struct fw_rsc_trace - trace buffer declaration
 * @da: device address
 * @len: length (in bytes)
 * @reserved: reserved (must be zero)
 * @name: human-readable name of the trace buffer
 *
 * This resource entry provides the host information about a trace buffer
 * into which the remote remote_proc will write log messages.
 *
 * @da specifies the device address of the buffer, @len specifies
 * its size, and @name may contain a human readable name of the trace buffer.
 *
 * After booting the remote remote_proc, the trace buffers are exposed to the
 * user via debugfs entries (called trace0, trace1, etc..).
 */
OPENAMP_PACKED_BEGIN
struct fw_rsc_trace {
	u32_t type;
	u32_t da;
	u32_t len;
	u32_t reserved;
	u8_t name[32];
} OPENAMP_PACKED_END;

OPENAMP_PACKED_BEGIN

enum fw_resource_type {
	RSC_CARVEOUT = 0,
	RSC_DEVMEM = 1,
	RSC_TRACE = 2,
	RSC_VDEV = 3,
	RSC_RPROC_MEM = 4,
	RSC_FW_CHKSUM = 5,
	RSC_LAST = 6,
	RSC_VENDOR_START = 128,
	RSC_VENDOR_END = 512,
};

/* end of section: copied from OpenAMP */

struct stm32_resource_table {
	unsigned int ver;
	unsigned int num;
	unsigned int reserved[2];
	unsigned int offset[1];

	/* rpmsg trace entry */
	struct fw_rsc_trace cm_trace;
} OPENAMP_PACKED_END;

void resource_table_init(volatile void **table_ptr, int *length);

#endif

