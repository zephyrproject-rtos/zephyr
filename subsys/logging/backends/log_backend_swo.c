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

#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/logging/log_output.h>
#include <zephyr/logging/log_backend_std.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>

/** The stimulus port from which SWO data is received and displayed */
#define ITM_PORT_LOGGER       0

/* If ITM has pin control properties, apply them for SWO pins */
#if DT_NODE_HAS_PROP(DT_NODELABEL(itm), pinctrl_0)
PINCTRL_DT_DEFINE(DT_NODELABEL(itm));
#endif

/* Set TPIU prescaler for the current debug trace clock frequency. */
#if CONFIG_LOG_BACKEND_SWO_FREQ_HZ == 0
#define SWO_FREQ_DIV  1
#else

#if CONFIG_LOG_BACKEND_SWO_REF_FREQ_HZ == 0
#error "SWO reference frequency is not configured"
#endif

#define SWO_FREQ_DIV \
	((CONFIG_LOG_BACKEND_SWO_REF_FREQ_HZ + (CONFIG_LOG_BACKEND_SWO_FREQ_HZ / 2)) / \
		CONFIG_LOG_BACKEND_SWO_FREQ_HZ)

#if SWO_FREQ_DIV > 0xFFFF
#error CONFIG_LOG_BACKEND_SWO_FREQ_HZ is too low. SWO clock divider is 16-bit. \
	Minimum supported SWO clock frequency is \
	[Reference Clock Frequency]/2^16.
#endif

#endif

static uint8_t buf[1];
static uint32_t log_format_current = CONFIG_LOG_BACKEND_SWO_OUTPUT_DEFAULT;

static int char_out(uint8_t *data, size_t length, void *ctx)
{
	ARG_UNUSED(ctx);

	for (size_t i = 0; i < length; i++) {
		ITM_SendChar(data[i]);
	}

	return length;
}

LOG_OUTPUT_DEFINE(log_output_swo, char_out, buf, sizeof(buf));

static void log_backend_swo_process(const struct log_backend *const backend,
				    union log_msg_generic *msg)
{
	uint32_t flags = log_backend_std_get_flags();

	log_format_func_t log_output_func = log_format_func_t_get(log_format_current);

	log_output_func(&log_output_swo, &msg->log, flags);
}

static int format_set(const struct log_backend *const backend, uint32_t log_type)
{
	log_format_current = log_type;
	return 0;
}

static void log_backend_swo_init(struct log_backend const *const backend)
{
	/* Enable DWT and ITM units */
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	/* Enable access to ITM registers */
	ITM->LAR  = 0xC5ACCE55;
	/* Disable stimulus ports ITM_STIM0-ITM_STIM31 */
	ITM->TER  = 0x0;
	/* Disable ITM */
	ITM->TCR  = 0x0;
	/* Select TPIU encoding protocol */
	TPI->SPPR = IS_ENABLED(CONFIG_LOG_BACKEND_SWO_PROTOCOL_NRZ) ? 2 : 1;
	/* Set SWO baud rate prescaler value: SWO_clk = ref_clock/(ACPR + 1) */
	TPI->ACPR = SWO_FREQ_DIV - 1;
	/* Enable unprivileged access to ITM stimulus ports */
	ITM->TPR  = 0x0;
	/* Configure Debug Watchpoint and Trace */
	DWT->CTRL &= (DWT_CTRL_POSTPRESET_Msk | DWT_CTRL_POSTINIT_Msk | DWT_CTRL_CYCCNTENA_Msk);
	DWT->CTRL |= (DWT_CTRL_POSTPRESET_Msk | DWT_CTRL_POSTINIT_Msk);
	/* Configure Formatter and Flush Control Register */
	TPI->FFCR = 0x00000100;
	/* Enable ITM, set TraceBusID=1, no local timestamp generation */
	ITM->TCR  = 0x0001000D;
	/* Enable stimulus port used by the logger */
	ITM->TER  = 1 << ITM_PORT_LOGGER;

	/* Initialize pin control settings, if any are defined */
#if DT_NODE_HAS_PROP(DT_NODELABEL(itm), pinctrl_0)
	const struct pinctrl_dev_config *pincfg =
		PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(itm));

	pinctrl_apply_state(pincfg, PINCTRL_STATE_DEFAULT);
#endif
}

static void log_backend_swo_panic(struct log_backend const *const backend)
{
}

static void dropped(const struct log_backend *const backend, uint32_t cnt)
{
	ARG_UNUSED(backend);

	log_backend_std_dropped(&log_output_swo, cnt);
}

const struct log_backend_api log_backend_swo_api = {
	.process = log_backend_swo_process,
	.panic = log_backend_swo_panic,
	.init = log_backend_swo_init,
	.dropped = IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) ? NULL : dropped,
	.format_set = format_set,
};

LOG_BACKEND_DEFINE(log_backend_swo, log_backend_swo_api, true);
