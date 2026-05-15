/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Reactive harness for the i3c_loopback TARGET (Board B).
 *
 * Boots, registers I3C target callbacks, then loops on the sync UART:
 * each line from Board A is parsed and acted upon — pre-stage read
 * data, raise an IBI, verify the last received write, etc.
 *
 * Not a ZTEST_SUITE — this app's only job is to react to Board A.
 */

#include "sync_proto.h"
#include "test_identity.h"

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/i3c/target_device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/sys_io.h>
#include <string.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(i3c_loopback_target, LOG_LEVEL_INF);

/* Early boot marker to prove flash is working */
static int early_boot_marker(void)
{
	printk("\n\n*** TARGET BUILD " __DATE__ " " __TIME__ " ***\n\n");
	return 0;
}
SYS_INIT(early_boot_marker, APPLICATION, 0);

static const struct device *const i3c_dev = DEVICE_DT_GET(DT_ALIAS(test_i3c));
static const struct device *const sync_uart = DEVICE_DT_GET(DT_CHOSEN(zephyr_sync_uart));

/*
 * LA debug marker (see boards/<...>.overlay zephyr,user/la-marker-gpios).
 * Pulsed from inside each I3C target callback so the logic analyzer can
 * tell whether the target IP saw a given address phase.  No pulse during
 * a NACK window => slave address-match comparator was disarmed.
 */
#define LA_MARKER_NODE DT_PATH(zephyr_user)
static const struct gpio_dt_spec la_marker =
	GPIO_DT_SPEC_GET_OR(LA_MARKER_NODE, la_marker_gpios, {0});

static inline void la_marker_pulse(void)
{
	if (la_marker.port != NULL) {
		/*
		 * Single register-write toggle (no busy-wait).  One edge
		 * per callback invocation; counting edges == counting
		 * callbacks.  Earlier busy-wait variants stalled the ISR
		 * and pushed NACK rates from 12 % to 97 %.
		 */
		gpio_pin_toggle_dt(&la_marker);
	}
}

#define TGT_BUF_MAX 64

/*
 * SLV_TX (target read response) staging contract.
 *
 * The DW IP requires read data to be pre-loaded into SLV_TX before the
 * controller initiates a private read.  read_requested_cb is purely a
 * notification (val == NULL); it is not a request for data.
 *
 * Empirically, if N bytes are staged but the controller reads M < N,
 * the leftover (N - M) bytes wedge the IP and every subsequent xfer
 * to this address times out with -EAGAIN.  The databook offers no
 * recovery sequence for this case, so these tests stay on the
 * documented happy path:
 *
 *   1) Stage exactly TGT_BUF_MAX bytes once at boot.
 *   2) Refill the same length after every STOP (refill thread, since
 *      i3c_target_tx_write takes a mutex and can't run in stop_cb).
 *   3) Test reads N <= TGT_BUF_MAX bytes; controller terminates via
 *      T-bit so no leftovers.
 */
#define SLV_TX_STAGE_LEN    TGT_BUF_MAX
#define SLV_TX_STAGE_PAT(i) ((uint8_t)(0x80U | (uint8_t)(i)))

static struct {
	uint8_t rx_buf[TGT_BUF_MAX];
	int rx_count;
	uint8_t read_buf[TGT_BUF_MAX];
	int read_idx;
	int read_len;
	int stop_count;
} state;

/* Refill thread: stop_cb runs in ISR context, but i3c_target_tx_write takes
 * a mutex.  Bounce the refill request to a thread via a semaphore.
 */
static K_SEM_DEFINE(slv_tx_refill_sem, 0, 1);

static void slv_tx_refill_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	for (;;) {
		k_sem_take(&slv_tx_refill_sem, K_FOREVER);
		(void)i3c_target_tx_write(i3c_dev, state.read_buf, (uint16_t)SLV_TX_STAGE_LEN, 0);
	}
}
K_THREAD_DEFINE(slv_tx_refill_tid, 1024, slv_tx_refill_thread, NULL, NULL, NULL, K_PRIO_PREEMPT(7),
		0, 0);

/* Sync UART listener: keeps replying to controller commands for the
 * lifetime of the target.  Without it, the polling-mode poll_in path
 * busy-waits forever once main() leaves its one-shot handshake.
 */
static void process_line(char *line);

static void sync_listener_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	char line[SYNC_LINE_MAX];

	for (;;) {
		int rc = sync_recv_line(sync_uart, line, sizeof(line), K_FOREVER);

		if (rc < 0) {
			continue;
		}
		process_line(line);
	}
}
/* Listener priority above the I3C target callback chain so the sync
 * UART poll-in loop is not starved by back-to-back transfers.
 */
K_THREAD_DEFINE(sync_listener_tid, 1536, sync_listener_thread, NULL, NULL, NULL, K_PRIO_PREEMPT(2),
		0, 0);

static int hex_nibble(char c)
{
	if (c >= '0' && c <= '9') {
		return c - '0';
	}
	if (c >= 'a' && c <= 'f') {
		return 10 + (c - 'a');
	}
	if (c >= 'A' && c <= 'F') {
		return 10 + (c - 'A');
	}
	return -1;
}

static int hex_decode(const char *s, uint8_t *out, size_t out_max)
{
	size_t n = 0;
	const size_t len = strlen(s);

	if ((len % 2U) != 0U) {
		return -EINVAL;
	}
	for (size_t i = 0; i < len && n < out_max; i += 2) {
		int hi = hex_nibble(s[i]);
		int lo = hex_nibble(s[i + 1]);

		if (hi < 0 || lo < 0) {
			return -EINVAL;
		}
		out[n++] = (uint8_t)((hi << 4) | lo);
	}
	return (int)n;
}

/*
 * --------------------------------------------------------------------------
 * I3C target callbacks
 * --------------------------------------------------------------------------
 */
static int tgt_write_requested(struct i3c_target_config *cfg)
{
	la_marker_pulse();
	ARG_UNUSED(cfg);
	state.rx_count = 0;
	return 0;
}

static int tgt_write_received(struct i3c_target_config *cfg, uint8_t val)
{
	la_marker_pulse();
	ARG_UNUSED(cfg);
	if (state.rx_count < TGT_BUF_MAX) {
		state.rx_buf[state.rx_count] = val;
	}
	state.rx_count++;
	return 0;
}

static int tgt_read_requested(struct i3c_target_config *cfg, uint8_t *val)
{
	la_marker_pulse();
	ARG_UNUSED(cfg);
	if (state.read_idx < state.read_len) {
		*val = state.read_buf[state.read_idx++];
	} else {
		*val = 0;
	}
	return 0;
}

static int tgt_read_processed(struct i3c_target_config *cfg, uint8_t *val)
{
	la_marker_pulse();
	ARG_UNUSED(cfg);
	if (state.read_idx < state.read_len) {
		*val = state.read_buf[state.read_idx++];
	} else {
		*val = 0;
	}
	return 0;
}

static int tgt_stop(struct i3c_target_config *cfg)
{
	la_marker_pulse();
	ARG_UNUSED(cfg);
	state.stop_count++;
	/* No auto-refill: controller drives staging via STAGE_READ so
	 * SLV_TX always holds exactly the next read's length (avoiding
	 * the queued != consumed wedge documented above).
	 */
	return 0;
}

#ifdef CONFIG_I3C_TARGET_BUFFER_MODE
static void tgt_buf_write_received(struct i3c_target_config *cfg, uint8_t *ptr, uint32_t len)
{
	la_marker_pulse();
	ARG_UNUSED(cfg);
	uint32_t n = MIN(len, (uint32_t)TGT_BUF_MAX);

	memcpy(state.rx_buf, ptr, n);
	state.rx_count = (int)n;
}

static int tgt_buf_read_requested(struct i3c_target_config *cfg, uint8_t **ptr, uint32_t *len,
				  uint8_t *hdr_mode)
{
	ARG_UNUSED(cfg);
	*ptr = state.read_buf;
	*len = (uint32_t)state.read_len;
	*hdr_mode = 0;
	return 0;
}
#endif

static const struct i3c_target_callbacks callbacks = {
	.write_requested_cb = tgt_write_requested,
	.write_received_cb = tgt_write_received,
	.read_requested_cb = tgt_read_requested,
	.read_processed_cb = tgt_read_processed,
	.stop_cb = tgt_stop,
#ifdef CONFIG_I3C_TARGET_BUFFER_MODE
	.buf_write_received_cb = tgt_buf_write_received,
	.buf_read_requested_cb = tgt_buf_read_requested,
#endif
};

static struct i3c_target_config target_cfg = {
	.address = 0,
	.callbacks = &callbacks,
};

/*
 * --------------------------------------------------------------------------
 * Command handler
 * --------------------------------------------------------------------------
 */
static void handle_ready(const char *id)
{
	state.rx_count = 0;
	state.read_idx = 0;
	state.stop_count = 0;
	sync_send(sync_uart, "ACK %s", id);
}

static void handle_stage_read(const char *hex)
{
	int n = hex_decode(hex, state.read_buf, sizeof(state.read_buf));

	if (n < 0) {
		sync_send(sync_uart, "FAIL stage_read bad-hex");
		return;
	}
	state.read_len = n;
	state.read_idx = 0;

	/* Queue EXACTLY n bytes into SLV_TX.  Controller side promises
	 * (via the handshake) to read exactly n.  Mismatch wedges the IP.
	 */
	int rc = i3c_target_tx_write(i3c_dev, state.read_buf, (uint16_t)n, 0);

	sync_send(sync_uart, "ACK stage_read %d %d", n, rc);
}

static void handle_verify(const char *id, const char *hex)
{
	uint8_t expected[TGT_BUF_MAX];
	int n = hex_decode(hex, expected, sizeof(expected));

	if (n < 0) {
		sync_send(sync_uart, "FAIL %s bad-hex", id);
		return;
	}
	if (n != state.rx_count || memcmp(expected, state.rx_buf, (size_t)n) != 0) {
		sync_send(sync_uart, "FAIL %s mismatch", id);
		return;
	}
	sync_send(sync_uart, "PASS %s", id);
}

#ifdef CONFIG_I3C_USE_IBI
static void handle_raise_tir(const char *hex)
{
	uint8_t payload[CONFIG_I3C_IBI_MAX_PAYLOAD_SIZE];
	int n = 0;

	if (hex != NULL && *hex != '\0') {
		n = hex_decode(hex, payload, sizeof(payload));
		if (n < 0) {
			sync_send(sync_uart, "FAIL tir bad-hex");
			return;
		}
	}

	struct i3c_ibi req = {
		.ibi_type = I3C_IBI_TARGET_INTR,
		.payload = payload,
		.payload_len = (uint8_t)n,
	};
	int rc = i3c_ibi_raise(i3c_dev, &req);

	sync_send(sync_uart, "IBI tir rc=%d", rc);
}

static void handle_raise_hj(void)
{
	struct i3c_ibi req = {.ibi_type = I3C_IBI_HOTJOIN};
	int rc = i3c_ibi_raise(i3c_dev, &req);

	sync_send(sync_uart, "IBI hj rc=%d", rc);
}

static void handle_raise_mr(void)
{
	struct i3c_ibi req = {.ibi_type = I3C_IBI_CONTROLLER_ROLE_REQUEST};
	int rc = i3c_ibi_raise(i3c_dev, &req);

	sync_send(sync_uart, "IBI mr rc=%d", rc);
}
#endif /* CONFIG_I3C_USE_IBI */

static void handle_reset(void)
{
	const uint32_t base = DT_REG_ADDR(DT_ALIAS(test_i3c));
	uint32_t nack_req = sys_read32(base + 0x9CU); /* SLV_NACK_REQ */
	uint32_t pres = sys_read32(base + 0x54U);     /* PRESENT_STATE */
	uint32_t dbsl = sys_read32(base + 0x50U);     /* DATA_BUFFER_STATUS_LEVEL */
	/*
	 * Capture CCC_DEVICE_STATUS at entry, before flush/RESUME clears
	 * volatile state.  Sticky bits (OVERFLOW_ERR/UNDERFLOW_ERR/
	 * BUFFER_NOT_AVAIL) only clear via the controller's GETSTATUS
	 * CCC; if that itself NACKs, we're stuck.
	 */
	uint32_t ccc_ds_in = sys_read32(base + 0x58U);

	memset(&state, 0, sizeof(state));

	/*
	 * If SLV_NACK_REQ is asserted the target IP will NACK all
	 * incoming transfers.  Clear it so the next test starts clean.
	 */
	if (nack_req != 0U) {
		sys_write32(0, base + 0x9CU);
	}

	/*
	 * Flush all queues (RX/TX FIFO + RESP/CMD queue) to discard
	 * stale STAGE_READ artefacts from a previous test whose reads
	 * never executed.  Without flushing CMD_QUEUE the old TX cmd is
	 * consumed on the next test's read, mixing old length with new
	 * data — manifests as data-mismatch in the repeated-start xfer
	 * test.  Then RESUME clears SLAVE_BUSY (§6.1.24 bit 9).
	 */
	sys_write32(BIT(4) | BIT(3) | BIT(2) | BIT(1),
		    base + 0x34U); /* RESET_CTRL: RX+TX+RESP+CMD */

	/* Poll until RESET_CTRL self-clears (flush consumed by IP). */
	{
		const uint32_t flush_mask = BIT(4) | BIT(3) | BIT(2) | BIT(1);
		int timeout = 10000;

		while ((sys_read32(base + 0x34U) & flush_mask) && --timeout) {
		}
	}

	sys_write32(sys_read32(base + 0x00U) | BIT(30), base + 0x00U); /* DEVICE_CTRL.RESUME */

	/* Poll until RESUME self-clears. */
	{
		int timeout = 10000;

		while ((sys_read32(base + 0x00U) & BIT(30)) && --timeout) {
		}
	}

	uint32_t ccc_ds = sys_read32(base + 0x58U); /* CCC_DEVICE_STATUS */
	uint32_t dbsl2 = sys_read32(base + 0x50U);  /* DATA_BUFFER_STATUS_LEVEL after flush */
	uint32_t qsl = sys_read32(base + 0x4CU);    /* QUEUE_STATUS_LEVEL */

	sync_send(sync_uart,
		  "READY reset NACK=%02x PRES=%08x DBSL=%08x->%08x QSL=%08x "
		  "CCC_DS_in=%08x->%08x",
		  nack_req, pres, dbsl, dbsl2, qsl, ccc_ds_in, ccc_ds);
}

/*
 * Read-only register snapshot used by the controller after a cascade-
 * NACK trigger to identify which target IP register flipped on the
 * boundary.  Mutates nothing.
 */
static void handle_dump(void)
{
	const uint32_t base = DT_REG_ADDR(DT_ALIAS(test_i3c));

	uint32_t dev_ctrl = sys_read32(base + 0x00U);
	uint32_t dev_addr = sys_read32(base + 0x04U);
	uint32_t intr_sts = sys_read32(base + 0x3CU);
	uint32_t intr_en = sys_read32(base + 0x40U);
	uint32_t intr_sig = sys_read32(base + 0x44U);
	uint32_t pres = sys_read32(base + 0x54U);
	uint32_t slv_evt = sys_read32(base + 0x38U);
	uint32_t slv_nack = sys_read32(base + 0x9CU);
	uint32_t qsl = sys_read32(base + 0x4CU);
	uint32_t dbsl = sys_read32(base + 0x50U);
	uint32_t dev_extd = sys_read32(base + 0xB0U);
	uint32_t ext_cmd2 = sys_read32(base + 0x16CU); /* vendor EXT_CMD_REG_2 */
	uint32_t ext_cmd3 = sys_read32(base + 0x170U); /* vendor EXT_CMD_REG_3 */
#ifdef CONFIG_I3C_LOOPBACK_TARGET_VENDOR_WRAPPER_GATE
	uint32_t ext_gate = sys_read32(base + CONFIG_I3C_LOOPBACK_TARGET_WRAPPER_GATE_OFFSET);
#else
	uint32_t ext_gate = 0U;
#endif

	sync_send(sync_uart,
		  "DUMP CTRL=%08x ADDR=%08x EXTD=%08x INTR=%08x EN=%08x SIG=%08x "
		  "PRES=%08x EVT=%08x NACK=%02x QSL=%08x DBSL=%08x "
		  "EXT2=%08x EXT3=%08x GATE=%08x",
		  dev_ctrl, dev_addr, dev_extd, intr_sts, intr_en, intr_sig, pres, slv_evt,
		  slv_nack, qsl, dbsl, ext_cmd2, ext_cmd3, ext_gate);
}

/*
 * Full IP reset on the target side.  Disables the IP, asserts
 * RESET_CTRL_ALL (SOFT + queues + FIFOs), polls until it self-clears,
 * then re-configures target identity, callbacks, and interrupts so
 * the IP will ACK on the bus again.  Nuclear option for persistent
 * NACK states where all registers read clean but private writes still
 * fail.
 */
static void handle_soft_rst(void)
{
	const uint32_t base = DT_REG_ADDR(DT_ALIAS(test_i3c));

	/* Step 1: disable IP */
	sys_write32(0x00000000U, base + 0x00U); /* DEVICE_CTRL = 0 */

	/* Step 2: RESET_CTRL_ALL (SOFT + all queues/FIFOs) = 0x3F */
	sys_write32(0x3FU, base + 0x34U); /* RESET_CTRL */

#ifdef CONFIG_I3C_LOOPBACK_TARGET_VENDOR_WRAPPER_GATE
	/*
	 * Re-open vendor wrapper gate.  Some IP integrations gate the
	 * DW protocol clock through a vendor wrapper register; SOFT_RST
	 * needs that clock running to self-clear.
	 */
	sys_write32(CONFIG_I3C_LOOPBACK_TARGET_WRAPPER_GATE_VALUE,
		    base + CONFIG_I3C_LOOPBACK_TARGET_WRAPPER_GATE_OFFSET);
#endif

	/* Step 4: poll until RESET_CTRL self-clears */
	uint32_t timeout = 100000U;

	while ((sys_read32(base + 0x34U) & 0x3FU) && --timeout) {
	}
	if (timeout == 0U) {
		LOG_ERR("[soft_rst] RESET_CTRL did not clear: 0x%08x", sys_read32(base + 0x34U));
		sync_send(sync_uart, "FAIL soft_rst reset_timeout");
		return;
	}

#ifndef CONFIG_I3C_LOOPBACK_TARGET_SOFT_RST_CLEARS_DEVICE_ADDR
	/*
	 * On some IP variants SOFT_RST does not clear DEVICE_ADDR, so
	 * the stale dynamic addr survives and the target may skip the
	 * next ENTDAA round.  i3c_configure() only RMWs the static-addr
	 * bits, so zero the whole register here first.
	 */
	sys_write32(0x00000000U, base + 0x04U); /* DEVICE_ADDR = 0 */
#endif

	/*
	 * Set DEVICE_CTRL_EXTENDED to SLAVE mode.  SOFT_RST resets it
	 * to MASTER; must be set before ENABLE.
	 */
	sys_write32(0x01U, base + 0xB0U); /* DEVICE_CTRL_EXTENDED = SLAVE */

	/* Re-configure target identity + enable + EXT_CMD. */
	struct i3c_config_target tcfg = {
		.enabled = true,
		.static_addr = TARGET_STATIC_ADDR,
		.pid = TARGET_PID,
		.pid_random = false,
		.bcr = TARGET_BCR,
		.dcr = TARGET_DCR,
		.max_read_len = 64,
		.max_write_len = 64,
		.supported_hdr = 0,
	};
	int rc = i3c_configure(i3c_dev, I3C_CONFIG_TARGET, &tcfg);

	if (rc != 0) {
		LOG_ERR("[soft_rst] i3c_configure(TARGET) failed (%d)", rc);
		sync_send(sync_uart, "FAIL soft_rst configure=%d", rc);
		return;
	}

	/*
	 * i3c_configure(TARGET) only RMWs the static-addr bits, so
	 * explicitly clear any stale dynamic addr [23:16] + valid bit.
	 */
	{
		uint32_t addr = sys_read32(base + 0x04U);

		addr &= ~(BIT(23) | GENMASK(22, 16)); /* clear DYNAMIC_ADDR_VALID + DA */
		sys_write32(addr, base + 0x04U);
	}

	/* Re-register target callbacks. */
	rc = i3c_target_register(i3c_dev, &target_cfg);
	if (rc != 0 && rc != -ENOSYS) {
		LOG_ERR("[soft_rst] i3c_target_register failed (%d)", rc);
	}

	/* Re-enable target interrupts (SOFT_RST zeroes both EN regs). */
	uint32_t slave_mask = BIT(12) | BIT(11) | BIT(9) | BIT(8) | BIT(6) | BIT(4);

	sys_write32(slave_mask, base + 0x40U); /* INTR_STATUS_EN */
	sys_write32(slave_mask, base + 0x44U); /* INTR_SIGNAL_EN */

	/* Also re-program queue threshold for IBI status (same as driver) */
	uint32_t thld_ctrl = sys_read32(base + 0x1CU); /* QUEUE_THLD_CTRL */

	thld_ctrl &= ~(0xFFU << 24); /* clear IBI_STS_THLD field */
	sys_write32(thld_ctrl, base + 0x1CU);

	/* Clear any latched interrupt status from before reset */
	sys_write32(0xFFFFFFFFU, base + 0x3CU); /* INTR_STATUS */

	uint32_t dev_ctrl = sys_read32(base + 0x00U);
	uint32_t dev_addr = sys_read32(base + 0x04U);
	uint32_t pres = sys_read32(base + 0x54U);

	/* Reset local state */
	memset(&state, 0, sizeof(state));

	sync_send(sync_uart, "READY soft_rst rc=%d CTRL=%08x ADDR=%08x PRES=%08x", rc, dev_ctrl,
		  dev_addr, pres);
}

/*
 * Vendor EXT_CMD kick — re-arms the target IP for SETDASA/ENTDAA after
 * RSTDAA cleared its DA.  Only built on IP integrations whose target
 * block requires this sequence; mirrors the equivalent kick the
 * vendor-specific driver glue performs.
 */
#ifdef CONFIG_I3C_LOOPBACK_TARGET_VENDOR_EXT_CMD_KICK
static void handle_rearm(void)
{
	const uint32_t base = DT_REG_ADDR(DT_ALIAS(test_i3c));

	sys_write32(0x80U, base + 0x138U); /* EXT_TX_DATA_PORT_2 */
	sys_write32(0x80U, base + 0x13CU); /* EXT_TX_DATA_PORT_3 */

	uint32_t cmd2 = sys_read32(base + 0x16CU); /* EXT_CMD_REG_2 */

	cmd2 |= (0x9AU << 24) | (0U << 16) | (1U << 6);
	sys_write32(cmd2, base + 0x16CU);

	uint32_t cmd3 = sys_read32(base + 0x170U); /* EXT_CMD_REG_3 */

	cmd3 |= (0x9AU << 24) | (1U << 16) | (1U << 6);
	sys_write32(cmd3, base + 0x170U);

	sync_send(sync_uart, "ACK rearm");
}
#else
static void handle_rearm(void)
{
	/* No vendor-specific kick needed; just acknowledge. */
	sync_send(sync_uart, "ACK rearm");
}
#endif

static void process_line(char *line)
{
	char *cmd = strtok(line, " ");

	if (cmd == NULL) {
		return;
	}

	if (strcmp(cmd, "READY") == 0) {
		char *id = strtok(NULL, " ");

		handle_ready(id ? id : "0");
	} else if (strcmp(cmd, "STAGE_READ") == 0) {
		char *hex = strtok(NULL, " ");

		handle_stage_read(hex ? hex : "");
	} else if (strcmp(cmd, "VERIFY") == 0) {
		char *id = strtok(NULL, " ");
		char *hex = strtok(NULL, " ");

		handle_verify(id ? id : "0", hex ? hex : "");
	} else if (strcmp(cmd, "RESET") == 0) {
		handle_reset();
	} else if (strcmp(cmd, "SOFT_RST") == 0) {
		handle_soft_rst();
	} else if (strcmp(cmd, "REARM") == 0) {
		handle_rearm();
	} else if (strcmp(cmd, "DUMP") == 0) {
		handle_dump();
#ifdef CONFIG_I3C_USE_IBI
	} else if (strcmp(cmd, "RAISE_TIR") == 0) {
		char *hex = strtok(NULL, " ");

		handle_raise_tir(hex ? hex : "");
	} else if (strcmp(cmd, "RAISE_HJ") == 0) {
		handle_raise_hj();
	} else if (strcmp(cmd, "RAISE_MR") == 0) {
		handle_raise_mr();
#endif
	} else if (strcmp(cmd, "HELLO") == 0) {
		sync_send(sync_uart, "HELLO");
	} else {
		LOG_WRN("unknown sync cmd: %s", cmd);
	}
}

int main(void)
{
	if (!device_is_ready(i3c_dev)) {
		LOG_ERR("I3C target device %s not ready", i3c_dev->name);
		return -ENODEV;
	}
	if (!device_is_ready(sync_uart)) {
		LOG_ERR("Sync UART %s not ready", sync_uart->name);
		return -ENODEV;
	}

	/* LA debug marker: configure as output low. Safe to skip if the
	 * board overlay didn't define la-marker-gpios.
	 */
	if (la_marker.port != NULL) {
		if (device_is_ready(la_marker.port)) {
			(void)gpio_pin_configure_dt(&la_marker, GPIO_OUTPUT_INACTIVE);
			/* Boot heartbeat: 5 wide pulses (1 ms high, 1 ms
			 * low) so the LA can confirm the pin is wired
			 * correctly before any I3C traffic begins.
			 */
			for (int i = 0; i < 5; i++) {
				gpio_pin_set_dt(&la_marker, 1);
				k_busy_wait(1000);
				gpio_pin_set_dt(&la_marker, 0);
				k_busy_wait(1000);
			}
			printk("[LA-MARKER] boot heartbeat done on %s pin %d\n",
			       la_marker.port->name, la_marker.pin);
		} else {
			LOG_WRN("LA marker GPIO port not ready");
		}
	} else {
		printk("[LA-MARKER] no la-marker-gpios in DT (port==NULL)\n");
	}

	/* sync_listener_thread owns the sync UART; it replies to HELLO and
	 * all later protocol commands.  Don't compete for RX here.
	 */

	struct i3c_config_target tcfg = {
		.enabled = true,
		.static_addr = TARGET_STATIC_ADDR,
		.pid = TARGET_PID,
		.pid_random = false,
		.bcr = TARGET_BCR,
		.dcr = TARGET_DCR,
		.max_read_len = 64,
		.max_write_len = 64,
		.supported_hdr = 0,
	};
	int rc = i3c_configure(i3c_dev, I3C_CONFIG_TARGET, &tcfg);

	if (rc != 0) {
		LOG_ERR("i3c_configure(TARGET) failed (%d)", rc);
	}

	rc = i3c_target_register(i3c_dev, &target_cfg);

	if (rc != 0 && rc != -ENOSYS) {
		LOG_ERR("i3c_target_register failed (%d)", rc);
	}

	/*
	 * The driver's dw_i3c_configure(TARGET) implements the canonical
	 * enable sequence (disable -> identity -> enable -> EXT_CMD).  On
	 * some IP variants DEVICE_CTRL.ENABLE reads back 0 even after
	 * EXT_CMD fires; that is expected, and the block is functionally
	 * enabled.
	 */

	/* Pre-staging is unnecessary: the controller drives every read
	 * via STAGE_READ over the sync UART (see handle_stage_read).
	 */

	/* Keep the target alive so the I3C hardware can respond. */
	while (1) {
		k_sleep(K_SECONDS(1));
	}

	return 0;
}
