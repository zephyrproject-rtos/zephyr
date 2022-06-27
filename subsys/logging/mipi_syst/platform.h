/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MIPI_SYST_PLATFORM_INCLUDED
#define MIPI_SYST_PLATFORM_INCLUDED

#include <zephyr/logging/log_output.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_MIPI_SYST_STP)
/*
 * Structure generating STP protocol data
 */
struct stp_writer_data {
	mipi_syst_u8   byteDone;
	mipi_syst_u8   current;
	mipi_syst_u16  master;
	mipi_syst_u16  channel;
	mipi_syst_u64  recordCount;
	mipi_syst_u64  timestamp;
};
#endif

#if defined(MIPI_SYST_PCFG_ENABLE_PLATFORM_STATE_DATA)
/*
 * Platform specific SyS-T global state extension
 */
struct mipi_syst_platform_state {
#if defined(CONFIG_MIPI_SYST_STP)
	struct stp_writer_data *stpWriter;
#endif

	void (*write_d8)(struct mipi_syst_handle *systh, mipi_syst_u8 v);
	void (*write_d16)(struct mipi_syst_handle *systh, mipi_syst_u16 v);
	void (*write_d32)(struct mipi_syst_handle *systh, mipi_syst_u32 v);
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_IO)
	void (*write_d64)(struct mipi_syst_handle *systh, mipi_syst_u64 v);
#endif
	void (*write_d32ts)(struct mipi_syst_handle *systh, mipi_syst_u32 v);
	void (*write_d32mts)(struct mipi_syst_handle *systh, mipi_syst_u32 v);
	void (*write_d64mts)(struct mipi_syst_handle *systh, mipi_syst_u64 v);
	void (*write_flag)(struct mipi_syst_handle *systh);
};

/*
 * Platform specific SyS-T handle state extension
 */
struct mipi_syst_platform_handle {
	mipi_syst_u32 flag;
#if defined(CONFIG_MIPI_SYST_STP)
	mipi_syst_u32 master;
	mipi_syst_u32 channel;
#endif
	struct log_output *log_output;
};

/*
 * IO output routine mapping
 * Call the function pointers in the global state
 */
#define MIPI_SYST_OUTPUT_D8(syst_handle, data) \
	((syst_handle)->systh_header-> \
		systh_platform.write_d8((syst_handle), (data)))
#define MIPI_SYST_OUTPUT_D16(syst_handle, data) \
	((syst_handle)->systh_header-> \
		systh_platform.write_d16((syst_handle), (data)))
#define MIPI_SYST_OUTPUT_D32(syst_handle, data) \
	((syst_handle)->systh_header-> \
		systh_platform.write_d32((syst_handle), (data)))
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_IO)
#define MIPI_SYST_OUTPUT_D64(syst_handle, data) \
	((syst_handle)->systh_header-> \
		systh_platform.write_d64((syst_handle), (data)))
#endif
#define MIPI_SYST_OUTPUT_D32TS(syst_handle, data) \
	((syst_handle)->systh_header-> \
		systh_platform.write_d32ts((syst_handle), (data)))
#define MIPI_SYST_OUTPUT_D32MTS(syst_handle, data) \
	((syst_handle)->systh_header-> \
		systh_platform.write_d32mts((syst_handle), (data)))
#define MIPI_SYST_OUTPUT_D64MTS(syst_handle, data) \
	((syst_handle)->systh_header-> \
		systh_platform.write_d64mts((syst_handle), (data)))
#define MIPI_SYST_OUTPUT_FLAG(syst_handle) \
	((syst_handle)->systh_header-> \
		systh_platform.write_flag((syst_handle)))

#else

#define MIPI_SYST_OUTPUT_D8(syst_handle, data)
#define MIPI_SYST_OUTPUT_D16(syst_handle, data)
#define MIPI_SYST_OUTPUT_D32(syst_handle, data)
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_IO)
#define MIPI_SYST_OUTPUT_D64(syst_handle, data)
#endif
#define MIPI_SYST_OUTPUT_D32TS(syst_handle, data)
#define MIPI_SYST_OUTPUT_FLAG(syst_handle)

#endif

#if defined(MIPI_SYST_PCFG_ENABLE_HEAP_MEMORY)
#define MIPI_SYST_HEAP_MALLOC(s)
#define MIPI_SYST_HEAP_FREE(p)
#endif

#if defined(MIPI_SYST_PCFG_ENABLE_TIMESTAMP)
#define MIPI_SYST_PLATFORM_CLOCK() mipi_syst_get_epoch()
#define MIPI_SYST_PLATFORM_FREQ()  CONFIG_SYS_CLOCK_TICKS_PER_SEC

mipi_syst_u64 mipi_syst_get_epoch(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
