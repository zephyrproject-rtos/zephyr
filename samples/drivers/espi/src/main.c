/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr.h>
#include <device.h>
#include <soc.h>
#include <drivers/gpio.h>
#include <drivers/espi.h>
#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(espi, CONFIG_ESPI_LOG_LEVEL);

/* eSPI host entity address  */
#define DEST_SLV_ADDR         0x02u
#define SRC_SLV_ADDR          0x21u

/* Temperature command opcode */
#define OOB_CMDCODE           0x01u
#define OOB_RESPONSE_LEN      0x05u
#define OOB_RESPONSE_INDEX    0x03u

/* Maximum bytes for OOB transactions */
#define MAX_RESP_SIZE         20u

/* eSPI flash parameters */
#define MAX_TEST_BUF_SIZE     1024u
#define MAX_FLASH_REQUEST     64u
#define TARGET_FLASH_REGION   0x72000ul

/* 20 MHz */
#define MIN_ESPI_FREQ         20u

#define K_WAIT_DELAY          100u

/* eSPI event */
#define EVENT_MASK            0x0000FFFFu
#define EVENT_DETAILS_MASK    0xFFFF0000u
#define EVENT_DETAILS_POS     16u
#define EVENT_TYPE(x)         (x & EVENT_MASK)
#define EVENT_DETAILS(x)      ((x & EVENT_DETAILS_MASK) >> EVENT_DETAILS_POS)

struct oob_header {
	uint8_t dest_slave_addr;
	uint8_t oob_cmd_code;
	uint8_t byte_cnt;
	uint8_t src_slave_addr;
};

#define PWR_SEQ_TIMEOUT    3000u

/* The devicetree node identifier for the board power rails pins. */
#define BRD_PWR_NODE DT_INST(0, microchip_mec15xx_board_power)

#if DT_NODE_HAS_STATUS(BRD_PWR_NODE, okay)
#define BRD_PWR_PWRGD           DT_GPIO_LABEL(BRD_PWR_NODE, pwrg_gpios)
#define BRD_PWR_RSMRST          DT_GPIO_LABEL(BRD_PWR_NODE, rsm_gpios)
#define BRD_PWR_RSMRST_PIN      DT_GPIO_PIN(BRD_PWR_NODE, rsm_gpios)
#define BRD_PWR_PWRGD_PIN       DT_GPIO_PIN(BRD_PWR_NODE, pwrg_gpios)

static struct device *pwrgd_dev;
static struct device *rsm_dev;
#endif

#define ESPI_DEV      DT_LABEL(DT_NODELABEL(espi0))

static struct device *espi_dev;
static struct espi_callback espi_bus_cb;
static struct espi_callback vw_rdy_cb;
static struct espi_callback vw_cb;
static struct espi_callback p80_cb;

static uint8_t espi_rst_sts;

#ifdef CONFIG_ESPI_FLASH_CHANNEL
static uint8_t flash_write_buf[MAX_TEST_BUF_SIZE];
static uint8_t flash_read_buf[MAX_TEST_BUF_SIZE];
#endif

static void host_warn_handler(uint32_t signal, uint32_t status)
{
	switch (signal) {
	case ESPI_VWIRE_SIGNAL_HOST_RST_WARN:
		LOG_INF("Host reset warning %d", status);
		if (!IS_ENABLED(CONFIG_ESPI_AUTOMATIC_WARNING_ACKNOWLEDGE)) {
			LOG_INF("HOST RST ACK %d", status);
			espi_send_vwire(espi_dev,
					ESPI_VWIRE_SIGNAL_HOST_RST_ACK,
					status);
		}
		break;
	case ESPI_VWIRE_SIGNAL_SUS_WARN:
		LOG_INF("Host suspend warning %d", status);
		if (!IS_ENABLED(CONFIG_ESPI_AUTOMATIC_WARNING_ACKNOWLEDGE)) {
			LOG_INF("SUS ACK %d", status);
			espi_send_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SUS_ACK,
					status);
		}
		break;
	default:
		break;
	}
}

/* eSPI bus event handler */
static void espi_reset_handler(struct device *dev,
			       struct espi_callback *cb,
			       struct espi_event event)
{
	if (event.evt_type == ESPI_BUS_RESET) {
		espi_rst_sts = event.evt_data;
		LOG_INF("eSPI BUS reset %d", event.evt_data);
	}
}

/* eSPI logical channels enable/disable event handler */
static void espi_ch_handler(struct device *dev, struct espi_callback *cb,
			    struct espi_event event)
{
	if (event.evt_type == ESPI_BUS_EVENT_CHANNEL_READY) {
		switch (event.evt_details) {
		case ESPI_CHANNEL_VWIRE:
			LOG_INF("VW channel event %x", event.evt_data);
			break;
		case ESPI_CHANNEL_FLASH:
			LOG_INF("Flash channel event %d", event.evt_data);
			break;
		case ESPI_CHANNEL_OOB:
			LOG_INF("OOB channel event %d", event.evt_data);
			break;
		default:
			LOG_ERR("Unknown channel event");
		}
	}
}

/* eSPI vwire received event handler */
static void vwire_handler(struct device *dev, struct espi_callback *cb,
			  struct espi_event event)
{
	if (event.evt_type == ESPI_BUS_EVENT_VWIRE_RECEIVED) {
		switch (event.evt_details) {
		case ESPI_VWIRE_SIGNAL_PLTRST:
			LOG_INF("PLT_RST changed %d", event.evt_data);
			break;
		case ESPI_VWIRE_SIGNAL_SLP_S3:
		case ESPI_VWIRE_SIGNAL_SLP_S4:
		case ESPI_VWIRE_SIGNAL_SLP_S5:
			LOG_INF("SLP signal changed %d", event.evt_data);
			break;
		case ESPI_VWIRE_SIGNAL_SUS_WARN:
		case ESPI_VWIRE_SIGNAL_HOST_RST_WARN:
			host_warn_handler(event.evt_details,
					      event.evt_data);
			break;
		}
	}
}

/* eSPI peripheral channel notifications handler */
static void periph_handler(struct device *dev, struct espi_callback *cb,
			   struct espi_event event)
{
	uint8_t periph_type;
	uint8_t periph_index;

	periph_type = EVENT_TYPE(event.evt_details);
	periph_index = EVENT_DETAILS(event.evt_details);

	switch (periph_type) {
	case ESPI_PERIPHERAL_DEBUG_PORT80:
		LOG_INF("Postcode %x", event.evt_data);
		break;
	case ESPI_PERIPHERAL_HOST_IO:
		LOG_INF("ACPI %x", event.evt_data);
		espi_remove_callback(espi_dev, &p80_cb);
		break;
	default:
		LOG_INF("%s periph 0x%x [%x]", __func__, periph_type,
			event.evt_data);
	}
}

int espi_init(void)
{
	int ret;
	/* Indicate to eSPI master simplest configuration: Single line,
	 * 20MHz frequency and only logical channel 0 and 1 are supported
	 */
	struct espi_cfg cfg = {
		ESPI_IO_MODE_SINGLE_LINE,
		ESPI_CHANNEL_VWIRE | ESPI_CHANNEL_PERIPHERAL,
		MIN_ESPI_FREQ,
	};

	/* If eSPI driver supports additional capabilities use them */
#ifdef CONFIG_ESPI_OOB_CHANNEL
	cfg.channel_caps |= ESPI_CHANNEL_OOB;
#endif
#ifdef CONFIG_ESPI_FLASH_CHANNEL
	cfg.channel_caps |= ESPI_CHANNEL_FLASH;
#endif

	ret = espi_config(espi_dev, &cfg);
	if (ret) {
		LOG_ERR("Failed to configure eSPI slave channels:%x err: %d",
			cfg.channel_caps, ret);
		return ret;
	} else {
		LOG_INF("eSPI slave configured successfully!");
	}

	LOG_INF("eSPI test - callbacks initialization... ");
	espi_init_callback(&espi_bus_cb, espi_reset_handler, ESPI_BUS_RESET);
	espi_init_callback(&vw_rdy_cb, espi_ch_handler,
			   ESPI_BUS_EVENT_CHANNEL_READY);
	espi_init_callback(&vw_cb, vwire_handler,
			   ESPI_BUS_EVENT_VWIRE_RECEIVED);
	espi_init_callback(&p80_cb, periph_handler,
			   ESPI_BUS_PERIPHERAL_NOTIFICATION);
	LOG_INF("complete");

	LOG_INF("eSPI test - callbacks registration... ");
	espi_add_callback(espi_dev, &espi_bus_cb);
	espi_add_callback(espi_dev, &vw_rdy_cb);
	espi_add_callback(espi_dev, &vw_cb);
	espi_add_callback(espi_dev, &p80_cb);
	LOG_INF("complete");

	return ret;
}

#if DT_NODE_HAS_STATUS(BRD_PWR_NODE, okay)
static int wait_for_pin(struct device *dev, uint8_t pin, uint16_t timeout,
			int exp_level)
{
	uint16_t loop_cnt = timeout;
	int level;

	do {
		level = gpio_pin_get(dev, pin);
		if (level < 0) {
			LOG_ERR("Failed to read %x %d", pin, level);
			return -EIO;
		}

		if (exp_level == level) {
			LOG_DBG("PIN %x = %x", pin, exp_level);
			break;
		}

		k_usleep(K_WAIT_DELAY);
		loop_cnt--;
	} while (loop_cnt > 0);

	if (loop_cnt == 0) {
		LOG_ERR("Timeout for %x %x", pin, level);
		return -ETIMEDOUT;
	}

	return 0;
}
#endif

static int wait_for_vwire(struct device *espi_dev,
			  enum espi_vwire_signal signal,
			  uint16_t timeout, uint8_t exp_level)
{
	int ret;
	uint8_t level;
	uint16_t loop_cnt = timeout;

	do {
		ret = espi_receive_vwire(espi_dev, signal, &level);
		if (ret) {
			LOG_ERR("Failed to read %x %d", signal, ret);
			return -EIO;
		}

		if (exp_level == level) {
			break;
		}

		k_usleep(K_WAIT_DELAY);
		loop_cnt--;
	} while (loop_cnt > 0);

	if (loop_cnt == 0) {
		LOG_ERR("VWIRE %d is %x", signal, level);
		return -ETIMEDOUT;
	}

	return 0;
}

static int wait_for_espi_reset(uint8_t exp_sts)
{
	uint16_t loop_cnt = CONFIG_ESPI_VIRTUAL_WIRE_TIMEOUT;

	do {
		if (exp_sts == espi_rst_sts) {
			break;
		}
		k_usleep(K_WAIT_DELAY);
		loop_cnt--;
	} while (loop_cnt > 0);

	if (loop_cnt == 0) {
		return -ETIMEDOUT;
	}

	return 0;
}

int espi_handshake(void)
{
	int ret;

	LOG_INF("eSPI test - Handshake with eSPI master...");
	ret = wait_for_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SUS_WARN,
			     CONFIG_ESPI_VIRTUAL_WIRE_TIMEOUT, 1);
	if (ret) {
		LOG_ERR("SUS_WARN Timeout");
		return ret;
	}

	LOG_INF("1st phase completed");
	ret = wait_for_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SLP_S5,
			     CONFIG_ESPI_VIRTUAL_WIRE_TIMEOUT, 1);
	if (ret) {
		LOG_ERR("SLP_S5 Timeout");
		return ret;
	}

	ret = wait_for_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SLP_S4,
			     CONFIG_ESPI_VIRTUAL_WIRE_TIMEOUT, 1);
	if (ret) {
		LOG_ERR("SLP_S4 Timeout");
		return ret;
	}

	ret = wait_for_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SLP_S3,
			     CONFIG_ESPI_VIRTUAL_WIRE_TIMEOUT, 1);
	if (ret) {
		LOG_ERR("SLP_S3 Timeout");
		return ret;
	}

	LOG_INF("2nd phase completed");

	ret = wait_for_vwire(espi_dev, ESPI_VWIRE_SIGNAL_PLTRST,
			     CONFIG_ESPI_VIRTUAL_WIRE_TIMEOUT, 1);
	if (ret) {
		LOG_ERR("PLT_RST Timeout");
		return ret;
	}

	LOG_INF("3rd phase completed");

	return 0;
}

#ifdef CONFIG_ESPI_FLASH_CHANNEL
int read_test_block(uint8_t *buf, uint32_t start_flash_adr, uint16_t block_len)
{
	uint8_t i = 0;
	uint32_t flash_addr = start_flash_adr;
	uint16_t transactions = block_len/MAX_FLASH_REQUEST;
	int ret = 0;
	struct espi_flash_packet pckt;

	for (i = 0; i < transactions; i++) {
		pckt.buf = buf;
		pckt.flash_addr = flash_addr;
		pckt.len = MAX_FLASH_REQUEST;

		ret = espi_read_flash(espi_dev, &pckt);
		if (ret) {
			LOG_ERR("espi_read_flash failed: %d", ret);
			return ret;
		}

		buf += MAX_FLASH_REQUEST;
		flash_addr += MAX_FLASH_REQUEST;
	}

	LOG_INF("%d read flash transactions completed", transactions);
	return 0;
}

int write_test_block(uint8_t *buf, uint32_t start_flash_adr, uint16_t block_len)
{
	uint8_t i = 0;
	uint32_t flash_addr = start_flash_adr;
	uint16_t transactions = block_len/MAX_FLASH_REQUEST;
	int ret = 0;
	struct espi_flash_packet pckt;

	/* Split operation in multiple MAX_FLASH_REQ transactions */
	for (i = 0; i < transactions; i++) {
		pckt.buf = buf;
		pckt.flash_addr = flash_addr;
		pckt.len = MAX_FLASH_REQUEST;

		ret = espi_write_flash(espi_dev, &pckt);
		if (ret) {
			LOG_ERR("espi_write_flash failed: %d", ret);
			return ret;
		}

		buf += MAX_FLASH_REQUEST;
		flash_addr += MAX_FLASH_REQUEST;
	}

	LOG_INF("%d write flash transactions completed", transactions);
	return 0;
}

static int espi_flash_test(uint32_t start_flash_addr, uint8_t blocks)
{
	uint8_t i;
	uint8_t pattern;
	uint32_t flash_addr;
	int ret = 0;

	LOG_INF("Test eSPI write flash");
	flash_addr = start_flash_addr;
	pattern = 0x99;
	for (i = 0; i <= blocks; i++) {
		memset(flash_write_buf, pattern++, sizeof(flash_write_buf));
		ret = write_test_block(flash_write_buf, flash_addr,
				       sizeof(flash_write_buf));
		if (ret) {
			LOG_ERR("Failed to write to eSPI");
			return ret;
		}

		flash_addr += sizeof(flash_write_buf);
	}

	LOG_INF("Test eSPI read flash");
	flash_addr = start_flash_addr;
	pattern = 0x99;
	for (i = 0; i <= blocks; i++) {
		/* Set expected content */
		memset(flash_write_buf, pattern, sizeof(flash_write_buf));
		/* Clear last read content */
		memset(flash_read_buf, 0, sizeof(flash_read_buf));
		ret = read_test_block(flash_read_buf, flash_addr,
				      sizeof(flash_read_buf));
		if (ret) {
			LOG_ERR("Failed to read from eSPI");
			return ret;
		}

		/* Compare buffers  */
		int cmp = memcmp(flash_write_buf, flash_read_buf,
				 sizeof(flash_write_buf));

		if (cmp != 0) {
			LOG_ERR("eSPI read mismmatch at %d expected %x",
				cmp, pattern);
		}

		flash_addr += sizeof(flash_read_buf);
		pattern++;
	}

	return 0;
}
#endif /* CONFIG_ESPI_FLASH_CHANNEL */

int get_pch_temp(struct device *dev, int *temp)
{
	struct espi_oob_packet req_pckt;
	struct espi_oob_packet resp_pckt;
	struct oob_header oob_hdr;
	uint8_t buf[MAX_RESP_SIZE];
	int ret;

	LOG_INF("%s", __func__);

	oob_hdr.dest_slave_addr = DEST_SLV_ADDR;
	oob_hdr.oob_cmd_code = OOB_CMDCODE;
	oob_hdr.byte_cnt = 1;
	oob_hdr.src_slave_addr = SRC_SLV_ADDR;

	/* Packetize OOB request */
	req_pckt.buf = (uint8_t *)&oob_hdr;
	req_pckt.len = sizeof(struct oob_header);
	resp_pckt.buf = (uint8_t *)&buf;
	resp_pckt.len = MAX_RESP_SIZE;

	ret = espi_send_oob(dev, &req_pckt);
	if (ret) {
		LOG_ERR("OOB Tx failed %d", ret);
		return ret;
	}

	ret = espi_receive_oob(dev, &resp_pckt);
	if (ret) {
		LOG_ERR("OOB Rx failed %d", ret);
		return ret;
	}

	LOG_INF("OOB transaction completed rcvd: %d bytes", resp_pckt.len);
	for (int i = 0; i < resp_pckt.len; i++) {
		LOG_INF("%x ", buf[i]);
	}

	if (resp_pckt.len == OOB_RESPONSE_LEN) {
		*temp = buf[OOB_RESPONSE_INDEX];
	} else {
		LOG_ERR("Incorrect size response");
	}

	return 0;
}

int espi_test(void)
{
	int ret;

	/* Account for the time serial port is detected so log messages can
	 * be seen
	 */
	k_sleep(K_SECONDS(1));

#if DT_NODE_HAS_STATUS(BRD_PWR_NODE, okay)
	pwrgd_dev = device_get_binding(BRD_PWR_PWRGD);
	if (!pwrgd_dev) {
		LOG_WRN("%s not found", BRD_PWR_PWRGD);
		return -1;
	}

	rsm_dev = device_get_binding(BRD_PWR_RSMRST);
	if (!rsm_dev) {
		LOG_WRN("%s not found", BRD_PWR_RSMRST);
		return -1;
	}

#endif
	espi_dev = device_get_binding(ESPI_DEV);
	if (!espi_dev) {
		LOG_WRN("Fail to find %s", ESPI_DEV);
		return -1;
	}

	LOG_INF("Hello eSPI test %s", CONFIG_BOARD);

#if DT_NODE_HAS_STATUS(BRD_PWR_NODE, okay)
	ret = gpio_pin_configure(pwrgd_dev, BRD_PWR_PWRGD_PIN,
				 GPIO_INPUT | GPIO_ACTIVE_HIGH);
	if (ret) {
		LOG_ERR("Unable to configure %d:%d",
			BRD_PWR_PWRGD_PIN, ret);
		return ret;
	}

	ret = gpio_pin_configure(rsm_dev, BRD_PWR_RSMRST_PIN,
				 GPIO_OUTPUT | GPIO_ACTIVE_HIGH);
	if (ret) {
		LOG_ERR("Unable to config %d: %d",
			BRD_PWR_RSMRST_PIN, ret);
		return ret;
	}

	ret = gpio_pin_set(rsm_dev, BRD_PWR_RSMRST_PIN, 0);
	if (ret) {
		LOG_ERR("Unable to initialize %d",
			BRD_PWR_RSMRST_PIN);
		return -1;
	}
#endif

	espi_init();

#if DT_NODE_HAS_STATUS(BRD_PWR_NODE, okay)
	ret = wait_for_pin(pwrgd_dev, BRD_PWR_PWRGD_PIN,
			   PWR_SEQ_TIMEOUT, 1);
	if (ret) {
		LOG_ERR("RSMRST_PWRGD timeout");
		return ret;
	}

	ret = gpio_pin_set(rsm_dev, BRD_PWR_RSMRST_PIN, 1);
	if (ret) {
		LOG_ERR("Failed to set rsm err: %d", ret);
		return ret;
	}
#endif
	ret = wait_for_espi_reset(1);
	if (ret) {
		LOG_INF("ESPI_RESET timeout");
		return ret;
	}

#ifdef CONFIG_ESPI_FLASH_CHANNEL
	/* Flash operation need to be perform before VW handshake or
	 * after eSPI host completes full initialization.
	 * This sample code can't assume a full initialized eSPI host
	 * so flash operations are perform here.
	 */
	bool flash_sts;

	do {
		flash_sts = espi_get_channel_status(espi_dev,
						    ESPI_CHANNEL_FLASH);
		k_busy_wait(100);
	} while (!flash_sts);

	/* eSPI flash test can fail and rest of operation can continue */
	ret = espi_flash_test(TARGET_FLASH_REGION, 1);
	if (ret) {
		LOG_INF("eSPI flash test failed %d", ret);
	}
#endif

	/* Showcase VW channel by exchanging virtual wires with eSPI host */
	ret = espi_handshake();
	if (ret) {
		LOG_ERR("eSPI VW handshake failed %d", ret);
		return ret;
	}

	/*  Attempt to use OOB channel to read temperature, regardless of
	 * if is enabled or not.
	 */
	for (int i = 0; i < 5; i++) {
		int temp;

		ret = get_pch_temp(espi_dev, &temp);
		if (ret)  {
			LOG_ERR("eSPI OOB transaction failed %d", ret);
		} else {
			LOG_INF("Temp: %d ", temp);
		}
	}

	/* Cleanup */
	k_sleep(K_SECONDS(1));
	espi_remove_callback(espi_dev, &espi_bus_cb);
	espi_remove_callback(espi_dev, &vw_rdy_cb);
	espi_remove_callback(espi_dev, &vw_cb);

	LOG_INF("eSPI sample completed err: %d", ret);

	return ret;
}

void main(void)
{
	espi_test();
}
