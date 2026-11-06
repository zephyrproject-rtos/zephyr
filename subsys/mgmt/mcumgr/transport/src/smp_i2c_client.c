/*
 * Copyright (c) 2026 Aerlync Labs Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief I2C controller-mode transport for the MCUmgr SMP protocol.
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

LOG_MODULE_REGISTER(smp_i2c_client, CONFIG_MCUMGR_TRANSPORT_I2C_CLIENT_LOG_LEVEL);

#define DT_DRV_COMPAT zephyr_smp_i2c_client

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "Only one zephyr,smp-i2c-client node is supported");

#define SMP_I2C_LEN_FIELD       2U
#define SMP_I2C_CLIENT_BUF_SIZE (SMP_I2C_LEN_FIELD + CONFIG_MCUMGR_TRANSPORT_I2C_CLIENT_MTU)

static const struct i2c_dt_spec peer_spec = I2C_DT_SPEC_INST_GET(0);

#if DT_INST_NODE_HAS_PROP(0, data_ready_gpios)
#define SMP_I2C_CLIENT_HAS_DRDY 1
static const struct gpio_dt_spec drdy_gpio = GPIO_DT_SPEC_INST_GET(0, data_ready_gpios);
static struct gpio_callback drdy_cb;
static K_SEM_DEFINE(drdy_sem, 0, 1);

static void drdy_isr(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);
	k_sem_give(&drdy_sem);
}
#else
#define SMP_I2C_CLIENT_HAS_DRDY 0
#endif

static K_MUTEX_DEFINE(xfer_lock);

static uint8_t tx_frame[SMP_I2C_CLIENT_BUF_SIZE];
static uint8_t rx_raw[SMP_I2C_CLIENT_BUF_SIZE];

static struct smp_transport smp_i2c_client_transport;

#ifdef CONFIG_SMP_CLIENT
static struct smp_client_transport_entry smp_i2c_client_entry = {
	.smpt = &smp_i2c_client_transport,
	.smpt_type = SMP_I2C_CLIENT_TRANSPORT,
};
#endif

/*
 * Wait until the target has a response staged.
 *
 * Returns 0 on success. In poll mode it also sets *payload_len to the polled
 * SMP payload length. DRDY mode leaves it 0 and reads the length later in
 * read_smp_response(). On failure it returns a negative errno. Keeping the
 * status separate from the length lets a zero-length DRDY "ready" stay
 * distinct from an error.
 */
static int wait_for_response_ready(uint16_t *payload_len)
{
	*payload_len = 0U;
#if SMP_I2C_CLIENT_HAS_DRDY
	int rc = k_sem_take(&drdy_sem, K_MSEC(CONFIG_MCUMGR_TRANSPORT_I2C_CLIENT_DRDY_TIMEOUT_MS));

	if (rc != 0) {
		LOG_WRN("DRDY timeout after %d ms",
			CONFIG_MCUMGR_TRANSPORT_I2C_CLIENT_DRDY_TIMEOUT_MS);
		return -ETIMEDOUT;
	}
	return 0;
#else
	uint8_t hdr[SMP_I2C_LEN_FIELD];
	int retries = CONFIG_MCUMGR_TRANSPORT_I2C_CLIENT_POLL_RETRIES;

	while (retries-- > 0) {
		int rc = i2c_read_dt(&peer_spec, hdr, sizeof(hdr));

		if (rc == -EIO) {
			/*
			 * A target that is erasing flash can hold the bus busy
			 * and NACK reads until it is done. Recover the bus and
			 * keep polling within the retry budget instead of
			 * aborting the whole transaction on the first NACK.
			 */
			(void)i2c_recover_bus(peer_spec.bus);
		} else if (rc != 0) {
			LOG_ERR("Poll read failed: %d", rc);
			return rc;
		} else if (hdr[0] != 0U || hdr[1] != 0U) {
			*payload_len = sys_get_be16(hdr);
			return 0;
		}

		k_msleep(CONFIG_MCUMGR_TRANSPORT_I2C_CLIENT_POLL_MS);
	}

	LOG_WRN("Response poll timeout");
	return -ETIMEDOUT;
#endif
}

/* Called with xfer_lock held, so the file-scope rx_raw buffer is exclusive. */
static struct net_buf *read_smp_response(uint16_t prefetched_len)
{
	uint16_t payload_len;
	int rc = 0;

#if SMP_I2C_CLIENT_HAS_DRDY
	uint8_t hdr[SMP_I2C_LEN_FIELD];

	ARG_UNUSED(prefetched_len); /* DRDY mode reads the length itself */
	rc = i2c_read_dt(&peer_spec, hdr, sizeof(hdr));
	if (rc != 0) {
		LOG_ERR("DRDY length read failed: %d", rc);
		return NULL;
	}
	payload_len = sys_get_be16(hdr);
#else
	payload_len = prefetched_len;
#endif

	if (payload_len == 0U) {
		LOG_WRN("Target returned zero-length payload");
		return NULL;
	}

	if (payload_len > CONFIG_MCUMGR_TRANSPORT_I2C_CLIENT_MTU) {
		LOG_ERR("Payload %u exceeds MTU %u", payload_len,
			CONFIG_MCUMGR_TRANSPORT_I2C_CLIENT_MTU);
		return NULL;
	}

	/*
	 * Read the full frame (2-byte length prefix + SMP payload). Starting a
	 * new I2C read transaction causes the target to stream from offset 0.
	 */
	rc = i2c_read_dt(&peer_spec, rx_raw, SMP_I2C_LEN_FIELD + payload_len);
	if (rc != 0) {
		LOG_ERR("Payload read failed: %d", rc);
		return NULL;
	}

	/*
	 * The target restages its TX buffer on every new READ. Check that the
	 * length prefix in the full frame matches what we polled. A mismatch
	 * means the buffer was replaced between the two reads.
	 */
	uint16_t confirm_len = sys_get_be16(rx_raw);

	if (confirm_len != payload_len) {
		LOG_WRN("Length mismatch: polled %u but frame says %u - discarding", payload_len,
			confirm_len);
		return NULL;
	}

	struct net_buf *nb = smp_packet_alloc();

	if (nb == NULL) {
		LOG_ERR("net_buf alloc failed for %u bytes", payload_len);
		return NULL;
	}

	net_buf_add_mem(nb, rx_raw + SMP_I2C_LEN_FIELD, payload_len);
	return nb;
}

static int smp_i2c_client_output(struct net_buf *nb)
{
	int rc = 0;
	uint16_t smp_len = nb->len;

	if (smp_len > CONFIG_MCUMGR_TRANSPORT_I2C_CLIENT_MTU) {
		LOG_ERR("Frame %u exceeds MTU %u", smp_len, CONFIG_MCUMGR_TRANSPORT_I2C_CLIENT_MTU);
		smp_packet_free(nb);
		return -EMSGSIZE;
	}

	k_mutex_lock(&xfer_lock, K_FOREVER);

	sys_put_be16(smp_len, tx_frame);
	memcpy(tx_frame + SMP_I2C_LEN_FIELD, nb->data, smp_len);

	smp_packet_free(nb);
	nb = NULL;

#if SMP_I2C_CLIENT_HAS_DRDY
	/* Discard any stale edge that arrived while we were idle. */
	k_sem_reset(&drdy_sem);
#endif
	rc = i2c_write_dt(&peer_spec, tx_frame, SMP_I2C_LEN_FIELD + smp_len);

	if (rc != 0) {
		LOG_ERR("I2C write failed: %d", rc);
		k_mutex_unlock(&xfer_lock);
		return rc;
	}

	/*
	 * The whole transaction (write, poll/DRDY wait, two reads) runs
	 * synchronously in the caller's thread under xfer_lock. It can block
	 * up to the poll/DRDY timeout, so CONFIG_SMP_CMD_DEFAULT_LIFE_TIME on
	 * the SMP client must exceed that budget.
	 */
	uint16_t payload_len = 0U;

	rc = wait_for_response_ready(&payload_len);
	if (rc != 0) {
		k_mutex_unlock(&xfer_lock);
		return rc;
	}

	struct net_buf *rsp = read_smp_response(payload_len);

	k_mutex_unlock(&xfer_lock);

	if (rsp == NULL) {
		return -EIO;
	}

	smp_rx_req(&smp_i2c_client_transport, rsp);

	return 0;
}

static uint16_t smp_i2c_client_get_mtu(const struct net_buf *nb)
{
	ARG_UNUSED(nb);
	return CONFIG_MCUMGR_TRANSPORT_I2C_CLIENT_MTU;
}

static void smp_i2c_client_start(void)
{
	int rc = 0;

	if (!i2c_is_ready_dt(&peer_spec)) {
		LOG_ERR("I2C bus not ready");
		return;
	}

#if SMP_I2C_CLIENT_HAS_DRDY
	if (!gpio_is_ready_dt(&drdy_gpio)) {
		LOG_ERR("DRDY GPIO not ready");
		return;
	}

	rc = gpio_pin_configure_dt(&drdy_gpio, GPIO_INPUT);
	if (rc < 0) {
		LOG_ERR("Failed to configure DRDY GPIO: %d", rc);
		return;
	}

	gpio_init_callback(&drdy_cb, drdy_isr, BIT(drdy_gpio.pin));
	rc = gpio_add_callback(drdy_gpio.port, &drdy_cb);
	if (rc < 0) {
		LOG_ERR("Failed to add DRDY callback: %d", rc);
		return;
	}

	rc = gpio_pin_interrupt_configure_dt(&drdy_gpio, GPIO_INT_EDGE_RISING);
	if (rc < 0) {
		LOG_ERR("Failed to configure DRDY interrupt: %d", rc);
		return;
	}
#endif

	smp_i2c_client_transport.functions.output = smp_i2c_client_output;
	/* get_mtu is deprecated and not called by the SMP layer, but still set
	 * for transports that read it. Silence the deprecation on assignment.
	 */
	TOOLCHAIN_DISABLE_WARNING(TOOLCHAIN_WARNING_DEPRECATED_DECLARATIONS);
	smp_i2c_client_transport.functions.get_mtu = smp_i2c_client_get_mtu;
	TOOLCHAIN_ENABLE_WARNING(TOOLCHAIN_WARNING_DEPRECATED_DECLARATIONS);

	rc = smp_transport_init(&smp_i2c_client_transport);
	if (rc != 0) {
		LOG_ERR("smp_transport_init failed: %d", rc);
		return;
	}

#ifdef CONFIG_SMP_CLIENT
	smp_client_transport_register(&smp_i2c_client_entry);
#endif

	LOG_INF("SMP I2C client ready (peer 0x%02x, MTU %u)", peer_spec.addr,
		CONFIG_MCUMGR_TRANSPORT_I2C_CLIENT_MTU);
}

MCUMGR_HANDLER_DEFINE(smp_i2c_client, smp_i2c_client_start);
