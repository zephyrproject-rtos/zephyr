/*
 * Copyright (c) 2026 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief I2C transport for the MCUmgr SMP protocol, target and controller.
 *
 * The wire carries the raw SMP packet with nothing added. The packet length is
 * taken from the SMP header (nh_len plus the header size), so no extra framing
 * is needed.
 *
 * The target answers SMP requests as an I2C target device. A request can arrive
 * in one I2C write or be split over several, and the common reassembly code
 * chains the pieces together, so a device with a small I2C buffer can still
 * receive a large message. A request that stays incomplete is dropped after a
 * timeout, so the target recovers when the controller reboots or sends corrupt
 * data. A response is read back in a single transaction.
 *
 * The controller drives a remote target as the I2C master and is used through
 * the SMP client API. Both roles live here and can be enabled together, for
 * example a gateway that is a target on one bus and a controller on another.
 *
 * When the controller reads before a response is staged it gets an all-zero
 * (empty) SMP header, which it treats as "not ready" and retries, or it waits
 * on an optional data-ready GPIO instead of polling. A real response never has
 * an all-zero header, because its operation field is a response code.
 */

#include <errno.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/mcumgr/mgmt/handlers.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>

#include <mgmt/mcumgr/transport/smp_internal.h>
#include <mgmt/mcumgr/transport/smp_reassembly.h>

LOG_MODULE_DECLARE(mcumgr_smp, CONFIG_MCUMGR_TRANSPORT_LOG_LEVEL);

#define SMP_I2C_HDR_SIZE sizeof(struct smp_hdr)

/* Full packet length (header and payload) as declared by an SMP header. */
#define SMP_I2C_PACKET_LEN(hdr) \
	((size_t)sys_be16_to_cpu(((const struct smp_hdr *)(hdr))->nh_len) + SMP_I2C_HDR_SIZE)

/* Target transport */

#if defined(CONFIG_MCUMGR_TRANSPORT_I2C)

#define DT_DRV_COMPAT zephyr_smp_i2c

/* This role keeps its state in module-level variables, so only one target
 * node is supported for now.
 */
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "Only one zephyr,smp-i2c node is supported");

static const struct device *const i2c_dev = DEVICE_DT_GET(DT_BUS(DT_DRV_INST(0)));

#if DT_INST_NODE_HAS_PROP(0, data_ready_gpios)
#define SMP_I2C_HAS_DRDY 1
static const struct gpio_dt_spec drdy_gpio = GPIO_DT_SPEC_INST_GET(0, data_ready_gpios);
#else
#define SMP_I2C_HAS_DRDY 0
#endif

/*
 * RX state. The ISR callbacks and the timeout work item both touch the
 * reassembly context, so rx_lock guards it along with the fields below.
 *
 * In byte mode the data arrives one byte at a time, but the reassembly code
 * needs the whole SMP header in the first chunk to learn the message length, so
 * rx_hdr collects those first bytes. rx_in_write is set while a write
 * transaction is in progress, so the timeout does not fire in the middle of one.
 * rx_discard is set once a message is complete or was rejected, and makes the
 * rest of that write transaction be ignored instead of read as a new message.
 */
static struct k_spinlock rx_lock;
static uint8_t rx_hdr[SMP_I2C_HDR_SIZE];
static uint8_t rx_hdr_len;
static bool rx_in_write;
static bool rx_discard;
static struct k_work_delayable rx_timeout_work;

/*
 * TX state is shared between the SMP workqueue thread and the I2C ISR
 * callbacks. It uses a spinlock rather than a mutex, because a mutex cannot be
 * taken from an ISR.
 *
 * tx_buf holds the raw SMP response. In buffer mode it is the DMA source, so it
 * lives in RAM. A staged response stays until the next request clears it, not
 * when a read starts, because the controller always reads a response before
 * sending the next request. tx_reading means a READ is draining tx_buf. A READ
 * that starts with no response staged serves zeros for its whole length, so a
 * response staged part way through cannot corrupt a read already running.
 */
static struct k_spinlock tx_lock;
static uint8_t tx_buf[CONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE];
static uint16_t tx_len;  /* raw SMP response bytes staged in tx_buf */
static uint16_t tx_pos;  /* next tx_buf index for the current READ */
static bool tx_ready;    /* a response is staged in tx_buf */
static bool tx_reading;  /* the current READ transaction is draining tx_buf */

#if defined(CONFIG_I2C_TARGET_BUFFER_MODE)
/*
 * Returned by buf_read_requested when no response is pending. It is an all-zero
 * SMP header and must be in RAM, because DMA cannot source from .rodata on some
 * platforms.
 */
static uint8_t zero_hdr[SMP_I2C_HDR_SIZE];
#endif

static struct smp_transport smp_i2c_transport;

/*
 * Drive the data-ready GPIO. Called from the I2C target ISR, so the line must
 * be an on-SoC GPIO whose set does not sleep (no off-chip expander).
 */
static inline void drdy_set(int val)
{
#if SMP_I2C_HAS_DRDY
	(void)gpio_pin_set_dt(&drdy_gpio, val);
#else
	ARG_UNUSED(val);
#endif
}

/* Caller holds tx_lock. Drops any staged response and returns true if one was staged. */
static bool tx_discard_locked(void)
{
	bool was_ready = tx_ready;

	tx_ready = false;
	tx_reading = false;
	tx_len = 0U;
	tx_pos = 0U;

	return was_ready;
}

/*
 * Feed received data into the reassembly context, handing the request to MCUmgr
 * once it is complete. The first chunk of a message must hold the whole SMP
 * header. Bytes past the end of the message are ignored, as the SPI transport
 * does. Returns true when the rest of the current write transaction should be
 * ignored, because the message is complete or was rejected. Caller holds rx_lock.
 */
static bool target_rx_chunk_locked(const uint8_t *data, size_t len)
{
	int expected = smp_reassembly_expected(&smp_i2c_transport);
	size_t chunk;
	int rc;

	if (expected < 0) {
		if (len < SMP_I2C_HDR_SIZE) {
			LOG_WRN("first chunk shorter than an SMP header: %u", (uint32_t)len);
			return true;
		}

		chunk = MIN(len, SMP_I2C_PACKET_LEN(data));
	} else {
		chunk = MIN(len, (size_t)expected);
	}

	rc = smp_reassembly_collect(&smp_i2c_transport, data, chunk);
	if (rc < 0) {
		LOG_WRN("reassembly failed: %d", rc);
		(void)smp_reassembly_drop(&smp_i2c_transport);
		return true;
	}

	if (rc == 0) {
		rc = smp_reassembly_complete(&smp_i2c_transport, false);
		if (rc != 0) {
			LOG_ERR("reassembly complete failed: %d", rc);
		}

		return true;
	}

	return false;
}

/*
 * Arm the timeout while a request is incomplete and cancel it once nothing is
 * pending. Caller holds rx_lock.
 */
static void target_rx_timer_update_locked(void)
{
	if (smp_reassembly_expected(&smp_i2c_transport) >= 0) {
		(void)k_work_reschedule(&rx_timeout_work,
					K_MSEC(CONFIG_MCUMGR_TRANSPORT_I2C_REASSEMBLY_TIMEOUT_MS));
	} else {
		(void)k_work_cancel_delayable(&rx_timeout_work);
	}
}

/*
 * No more data arrived for an incomplete request, for example because the
 * controller rebooted or the data was corrupt. Drop it so the next request is
 * not appended to it.
 */
static void target_rx_timeout(struct k_work *work)
{
	ARG_UNUSED(work);
	k_spinlock_key_t key;

	key = k_spin_lock(&rx_lock);

	if (rx_in_write != false) {
		/* A write is still in progress, so look again after another period. */
		(void)k_work_reschedule(&rx_timeout_work,
					K_MSEC(CONFIG_MCUMGR_TRANSPORT_I2C_REASSEMBLY_TIMEOUT_MS));
	} else if (k_work_delayable_remaining_get(&rx_timeout_work) == 0 &&
		   smp_reassembly_expected(&smp_i2c_transport) >= 0) {
		LOG_WRN("dropping incomplete request after %d ms",
			CONFIG_MCUMGR_TRANSPORT_I2C_REASSEMBLY_TIMEOUT_MS);
		(void)smp_reassembly_drop(&smp_i2c_transport);
	}

	k_spin_unlock(&rx_lock, key);
}

/* Byte-mode ISR callbacks */

static int smp_i2c_write_requested(struct i2c_target_config *cfg)
{
	ARG_UNUSED(cfg);
	k_spinlock_key_t key;
	bool drdy_clear;

	key = k_spin_lock(&rx_lock);
	rx_in_write = true;
	rx_discard = false;
	rx_hdr_len = 0U;
	k_spin_unlock(&rx_lock, key);

	/*
	 * A WRITE cannot arrive while a READ is still in progress, so a new
	 * WRITE means any earlier response has already been read. Drop it here.
	 */
	key = k_spin_lock(&tx_lock);
	drdy_clear = tx_discard_locked();
	k_spin_unlock(&tx_lock, key);

	if (drdy_clear != false) {
		drdy_set(0);
	}

	return 0;
}

static int smp_i2c_write_received(struct i2c_target_config *cfg, uint8_t val)
{
	ARG_UNUSED(cfg);
	k_spinlock_key_t key;

	key = k_spin_lock(&rx_lock);

	if (rx_discard == false) {
		if (smp_reassembly_expected(&smp_i2c_transport) >= 0) {
			rx_discard = target_rx_chunk_locked(&val, 1U);
		} else {
			rx_hdr[rx_hdr_len++] = val;

			if (rx_hdr_len == SMP_I2C_HDR_SIZE) {
				rx_hdr_len = 0U;
				rx_discard = target_rx_chunk_locked(rx_hdr, SMP_I2C_HDR_SIZE);
			}
		}
	}

	k_spin_unlock(&rx_lock, key);

	return 0;
}

static int smp_i2c_read_requested(struct i2c_target_config *cfg, uint8_t *val)
{
	ARG_UNUSED(cfg);
	k_spinlock_key_t key;

	key = k_spin_lock(&tx_lock);
	tx_reading = tx_ready;
	tx_pos = 0U;
	*val = tx_reading ? tx_buf[tx_pos++] : 0x00U;
	k_spin_unlock(&tx_lock, key);

	return 0;
}

static int smp_i2c_read_processed(struct i2c_target_config *cfg, uint8_t *val)
{
	ARG_UNUSED(cfg);
	k_spinlock_key_t key;

	key = k_spin_lock(&tx_lock);
	if (tx_reading && tx_pos < tx_len) {
		*val = tx_buf[tx_pos++];
	} else {
		*val = 0x00U;
	}
	k_spin_unlock(&tx_lock, key);

	return 0;
}

/*
 * stop() fires at the end of every I2C transaction. At the end of a WRITE it
 * restarts the incomplete-request timeout if the request is not complete yet.
 * At the end of a READ (byte mode) it drops the staged response and deasserts
 * DRDY once the controller has clocked out every byte. A READ leaves the
 * timeout alone, so a controller that only polls cannot keep a stale partial
 * request alive.
 *
 * Some buffer-mode target drivers do not call stop() after a READ, so the
 * staged response is held until the next request clears it in
 * smp_i2c_buf_write_received().
 */
static int smp_i2c_stop(struct i2c_target_config *cfg)
{
	ARG_UNUSED(cfg);
	k_spinlock_key_t key;
	bool drdy_clear = false;

	key = k_spin_lock(&rx_lock);
	if (rx_in_write != false) {
		rx_in_write = false;
		rx_discard = false;
		rx_hdr_len = 0U;
		target_rx_timer_update_locked();
	}
	k_spin_unlock(&rx_lock, key);

	key = k_spin_lock(&tx_lock);

	if (tx_reading && tx_pos >= tx_len) {
		/* Controller clocked the whole frame out. */
		drdy_clear = tx_discard_locked();
	}
	tx_reading = false;

	k_spin_unlock(&tx_lock, key);

	if (drdy_clear != false) {
		drdy_set(0);
		LOG_DBG("response consumed");
	}

	return 0;
}

/* Buffer-mode ISR callbacks */

#if defined(CONFIG_I2C_TARGET_BUFFER_MODE)

static void smp_i2c_buf_write_received(struct i2c_target_config *cfg, uint8_t *ptr, uint32_t len)
{
	ARG_UNUSED(cfg);
	k_spinlock_key_t key;
	bool drdy_clear;

	/*
	 * A new request means the controller has already read any staged
	 * response, so drop it now. In buffer mode this is where the response is
	 * cleared, since a buffer-mode driver may not call stop() after a READ.
	 */
	key = k_spin_lock(&tx_lock);
	drdy_clear = tx_discard_locked();
	k_spin_unlock(&tx_lock, key);
	if (drdy_clear != false) {
		drdy_set(0);
	}

	key = k_spin_lock(&rx_lock);
	(void)target_rx_chunk_locked(ptr, len);
	target_rx_timer_update_locked();
	k_spin_unlock(&rx_lock, key);
}

static int smp_i2c_buf_read_requested(struct i2c_target_config *cfg, uint8_t **ptr, uint32_t *len)
{
	ARG_UNUSED(cfg);
	k_spinlock_key_t key;
	bool do_drdy_clear = false;

	key = k_spin_lock(&tx_lock);

	if (tx_ready == true) {
		*ptr = tx_buf;
		*len = tx_len;
		/*
		 * Keep the response latched. A buffer-mode target copies tx_buf
		 * into its own DMA buffer here but may never call stop() after a
		 * READ, and the controller reads the frame twice (the header,
		 * then the whole packet). It is cleared only on the next request.
		 * DRDY is an edge hint, so deassert it on the first read.
		 */
		do_drdy_clear = true;
	} else {
		*ptr = zero_hdr;
		*len = SMP_I2C_HDR_SIZE;
	}

	k_spin_unlock(&tx_lock, key);

	/* Deassert DRDY outside the spinlock to keep the ISR critical section
	 * short (see drdy_set, which needs an on-SoC GPIO).
	 */
	if (do_drdy_clear == true) {
		drdy_set(0);
	}

	return 0;
}

#endif /* CONFIG_I2C_TARGET_BUFFER_MODE */

/* Each I2C target driver uses only the callback set it supports. */
static const struct i2c_target_callbacks smp_i2c_callbacks = {
	.write_requested = smp_i2c_write_requested,
	.write_received = smp_i2c_write_received,
	.read_requested = smp_i2c_read_requested,
	.read_processed = smp_i2c_read_processed,
	.stop = smp_i2c_stop,
#if defined(CONFIG_I2C_TARGET_BUFFER_MODE)
	.buf_write_received = smp_i2c_buf_write_received,
	.buf_read_requested = smp_i2c_buf_read_requested,
#endif
};

static struct i2c_target_config smp_i2c_target_cfg = {
	.address = DT_REG_ADDR(DT_DRV_INST(0)),
	.callbacks = &smp_i2c_callbacks,
};

/*
 * The MCUmgr workqueue calls smp_i2c_tx_pkt() to stage a raw SMP response.
 *
 * While a byte-mode READ is draining tx_buf (tx_reading set) it returns -EBUSY
 * instead of corrupting the transfer, and the SMP client retries after
 * CONFIG_SMP_CMD_RETRY_TIME. In buffer mode tx_reading is never set, because
 * the controller reads a response before sending the next request, so staging
 * is always safe.
 */
static int smp_i2c_tx_pkt(struct net_buf *nb)
{
	k_spinlock_key_t key;
	uint16_t pkt_len = nb->len;

	if (pkt_len > sizeof(tx_buf)) {
		LOG_ERR("response too large: %u > %u", pkt_len, (uint32_t)sizeof(tx_buf));
		smp_packet_free(nb);
		return -EMSGSIZE;
	}

	key = k_spin_lock(&tx_lock);

	if (tx_reading) {
		/*
		 * A byte-mode READ is clocking bytes out of tx_buf right now, and
		 * overwriting it would tear the active transfer. This transport
		 * holds only one response at a time.
		 */
		k_spin_unlock(&tx_lock, key);
		LOG_WRN("read in progress; dropping new response");
		smp_packet_free(nb);
		return -EBUSY;
	}

	memcpy(tx_buf, nb->data, pkt_len);
	tx_len = pkt_len;
	tx_pos = 0U;
	tx_ready = true;

	k_spin_unlock(&tx_lock, key);

	/* DRDY goes high only after tx_ready is set. */
	drdy_set(1);

	LOG_DBG("staged %u byte response", pkt_len);
	smp_packet_free(nb);

	return 0;
}

static void smp_i2c_start(void)
{
	int rc;

	if (!device_is_ready(i2c_dev)) {
		LOG_ERR("I2C device not ready");
		return;
	}

#if SMP_I2C_HAS_DRDY
	if (!gpio_is_ready_dt(&drdy_gpio)) {
		LOG_ERR("data-ready GPIO not ready");
		return;
	}

	rc = gpio_pin_configure_dt(&drdy_gpio, GPIO_OUTPUT_INACTIVE);
	if (rc != 0) {
		LOG_ERR("data-ready GPIO configure failed: %d", rc);
		return;
	}
#endif

	k_work_init_delayable(&rx_timeout_work, target_rx_timeout);

	smp_i2c_transport.functions.output = smp_i2c_tx_pkt;

	rc = smp_transport_init(&smp_i2c_transport);
	if (rc != 0) {
		LOG_ERR("smp_transport_init failed: %d", rc);
		return;
	}

	rc = i2c_target_register(i2c_dev, &smp_i2c_target_cfg);
	if (rc != 0) {
		LOG_ERR("i2c_target_register failed: %d", rc);
	}
}

MCUMGR_HANDLER_DEFINE(smp_i2c, smp_i2c_start);

#undef DT_DRV_COMPAT

#endif /* CONFIG_MCUMGR_TRANSPORT_I2C */

/* Controller transport */

#if defined(CONFIG_MCUMGR_TRANSPORT_I2C_CLIENT)

#define DT_DRV_COMPAT zephyr_smp_i2c_client

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "Only one zephyr,smp-i2c-client node is supported");

static const struct i2c_dt_spec cli_peer = I2C_DT_SPEC_INST_GET(0);

#if DT_INST_NODE_HAS_PROP(0, data_ready_gpios)
#define SMP_I2C_CLIENT_HAS_DRDY 1
static const struct gpio_dt_spec cli_drdy_gpio = GPIO_DT_SPEC_INST_GET(0, data_ready_gpios);
static struct gpio_callback cli_drdy_cb;
static K_SEM_DEFINE(cli_drdy_sem, 0, 1);

static void cli_drdy_isr(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);
	k_sem_give(&cli_drdy_sem);
}
#else
#define SMP_I2C_CLIENT_HAS_DRDY 0
#endif

/*
 * True when the first SMP_I2C_HDR_SIZE bytes are all zero. The target returns
 * such a header when it has no response staged, and a real response never does,
 * so the controller uses this as the "not ready" marker in poll mode.
 */
static bool smp_i2c_empty_header(const uint8_t *hdr)
{
	for (size_t i = 0U; i < SMP_I2C_HDR_SIZE; i++) {
		if (hdr[i] != 0U) {
			return false;
		}
	}

	return true;
}

/* Serialises the transactions, so only one request is in flight at a time. */
static K_MUTEX_DEFINE(cli_xfer_lock);

static struct smp_transport smp_i2c_client_transport;

static struct smp_client_transport_entry smp_i2c_client_entry = {
	.smpt = &smp_i2c_client_transport,
	.smpt_type = SMP_I2C_CLIENT_TRANSPORT,
};

/*
 * Wait until the target has a response staged. DRDY mode waits on the
 * semaphore and leaves *have_hdr false. Poll mode reads the SMP header until it
 * is non-empty and keeps that header in hdr, setting *have_hdr true, so the
 * caller does not read it a second time.
 */
static int cli_wait_for_response_ready(uint8_t *hdr, bool *have_hdr)
{
#if SMP_I2C_CLIENT_HAS_DRDY
	ARG_UNUSED(hdr);
	int rc;

	*have_hdr = false;
	rc = k_sem_take(&cli_drdy_sem, K_MSEC(CONFIG_MCUMGR_TRANSPORT_I2C_CLIENT_DRDY_TIMEOUT_MS));
	if (rc != 0) {
		LOG_WRN("DRDY timeout after %d ms",
			CONFIG_MCUMGR_TRANSPORT_I2C_CLIENT_DRDY_TIMEOUT_MS);
		return -ETIMEDOUT;
	}

	return 0;
#else
	int retries = CONFIG_MCUMGR_TRANSPORT_I2C_CLIENT_POLL_RETRIES;

	*have_hdr = false;

	while (retries-- > 0) {
		int rc = i2c_read_dt(&cli_peer, hdr, SMP_I2C_HDR_SIZE);

		if (rc == -EIO) {
			/*
			 * A target erasing flash can hold the bus busy and NACK
			 * reads until it is done. Recover the bus and keep polling
			 * within the budget instead of aborting on the first NACK.
			 */
			(void)i2c_recover_bus(cli_peer.bus);
		} else if (rc != 0) {
			LOG_ERR("poll read failed: %d", rc);
			return rc;
		} else if (!smp_i2c_empty_header(hdr)) {
			*have_hdr = true;
			return 0;
		}

		k_msleep(CONFIG_MCUMGR_TRANSPORT_I2C_CLIENT_POLL_MS);
	}

	LOG_WRN("response poll timeout");
	return -ETIMEDOUT;
#endif
}

/*
 * Called with cli_xfer_lock held. In poll mode hdr already holds the header
 * read while waiting (have_hdr true). In DRDY mode it is read here. The packet
 * is read straight into a net_buf, so no separate receive buffer is needed.
 */
static struct net_buf *cli_read_smp_response(uint8_t *hdr, bool have_hdr)
{
	struct net_buf *nb;
	size_t total;
	int rc;

	if (!have_hdr) {
		rc = i2c_read_dt(&cli_peer, hdr, SMP_I2C_HDR_SIZE);
		if (rc != 0) {
			LOG_ERR("header read failed: %d", rc);
			return NULL;
		}
	}

	if (smp_i2c_empty_header(hdr)) {
		LOG_WRN("target returned an empty header");
		return NULL;
	}

	total = SMP_I2C_PACKET_LEN(hdr);
	if (total > CONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE) {
		LOG_ERR("response %u exceeds net_buf %u", (uint32_t)total,
			(uint32_t)CONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE);
		return NULL;
	}

	nb = smp_packet_alloc();
	if (nb == NULL) {
		LOG_ERR("net_buf alloc failed for %u bytes", (uint32_t)total);
		return NULL;
	}

	/*
	 * Read the whole packet. A new I2C read starts the target streaming from
	 * offset 0, so this re-reads the header and then the payload.
	 */
	rc = i2c_read_dt(&cli_peer, net_buf_tail(nb), total);
	if (rc != 0) {
		LOG_ERR("packet read failed: %d", rc);
		smp_packet_free(nb);
		return NULL;
	}

	/* A different length means the target replaced the response between the two reads. */
	if (SMP_I2C_PACKET_LEN(net_buf_tail(nb)) != total) {
		LOG_WRN("length changed between reads - discarding");
		smp_packet_free(nb);
		return NULL;
	}

	net_buf_add(nb, total);

	return nb;
}

static int smp_i2c_client_output(struct net_buf *nb)
{
	struct net_buf *rsp;
	uint8_t hdr[SMP_I2C_HDR_SIZE];
	bool have_hdr;
	int rc;

	k_mutex_lock(&cli_xfer_lock, K_FOREVER);

#if SMP_I2C_CLIENT_HAS_DRDY
	/* Discard any stale edge that arrived while we were idle. */
	k_sem_reset(&cli_drdy_sem);
#endif
	rc = i2c_write_dt(&cli_peer, nb->data, nb->len);
	smp_packet_free(nb);
	if (rc != 0) {
		LOG_ERR("I2C write failed: %d", rc);
		k_mutex_unlock(&cli_xfer_lock);
		return rc;
	}

	/*
	 * The whole transaction (write, wait, two reads) runs synchronously in
	 * the caller's thread under cli_xfer_lock. It can block up to the poll or
	 * DRDY timeout, so CONFIG_SMP_CMD_DEFAULT_LIFE_TIME on the SMP client must
	 * exceed that budget.
	 */
	rc = cli_wait_for_response_ready(hdr, &have_hdr);
	if (rc != 0) {
		k_mutex_unlock(&cli_xfer_lock);
		return rc;
	}

	rsp = cli_read_smp_response(hdr, have_hdr);

	k_mutex_unlock(&cli_xfer_lock);

	if (rsp == NULL) {
		return -EIO;
	}

	smp_rx_req(&smp_i2c_client_transport, rsp);

	return 0;
}

static void smp_i2c_client_start(void)
{
	int rc;

	if (!i2c_is_ready_dt(&cli_peer)) {
		LOG_ERR("I2C bus not ready");
		return;
	}

#if SMP_I2C_CLIENT_HAS_DRDY
	if (!gpio_is_ready_dt(&cli_drdy_gpio)) {
		LOG_ERR("DRDY GPIO not ready");
		return;
	}

	rc = gpio_pin_configure_dt(&cli_drdy_gpio, GPIO_INPUT);
	if (rc < 0) {
		LOG_ERR("failed to configure DRDY GPIO: %d", rc);
		return;
	}

	gpio_init_callback(&cli_drdy_cb, cli_drdy_isr, BIT(cli_drdy_gpio.pin));
	rc = gpio_add_callback(cli_drdy_gpio.port, &cli_drdy_cb);
	if (rc < 0) {
		LOG_ERR("failed to add DRDY callback: %d", rc);
		return;
	}

	rc = gpio_pin_interrupt_configure_dt(&cli_drdy_gpio, GPIO_INT_EDGE_RISING);
	if (rc < 0) {
		LOG_ERR("failed to configure DRDY interrupt: %d", rc);
		return;
	}
#endif

	smp_i2c_client_transport.functions.output = smp_i2c_client_output;

	rc = smp_transport_init(&smp_i2c_client_transport);
	if (rc != 0) {
		LOG_ERR("smp_transport_init failed: %d", rc);
		return;
	}

	smp_client_transport_register(&smp_i2c_client_entry);

	LOG_INF("SMP I2C client ready (peer 0x%02x)", cli_peer.addr);
}

MCUMGR_HANDLER_DEFINE(smp_i2c_client, smp_i2c_client_start);

#endif /* CONFIG_MCUMGR_TRANSPORT_I2C_CLIENT */
