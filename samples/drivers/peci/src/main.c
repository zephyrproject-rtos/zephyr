/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <device.h>
#include <drivers/peci.h>
#include <soc.h>
#include <drivers/gpio.h>
#include <drivers/espi.h>
#include <drivers/espi_saf.h>
#include <drivers/spi.h>
#include <logging/log_ctrl.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(espi, CONFIG_ESPI_LOG_LEVEL);
#define ESPI_FREQ_20MHZ       20u
#define ESPI_FREQ_25MHZ       25u
#define ESPI_FREQ_66MHZ       66u

#define K_WAIT_DELAY          100u

#define TASK_STACK_SIZE         1024
#define PRIORITY                7

/* PECI Host address */
#define PECI_HOST_ADDR          0x30u
/* PECI Host bitrate 1Mbps */
#define PECI_HOST_BITRATE       1000u

#define PECI_CONFIGINDEX_TJMAX  16u
#define PECI_CONFIGHOSTID       0u
#define PECI_CONFIGPARAM        0u

#define PECI_SAFE_TEMP          72
/* eSPI event */
#define EVENT_MASK            0x0000FFFFu
#define EVENT_DETAILS_MASK    0xFFFF0000u
#define EVENT_DETAILS_POS     16u
#define EVENT_TYPE(x)         (x & EVENT_MASK)
#define EVENT_DETAILS(x)      ((x & EVENT_DETAILS_MASK) >> EVENT_DETAILS_POS)
#define CONFIG_ESPI_VIRTUAL_WIRE_TIMEOUT 5000


#define PWR_SEQ_TIMEOUT    3000u

/* The devicetree node identifier for the board power rails pins. */
#define BRD_PWR_NODE DT_INST(0, microchip_mec15xx_board_power)

#if DT_NODE_HAS_STATUS(BRD_PWR_NODE, okay)
#define BRD_PWR_PWRGD           DT_GPIO_LABEL(BRD_PWR_NODE, pwrg_gpios)
#define BRD_PWR_RSMRST          DT_GPIO_LABEL(BRD_PWR_NODE, rsm_gpios)
#define BRD_PWR_RSMRST_PIN      DT_GPIO_PIN(BRD_PWR_NODE, rsm_gpios)
#define BRD_PWR_PWRGD_PIN       DT_GPIO_PIN(BRD_PWR_NODE, pwrg_gpios)

static const struct device *pwrgd_dev;
static const struct device *rsm_dev;
#endif

#define ESPI_DEV      DT_LABEL(DT_NODELABEL(espi0))
static const struct device *espi_dev;
static struct espi_callback espi_bus_cb;
static struct espi_callback vw_rdy_cb;
static struct espi_callback vw_cb;
static struct espi_callback p80_cb;
static uint8_t espi_rst_sts;
#ifdef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
static struct espi_callback oob_cb;
#endif

static const struct device *peci_dev;
static bool peci_initialized;
static uint8_t tjmax;
static uint8_t rx_fcs;
static void monitor_temperature_func(void *dummy1, void *dummy2, void *dummy3);

static struct k_thread temp_id;
K_THREAD_STACK_DEFINE(temp_stack, TASK_STACK_SIZE);

int peci_ping(void)
{
	int ret;
	struct peci_msg packet;

	printk("%s\n", __func__);

	packet.addr = PECI_HOST_ADDR;
	packet.cmd_code = PECI_CMD_PING;
	packet.tx_buffer.buf = NULL;
	packet.tx_buffer.len = PECI_PING_WR_LEN;
	packet.rx_buffer.buf = NULL;
	packet.rx_buffer.len = PECI_PING_RD_LEN;

	ret = peci_transfer(peci_dev, &packet);
	if (ret) {
		printk("ping failed %d\n", ret);
		return ret;
	}

	return 0;
}

int peci_get_tjmax(uint8_t *tjmax)
{
	int ret;
	int retries = 3;
	uint8_t peci_resp;
	struct peci_msg packet;

	uint8_t peci_resp_buf[PECI_RD_PKG_LEN_DWORD+1];
	uint8_t peci_req_buf[] = { PECI_CONFIGHOSTID,
				PECI_CONFIGINDEX_TJMAX,
				PECI_CONFIGPARAM & 0x00FF,
				(PECI_CONFIGPARAM & 0xFF00) >> 8,
	};

	packet.tx_buffer.buf = peci_req_buf;
	packet.tx_buffer.len = PECI_RD_PKG_WR_LEN;
	packet.rx_buffer.buf = peci_resp_buf;
	packet.rx_buffer.len = PECI_RD_PKG_LEN_DWORD;

	do {
		rx_fcs = 0;
		packet.addr = PECI_HOST_ADDR;
		packet.cmd_code = PECI_CMD_RD_PKG_CFG0;

		ret = peci_transfer(peci_dev, &packet);

		for (int i = 0; i < PECI_RD_PKG_LEN_DWORD; i++) {
			printk("%02x\n", packet.rx_buffer.buf[i]);
		}

		peci_resp = packet.rx_buffer.buf[0];
		rx_fcs = packet.rx_buffer.buf[PECI_RD_PKG_LEN_DWORD];
		k_sleep(K_MSEC(1));
		printk("\npeci_resp %x\n", peci_resp);
		retries--;
	} while ((peci_resp != PECI_CC_RSP_SUCCESS) && (retries > 0));

	*tjmax = packet.rx_buffer.buf[3];

	return 0;
}

int peci_get_temp(int *temperature)
{
	int16_t raw_cpu_temp;
	int ret;
	struct peci_msg packet = {0};

	uint8_t peci_resp_buf[PECI_GET_TEMP_RD_LEN+1];

	rx_fcs = 0;
	packet.tx_buffer.buf = NULL;
	packet.tx_buffer.len = PECI_GET_TEMP_WR_LEN;
	packet.rx_buffer.buf = peci_resp_buf;
	packet.rx_buffer.len = PECI_GET_TEMP_RD_LEN;

	packet.addr = PECI_HOST_ADDR;
	packet.cmd_code = PECI_CMD_GET_TEMP0;

	ret = peci_transfer(peci_dev, &packet);
	if (ret) {
		printk("Get temp failed %d\n", ret);
		return ret;
	}

	rx_fcs = packet.rx_buffer.buf[PECI_GET_TEMP_RD_LEN];
	printk("R FCS %x\n", rx_fcs);
	printk("Temp bytes: %02x", packet.rx_buffer.buf[0]);
	printk("%02x\n", packet.rx_buffer.buf[1]);

	raw_cpu_temp = (int16_t)(packet.rx_buffer.buf[0] |
			(int16_t)((packet.rx_buffer.buf[1] << 8) & 0xFF00));

	if (raw_cpu_temp == 0x8000) {
		printk("Invalid temp %x\n", raw_cpu_temp);
		*temperature = PECI_SAFE_TEMP;
		return -1;
	}

	raw_cpu_temp = (raw_cpu_temp >> 6) | 0x7E00;
	*temperature = raw_cpu_temp + tjmax;

	return 0;
}

void read_temp(void)
{
	int ret;
	int temp;

	ret = peci_get_temp(&temp);

	if (!ret) {
		printk("Temperature %d C\n", temp);
	}
}

void get_max_temp(void)
{
	int ret;

	ret = peci_get_tjmax(&tjmax);
	if (ret) {
		printk("Fail to obtain maximum temperature: %d\n", ret);
	} else {
		printk("Maximum temperature: %u\n", tjmax);
	}
}

static void monitor_temperature_func(void *dummy1, void *dummy2, void *dummy3)
{
	while (true) {
		k_sleep(K_MSEC(1000));
		if (peci_initialized) {
			read_temp();
		
			peci_ping();
		}
		
	}
}

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
static void espi_reset_handler(const struct device *dev,
			       struct espi_callback *cb,
			       struct espi_event event)
{
	if (event.evt_type == ESPI_BUS_RESET) {
		espi_rst_sts = event.evt_data;
		LOG_INF("eSPI BUS reset %d", event.evt_data);
	}
}

/* eSPI logical channels enable/disable event handler */
static void espi_ch_handler(const struct device *dev,
			    struct espi_callback *cb,
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
static void vwire_handler(const struct device *dev, struct espi_callback *cb,
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
static void periph_handler(const struct device *dev, struct espi_callback *cb,
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
		.io_caps = ESPI_IO_MODE_SINGLE_LINE,
		.channel_caps = ESPI_CHANNEL_VWIRE | ESPI_CHANNEL_PERIPHERAL,
		.max_freq = ESPI_FREQ_20MHZ,
	};

	/* If eSPI driver supports additional capabilities use them */
#ifdef CONFIG_ESPI_OOB_CHANNEL
	cfg.channel_caps |= ESPI_CHANNEL_OOB;
#endif
#ifdef CONFIG_ESPI_FLASH_CHANNEL
	cfg.channel_caps |= ESPI_CHANNEL_FLASH;
	cfg.io_caps |= ESPI_IO_MODE_QUAD_LINES;
	cfg.max_freq = ESPI_FREQ_25MHZ;
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
#ifdef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
			   espi_init_callback(&oob_cb, oob_rx_handler,
			   ESPI_BUS_EVENT_OOB_RECEIVED);
#endif
	LOG_INF("complete");

	LOG_INF("eSPI test - callbacks registration... ");
	espi_add_callback(espi_dev, &espi_bus_cb);
	espi_add_callback(espi_dev, &vw_rdy_cb);
	espi_add_callback(espi_dev, &vw_cb);
	espi_add_callback(espi_dev, &p80_cb);
#ifdef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	espi_add_callback(espi_dev, &oob_cb);
#endif
	LOG_INF("complete");

	return ret;
}

#if DT_NODE_HAS_STATUS(BRD_PWR_NODE, okay)
static int wait_for_pin(const struct device *dev, uint8_t pin,
			uint16_t timeout,
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

static int wait_for_vwire(const struct device *espi_dev,
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

void main(void)
{
#if DT_NODE_HAS_STATUS(DT_ALIAS(peci_0), okay)
	int ret;
#endif
	k_sleep(K_SECONDS(1));

	printk("PECI sample test\n");
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

	k_thread_create(&temp_id, temp_stack, TASK_STACK_SIZE,
		monitor_temperature_func, NULL, NULL, NULL, PRIORITY,
		K_INHERIT_PERMS, K_FOREVER);

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

#ifndef CONFIG_ESPI_AUTOMATIC_BOOT_DONE_ACKNOWLEDGE
	/* When automatic acknowledge is disabled to perform lengthy operations
	 * in the eSPI slave, need to explicitly send slave boot
	 */
	bool vw_ch_sts;

	/* Simulate lengthy operation during boot */
	k_sleep(K_SECONDS(2));

	do {
		vw_ch_sts = espi_get_channel_status(espi_dev,
						    ESPI_CHANNEL_VWIRE);
		k_busy_wait(100);
	} while (!vw_ch_sts);


	send_slave_bootdone();
#endif


	/* Showcase VW channel by exchanging virtual wires with eSPI host */
	ret = espi_handshake();
	if (ret) {
		LOG_ERR("eSPI VW handshake failed %d", ret);
		return ret;
	}

#if DT_NODE_HAS_STATUS(DT_ALIAS(peci_0), okay)
	peci_dev = DEVICE_DT_GET(DT_ALIAS(peci_0));
	if (!device_is_ready(peci_dev)) {
		printk("Err: PECI device is not ready\n");
		return;
	}

	ret = peci_config(peci_dev, 1000u);
	if (ret) {
		printk("Err: Fail to configure bitrate\n");
		return;
	}

	peci_enable(peci_dev);

	tjmax = 100;

	get_max_temp();
	printk("Start thread...\n");
	k_thread_start(&temp_id);

	peci_initialized = true;
#endif

	/* Cleanup */
	k_sleep(K_SECONDS(1));
	espi_remove_callback(espi_dev, &espi_bus_cb);
	espi_remove_callback(espi_dev, &vw_rdy_cb);
	espi_remove_callback(espi_dev, &vw_cb);



}
