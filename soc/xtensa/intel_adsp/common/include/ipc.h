/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INCLUDE_PLATFORM_IPC_H__
#define __INCLUDE_PLATFORM_IPC_H__

/** Shift-left bits to extract the global cmd type */
#define ADSP_GLB_TYPE_SHIFT			28
#define ADSP_GLB_TYPE_MASK			(0xf << ADSP_GLB_TYPE_SHIFT)
#define ADSP_GLB_TYPE(x)			((x) << ADSP_GLB_TYPE_SHIFT)

/** Shift-left bits to extract the command type */
#define ADSP_CMD_TYPE_SHIFT			16
#define ADSP_CMD_TYPE_MASK			(0xfff << ADSP_CMD_TYPE_SHIFT)
#define ADSP_CMD_TYPE(x)			((x) << ADSP_CMD_TYPE_SHIFT)

#define ADSP_IPC_FW_READY			ADSP_GLB_TYPE(0x7U)

/* extended data types that can be appended onto end of adsp_ipc_fw_ready */
enum adsp_ipc_ext_data {
	ADSP_IPC_EXT_DMA_BUFFER = 0,
	ADSP_IPC_EXT_WINDOW,
};

enum adsp_ipc_region {
	ADSP_IPC_REGION_DOWNBOX  = 0,
	ADSP_IPC_REGION_UPBOX,
	ADSP_IPC_REGION_TRACE,
	ADSP_IPC_REGION_DEBUG,
	ADSP_IPC_REGION_STREAM,
	ADSP_IPC_REGION_REGS,
	ADSP_IPC_REGION_EXCEPTION,
};

/**
 * Structure Header - Header for all IPC structures except command structs.
 * The size can be greater than the structure size and that means there is
 * extended bespoke data beyond the end of the structure including variable
 * arrays.
 */
struct adsp_ipc_hdr {
	uint32_t size;			/**< size of structure */
} __packed;

/**
 * Command Header - Header for all IPC commands. Identifies IPC message.
 * The size can be greater than the structure size and that means there is
 * extended bespoke data beyond the end of the structure including variable
 * arrays.
 */
struct adsp_ipc_cmd_hdr {
	uint32_t size;			/**< size of structure */
	uint32_t cmd;			/**< command */
} __packed;

/* FW version */
struct adsp_ipc_fw_version {
	struct adsp_ipc_hdr hdr;
	uint16_t major;
	uint16_t minor;
	uint16_t micro;
	uint16_t build;
	uint8_t date[12];
	uint8_t time[10];
	uint8_t tag[6];
	uint32_t abi_version;

	/* reserved for future use */
	uint32_t reserved[4];
} __packed;

/* FW ready Message - sent by firmware when boot has completed */
struct adsp_ipc_fw_ready {
	struct adsp_ipc_cmd_hdr hdr;
	uint32_t dspbox_offset;		/* dsp initiated IPC mailbox */
	uint32_t hostbox_offset;	/* host initiated IPC mailbox */
	uint32_t dspbox_size;
	uint32_t hostbox_size;
	struct adsp_ipc_fw_version version;

	/* Miscellaneous flags */
	uint64_t flags;

	/* reserved for future use */
	uint32_t reserved[4];
} __packed;

struct adsp_ipc_ext_data_hdr {
	struct adsp_ipc_cmd_hdr hdr;
	uint32_t type;			/* ADSP_IPC_EXT_ */
} __packed;

struct adsp_ipc_window_elem {
	struct adsp_ipc_hdr hdr;
	uint32_t type;			/* ADSP_IPC_REGION_ */
	uint32_t id;			/* platform specific */
	uint32_t flags;			/**< R, W, RW, etc */
	uint32_t size;			/* size of region in bytes */

	/* offset in window region as windows can be partitioned */
	uint32_t offset;
} __packed;

/* extended data memory windows for IPC, trace and debug */
struct adsp_ipc_window {
	struct adsp_ipc_ext_data_hdr ext_hdr;
	uint32_t num_windows;
	struct adsp_ipc_window_elem window[];
} __packed;

#endif /* __INCLUDE_PLATFORM_IPC_H__ */
