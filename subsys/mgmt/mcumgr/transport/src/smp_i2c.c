/*
 * Copyright (c) 2026 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief I2C target transport for the MCUmgr SMP protocol.
 *
 * WRITE carries an SMP command framed as a 2-byte big-endian length prefix
 * then the raw SMP packet. READ returns a response with the same prefix, or
 * [0x00][0x00] while idle. An optional data-ready GPIO replaces polling.
 *
 * Byte-mode and buffer-mode (DMA) target drivers are both supported. On a
 * buffer-mode driver that never calls stop() after a READ, a staged response
 * is held until the next WRITE rather than cleared when a read starts, so the
 * controller can read the length then the frame.
 *
 * One WRITE carries one SMP packet, and there is no reassembly. The target
 * stays registered for the application lifetime (no PM_DEVICE or unregister).
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

LOG_MODULE_DECLARE(mcumgr_smp, CONFIG_MCUMGR_TRANSPORT_LOG_LEVEL);

#define DT_DRV_COMPAT zephyr_smp_i2c

/* On the wire a frame is [len_high][len_low] then the SMP payload. */
#define SMP_I2C_LEN_FIELD   2U
/* Length prefix plus the SMP payload. In buffer mode this is also the
 * minimum DMA buffer the target driver must provide (see Kconfig.i2c).
 */
#define SMP_I2C_TX_BUF_SIZE (SMP_I2C_LEN_FIELD + CONFIG_MCUMGR_TRANSPORT_I2C_MTU)

/* This transport keeps its state in module-level variables, so only one
 * node is supported for now.
 */
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "Only one zephyr,smp-i2c node is supported");

BUILD_ASSERT(CONFIG_MCUMGR_TRANSPORT_I2C_MTU <= CONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE,
	     "CONFIG_MCUMGR_TRANSPORT_I2C_MTU must not exceed "
	     "CONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE");

static const struct device *const i2c_dev = DEVICE_DT_GET(DT_BUS(DT_DRV_INST(0)));

#if DT_INST_NODE_HAS_PROP(0, data_ready_gpios)
#define SMP_I2C_HAS_DRDY 1
static const struct gpio_dt_spec drdy_gpio = GPIO_DT_SPEC_INST_GET(0, data_ready_gpios);
#else
#define SMP_I2C_HAS_DRDY 0
#endif

/* RX state machine for byte mode. Buffer mode parses the frame in one shot. */
enum smp_i2c_rx_state {
	SMP_I2C_RX_LEN_HIGH,
	SMP_I2C_RX_LEN_LOW,
	SMP_I2C_RX_DATA,
	SMP_I2C_RX_ERROR,
};

static enum smp_i2c_rx_state rx_state;
static uint16_t rx_expected;
static uint16_t rx_received;
static struct net_buf *rx_nb;

/*
 * TX state is shared between the SMP workqueue thread and the I2C ISR
 * callbacks. It uses a spinlock rather than a mutex, because a mutex cannot
 * be taken from an ISR.
 *
 * In buffer mode tx_buf is the DMA source, so it has to live in RAM. A staged
 * response stays put until the next WRITE clears it, not when a READ starts,
 * because the controller always reads a response before sending the next
 * request. tx_reading means a READ is currently draining tx_buf. A READ that
 * starts with no response staged serves zeros for its whole length, so a
 * response staged part way through can never corrupt a read already running.
 */
static struct k_spinlock tx_lock;
static uint8_t tx_buf[SMP_I2C_TX_BUF_SIZE];
static uint16_t tx_len; /* SMP packet bytes staged in tx_buf (prefix is 2 more) */
static uint16_t tx_pos; /* next tx_buf index for the current READ */
static bool tx_ready;   /* a response is staged in tx_buf */
static bool tx_reading; /* the current READ transaction is draining tx_buf */

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

#if defined(CONFIG_I2C_TARGET_BUFFER_MODE)
/*
 * Returned by buf_read_requested when no response is pending. Must be in
 * RAM, because DMA cannot source from .rodata on some platforms.
 */
static uint8_t zero_buf[SMP_I2C_LEN_FIELD];
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

/* Byte-mode ISR callbacks */

static int smp_i2c_write_requested(struct i2c_target_config *cfg)
{
	ARG_UNUSED(cfg);
	k_spinlock_key_t key;
	bool drdy_clear = false;

	/* Discard any partial frame left by an interrupted transaction. */
	if (rx_nb != NULL) {
		smp_packet_free(rx_nb);
		rx_nb = NULL;
	}

	rx_state = SMP_I2C_RX_LEN_HIGH;
	rx_expected = 0U;
	rx_received = 0U;

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

	switch (rx_state) {
	case SMP_I2C_RX_LEN_HIGH:
		rx_expected = (uint16_t)val << 8;
		rx_state = SMP_I2C_RX_LEN_LOW;
		break;

	case SMP_I2C_RX_LEN_LOW:
		rx_expected |= val;

		if (rx_expected == 0U || rx_expected > CONFIG_MCUMGR_TRANSPORT_I2C_MTU) {
			rx_state = SMP_I2C_RX_ERROR;
			break;
		}

		rx_nb = smp_packet_alloc();
		if (rx_nb == NULL) {
			rx_state = SMP_I2C_RX_ERROR;
			break;
		}

		rx_state = SMP_I2C_RX_DATA;
		break;

	case SMP_I2C_RX_DATA:
		/* Reject more bytes than the length prefix declared. */
		if (rx_received >= rx_expected || net_buf_tailroom(rx_nb) == 0U) {
			smp_packet_free(rx_nb);
			rx_nb = NULL;
			rx_state = SMP_I2C_RX_ERROR;
			break;
		}

		net_buf_add_u8(rx_nb, val);
		rx_received++;
		break;

	case SMP_I2C_RX_ERROR:
		/* Drain remaining bytes until STOP resets the state machine. */
		break;
	}

	return 0;
}

static int smp_i2c_read_requested(struct i2c_target_config *cfg, uint8_t *val)
{
	k_spinlock_key_t key;

	ARG_UNUSED(cfg);

	key = k_spin_lock(&tx_lock);
	tx_reading = tx_ready;
	tx_pos = 0U;
	*val = tx_reading ? tx_buf[tx_pos++] : 0x00U;
	k_spin_unlock(&tx_lock, key);

	return 0;
}

static int smp_i2c_read_processed(struct i2c_target_config *cfg, uint8_t *val)
{
	k_spinlock_key_t key;

	ARG_UNUSED(cfg);

	key = k_spin_lock(&tx_lock);
	if (tx_reading && tx_pos < (SMP_I2C_LEN_FIELD + tx_len)) {
		*val = tx_buf[tx_pos++];
	} else {
		*val = 0x00U;
	}
	k_spin_unlock(&tx_lock, key);

	return 0;
}

/*
 * stop() fires at the end of every I2C transaction. At the end of a WRITE it
 * hands a complete frame to the SMP workqueue, or frees a partial or errored
 * one. At the end of a READ (byte mode) it drops the staged response and
 * deasserts DRDY once the controller has clocked out every byte.
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

	if (rx_state == SMP_I2C_RX_DATA && rx_nb != NULL) {
		if (rx_received == rx_expected) {
			smp_rx_req(&smp_i2c_transport, rx_nb);
		} else {
			LOG_WRN("truncated frame %u/%u", rx_received, rx_expected);
			smp_packet_free(rx_nb);
		}
		rx_nb = NULL;
	} else if (rx_nb != NULL) {
		smp_packet_free(rx_nb);
		rx_nb = NULL;
	}

	rx_state = SMP_I2C_RX_LEN_HIGH;
	rx_expected = 0U;
	rx_received = 0U;

	key = k_spin_lock(&tx_lock);

	if (tx_reading && tx_pos >= (SMP_I2C_LEN_FIELD + tx_len)) {
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

static void smp_i2c_buf_write_received(struct i2c_target_config *cfg, uint8_t *ptr,
					       uint32_t len)
{
	ARG_UNUSED(cfg);
	uint16_t smp_len;
	struct net_buf *nb;
	k_spinlock_key_t key;
	bool drdy_clear;

	/*
	 * A new request means the controller has already read any staged
	 * response, so drop it now. Otherwise a later READ would replay a stale
	 * frame. In buffer mode this is where the response is cleared, since a
	 * buffer-mode driver may not call stop() after a READ.
	 */
	key = k_spin_lock(&tx_lock);
	drdy_clear = tx_discard_locked();
	k_spin_unlock(&tx_lock, key);
	if (drdy_clear != false) {
		drdy_set(0);
	}

	if (len < SMP_I2C_LEN_FIELD) {
		LOG_WRN("frame too short: %u", len);
		return;
	}

	smp_len = ((uint16_t)ptr[0] << 8) | ptr[1];

	if (smp_len == 0U || smp_len > CONFIG_MCUMGR_TRANSPORT_I2C_MTU) {
		LOG_ERR("invalid frame length %u", smp_len);
		return;
	}

	if (len < (uint32_t)(SMP_I2C_LEN_FIELD + smp_len)) {
		/*
		 * This also guards against an undersized target DMA buffer. A
		 * driver whose hardware buffer is smaller than MTU + 2 hands us
		 * fewer bytes than the frame declares, and we drop it here
		 * instead of reading past the end.
		 */
		LOG_WRN("frame truncated: got %u expected %u", len,
			(uint32_t)(SMP_I2C_LEN_FIELD + smp_len));
		return;
	}

	nb = smp_packet_alloc();
	if (nb == NULL) {
		LOG_ERR("net_buf alloc failed");
		return;
	}

	net_buf_add_mem(nb, &ptr[SMP_I2C_LEN_FIELD], smp_len);
	smp_rx_req(&smp_i2c_transport, nb);
}

static int smp_i2c_buf_read_requested(struct i2c_target_config *cfg, uint8_t **ptr,
					      uint32_t *len)
{
	ARG_UNUSED(cfg);
	k_spinlock_key_t key;
	bool do_drdy_clear = false;

	key = k_spin_lock(&tx_lock);

	if (tx_ready == true) {
		*ptr = tx_buf;
		*len = SMP_I2C_LEN_FIELD + tx_len;
		/*
		 * Keep the response latched. A buffer-mode target copies tx_buf
		 * into its own DMA buffer here but may never call stop() after a
		 * READ, and the controller reads the frame twice (first the
		 * 2-byte length, then the whole frame). The frame is cleared only
		 * when the next request arrives. DRDY is an edge hint, so
		 * deassert it on the first read.
		 */
		do_drdy_clear = true;
	} else {
		*ptr = zero_buf;
		*len = SMP_I2C_LEN_FIELD;
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

/* Thread-context SMP transport callbacks */

/*
 * The MCUmgr workqueue calls smp_i2c_tx_pkt() to stage a response.
 *
 * While a byte-mode READ is draining tx_buf (tx_reading set) it returns
 * -EBUSY instead of corrupting the transfer, and the SMP client retries after
 * CONFIG_SMP_CMD_RETRY_TIME. In buffer mode tx_reading is never set, because
 * the controller reads a response before sending the next request, so staging
 * is always safe.
 */
static int smp_i2c_tx_pkt(struct net_buf *nb)
{
	k_spinlock_key_t key;
	uint16_t pkt_len = nb->len;

	if (pkt_len > CONFIG_MCUMGR_TRANSPORT_I2C_MTU) {
		LOG_ERR("response too large: %u > MTU %u", pkt_len,
			(uint32_t)CONFIG_MCUMGR_TRANSPORT_I2C_MTU);
		smp_packet_free(nb);
		return -EMSGSIZE;
	}

	key = k_spin_lock(&tx_lock);

	if (tx_reading) {
		/*
		 * A byte-mode READ is clocking bytes out of tx_buf right now,
		 * and overwriting it would tear the active transfer. This
		 * transport holds only one response at a time.
		 */
		k_spin_unlock(&tx_lock, key);
		LOG_WRN("read in progress; dropping new response");
		smp_packet_free(nb);
		return -EBUSY;
	}

	tx_buf[0] = (uint8_t)(pkt_len >> 8);
	tx_buf[1] = (uint8_t)(pkt_len & 0xFFU);
	memcpy(&tx_buf[SMP_I2C_LEN_FIELD], nb->data, pkt_len);
	tx_len = pkt_len;
	tx_pos = 0U;
	tx_ready = true;

	k_spin_unlock(&tx_lock, key);

	/* DRDY goes high only after tx_ready is set. */
	drdy_set(1);

	LOG_DBG("buffered %u byte response", pkt_len);
	smp_packet_free(nb);

	return 0;
}

static uint16_t smp_i2c_get_mtu(const struct net_buf *nb)
{
	/* One peer per I2C bus, so a single fixed MTU applies. */
	ARG_UNUSED(nb);
	return CONFIG_MCUMGR_TRANSPORT_I2C_MTU;
}

/* Transport initialisation */

static void smp_i2c_start(void)
{
	int rc = 0;

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

	smp_i2c_transport.functions.output = smp_i2c_tx_pkt;
	/* get_mtu is deprecated and not called by the SMP layer, but still set
	 * for transports that read it. Silence the deprecation on assignment.
	 */
	TOOLCHAIN_DISABLE_WARNING(TOOLCHAIN_WARNING_DEPRECATED_DECLARATIONS);
	smp_i2c_transport.functions.get_mtu = smp_i2c_get_mtu;
	TOOLCHAIN_ENABLE_WARNING(TOOLCHAIN_WARNING_DEPRECATED_DECLARATIONS);

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
