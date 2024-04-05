/*
 * Copyright (c) 2023 Microchip Technology, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <soc.h>
#include <mec_acpi_ec_api.h>
#include <mec_espi_api.h>
#include <mec_pcr_api.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/sys_io.h>

#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

#include "espi_debug.h"

/* #define APP_ESPI_EVENT_CAPTURE */

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)
#define ESPI0_NODE       DT_NODELABEL(espi0)

PINCTRL_DT_DEFINE(ZEPHYR_USER_NODE);

const struct pinctrl_dev_config *app_pinctrl_cfg = PINCTRL_DT_DEV_CONFIG_GET(ZEPHYR_USER_NODE);

const struct gpio_dt_spec target_n_ready_out_dt =
	GPIO_DT_SPEC_GET_BY_IDX(ZEPHYR_USER_NODE, espi_gpios, 0);

const struct gpio_dt_spec vcc_pwrgd_alt_in_dt =
	GPIO_DT_SPEC_GET_BY_IDX(ZEPHYR_USER_NODE, espi_gpios, 1);

static const struct device *espi_dev = DEVICE_DT_GET(ESPI0_NODE);

struct espi_io_regs *const espi_iobase = (struct espi_io_regs *)DT_REG_ADDR_BY_NAME(ESPI0_NODE, io);

#define OS_ACPI_EC_NODE DT_CHOSEN(espi_os_acpi)
struct acpi_ec_regs *const os_acpi_ec_regs = (struct acpi_ec_regs *)DT_REG_ADDR(OS_ACPI_EC_NODE);

static volatile uint32_t spin_val;
static volatile int ret_val;
static volatile int app_main_exit;

struct espi_cfg ecfg;
struct espi_callback espi_cb_reset;
struct espi_callback espi_cb_chan_ready;
struct espi_callback espi_cb_vw;
struct espi_callback espi_cb_periph;
struct gpio_callback gpio_cb_vcc_pwrgd;

#define ESPI_CHAN_READY_PC_IDX 0
#define ESPI_CHAN_READY_VW_IDX 1
#define ESPI_CHAN_READY_OOB_IDX 2
#define ESPI_CHAN_READY_FC_IDX 3
volatile uint8_t chan_ready[4];

#ifdef APP_ESPI_EVENT_CAPTURE
/* Use Zephyr FIFO for eSPI events */
struct espi_ev_data {
	void *fifo_reserved;
	struct espi_event ev;
};

BUILD_ASSERT(((sizeof(struct espi_ev_data) % 4) == 0),
	"!!! struct espi_ev_data size is not a multiple of 4 !!!");

struct k_fifo espi_ev_fifo;

#define MAX_ESPI_EVENTS 64
atomic_t espi_ev_count = ATOMIC_INIT(0);
static struct espi_ev_data espi_evs[MAX_ESPI_EVENTS];

static struct espi_ev_data *get_espi_ev_data(void);
static int init_espi_events(struct espi_ev_data *evs, size_t nevents);
static int push_espi_event(struct k_fifo *the_fifo, struct espi_ev_data *evd);
/* Not used static int pop_espi_event(struct k_fifo *the_fifo, struct espi_event *ev); */
/* Not used static void print_espi_event(struct espi_event *ev); */
static void print_espi_events(struct k_fifo *the_fifo);
#endif /* APP_ESPI_EVENT_CAPTURE */

static void spin_on(uint32_t id, int rval);

static void espi_reset_cb(const struct device *dev,
			  struct espi_callback *cb,
			  struct espi_event ev);

static void espi_chan_ready_cb(const struct device *dev,
			       struct espi_callback *cb,
			       struct espi_event ev);

static void espi_vw_received_cb(const struct device *dev,
				struct espi_callback *cb,
				struct espi_event ev);

static void espi_periph_cb(const struct device *dev,
			   struct espi_callback *cb,
			   struct espi_event ev);

#define GPIO_0242_PIN_MASK BIT(2)
static void gpio_cb(const struct device *port,
		    struct gpio_callback *cb,
		    gpio_port_pins_t pins);

int main(void)
{
	int ret = 0;
	bool vw_en = false;
	bool oob_en = false;
	bool fc_en = false;
	bool pc_en = false;
	uint8_t vw_state = 0;

	LOG_INF("MEC5 eSPI Target sample application for board: %s", DT_N_COMPAT_MODEL_IDX_0);
#ifdef APP_ESPI_EVENT_CAPTURE
	k_fifo_init(&espi_ev_fifo);
	init_espi_events(espi_evs, MAX_ESPI_EVENTS);
#endif
	ret = pinctrl_apply_state(app_pinctrl_cfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("App pin control state apply: %d", ret);
		spin_on((uint32_t)__LINE__, 1);
	}

	ret = gpio_pin_configure_dt(&target_n_ready_out_dt, GPIO_OUTPUT_HIGH);
	if (ret) {
		LOG_ERR("Configure Target_nReady GPIO as output hi (%d)", ret);
		spin_on((uint32_t)__LINE__, 2);
	}

	gpio_init_callback(&gpio_cb_vcc_pwrgd, gpio_cb, GPIO_0242_PIN_MASK);
	ret = gpio_add_callback_dt(&vcc_pwrgd_alt_in_dt, &gpio_cb_vcc_pwrgd);
	if (ret) {
		LOG_ERR("GPIO add callback for VCC_PWRGD error (%d)", ret);
		spin_on((uint32_t)__LINE__, ret);
	}

	ret = gpio_pin_configure_dt(&vcc_pwrgd_alt_in_dt, GPIO_INPUT);
	if (ret) {
		LOG_ERR("Configure Target_nReady GPIO as output hi (%d)", ret);
		spin_on((uint32_t)__LINE__, 2);
	}

	ret =  gpio_pin_interrupt_configure_dt(&vcc_pwrgd_alt_in_dt,
					       (GPIO_INPUT | GPIO_INT_EDGE_BOTH));
	if (ret) {
		LOG_ERR("GPIO interrupt config for VCC_PWRGD error (%d)", ret);
		spin_on((uint32_t)__LINE__, ret);
	}

	/* let pins settle */
	k_sleep(K_MSEC(10));

	if (!device_is_ready(espi_dev)) {
		LOG_ERR("eSPI device is not ready! (%d)", -1);
		spin_on((uint32_t)__LINE__, 3);
	}

	LOG_INF("eSPI driver is ready");

	LOG_INF("Releasing SoC internal RESET_VCC. Unblocks VCC_PWRGD input");
	mec_pcr_release_reset_vcc(1);

	LOG_INF("Platform configuration: Select nPLTRST = eSPI nPLTRST VWire");
	mec_pcr_host_reset_select(MEC_PCR_PLATFORM_RST_IS_ESPI_PLTRST);

	LOG_INF("Call eSPI driver config API");
	ecfg.io_caps = ESPI_IO_MODE_SINGLE_LINE;
	ecfg.channel_caps = (ESPI_CHANNEL_PERIPHERAL | ESPI_CHANNEL_VWIRE
			     | ESPI_CHANNEL_OOB | ESPI_CHANNEL_FLASH);
	ecfg.max_freq = 20u;

	ret = espi_config(espi_dev, &ecfg);
	if (ret) {
		LOG_ERR("eSPI configuration API error (%d)", ret);
		spin_on((uint32_t)__LINE__, ret);
	}

	LOG_INF("eSPI Target after config");
	espi_debug_print_config();

	LOG_INF("Target eSPI: register event callbacks");
	espi_cb_reset.handler = espi_reset_cb;
	espi_cb_reset.evt_type = ESPI_BUS_RESET;
	ret = espi_add_callback(espi_dev, &espi_cb_reset);
	if (ret) {
		LOG_ERR("Add callback for eSPI Reset error (%d)", ret);
		spin_on((uint32_t)__LINE__, ret);
	}

	espi_cb_chan_ready.handler = espi_chan_ready_cb;
	espi_cb_chan_ready.evt_type = ESPI_BUS_EVENT_CHANNEL_READY;
	ret = espi_add_callback(espi_dev, &espi_cb_chan_ready);
	if (ret) {
		LOG_ERR("Add callback for VW ChanEn error (%d)", ret);
		spin_on((uint32_t)__LINE__, ret);
	}

	espi_cb_vw.handler = espi_vw_received_cb;
	espi_cb_vw.evt_type = ESPI_BUS_EVENT_VWIRE_RECEIVED;
	ret = espi_add_callback(espi_dev, &espi_cb_vw);
	if (ret) {
		LOG_ERR("Add callback for VW received error (%d)", ret);
		spin_on((uint32_t)__LINE__, ret);
	}

	espi_cb_periph.handler = espi_periph_cb;
	espi_cb_periph.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION;
	ret = espi_add_callback(espi_dev, &espi_cb_periph);
	if (ret) {
		LOG_ERR("Add callback for peripheral channel error (%d)", ret);
		spin_on((uint32_t)__LINE__, ret);
	}

	LOG_INF("Signal Host emulator Target is Ready");
	ret = gpio_pin_set_dt(&target_n_ready_out_dt, 0);

	/* poll channel enables */
	LOG_INF("Poll driver for VW channel enable");
	do {
		vw_en = espi_get_channel_status(espi_dev, ESPI_CHANNEL_VWIRE);
	} while (!vw_en);
	LOG_INF("VW channel is enabled");

	LOG_INF("Poll driver for OOB channel enable");
	do {
		oob_en = espi_get_channel_status(espi_dev, ESPI_CHANNEL_OOB);
	} while (!oob_en);
	LOG_INF("OOB channel is enabled");

	LOG_INF("Poll driver for Flash channel enable");
	do {
		fc_en = espi_get_channel_status(espi_dev, ESPI_CHANNEL_FLASH);
	} while (!fc_en);
	LOG_INF("Flash channel is enabled");

	LOG_INF("Call eSPI driver to send VW TARGET_BOOT_LOAD_DONE/STATUS = 1");
	ret = espi_send_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SLV_BOOT_STS, 1);
	if (ret) {
		LOG_ERR("eSPI driver send ESPI_VWIRE_SIGNAL_SLV_BOOT_STS=1 error (%d)", ret);
		spin_on((uint32_t)__LINE__, ret);
	}
	ret = espi_send_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SLV_BOOT_DONE, 1);
	if (ret) {
		LOG_ERR("eSPI driver send ESPI_VWIRE_SIGNAL_SLV_BOOT_DONE=1 error (%d)", ret);
		spin_on((uint32_t)__LINE__, ret);
	}

	LOG_INF("Poll driver for Peripheral channel enable");
	do {
		pc_en = espi_get_channel_status(espi_dev, ESPI_CHANNEL_PERIPHERAL);
	} while (!pc_en);
	LOG_INF("Peripheral channel is enabled");

	k_sleep(K_MSEC(1000));

	LOG_INF("Send EC_IRQ=7 Serial IRQ to the Host");
	mec_espi_ld_sirq_set(espi_iobase, MEC_ESPI_LDN_EC, 0, 7u);
	mec_espi_gen_ec_sirq(espi_iobase);

	k_sleep(K_MSEC(500));

	LOG_INF("Toggle nSCI VWire");
	vw_state = 0;
	ret = espi_receive_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SCI, &vw_state);
	if (ret) {
		LOG_ERR("Driver receive VWire error (%d)", ret);
		spin_on((uint32_t)__LINE__, ret);
	}

	vw_state ^= BIT(0);
	ret = espi_send_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SCI, vw_state);
	if (ret) {
		LOG_ERR("Driver receive VWire error (%d)", ret);
		spin_on((uint32_t)__LINE__, ret);
	}

	k_sleep(K_MSEC(500)); /* delay due to Host eSPI emulator polling */

	vw_state ^= BIT(0);
	ret = espi_send_vwire(espi_dev, ESPI_VWIRE_SIGNAL_SCI, vw_state);
	if (ret) {
		LOG_ERR("Driver receive VWire error (%d)", ret);
		spin_on((uint32_t)__LINE__, ret);
	}

#ifdef APP_ESPI_EVENT_CAPTURE
	LOG_INF("Print events");
	while (run) {
		print_espi_events(&espi_ev_fifo);
	}
#endif

	LOG_INF("Application Done");
	spin_on(256, 0);
	app_main_exit = 1;

	return 0;
}

static void spin_on(uint32_t id, int rval)
{
	spin_val = id;
	ret_val = rval;

	LOG_INF("spin id = %u", id);
	log_panic(); /* flush log buffers */

	while (spin_val) {
		;
	}
}

/* GPIO callbacks */
static void gpio_cb(const struct device *port,
		    struct gpio_callback *cb,
		    gpio_port_pins_t pins)
{
	if (pins & GPIO_0242_PIN_MASK) {
		int state = gpio_pin_get_dt(&vcc_pwrgd_alt_in_dt);

		LOG_INF("GPIO CB: VCC_PWRGD_ALT = %d", state);
	} else {
		LOG_ERR("GPIO CB: unknown pin 0x%08x", pins);
	}
}

/* eSPI driver callbacks */
static void espi_reset_cb(const struct device *dev,
			  struct espi_callback *cb,
			  struct espi_event ev)
{
#ifdef APP_ESPI_EVENT_CAPTURE
	struct espi_ev_data *pevd = get_espi_ev_data();

	if (pevd) {
		pevd->ev.evt_type = ev.evt_type;
		pevd->ev.evt_details = ev.evt_details;
		pevd->ev.evt_data = ev.evt_data;
		push_espi_event(&espi_ev_fifo, pevd);
	}
#endif
	LOG_INF("eSPI CB: eSPI_nReset");
}

static void espi_chan_ready_cb(const struct device *dev,
			       struct espi_callback *cb,
			       struct espi_event ev)
{
#ifdef APP_ESPI_EVENT_CAPTURE
	struct espi_ev_data *pevd = NULL;

	pevd = get_espi_ev_data();
	if (pevd) {
		pevd->ev.evt_type = ev.evt_type;
		pevd->ev.evt_details = ev.evt_details;
		pevd->ev.evt_data = ev.evt_data;
		push_espi_event(&espi_ev_fifo, pevd);
	}
#endif

	LOG_INF("eSPI CB: channel ready: %u 0x%x 0x%x",
		ev.evt_type, ev.evt_data, ev.evt_details);

	if (ev.evt_details == ESPI_CHANNEL_VWIRE) {
		if (ev.evt_data == 1) {
			chan_ready[ESPI_CHAN_READY_VW_IDX] = 1;
		} else {
			chan_ready[ESPI_CHAN_READY_VW_IDX] = 0;
		}
	}

	if (ev.evt_details == ESPI_CHANNEL_PERIPHERAL) {
		if (ev.evt_data == 1) {
			chan_ready[ESPI_CHAN_READY_PC_IDX] = 1;
		} else {
			chan_ready[ESPI_CHAN_READY_PC_IDX] = 0;
		}
	}

	if (ev.evt_details == ESPI_CHANNEL_OOB) {
		if (ev.evt_data == 1) {
			chan_ready[ESPI_CHAN_READY_OOB_IDX] = 1;
		} else {
			chan_ready[ESPI_CHAN_READY_OOB_IDX] = 0;
		}
	}

	if (ev.evt_details == ESPI_CHANNEL_FLASH) {
		if (ev.evt_data == 1) {
			chan_ready[ESPI_CHAN_READY_FC_IDX] = 1;
		} else {
			chan_ready[ESPI_CHAN_READY_FC_IDX] = 0;
		}
	}
}

static void espi_vw_received_cb(const struct device *dev,
			       struct espi_callback *cb,
			       struct espi_event ev)
{
#ifdef APP_ESPI_EVENT_CAPTURE
	struct espi_ev_data *pevd = NULL;

	pevd = get_espi_ev_data();
	if (pevd) {
		pevd->ev.evt_type = ev.evt_type;
		pevd->ev.evt_details = ev.evt_details;
		pevd->ev.evt_data = ev.evt_data;
		push_espi_event(&espi_ev_fifo, pevd);
	}
#endif
	LOG_INF("eSPI CB: VW received: %u 0x%x 0x%x",
		ev.evt_type, ev.evt_data, ev.evt_details);
}

static void app_handle_acpi_ec(struct acpi_ec_regs *os_acpi_ec_regs)
{
	uint32_t data;
	uint8_t four_byte_mode, status;

	status = mec_acpi_ec_status(os_acpi_ec_regs);
	four_byte_mode = mec_acpi_ec_is_4byte_mode(os_acpi_ec_regs);

	if (status & MEC_ACPI_EC_STS_CMD) {
		data = mec_acpi_ec_host_to_ec_data_rd8(os_acpi_ec_regs, 0);
		LOG_INF("App ACPI_EC Clear IBF: Command received: 0x%02x", data & 0xffu);
	} else {
		if (four_byte_mode) {
			data = mec_acpi_ec_host_to_ec_data_rd32(os_acpi_ec_regs);
			LOG_INF("App ACPI_EC Clear IBF: 4-byte Data: 0x%04x", data);
		} else {
			data = mec_acpi_ec_host_to_ec_data_rd32(os_acpi_ec_regs);
			LOG_INF("App ACPI_EC Clear IBF: Data: 0x%02x", data & 0xffu);
		}
	}
	mec_acpi_ec_girq_clr(os_acpi_ec_regs, MEC_ACPI_EC_IBF_IRQ | MEC_ACPI_EC_OBE_IRQ);
}

static void espi_periph_cb(const struct device *dev,
			   struct espi_callback *cb,
			   struct espi_event ev)
{
	uint32_t details;
#ifdef APP_ESPI_EVENT_CAPTURE
	struct espi_ev_data *pevd = NULL;

	pevd = get_espi_ev_data();
	if (pevd) {
		pevd->ev.evt_type = ev.evt_type;
		pevd->ev.evt_details = ev.evt_details;
		pevd->ev.evt_data = ev.evt_data;
		push_espi_event(&espi_ev_fifo, pevd);
	}
#endif
	LOG_INF("eSPI CB: PC: %u 0x%x 0x%x", ev.evt_type, ev.evt_data, ev.evt_details);

	details = ev.evt_details & 0xffu;
	if (details == ESPI_PERIPHERAL_UART) {
		LOG_INF("  PC Host UART");
	} else if (details == ESPI_PERIPHERAL_8042_KBC) {
		LOG_INF("  PC 8042-KBC IBF");
	} else if (details == ESPI_PERIPHERAL_HOST_IO) {
		LOG_INF("  PC Host I/O");
#ifndef CONFIG_ESPI_PERIPHERAL_ACPI_EC_IBF_EVT_DATA
	/* if not defined the driver ISR does not clear ACPI_EC IBF status
	 * resulting in the interrupt not being cleared! Application must handle it.
	 * This means application must know which hardware ACPI_EC instance is being
	 * used. ACPI_EC definition requires reading data OS wrote to clear HW input
	 * buffer full (IBF) status.
	 */
		app_handle_acpi_ec(os_acpi_ec_regs);
#endif
	} else if (details == ESPI_PERIPHERAL_DEBUG_PORT80) {
		LOG_INF("  PC BIOS Debug I/O capture");
	} else if (details == ESPI_PERIPHERAL_HOST_IO_PVT) {
		LOG_INF("  PC Host PVT I/O");
	} else if (details == ESPI_PERIPHERAL_EC_HOST_CMD) {
		LOG_INF("  PC Host EC Cmd");
	} else if (details == ESPI_PERIPHERAL_HOST_MAILBOX) {
		LOG_INF("  PC Mailbox");
	}
}

#ifdef APP_ESPI_EVENT_CAPTURE
struct espi_vw_znames {
	enum espi_vwire_signal signal;
	const char *name_cstr;
};

const struct espi_vw_znames vw_names[] = {
	{ ESPI_VWIRE_SIGNAL_SLP_S3, "SLP_S3#" },
	{ ESPI_VWIRE_SIGNAL_SLP_S4, "SLP_S4#" },
	{ ESPI_VWIRE_SIGNAL_SLP_S5, "SLP_S5#" },
	{ ESPI_VWIRE_SIGNAL_OOB_RST_WARN, "OOB_RST_WARN" },
	{ ESPI_VWIRE_SIGNAL_PLTRST, "PLTRST#" },
	{ ESPI_VWIRE_SIGNAL_SUS_STAT, "SUS_STAT#" },
	{ ESPI_VWIRE_SIGNAL_NMIOUT, "NMIOUT#" },
	{ ESPI_VWIRE_SIGNAL_SMIOUT, "SMIOUT#" },
	{ ESPI_VWIRE_SIGNAL_HOST_RST_WARN, "HOST_RST_WARN" },
	{ ESPI_VWIRE_SIGNAL_SLP_A, "SLP_A#" },
	{ ESPI_VWIRE_SIGNAL_SUS_PWRDN_ACK, "SUS_PWRDN_ACK" },
	{ ESPI_VWIRE_SIGNAL_SUS_WARN, "SUS_WARN#" },
	{ ESPI_VWIRE_SIGNAL_SLP_WLAN, "SLP_WLAN#" },
	{ ESPI_VWIRE_SIGNAL_SLP_LAN, "SLP_LAN#" },
	{ ESPI_VWIRE_SIGNAL_HOST_C10, "HOST_C10" },
	{ ESPI_VWIRE_SIGNAL_DNX_WARN, "DNX_WARN" },
	{ ESPI_VWIRE_SIGNAL_PME, "PME#" },
	{ ESPI_VWIRE_SIGNAL_WAKE, "WAKE#" },
	{ ESPI_VWIRE_SIGNAL_OOB_RST_ACK, "OOB_RST_ACK" },
	{ ESPI_VWIRE_SIGNAL_SLV_BOOT_STS, "TARGET_BOOT_STS" },
	{ ESPI_VWIRE_SIGNAL_ERR_NON_FATAL, "ERR_NON_FATAL" },
	{ ESPI_VWIRE_SIGNAL_ERR_FATAL, "ERR_FATAL" },
	{ ESPI_VWIRE_SIGNAL_SLV_BOOT_DONE, "TARGET_BOOT_DONE" },
	{ ESPI_VWIRE_SIGNAL_HOST_RST_ACK, "HOST_RST_ACK" },
	{ ESPI_VWIRE_SIGNAL_RST_CPU_INIT, "RST_CPU_INIT" },
	{ ESPI_VWIRE_SIGNAL_SMI, "SMI#" },
	{ ESPI_VWIRE_SIGNAL_SCI, "SCI#" },
	{ ESPI_VWIRE_SIGNAL_DNX_ACK, "DNX_ACK" },
	{ ESPI_VWIRE_SIGNAL_SUS_ACK, "SUS_ACK#" },
	{ ESPI_VWIRE_SIGNAL_SLV_GPIO_0, "TARGET_GPIO_0" },
	{ ESPI_VWIRE_SIGNAL_SLV_GPIO_1, "TARGET_GPIO_1" },
	{ ESPI_VWIRE_SIGNAL_SLV_GPIO_2, "TARGET_GPIO_2" },
	{ ESPI_VWIRE_SIGNAL_SLV_GPIO_3, "TARGET_GPIO_3" },
	{ ESPI_VWIRE_SIGNAL_SLV_GPIO_4, "TARGET_GPIO_4" },
	{ ESPI_VWIRE_SIGNAL_SLV_GPIO_5, "TARGET_GPIO_5" },
	{ ESPI_VWIRE_SIGNAL_SLV_GPIO_6, "TARGET_GPIO_6" },
	{ ESPI_VWIRE_SIGNAL_SLV_GPIO_7, "TARGET_GPIO_7" },
	{ ESPI_VWIRE_SIGNAL_SLV_GPIO_8, "TARGET_GPIO_8" },
	{ ESPI_VWIRE_SIGNAL_SLV_GPIO_9, "TARGET_GPIO_9" },
	{ ESPI_VWIRE_SIGNAL_SLV_GPIO_10, "TARGET_GPIO_10" },
	{ ESPI_VWIRE_SIGNAL_SLV_GPIO_11, "TARGET_GPIO_11" },
};

const char *unknown_vwire = "Unknown VWire";

static const char *get_vw_name(uint32_t vwire_enum_val)
{
	for (size_t n = 0; n < ARRAY_SIZE(vw_names); n++) {
		const struct espi_vw_znames *vwn = &vw_names[n];

		if (vwn->signal == (enum espi_vwire_signal)vwire_enum_val) {
			return vwn->name_cstr;
		}
	}

	return unknown_vwire;
}

static int init_espi_events(struct espi_ev_data *evs, size_t nevents)
{
	if (!evs) {
		return -EINVAL;
	}

	memset(evs, 0, sizeof(struct espi_ev_data) * nevents);

	return 0;
}

static struct espi_ev_data *get_espi_ev_data(void)
{
	struct espi_ev_data *p = NULL;
	atomic_val_t evcnt = atomic_get(&espi_ev_count);

	if (evcnt >= MAX_ESPI_EVENTS) {
		return NULL;
	}

	p = &espi_evs[evcnt];
	atomic_inc(&espi_ev_count);

	return p;
}

static int push_espi_event(struct k_fifo *the_fifo, struct espi_ev_data *evd)
{
	if (!the_fifo || !evd) {
		return -EINVAL;
	}

	/* safe for ISRs */
	k_fifo_put(the_fifo, evd);

	return 0;
}

static int pop_espi_event(struct k_fifo *the_fifo, struct espi_event *ev)
{
	struct espi_ev_data *p = NULL;
	struct espi_event *pev = NULL;

	if (!the_fifo || !ev) {
		return -EINVAL;
	}

	if (k_fifo_is_empty(the_fifo)) {
		return -EAGAIN;
	}

	p = k_fifo_get(the_fifo, K_NO_WAIT);
	pev = &p->ev;
	ev->evt_type = pev->evt_type;
	ev->evt_data = pev->evt_data;
	ev->evt_details = pev->evt_details;

	return 0;
}

static void pr_decode_espi_event(uint32_t evt_type, uint32_t evt_data, uint32_t evt_details)
{
	switch (evt_type) {
	case ESPI_BUS_RESET:
		LOG_INF("eSPI Event: Bus Reset: data=0x%0x details=0x%0x", evt_data, evt_details);
		break;
	case ESPI_BUS_EVENT_CHANNEL_READY:
		LOG_INF("eSPI Event: Channel Ready: data=0x%0x details=0x%0x",
			evt_data, evt_details);
		break;
	case ESPI_BUS_EVENT_VWIRE_RECEIVED:
		const char *vwname = get_vw_name(evt_details);

		LOG_INF("eSPI Event: Host-to-Target VW Received: data=0x%0x details=0x%0x VW=%s",
			evt_data, evt_details, vwname);
		break;
	case ESPI_BUS_EVENT_OOB_RECEIVED:
		LOG_INF("eSPI Event: OOB Received: data=0x%0x details=0x%0x",
			evt_data, evt_details);
		break;
	case ESPI_BUS_PERIPHERAL_NOTIFICATION:
		LOG_INF("eSPI Event: Peripheral Notification: data=0x%0x details=0x%0x",
			evt_data, evt_details);
		break;
	case ESPI_BUS_SAF_NOTIFICATION:
		LOG_INF("eSPI Event: SAF Notification: data=0x%0x details=0x%0x",
			evt_data, evt_details);
		break;
	default:
		LOG_INF("eSPI Event: Unknown type=%u, data=0x%0x details=0x%0x",
			evt_type, evt_data, evt_details);
		break;
	}
}

static void print_espi_event(struct espi_event *ev)
{
	uint32_t t, dat, det;

	if (!ev) {
		return;
	}

	t = ev->evt_type;
	dat = ev->evt_data;
	det = ev->evt_details;
	pr_decode_espi_event(t, dat, det);
}

/* Print out the remaining events in the FIFO */
static void print_espi_events(struct k_fifo *the_fifo)
{
	struct espi_ev_data *p = NULL;
	uint32_t t = 0, dat = 0, det = 0;

	if (!the_fifo) {
		return;
	}

	p = k_fifo_get(the_fifo, K_NO_WAIT);
	while (p) {
		t = p->ev.evt_type;
		dat = p->ev.evt_data;
		det = p->ev.evt_details;
		pr_decode_espi_event(t, dat, det);
	}
}
#endif /* APP_ESPI_EVENT_CAPTURE */
