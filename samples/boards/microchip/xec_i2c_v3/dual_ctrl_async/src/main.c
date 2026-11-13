/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Dual-controller asynchronous I2C sample.
 *
 * Two application threads, one per hardware controller, each fire the async
 * (CONFIG_I2C_CALLBACK) transfer API at its own device on its own physical
 * port:
 *
 *   Thread A: i2c_smb_0 / port 0 -> PCA9555 @0x26  (100 kHz)
 *   Thread B: i2c_smb_1 / port 7 -> FRAM   @0x50  (400 kHz)
 *
 * Both drivers (byte-mode and network-layer) service async completions from a
 * single shared kernel work queue used by every controller instance. Running
 * two controllers concurrently exercises that shared queue and each
 * controller's independent per-transfer watchdog / single-delivery claim,
 * which no single-threaded sample stresses. Each thread owns its buffers,
 * message array and completion object - nothing is shared across threads.
 *
 * After a fixed number of iterations per thread, main() aggregates the error
 * counts and prints a PASS/FAIL verdict, returning non-zero on any failure.
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/dt-bindings/i2c/i2c.h>
#include <zephyr/random/random.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

/* PCA9555 command (register) bytes */
#define PCA9555_CMD_PORT0_IN 0U

/* PCA9555 input-port value with the EVB's default jumpers (see overlay). */
#define PCA9555_PORT0_IN_EXPECTED 0xfffcU

/* Iterations each thread runs. */
#define ASYNC_ITERS 10000U

/* Per-transfer completion timeout. */
#define I2C_CB_TIMEOUT K_MSEC(2000)

/* How long main() waits for a worker thread to finish its whole run. At
 * 400 kHz a max-size FRAM round trip is ~12 ms, so 10000 iterations stay well
 * under this bound; a timeout here means a thread is stuck -> FAIL.
 */
#define THREAD_JOIN_TIMEOUT K_MINUTES(10)

#define THREAD_STACK_SIZE 2048
#define THREAD_PRIORITY   5

#define NODE_PCA9555 DT_NODELABEL(pca9555_evb)
#define NODE_FRAM    DT_NODELABEL(mb85rc256v_fram)

/* FRAM payload bounds: 2-byte offset header + up to 254 data bytes. */
#define FRAM_MAX_DATA 254U
#define BUF_SIZE      256U
#define MAX_MSGS      4U

/* Per-transfer completion object (one per thread). */
struct app_i2c_cb_s {
	struct k_sem sem;
	volatile uint32_t count;
	volatile int result;
};

/* Everything a worker thread touches. Buffers/msgs/cb are per-thread so the
 * two controllers never share transfer state.
 */
struct ctrl_thread_ctx {
	const char *name;
	struct i2c_dt_spec dev;
	struct app_i2c_cb_s cb;
	struct i2c_msg msgs[MAX_MSGS];
	uint8_t tx[BUF_SIZE];
	uint8_t rx[BUF_SIZE];
	uint32_t iters;
	uint64_t errors;
	struct k_sem done;
};

static struct ctrl_thread_ctx ctx_a = {
	.name = "A:PCA9555",
	.dev = I2C_DT_SPEC_GET(NODE_PCA9555),
	.iters = ASYNC_ITERS,
};

static struct ctrl_thread_ctx ctx_b = {
	.name = "B:FRAM",
	.dev = I2C_DT_SPEC_GET(NODE_FRAM),
	.iters = ASYNC_ITERS,
};

static K_THREAD_STACK_DEFINE(stack_a, THREAD_STACK_SIZE);
static K_THREAD_STACK_DEFINE(stack_b, THREAD_STACK_SIZE);
static struct k_thread thread_a;
static struct k_thread thread_b;

static void app_i2c_cb_init(struct app_i2c_cb_s *p)
{
	k_sem_init(&p->sem, 0, 1);
	p->count = 0;
	p->result = 0;
}

static void app_i2c_cb_prep(struct app_i2c_cb_s *p)
{
	k_sem_reset(&p->sem);
	p->count = 0;
	p->result = 0;
}

/* Runs on the driver's shared work-queue thread, not in ISR context. */
static void app_i2c_cb_func(const struct device *i2c_port_dev, int result, void *data)
{
	struct app_i2c_cb_s *cbs = data;

	ARG_UNUSED(i2c_port_dev);

	if (cbs != NULL) {
		cbs->count++;
		cbs->result = result;
		k_sem_give(&cbs->sem);
	}
}

/* Submit an async transfer, wait for its completion semaphore, and fold the
 * take result and the callback result into a single return code.
 */
static int wait_cb(struct ctrl_thread_ctx *c)
{
	int rc = k_sem_take(&c->cb.sem, I2C_CB_TIMEOUT);

	switch (rc) {
	case 0:
		break;
	case -EAGAIN:
		LOG_ERR("%s: completion timeout (-EAGAIN)", c->name);
		return -ETIMEDOUT;
	default:
		LOG_ERR("%s: sem take error (%d)", c->name, rc);
		return rc;
	}

	if (c->cb.result != 0) {
		LOG_ERR("%s: transfer callback error (%d)", c->name, c->cb.result);
		return c->cb.result;
	}

	return 0;
}

/* Thread A: async read of the PCA9555 input port, verify against the expected
 * (jumper-dependent) value.
 */
static int test_pca9555(struct ctrl_thread_ctx *c)
{
	uint16_t val;
	int rc;

	memset(c->tx, 0x55, sizeof(c->tx));
	memset(c->rx, 0xAA, sizeof(c->rx));
	c->tx[0] = PCA9555_CMD_PORT0_IN;

	app_i2c_cb_prep(&c->cb);

	rc = i2c_write_read_cb_dt(&c->dev, c->msgs, 2U, (const void *)c->tx, 1U, (void *)c->rx, 2U,
				  app_i2c_cb_func, (void *)&c->cb);
	if (rc != 0) {
		LOG_ERR("%s: write_read_cb submit error (%d)", c->name, rc);
		return rc;
	}

	rc = wait_cb(c);
	if (rc != 0) {
		return rc;
	}

	val = ((uint16_t)c->rx[1] << 8) | c->rx[0];
	if (val != PCA9555_PORT0_IN_EXPECTED) {
		LOG_ERR("%s: input port = 0x%04x expected 0x%04x", c->name, val,
			PCA9555_PORT0_IN_EXPECTED);
		return -EBADMSG;
	}

	return 0;
}

/* Random FRAM offset (safely below the 32 KiB top) and data length. */
static void fram_random(uint16_t *ofs, uint32_t *nbytes)
{
	uint32_t temp;
	uint16_t o;
	uint32_t nb = 0;

	do {
		sys_rand_get(&temp, sizeof(temp));
		o = (uint16_t)temp;
	} while (o >= (0x8000U - 0x100U));

	while ((nb == 0U) || (nb > FRAM_MAX_DATA)) {
		sys_rand_get(&temp, sizeof(temp));
		nb = temp & 0xffU;
	}

	*ofs = o;
	*nbytes = nb;
}

/* Thread B: async write random data to the FRAM, async read it back, compare. */
static int test_fram(struct ctrl_thread_ctx *c)
{
	uint16_t ofs;
	uint32_t nbytes;
	int rc;

	fram_random(&ofs, &nbytes);

	memset(c->tx, 0x55, sizeof(c->tx));
	memset(c->rx, 0xAA, sizeof(c->rx));
	c->tx[0] = (uint8_t)(ofs >> 8);
	c->tx[1] = (uint8_t)(ofs);
	sys_rand_get(&c->tx[2], nbytes);

	/* Async write: offset header + data. */
	app_i2c_cb_prep(&c->cb);
	c->msgs[0].buf = c->tx;
	c->msgs[0].len = nbytes + 2U;
	c->msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	rc = i2c_transfer_cb(c->dev.bus, c->msgs, 1U, c->dev.addr, app_i2c_cb_func, (void *)&c->cb);
	if (rc != 0) {
		LOG_ERR("%s: write submit error (%d)", c->name, rc);
		return rc;
	}

	rc = wait_cb(c);
	if (rc != 0) {
		return rc;
	}

	/* Async write-offset then read-back. */
	c->tx[0] = (uint8_t)(ofs >> 8);
	c->tx[1] = (uint8_t)(ofs);

	app_i2c_cb_prep(&c->cb);

	rc = i2c_write_read_cb_dt(&c->dev, c->msgs, 2U, (const void *)c->tx, 2U, (void *)c->rx,
				  nbytes, app_i2c_cb_func, (void *)&c->cb);
	if (rc != 0) {
		LOG_ERR("%s: write_read_cb submit error (%d)", c->name, rc);
		return rc;
	}

	rc = wait_cb(c);
	if (rc != 0) {
		return rc;
	}

	if (memcmp(&c->tx[2], c->rx, nbytes) != 0) {
		LOG_ERR("%s: read-back mismatch (ofs 0x%04x, %u bytes)", c->name, ofs, nbytes);
		return -EBADMSG;
	}

	return 0;
}

/* Common worker: run `iters` of `test_fn`, tally errors, signal main. */
static void worker(struct ctrl_thread_ctx *c, int (*test_fn)(struct ctrl_thread_ctx *ctx))
{
	uint64_t logged = 0;

	LOG_INF("%s: start %u async iterations on %s", c->name, c->iters, c->dev.bus->name);

	for (uint32_t i = 0; i < c->iters; i++) {
		if (test_fn(c) != 0) {
			c->errors++;
		}

		if (c->errors != logged) {
			logged = c->errors;
			LOG_ERR("%s: error count %llu", c->name, c->errors);
		}
	}

	LOG_INF("%s: done, %llu errors in %u iterations", c->name, c->errors, c->iters);
	k_sem_give(&c->done);
}

static void thread_a_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	worker((struct ctrl_thread_ctx *)p1, test_pca9555);
}

static void thread_b_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	worker((struct ctrl_thread_ctx *)p1, test_fram);
}

/* Verify a device spec's bus is ready. */
static bool ctx_ready(struct ctrl_thread_ctx *c)
{
	if (!device_is_ready(c->dev.bus)) {
		LOG_ERR("%s: I2C bus %s NOT ready", c->name,
			(c->dev.bus != NULL) ? c->dev.bus->name : "<null>");
		return false;
	}

	LOG_INF("%s: I2C bus %s ready (addr 0x%02x)", c->name, c->dev.bus->name, c->dev.addr);
	return true;
}

int main(void)
{
	bool pass;
	int join;

#ifdef CONFIG_BOARD_QUALIFIERS
	LOG_INF("Microchip XEC I2Cv3 dual_ctrl_async: board %s/%s", CONFIG_BOARD,
		CONFIG_BOARD_QUALIFIERS);
#else
	LOG_INF("Microchip XEC I2Cv3 dual_ctrl_async: board %s", CONFIG_BOARD);
#endif

	app_i2c_cb_init(&ctx_a.cb);
	app_i2c_cb_init(&ctx_b.cb);
	k_sem_init(&ctx_a.done, 0, 1);
	k_sem_init(&ctx_b.done, 0, 1);

	if (!ctx_ready(&ctx_a) || !ctx_ready(&ctx_b)) {
		LOG_ERR("==== DUAL-CTRL ASYNC: FAIL (device not ready) ====");
		log_flush();
		return -ENODEV;
	}

	log_flush();

	/* Launch both controllers concurrently. */
	k_thread_create(&thread_a, stack_a, K_THREAD_STACK_SIZEOF(stack_a), thread_a_entry, &ctx_a,
			NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&thread_a, "i2c_a");

	k_thread_create(&thread_b, stack_b, K_THREAD_STACK_SIZEOF(stack_b), thread_b_entry, &ctx_b,
			NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&thread_b, "i2c_b");

	/* Wait for both to finish (or time out -> FAIL). */
	pass = true;

	join = k_sem_take(&ctx_a.done, THREAD_JOIN_TIMEOUT);
	if (join != 0) {
		LOG_ERR("%s: thread join timeout (%d)", ctx_a.name, join);
		pass = false;
	}

	join = k_sem_take(&ctx_b.done, THREAD_JOIN_TIMEOUT);
	if (join != 0) {
		LOG_ERR("%s: thread join timeout (%d)", ctx_b.name, join);
		pass = false;
	}

	if (ctx_a.errors != 0U || ctx_b.errors != 0U) {
		pass = false;
	}

	if (pass) {
		LOG_INF("==== DUAL-CTRL ASYNC: PASS (A err=%llu B err=%llu, %u iters each) ====",
			ctx_a.errors, ctx_b.errors, ASYNC_ITERS);
	} else {
		LOG_ERR("==== DUAL-CTRL ASYNC: FAIL (A err=%llu B err=%llu) ====", ctx_a.errors,
			ctx_b.errors);
	}

	log_flush();

	return pass ? 0 : -EIO;
}
