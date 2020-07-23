/*
 * Copyright (c) 2018 Piotr Mienkowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Serial Wire Output (SWO) backend implementation.
 *
 * SWO/SWV has been developed by ARM. The following code works only on ARM
 * architecture.
 *
 * An SWO viewer program will typically set-up the SWO port including its
 * frequency when connected to the debug probe. Such configuration can persist
 * only until the MCU reset. The SWO backend initialization function will
 * re-configure the SWO port upon boot and set the frequency as specified by
 * the LOG_BACKEND_SWO_FREQ_HZ Kconfig option. To ensure flawless operation
 * this frequency should much the one set by the SWO viewer program.
 *
 * The initialization code assumes that SWO core frequency is equal to HCLK
 * as defined by the clock-frequency property in the CPU node. This may require
 * additional, vendor specific configuration.
 */

#include <logging/log_backend.h>
#include <logging/log_core.h>
#include <logging/log_msg.h>
#include <logging/log_output.h>
#include "log_backend_std.h"
#include <soc.h>

/** The stimulus port from which SWO data is received and displayed */
#define ITM_PORT_LOGGER       0

/* Set TPIU prescaler for the current debug trace clock frequency. */
#if CONFIG_LOG_BACKEND_SWO_FREQ_HZ == 0
#define SWO_FREQ_DIV  1
#else
#if DT_NODE_HAS_PROP(DT_PATH(cpus, cpu_0), clock_frequency)
#define CPU_FREQ DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency)
#else
#error "Missing DT 'clock-frequency' property on cpu@0 node"
#endif
#define SWO_FREQ (CPU_FREQ + (CONFIG_LOG_BACKEND_SWO_FREQ_HZ / 2))
#define SWO_FREQ_DIV  (SWO_FREQ / CONFIG_LOG_BACKEND_SWO_FREQ_HZ)
#if SWO_FREQ_DIV > 0xFFFF
#error CONFIG_LOG_BACKEND_SWO_FREQ_HZ is too low. SWO clock divider is 16-bit. \
	Minimum supported SWO clock frequency is \
	[CPU Clock Frequency]/2^16.
#endif
#endif

static uint8_t buf[1];

static int char_out(uint8_t *data, size_t length, void *ctx)
{
	ARG_UNUSED(ctx);

	for (size_t i = 0; i < length; i++) {
		ITM_SendChar(data[i]);
	}

	return length;
}

LOG_OUTPUT_DEFINE(log_output_swo, char_out, buf, sizeof(buf));

static void log_backend_swo_put(const struct log_backend *const backend,
		struct log_msg *msg)
{
	uint32_t flag = IS_ENABLED(CONFIG_LOG_BACKEND_SWO_SYST_ENABLE) ?
		LOG_OUTPUT_FLAG_FORMAT_SYST : 0;

	log_backend_std_put(&log_output_swo, flag, msg);
}

static void log_backend_swo_init(void)
{
	/* Enable DWT and ITM units */
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	/* Enable access to ITM registers */
	ITM->LAR  = 0xC5ACCE55;
	/* Disable stimulus ports ITM_STIM0-ITM_STIM31 */
	ITM->TER  = 0x0;
	/* Disable ITM */
	ITM->TCR  = 0x0;
	/* Select NRZ (UART) encoding protocol */
	TPI->SPPR = 2;
	/* Set SWO baud rate prescaler value: SWO_clk = ref_clock/(ACPR + 1) */
	TPI->ACPR = SWO_FREQ_DIV - 1;
	/* Enable unprivileged access to ITM stimulus ports */
	ITM->TPR  = 0x0;
	/* Configure Debug Watchpoint and Trace */
	DWT->CTRL = 0x400003FE;
	/* Configure Formatter and Flush Control Register */
	TPI->FFCR = 0x00000100;
	/* Enable ITM, set TraceBusID=1, no local timestamp generation */
	ITM->TCR  = 0x0001000D;
	/* Enable stimulus port used by the logger */
	ITM->TER  = 1 << ITM_PORT_LOGGER;
}

static void log_backend_swo_panic(struct log_backend const *const backend)
{
}

static void dropped(const struct log_backend *const backend, uint32_t cnt)
{
	ARG_UNUSED(backend);

	log_backend_std_dropped(&log_output_swo, cnt);
}

static void log_backend_swo_sync_string(const struct log_backend *const backend,
		struct log_msg_ids src_level, uint32_t timestamp,
		const char *fmt, va_list ap)
{
	uint32_t flag = IS_ENABLED(CONFIG_LOG_BACKEND_SWO_SYST_ENABLE) ?
		LOG_OUTPUT_FLAG_FORMAT_SYST : 0;

	log_backend_std_sync_string(&log_output_swo, flag, src_level,
				    timestamp, fmt, ap);
}

static void log_backend_swo_sync_hexdump(
		const struct log_backend *const backend,
		struct log_msg_ids src_level, uint32_t timestamp,
		const char *metadata, const uint8_t *data, uint32_t length)
{
	uint32_t flag = IS_ENABLED(CONFIG_LOG_BACKEND_SWO_SYST_ENABLE) ?
		LOG_OUTPUT_FLAG_FORMAT_SYST : 0;

	log_backend_std_sync_hexdump(&log_output_swo, flag, src_level,
				     timestamp, metadata, data, length);
}

const struct log_backend_api log_backend_swo_api = {
	.put = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? NULL : log_backend_swo_put,
	.put_sync_string = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ?
			log_backend_swo_sync_string : NULL,
	.put_sync_hexdump = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ?
			log_backend_swo_sync_hexdump : NULL,
	.panic = log_backend_swo_panic,
	.init = log_backend_swo_init,
	.dropped = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? NULL : dropped,
};

LOG_BACKEND_DEFINE(log_backend_swo, log_backend_swo_api, true);
