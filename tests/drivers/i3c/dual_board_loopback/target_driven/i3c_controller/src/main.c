/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Reactive controller harness for the target_driven pair (Board A).
 *
 * Board A (this app) is the I3C controller and Board B is the I3C target,
 * the same way round as in controller_driven; what differs is which board
 * runs ztest.  Here Board B owns the ZTEST suite and Board A is the
 * stimulus generator: it enumerates the bus once at boot, announces the
 * result, then serves one bus operation per sync-UART command.
 *
 * Not a ZTEST_SUITE - this app's only job is to react to Board B.
 */

#include "hex_util.h"
#include "sync_proto.h"
#include "test_identity.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/i3c/ccc.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <stdlib.h>
#include <string.h>

LOG_MODULE_REGISTER(target_driven_ctrl, LOG_LEVEL_INF);

static int early_boot_marker(void)
{
	printk("\n\n*** TD-CONTROLLER BUILD " __DATE__ " " __TIME__ " ***\n\n");
	return 0;
}
SYS_INIT(early_boot_marker, APPLICATION, 0);

#define XFER_BUF_MAX 64

static const struct device *const i3c_dev = DEVICE_DT_GET(DT_ALIAS(test_i3c));
static const struct device *const sync_uart = DEVICE_DT_GET(DT_CHOSEN(zephyr_sync_uart));

/*
 * Peer descriptors have no local driver instance to point at, but the
 * I3C subsystem dereferences i3c_device_desc.dev without checking it.
 */
static const struct device i3c_peer_dev = {
	.name = "i3c-peer",
};

static struct i3c_device_desc target_b_desc = {
	.bus = DEVICE_DT_GET(DT_ALIAS(test_i3c)),
	.dev = &i3c_peer_dev,
	.pid = TARGET_PID,
	.static_addr = TARGET_STATIC_ADDR,
	.init_dynamic_addr = TARGET_STATIC_ADDR,
	.flags = 0,
};

/* SETDASA/SETNEWDA carry the address left-shifted by one (ccc.h). */
#define CCC_ADDR_PAYLOAD(addr) ((uint8_t)((addr) << 1))

/*
 * --------------------------------------------------------------------------
 * Bus bring-up
 * --------------------------------------------------------------------------
 */
static int heal_target(void)
{
	struct i3c_ccc_address da = {.addr = CCC_ADDR_PAYLOAD(TARGET_STATIC_ADDR)};
	int rc;

	(void)i3c_recover_bus(i3c_dev);
	(void)i3c_ccc_do_rstdaa_all(i3c_dev);

	/*
	 * SETDASA is only legal for a device with no dynamic address, and the
	 * driver keys the DAT entry on whatever address the descriptor holds
	 * at attach time.  Detach against the static address so the freed
	 * slot is the one the descriptor actually named.
	 */
	target_b_desc.dynamic_addr = TARGET_STATIC_ADDR;
	(void)i3c_detach_i3c_device(&target_b_desc);
	target_b_desc.dynamic_addr = 0U;

	rc = i3c_attach_i3c_device(&target_b_desc);
	if ((rc != 0) && (rc != -EALREADY)) {
		return rc;
	}

	rc = i3c_ccc_do_setdasa(&target_b_desc, da);
	if (rc != 0) {
		return rc;
	}

	(void)i3c_detach_i3c_device(&target_b_desc);
	target_b_desc.dynamic_addr = TARGET_STATIC_ADDR;
	rc = i3c_attach_i3c_device(&target_b_desc);
	if (rc == -EALREADY) {
		rc = 0;
	}

	return rc;
}

static int enumerate_bus(void)
{
	struct i3c_dev_list dev_list = {
		.i3c = &target_b_desc,
		.i2c = NULL,
		.num_i3c = 1,
		.num_i2c = 0,
	};
	int rc;

	rc = i3c_attach_i3c_device(&target_b_desc);
	if ((rc != 0) && (rc != -EALREADY)) {
		LOG_ERR("attach target_b failed (%d)", rc);
		return rc;
	}

	rc = i3c_bus_init(i3c_dev, &dev_list);
	LOG_INF("i3c_bus_init rc=%d DA=0x%02x", rc, target_b_desc.dynamic_addr);

	if (target_b_desc.dynamic_addr == 0U) {
		rc = heal_target();
		LOG_INF("heal rc=%d DA=0x%02x", rc, target_b_desc.dynamic_addr);
	}

	return (target_b_desc.dynamic_addr != 0U) ? 0 : -ENODEV;
}

/*
 * --------------------------------------------------------------------------
 * Command handlers
 * --------------------------------------------------------------------------
 */
static void handle_do_write(const char *hex)
{
	uint8_t buf[XFER_BUF_MAX];
	struct i3c_msg msg;
	int n = hex_to_bytes(hex, buf, sizeof(buf));

	if (n < 0) {
		sync_send(sync_uart, "DONE write rc=%d", n);
		return;
	}

	msg.buf = buf;
	msg.len = (uint32_t)n;
	msg.flags = I3C_MSG_WRITE | I3C_MSG_STOP;

	sync_send(sync_uart, "DONE write rc=%d", i3c_transfer(&target_b_desc, &msg, 1));
}

#define BURST_MAX 64U

/*
 * ARMED goes out before the first transfer so Board B learns when the bus
 * becomes busy; one transfer is too short for B to aim an IBI at, a burst is
 * not.  Bytes match fill_pattern() with seed base+i so B can verify the stream.
 */
static void handle_burst_write(const char *count_str, const char *len_str, const char *base_str)
{
	uint8_t buf[XFER_BUF_MAX];
	unsigned long count = strtoul(count_str, NULL, 0);
	unsigned long len = strtoul(len_str, NULL, 0);
	unsigned long base = strtoul(base_str, NULL, 0);
	unsigned int sent = 0;
	struct i3c_msg msg;
	int rc = 0;

	if ((count > BURST_MAX) || (len == 0UL) || (len > sizeof(buf))) {
		sync_send(sync_uart, "ARMED");
		sync_send(sync_uart, "DONE burst rc=%d sent=0", -EINVAL);
		return;
	}

	sync_send(sync_uart, "ARMED");

	for (unsigned long i = 0; i < count; i++) {
		for (unsigned long j = 0; j < len; j++) {
			buf[j] = (uint8_t)(base + i + (j * 0x11U));
		}

		msg.buf = buf;
		msg.len = (uint32_t)len;
		msg.flags = I3C_MSG_WRITE | I3C_MSG_STOP;

		rc = i3c_transfer(&target_b_desc, &msg, 1);
		if (rc != 0) {
			break;
		}
		sent++;
	}

	sync_send(sync_uart, "DONE burst rc=%d sent=%u", rc, sent);
}

static void handle_do_read(const char *len_str)
{
	uint8_t buf[XFER_BUF_MAX];
	char hex[(XFER_BUF_MAX * 2U) + 1U];
	struct i3c_msg msg;
	long want = strtol(len_str, NULL, 0);
	int rc;

	if ((want < 0) || (want > (long)sizeof(buf))) {
		sync_send(sync_uart, "DONE read rc=%d", -EINVAL);
		return;
	}

	memset(buf, 0, sizeof(buf));
	msg.buf = buf;
	msg.len = (uint32_t)want;
	msg.flags = I3C_MSG_READ | I3C_MSG_STOP;

	rc = i3c_transfer(&target_b_desc, &msg, 1);
	if (bytes_to_hex(buf, (size_t)want, hex, sizeof(hex)) != 0) {
		hex[0] = '\0';
	}
	sync_send(sync_uart, "DONE read rc=%d %s", rc, hex);
}

/*
 * Abort a private write part-way so the target-side test can prove the
 * target survives it.
 *
 * DEVICE_CTRL bit 29 ABORT (DW databook 6.1.1) is the only way to do this and no
 * API exposes it: "allows the user to relinquish the I3C Bus before completing
 * the issued transfer", with the STOP issued after the data byte in flight, so
 * the target sees a truncated write rather than none at all.  The payload is
 * generated here rather than sent over the sync UART so it can be long enough
 * for the abort to land inside it.
 */
#define DEV_CTRL_ABORT BIT(29)

/*
 * ABORT only acts on a transfer already in flight -- setting it beforehand is
 * a no-op (measured: the whole payload still arrived).  A tick is 100 us and
 * the write itself is of that order, so a k_timer cannot land inside it;
 * this thread runs when i3c_transfer() blocks and busy-waits instead.
 */
static uint32_t abort_delay_us;
static K_SEM_DEFINE(abort_go, 0, 1);

static void abort_thread_fn(void *p1, void *p2, void *p3)
{
	const mm_reg_t regs = (mm_reg_t)DT_REG_ADDR(DT_ALIAS(test_i3c));

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	for (;;) {
		k_sem_take(&abort_go, K_FOREVER);
		k_busy_wait(abort_delay_us);
		sys_write32(sys_read32(regs + 0x0) | DEV_CTRL_ABORT, regs + 0x0);
	}
}

/* Priority 1: below main, so the transfer starts before this runs. */
K_THREAD_DEFINE(abort_tid, 768, abort_thread_fn, NULL, NULL, NULL, 1, 0, 0);

static void handle_do_write_abort(const char *len_str, const char *delay_str)
{
	static uint8_t buf[256];
	unsigned long want = strtoul(len_str, NULL, 0);
	struct i3c_msg msg;
	int rc;

	if ((want == 0UL) || (want > sizeof(buf))) {
		sync_send(sync_uart, "DONE write_abort rc=%d sent=0", -EINVAL);
		return;
	}

	for (size_t i = 0; i < want; i++) {
		buf[i] = (uint8_t)(0xa0u + i);
	}

	msg.buf = buf;
	msg.len = (uint32_t)want;
	msg.flags = I3C_MSG_WRITE | I3C_MSG_STOP;

	abort_delay_us = (uint32_t)strtoul(delay_str, NULL, 0);
	k_sem_give(&abort_go);
	rc = i3c_transfer(&target_b_desc, &msg, 1);

	/* The abort halts the controller, so it needs the same recovery the
	 * unowned-address probe does before the next case runs.
	 */
	(void)i3c_recover_bus(i3c_dev);

	sync_send(sync_uart, "DONE write_abort rc=%d sent=%u", rc, (unsigned int)want);
}

/*
 * Drive one transfer at an arbitrary address, so the target-side tests can
 * present traffic this target does not own.
 *
 * Filling in a descriptor is not enough to put the address on the wire:
 * dw_i3c_xfers() resolves the DAT slot with get_i3c_addr_pos(), which
 * searches the bus's attached-device list, so an unattached descriptor fails
 * -EINVAL without generating a single SCL edge.  Attaching it is in turn
 * refused with -EADDRNOTAVAIL for any slot i3c_addr_slots_init() marked
 * reserved, which per MIPI I3C Basic v1.0 5.1.2.2.5 is 7'h7E and its seven
 * single-bit neighbours -- precisely the error type S0 addresses.  Free the
 * slot for the duration and restore it afterwards, so the stimulus is
 * produced by the ordinary driver path rather than by poking registers.
 */
static int xfer_at_addr(uint8_t addr, struct i3c_msg *msg)
{
	struct i3c_driver_data *bus_data = (struct i3c_driver_data *)i3c_dev->data;
	struct i3c_addr_slots *slots = &bus_data->attached_dev.addr_slots;
	enum i3c_addr_slot_status saved = i3c_addr_slots_status(slots, addr);
	struct i3c_device_desc scratch = {
		.bus = i3c_dev,
		.dev = &i3c_peer_dev,
		.dynamic_addr = addr,
	};
	int rc;

	i3c_addr_slots_set(slots, addr, I3C_ADDR_SLOT_STATUS_FREE);

	rc = i3c_attach_i3c_device(&scratch);
	if (rc == 0) {
		rc = i3c_transfer(&scratch, msg, 1);
		(void)i3c_detach_i3c_device(&scratch);
	}

	i3c_addr_slots_set(slots, addr, saved);

	/* Nothing answers the address, and the resulting NACK halts the
	 * controller, so the next command would fail on a stale error.
	 */
	(void)i3c_recover_bus(i3c_dev);

	return rc;
}

static void handle_do_write_addr(const char *addr_str, const char *hex)
{
	uint8_t buf[XFER_BUF_MAX];
	struct i3c_msg msg;
	long addr = strtol(addr_str, NULL, 0);
	int n = hex_to_bytes(hex, buf, sizeof(buf));

	if ((n < 0) || (addr <= 0) || (addr > 0x7F)) {
		sync_send(sync_uart, "DONE write_addr rc=%d", -EINVAL);
		return;
	}

	msg.buf = buf;
	msg.len = (uint32_t)n;
	msg.flags = I3C_MSG_WRITE | I3C_MSG_STOP;

	/* A NACK is the expected outcome for an unowned address; the test
	 * asserts on Board B's counters, not on this return code.
	 */
	sync_send(sync_uart, "DONE write_addr rc=%d", xfer_at_addr((uint8_t)addr, &msg));
}

/* 7'h7E/R is an error type S0 trigger and has no write-side equivalent. */
static void handle_do_read_addr(const char *addr_str, const char *len_str)
{
	uint8_t buf[XFER_BUF_MAX];
	char hex[(XFER_BUF_MAX * 2U) + 1U];
	struct i3c_msg msg;
	long addr = strtol(addr_str, NULL, 0);
	long want = strtol(len_str, NULL, 0);
	int rc;

	if ((addr <= 0) || (addr > 0x7F) || (want <= 0) || (want > (long)sizeof(buf))) {
		sync_send(sync_uart, "DONE read_addr rc=%d", -EINVAL);
		return;
	}

	memset(buf, 0, sizeof(buf));
	msg.buf = buf;
	msg.len = (uint32_t)want;
	msg.flags = I3C_MSG_READ | I3C_MSG_STOP;

	rc = xfer_at_addr((uint8_t)addr, &msg);
	if (bytes_to_hex(buf, (size_t)want, hex, sizeof(hex)) != 0) {
		hex[0] = '\0';
	}
	sync_send(sync_uart, "DONE read_addr rc=%d %s", rc, hex);
}

static void handle_a_dump(void)
{
	/* PID is 48-bit; split so Board B never has to scan a 64-bit field. */
	sync_send(sync_uart, "ADUMP da=%02x bcr=%02x dcr=%02x pidhi=%04x pidlo=%08x",
		  target_b_desc.dynamic_addr, target_b_desc.bcr, target_b_desc.dcr,
		  (unsigned int)((target_b_desc.pid >> 32) & 0xffffU),
		  (unsigned int)(target_b_desc.pid & 0xffffffffU));
}

/*
 * One CCC per request.  `val` carries whatever the CCC reads back (the
 * GETMxL length, the DA after ENTDAA) and is 0 for the rest, so Board B
 * can parse every reply the same way.
 */
static void handle_do_ccc(const char *name, const char *arg_str)
{
	unsigned long arg = strtoul(arg_str, NULL, 0);
	unsigned int val = 0;
	int rc;

	if (strcmp(name, "SETMWL") == 0) {
		struct i3c_ccc_mwl mwl = {.len = (uint16_t)arg};

		rc = i3c_ccc_do_setmwl(&target_b_desc, &mwl);
	} else if (strcmp(name, "GETMWL") == 0) {
		struct i3c_ccc_mwl mwl = {0};

		rc = i3c_ccc_do_getmwl(&target_b_desc, &mwl);
		val = mwl.len;
	} else if (strcmp(name, "SETMRL") == 0) {
		struct i3c_ccc_mrl mrl = {.len = (uint16_t)arg, .ibi_len = 0};

		rc = i3c_ccc_do_setmrl(&target_b_desc, &mrl);
	} else if (strcmp(name, "GETMRL") == 0) {
		struct i3c_ccc_mrl mrl = {0};

		rc = i3c_ccc_do_getmrl(&target_b_desc, &mrl);
		val = mrl.len;
	} else if (strcmp(name, "SETNEWDA") == 0) {
		rc = i3c_bus_setnewda(&target_b_desc, (uint8_t)arg);
		val = target_b_desc.dynamic_addr;
	} else if (strcmp(name, "RSTDAA") == 0) {
		/* Broadcast: the directed form is deprecated since I3C v1.1 and
		 * this target NACKs it.
		 */
		rc = i3c_ccc_do_rstdaa_all(i3c_dev);
		target_b_desc.dynamic_addr = 0U;
		val = target_b_desc.dynamic_addr;
	} else if (strcmp(name, "ENTDAA") == 0) {
		rc = i3c_do_daa(i3c_dev);
		val = target_b_desc.dynamic_addr;
	} else if (strcmp(name, "DISEC") == 0) {
		struct i3c_ccc_events events = {.events = (uint8_t)arg};

		rc = i3c_ccc_do_events_set(&target_b_desc, false, &events);
	} else if (strcmp(name, "ENEC") == 0) {
		struct i3c_ccc_events events = {.events = (uint8_t)arg};

		rc = i3c_ccc_do_events_set(&target_b_desc, true, &events);
	} else if (strcmp(name, "GETSTATUS") == 0) {
		union i3c_ccc_getstatus status = {0};

		rc = i3c_ccc_do_getstatus_fmt1(&target_b_desc, &status);
		val = status.fmt1.status;
	} else if (strcmp(name, "SETDASA") == 0) {
		/* The payload carries the DA left-shifted by one. */
		struct i3c_ccc_address da = {.addr = (uint8_t)(arg << 1)};

		rc = i3c_ccc_do_setdasa(&target_b_desc, da);
		if (rc == 0) {
			/* The helper does not write it back, and later transfers
			 * look the target up by this field.
			 */
			target_b_desc.dynamic_addr = (uint8_t)arg;
		}
		val = target_b_desc.dynamic_addr;
	} else if (strcmp(name, "GETMXDS") == 0) {
		/* Poisoned: MAX_DATA_SPEED reads 0 on this IP, so an all-zero
		 * reply is indistinguishable from one the CCC never wrote.
		 */
		union i3c_ccc_getmxds caps = {.fmt1 = {.maxwr = 0xff, .maxrd = 0xff}};

		rc = i3c_ccc_do_getmxds_fmt1(&target_b_desc, &caps);
		val = (unsigned int)caps.fmt1.maxwr | ((unsigned int)caps.fmt1.maxrd << 8);
	} else {
		rc = -ENOTSUP;
	}

	sync_send(sync_uart, "DONE ccc rc=%d val=%u", rc, val);
}

/*
 * Issue one CCC with fields no i3c_ccc_do_*() wrapper can express: an
 * arbitrary CCC ID, a direction the command does not define, or a payload
 * shorter than the command's defined length.  These are the error type S5
 * stimuli -- per MIPI I3C Basic v1.0 5.1.10.1.6, "An example of an illegally
 * formatted CCC would be if the Slave receives the Dynamic Address/Write
 * during the GETBCR CCC", which is a direction the typed helpers never
 * produce.
 *
 * Unlike the reserved addresses in xfer_at_addr(), nothing here is gated by
 * the subsystem: dw_i3c_do_ccc() validates only that a non-zero length has a
 * buffer and that the target count fits the hardware, so the malformed frame
 * reaches the wire unaltered.
 */
static void handle_do_raw_ccc(const char *id_str, const char *rnw_str, const char *len_str)
{
	static uint8_t buf[8];
	unsigned long id = strtoul(id_str, NULL, 0);
	unsigned long rnw = strtoul(rnw_str, NULL, 0);
	unsigned long len = strtoul(len_str, NULL, 0);
	struct i3c_ccc_target_payload tgt = {0};
	struct i3c_ccc_payload payload = {0};
	int err;
	int rc;

	if ((id > 0xffUL) || (len > sizeof(buf))) {
		sync_send(sync_uart, "DONE raw_ccc rc=%d err=%d", -EINVAL, I3C_ERROR_CE_INVALID);
		return;
	}

	for (size_t i = 0; i < len; i++) {
		buf[i] = (uint8_t)(0x50U + i);
	}

	payload.ccc.id = (uint8_t)id;

	/* Populating targets does not force a directed frame: routes
	 * on i3c_ccc_is_payload_broadcast(), which reads the id alone.
	 */
	if (id <= I3C_CCC_BROADCAST_MAX_ID) {
		payload.ccc.data = (len > 0UL) ? buf : NULL;
		payload.ccc.data_len = len;
	} else {
		tgt.addr = target_b_desc.dynamic_addr;
		tgt.rnw = (rnw != 0UL) ? 1U : 0U;
		tgt.data = (len > 0UL) ? buf : NULL;
		tgt.data_len = len;

		payload.targets.payloads = &tgt;
		payload.targets.num_targets = 1U;
	}

	rc = i3c_do_ccc(i3c_dev, &payload);
	err = (id <= I3C_CCC_BROADCAST_MAX_ID) ? (int)payload.ccc.err : (int)tgt.err;

	/* A NACKed CCC halts the controller the same way an unowned address
	 * does.
	 */
	(void)i3c_recover_bus(i3c_dev);

	sync_send(sync_uart, "DONE raw_ccc rc=%d err=%d", rc, err);
}

/*
 * Write then read in one transfer, with a Repeated START and no STOP in
 * between: only the last message carries I3C_MSG_STOP.
 *
 * Every other verb here emits a single message with its own STOP, so this is
 * the only way the suite can present the framing MIPI I3C Basic v1.0 5.1.2.3.4
 * describes, where a Repeated START rather than a STOP ends the read.
 */
static void handle_do_xfer(const char *hex, const char *len_str)
{
	uint8_t wbuf[XFER_BUF_MAX];
	uint8_t rbuf[XFER_BUF_MAX];
	char out[(XFER_BUF_MAX * 2U) + 1U];
	struct i3c_msg msgs[2];
	long want = strtol(len_str, NULL, 0);
	int n = hex_to_bytes(hex, wbuf, sizeof(wbuf));
	int rc;

	if ((n < 0) || (want < 0) || (want > (long)sizeof(rbuf))) {
		sync_send(sync_uart, "DONE xfer rc=%d", -EINVAL);
		return;
	}

	memset(rbuf, 0, sizeof(rbuf));

	msgs[0].buf = wbuf;
	msgs[0].len = (uint32_t)n;
	msgs[0].flags = I3C_MSG_WRITE;

	msgs[1].buf = rbuf;
	msgs[1].len = (uint32_t)want;
	msgs[1].flags = I3C_MSG_READ | I3C_MSG_RESTART | I3C_MSG_STOP;

	rc = i3c_transfer(&target_b_desc, msgs, 2);

	bytes_to_hex(rbuf, (size_t)want, out, sizeof(out));
	sync_send(sync_uart, "DONE xfer rc=%d %s", rc, out);
}

static void handle_re_enum(void)
{
	int rc = enumerate_bus();

	sync_send(sync_uart, "ENUM da=%02x rc=%d", target_b_desc.dynamic_addr, rc);
}

static void handle_recover(void)
{
	sync_send(sync_uart, "DONE recover rc=%d", i3c_recover_bus(i3c_dev));
}

#ifdef CONFIG_I3C_USE_IBI
static struct k_sem ibi_sem;
static uint8_t last_ibi_payload[CONFIG_I3C_IBI_MAX_PAYLOAD_SIZE];
static uint8_t last_ibi_len;

static int ibi_cb(struct i3c_device_desc *target, struct i3c_ibi_payload *payload)
{
	ARG_UNUSED(target);

	if (payload != NULL) {
		last_ibi_len = MIN(payload->payload_len, sizeof(last_ibi_payload));
		memcpy(last_ibi_payload, payload->payload, last_ibi_len);
	} else {
		last_ibi_len = 0;
	}
	k_sem_give(&ibi_sem);
	return 0;
}

static void handle_set_ibi(const char *onoff)
{
	int rc;

	if (strcmp(onoff, "on") == 0) {
		k_sem_reset(&ibi_sem);
		target_b_desc.ibi_cb = ibi_cb;
		rc = i3c_ibi_enable(&target_b_desc);
		if (rc != 0) {
			target_b_desc.ibi_cb = NULL;
		}
	} else {
		rc = i3c_ibi_disable(&target_b_desc);
		target_b_desc.ibi_cb = NULL;
	}

	sync_send(sync_uart, "DONE ibi rc=%d", rc);
}

/*
 * Blocks for up to <ms> so the target can raise while we are waiting;
 * NO_IBI is a result, not an error.
 */
static void handle_expect_ibi(const char *ms_str)
{
	char hex[(CONFIG_I3C_IBI_MAX_PAYLOAD_SIZE * 2U) + 1U];
	long ms = strtol(ms_str, NULL, 0);

	if (k_sem_take(&ibi_sem, K_MSEC(ms)) != 0) {
		sync_send(sync_uart, "NO_IBI");
		return;
	}

	if (bytes_to_hex(last_ibi_payload, last_ibi_len, hex, sizeof(hex)) != 0) {
		hex[0] = '\0';
	}
	sync_send(sync_uart, "GOT_IBI len=%u %s", last_ibi_len, hex);
}

static void handle_set_hj(const char *onoff)
{
	int rc = i3c_ibi_hj_response(i3c_dev, strcmp(onoff, "on") == 0);

	sync_send(sync_uart, "DONE hj rc=%d", rc);
}

/*
 * The IBI workqueue answers a hot join by running DAA, so B reappearing with a
 * dynamic address is the only application-visible proof it was accepted.
 */
static void handle_expect_hj(const char *ms_str)
{
	k_timepoint_t deadline = sys_timepoint_calc(K_MSEC(strtol(ms_str, NULL, 0)));

	while (!sys_timepoint_expired(deadline)) {
		if (target_b_desc.dynamic_addr != 0U) {
			sync_send(sync_uart, "GOT_HJ da=%u", target_b_desc.dynamic_addr);
			return;
		}
		k_msleep(10);
	}

	sync_send(sync_uart, "NO_HJ");
}
#endif /* CONFIG_I3C_USE_IBI */

static void process_line(char *line)
{
	char *cmd = strtok(line, " ");

	if (cmd == NULL) {
		return;
	}

	if (strcmp(cmd, "DO_WRITE") == 0) {
		char *hex = strtok(NULL, " ");

		handle_do_write((hex != NULL) ? hex : "");
	} else if (strcmp(cmd, "BURST_WRITE") == 0) {
		char *count = strtok(NULL, " ");
		char *len = strtok(NULL, " ");
		char *base = strtok(NULL, " ");

		handle_burst_write((count != NULL) ? count : "0", (len != NULL) ? len : "0",
				   (base != NULL) ? base : "0");
	} else if (strcmp(cmd, "DO_READ") == 0) {
		char *len = strtok(NULL, " ");

		handle_do_read((len != NULL) ? len : "0");
	} else if (strcmp(cmd, "DO_WRITE_ADDR") == 0) {
		char *addr = strtok(NULL, " ");
		char *hex = strtok(NULL, " ");

		handle_do_write_addr((addr != NULL) ? addr : "0", (hex != NULL) ? hex : "");
	} else if (strcmp(cmd, "DO_READ_ADDR") == 0) {
		char *addr = strtok(NULL, " ");
		char *len = strtok(NULL, " ");

		handle_do_read_addr((addr != NULL) ? addr : "0", (len != NULL) ? len : "0");
	} else if (strcmp(cmd, "DO_WRITE_ABORT") == 0) {
		char *len = strtok(NULL, " ");
		char *delay = strtok(NULL, " ");

		handle_do_write_abort((len != NULL) ? len : "0", (delay != NULL) ? delay : "0");
	} else if (strcmp(cmd, "A_DUMP") == 0) {
		handle_a_dump();
	} else if (strcmp(cmd, "DO_CCC") == 0) {
		char *name = strtok(NULL, " ");
		char *arg = strtok(NULL, " ");

		handle_do_ccc((name != NULL) ? name : "", (arg != NULL) ? arg : "0");
	} else if (strcmp(cmd, "DO_RAW_CCC") == 0) {
		char *id = strtok(NULL, " ");
		char *rnw = strtok(NULL, " ");
		char *len = strtok(NULL, " ");

		handle_do_raw_ccc((id != NULL) ? id : "0", (rnw != NULL) ? rnw : "0",
				  (len != NULL) ? len : "0");
	} else if (strcmp(cmd, "DO_XFER") == 0) {
		char *hex = strtok(NULL, " ");
		char *len = strtok(NULL, " ");

		handle_do_xfer((hex != NULL) ? hex : "", (len != NULL) ? len : "0");
	} else if (strcmp(cmd, "RE_ENUM") == 0) {
		handle_re_enum();
	} else if (strcmp(cmd, "RECOVER") == 0) {
		handle_recover();
#ifdef CONFIG_I3C_USE_IBI
	} else if (strcmp(cmd, "SET_IBI") == 0) {
		char *onoff = strtok(NULL, " ");

		handle_set_ibi((onoff != NULL) ? onoff : "off");
	} else if (strcmp(cmd, "EXPECT_IBI") == 0) {
		char *ms = strtok(NULL, " ");

		handle_expect_ibi((ms != NULL) ? ms : "1000");
	} else if (strcmp(cmd, "SET_HJ") == 0) {
		char *onoff = strtok(NULL, " ");

		handle_set_hj((onoff != NULL) ? onoff : "off");
	} else if (strcmp(cmd, "EXPECT_HJ") == 0) {
		char *ms = strtok(NULL, " ");

		handle_expect_hj((ms != NULL) ? ms : "1000");
#endif
	} else if (strcmp(cmd, "HELLO") == 0) {
		/*
		 * Board B greets until it is enumerated, so the greetings that
		 * queued up behind this one ask for what this ENUM already
		 * answers. Drop them, or they re-enumerate the bus mid-suite.
		 */
		int enum_rc = enumerate_bus();

		sync_drain(sync_uart);
		sync_send(sync_uart, "ENUM da=%02x rc=%d", target_b_desc.dynamic_addr, enum_rc);
	} else {
		LOG_WRN("unknown sync cmd: %s", cmd);
	}
}

int main(void)
{
	char line[SYNC_LINE_MAX];

	if (!device_is_ready(i3c_dev)) {
		LOG_ERR("I3C controller %s not ready", i3c_dev->name);
		return -ENODEV;
	}
	if (!device_is_ready(sync_uart)) {
		LOG_ERR("Sync UART %s not ready", sync_uart->name);
		return -ENODEV;
	}

#ifdef CONFIG_I3C_USE_IBI
	k_sem_init(&ibi_sem, 0, 1);
#endif

	/* Drain RX junk accumulated while Board B was being flashed. */
	sync_drain(sync_uart);

	/*
	 * Do not enumerate here: Board B greets once it has configured target
	 * mode, and the HELLO handler enumerates then. Enumerating now would
	 * address a target that is not listening yet and poison every later
	 * transfer.
	 */
	for (;;) {
		if (sync_recv_line(sync_uart, line, sizeof(line), K_FOREVER) < 0) {
			continue;
		}
		process_line(line);
	}

	return 0;
}
