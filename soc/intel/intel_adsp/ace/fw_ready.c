/*
 * Copyright (c) 2025 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/toolchain.h>
#include <zephyr/arch/xtensa/cache.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/cache.h>

#include <intel_adsp_ipc_devtree.h>

#include <adsp_ipc_regs.h>
#include <adsp_memory.h>

struct ipc_hdr {
	uint32_t size;				/**< size of structure */
} __packed __aligned(4);

struct ipc_cmd_hdr {
	uint32_t size;				/**< size of structure */
	uint32_t cmd;				/**< SOF_IPC_GLB_ + cmd */
} __packed __aligned(4);

struct ipc_fw_version {
	struct ipc_hdr hdr;
	uint16_t major;
	uint16_t minor;
	uint16_t micro;
	uint16_t build;
	uint8_t date[12];
	uint8_t time[10];
	uint8_t tag[6];
	uint32_t abi_version;
	uint32_t src_hash;
	uint32_t reserved[3];
} __packed __aligned(4);

struct ipc_fw_ready {
	struct ipc_cmd_hdr hdr;
	uint32_t dspbox_offset;
	uint32_t hostbox_offset;
	uint32_t dspbox_size;
	uint32_t hostbox_size;
	struct ipc_fw_version version;
	uint64_t flags;
	uint32_t reserved[4];
} __packed __aligned(4);

static const struct ipc_fw_ready ready
	Z_GENERIC_DOT_SECTION(fw_ready) __used = {
	.hdr = {
		.cmd = (0x7U << 28),		/* SOF_IPC_FW_READY */
		.size = sizeof(struct ipc_fw_ready),
	},

	.version = {
		.hdr.size = sizeof(struct ipc_fw_version),
		.micro = 0,
		.minor = 0,
		.major = 0,

		.build = 0xBEEF,
		.date = __DATE__,
		.time = __TIME__,

		.tag = "zephyr",
		.abi_version = 0,
		.src_hash = 0,
	},

	.flags = 0,
};

#define IPC4_NOTIFY_FW_READY			8

#define IPC4_GLB_NOTIFICATION			27

#define IPC4_GLB_NOTIFY_DIR_MASK		BIT(29)
#define IPC4_REPLY_STATUS_MASK			0xFFFFFF
#define IPC4_GLB_NOTIFY_TYPE_SHIFT		16
#define IPC4_GLB_NOTIFY_MSG_TYPE_SHIFT		24

#define IPC4_FW_READY \
	(((IPC4_NOTIFY_FW_READY) << (IPC4_GLB_NOTIFY_TYPE_SHIFT)) |\
	 ((IPC4_GLB_NOTIFICATION) << (IPC4_GLB_NOTIFY_MSG_TYPE_SHIFT)))

#define SRAM_SW_REG_BASE			(L2_SRAM_BASE + 0x4000)
#define SRAM_SW_REG_SIZE			0x1000

#define SRAM_OUTBOX_BASE			(SRAM_SW_REG_BASE + SRAM_SW_REG_SIZE)
#define SRAM_OUTBOX_SIZE			0x1000

int notify_host_boot_complete(void)
{
	memcpy((void *)SRAM_OUTBOX_BASE, &ready, sizeof(ready));

	sys_cache_data_flush_range((void *)SRAM_OUTBOX_BASE, SRAM_OUTBOX_SIZE);

	intel_adsp_ipc_send_message(INTEL_ADSP_IPC_HOST_DEV,
				    IPC4_FW_READY, 0);

	return 0;
}

SYS_INIT(notify_host_boot_complete, POST_KERNEL, 0);
