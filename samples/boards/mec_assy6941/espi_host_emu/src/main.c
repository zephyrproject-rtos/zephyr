/*
 * Copyright (c) 2024 Microchip Technology, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <soc.h>
#include <mec_pcr_api.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/sys_io.h>

#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, CONFIG_LOG_DEFAULT_LEVEL);

#include "espi_debug.h"
#include "espi_hc_emu.h"

struct espi_status_data {
	void *buf;
	uint32_t bufsz;
	uint32_t nitems;
};

#define APP_FLAG_GET_STATUS_LOOP BIT(0)

static volatile uint32_t app_flags;
static volatile uint32_t spin_val;
static volatile int ret_val;
static volatile int app_main_exit;

static struct espi_status_data esd;
static struct espi_hc_context hc;
/* eSPI specification: maximum 64 VWire groups */
/* static struct espi_hc_vw hcvw[64]; */
static struct espi_hc_vw hcvw2[64];

#define ESPI_CHAN_READY_PC_IDX 0
#define ESPI_CHAN_READY_VW_IDX 1
#define ESPI_CHAN_READY_OOB_IDX 2
#define ESPI_CHAN_READY_FC_IDX 3
volatile uint8_t chan_ready[4];

static uint8_t c2t_vw_host_idxs[] = {
	0x02u, 0x03u, 0x07u, 0x41u, 0x42u, 0x43u, 0x44u, 0x47u, 0x4au
};

static void spin_on(uint32_t id, int rval);
static int wait_espi_chan_ready(struct espi_hc_context *pch, uint16_t cfgid, int timeout_cnt);
int app_process_espi_status(struct espi_hc_context *hc, uint16_t espi_status,
			    struct espi_status_data *data);

int main(void)
{
	int ret = 0;
	struct espi_hc_vw vwg = {0};
	uint32_t io_addr_len = 0;
	uint32_t io_data = 0;
	uint32_t chan_config = 0;
	uint16_t cfgid = 0;
	uint16_t cmd_status = 0;

	LOG_INF("MEC5 eSPI Host Controller emulation for board: %s", DT_N_COMPAT_MODEL_IDX_0);

	ret = espi_hc_ctx_emu_init(&hc);
	if (ret) {
		LOG_ERR("eSPI HC CTX EMU init error %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}

	LOG_INF("Init eSPI HC emulation using QSPI0");
	ret = espi_hc_emu_init(MHZ(4));
	if (ret) {
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}

	LOG_INF("Wait for Target nREADY");
	do {
		ret = espi_hc_emu_is_target_ready();
	} while (ret == 0);

	LOG_INF("Target signalled Ready. De-assert ESPI_nRESET and send GET_CONFIG");
	ret = espi_hc_ctx_emu_ctrl(&hc, 1);
	if (ret) {
		LOG_ERR("Error eSPI HC emu control (enable)");
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}

	/* espi_hc_emu_espi_reset_n(1); */

	LOG_INF("Delay 100 ms to allow Target to print debug messages");
	k_sleep(K_MSEC(100));

	/* Get eSPI Device ID */
	LOG_INF("Read eSPI Device ID from Target");
	cfgid = ESPI_GET_CONFIG_DEV_ID;
	ret = espi_hc_ctx_emu_get_config(&hc, cfgid);
	if (ret) {
		LOG_ERR("FAIL: GET_CONFIG %u err (%d)", cfgid, ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}
	espi_debug_print_cap_word(cfgid, hc.version_id);
	espi_debug_pr_status(hc.pkt_status);

	LOG_INF("Read General Capabilities from Target");
	cfgid = ESPI_GET_CONFIG_GLB_CAP;
	ret = espi_hc_ctx_emu_get_config(&hc, cfgid);
	if (ret) {
		LOG_ERR("FAIL: GET_CONFIG %u err (%d)", cfgid, ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}
	espi_debug_print_cap_word(cfgid, hc.global_cap_cfg);
	espi_debug_pr_status(hc.pkt_status);

	if ((hc.global_cap_cfg & 0xf) != 0xfu) {
		LOG_ERR("PC, VW, OOB, FC are not all supported!");
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}

	/* Read VW Capabilities from Target */
	LOG_INF("Read VWire Capabilities from Target");
	cfgid = ESPI_GET_CONFIG_VW_CAP;
	ret = espi_hc_ctx_emu_get_config(&hc, cfgid);
	if (ret) {
		LOG_ERR("FAIL: GET_CONFIG %u err (%d)", cfgid, ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}
	espi_debug_print_cap_word(cfgid, hc.vw_cap_cfg);
	espi_debug_pr_status(hc.pkt_status);

	/* Update VW Cap-Config
	 * Set Selected number of VW groups = Supported
	 * Enable VW Channel
	 * b[21:16] = operating maximum number of VW groups (R/W)
	 * b[13:8] = maximum number of VW groups the Target HW supports (R/O)
	 * b[1] = VW Channel Ready from Target (R/O)
	 * b[0] = VW Channel Enable set by Host (R/W)
	 */
	LOG_INF("Update VW Cap/Cfg: Set max supported VW groups to 8 and enable VW channel");
	chan_config = hc.vw_cap_cfg;
	chan_config &= ~(0x3fu << 16);
	chan_config |= (0x08u << 16); /* Set operating max to 8 VW groups */
	chan_config |= BIT(0); /* enable VW channel */
	LOG_INF("New VW Caps/Cfg = 0x%08x", chan_config);

	cfgid = ESPI_GET_CONFIG_VW_CAP;
	ret = espi_hc_ctx_emu_set_config(&hc, cfgid, chan_config);
	if (ret) {
		LOG_ERR("FAIL: sET_CONFIG %u err (%d)", cfgid, ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}
	espi_debug_pr_status(hc.pkt_status);

	LOG_INF("Poll reading VW Config until Target sets VW Ready");
	ret = wait_espi_chan_ready(&hc, cfgid, -1);
	if (ret) {
		goto app_exit;
	}

	espi_debug_print_cap_word(cfgid, hc.vw_cap_cfg);

	LOG_INF("Issue GET_STATUS");
	ret = espi_hc_ctx_get_status(&hc);
	if (ret) {
		LOG_ERR("GET_STATUS failed (%d)", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}
	espi_debug_pr_status(hc.pkt_status);

	esd.buf = &hcvw2;
	esd.bufsz = sizeof(hcvw2);
	esd.nitems = 0u;
	ret = app_process_espi_status(&hc, hc.pkt_status, &esd);
	if (ret) {
		LOG_ERR("App process eSPI status: %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}

	LOG_INF("Read OOB Channel configuration from Target");
	cfgid = ESPI_GET_CONFIG_OOB_CAP;
	ret = espi_hc_ctx_emu_get_config(&hc, cfgid);
	if (ret) {
		LOG_ERR("FAIL: GET_CONFIG %u err (%d)", cfgid, ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}
	espi_debug_print_cap_word(cfgid, hc.oob_cap_cfg);
	espi_debug_pr_status(hc.pkt_status);

	/* Enable OOB channel with maximum packet size 64 bytes
	 * b[10:8] = OOB message max. payload size (R/W)
	 * b[1] = ready (R/O)
	 * b[0] = enable (R/W)
	 */
	LOG_INF("Update OOB Config: Set max packet size to 64 bytes and enable OOB channel");
	chan_config = hc.oob_cap_cfg;
	LOG_INF("Current OOB Cap/Cfg = 0x%08x", chan_config);
	chan_config &= ~(0x7u << 8);
	chan_config |= (1u << 8); /* b[10:8]=001b is 64 bytes */
	chan_config |= BIT(0); /* enable OOB channel */
	LOG_INF("New OOB Caps/Cfg = 0x%08x", chan_config);

	cfgid = ESPI_GET_CONFIG_OOB_CAP;
	ret = espi_hc_ctx_emu_set_config(&hc, cfgid, chan_config);
	if (ret) {
		LOG_ERR("FAIL: SET_CONFIG %u err (%d)", cfgid, ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}
	espi_debug_pr_status(hc.pkt_status);

	LOG_INF("Poll reading OOB Config until Target sets OOB Ready");
	ret = wait_espi_chan_ready(&hc, cfgid, -1);
	if (ret) {
		goto app_exit;
	}

	LOG_INF("Issue GET_STATUS");
	ret = espi_hc_ctx_get_status(&hc);
	if (ret) {
		LOG_ERR("GET_STATUS failed (%d)", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}
	espi_debug_pr_status(hc.pkt_status);

	esd.buf = &hcvw2;
	esd.bufsz = sizeof(hcvw2);
	esd.nitems = 0u;
	ret = app_process_espi_status(&hc, hc.pkt_status, &esd);
	if (ret) {
		LOG_ERR("App process eSPI status: %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}

	/* Based on soft-straps reconfigure VW and OOB channels
	 * Increase VW operating max. groups to 0x3f
	 * No change to OOB. All we could do is increase max payload size.
	 */
	LOG_INF("Update VW Cap/Cfg: Set max supported VW groups to 0x3f");
	chan_config = hc.vw_cap_cfg;
	LOG_INF("Current VW Cap/Cfg = 0x%08x", chan_config);
	chan_config &= ~(0x3fu << 16);
	chan_config |= (0x3fu << 16); /* Set operating max to 63 VW groups */
	LOG_INF("New VW Caps/Cfg = 0x%08x", chan_config);

	cfgid = ESPI_GET_CONFIG_VW_CAP;
	ret = espi_hc_ctx_emu_set_config(&hc, cfgid, chan_config);
	if (ret) {
		LOG_ERR("FAIL: VW SET_CONFIG (%d)", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}
	espi_debug_pr_status(hc.pkt_status);

	LOG_INF("Issue GET_STATUS");
	ret = espi_hc_ctx_get_status(&hc);
	if (ret) {
		LOG_ERR("GET_STATUS failed (%d)", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}

	espi_debug_pr_status(hc.pkt_status);

	esd.buf = &hcvw2;
	esd.bufsz = sizeof(hcvw2);
	esd.nitems = 0u;
	ret = app_process_espi_status(&hc, hc.pkt_status, &esd);
	if (ret) {
		LOG_ERR("App process eSPI status: %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}

	/* Enable Flash Channel
	 * Set b[14:12]=001b max. read request size of 64 bytes
	 * Set b[11]=0b CAFS
	 * Set b[10:8]=001b max max payload size of 64 bytes
	 * Set b[4:2]=001b flash block erase size 4KB
	 * b[1]=ready (R/O)
	 * b[0]=enable (R/W)
	 */
	LOG_INF("Update FC Cap/Cfg and enable");
	chan_config = hc.fc_cap_cfg;
	LOG_INF("Current FC Cap/Cfg = 0x%08x", chan_config);
	chan_config &= ~((0x7u << 12) | BIT(11) | (0x7u << 8) | (0x7u << 2));
	chan_config |= ((1u << 12) | (1u << 8) | (1u << 2) | BIT(0));
	LOG_INF("New VW Caps/Cfg = 0x%08x", chan_config);

	cfgid = ESPI_GET_CONFIG_FC_CAP;
	ret = espi_hc_ctx_emu_set_config(&hc, cfgid, chan_config);
	if (ret) {
		LOG_ERR("FAIL: FC SET_CONFIG (%d)", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}
	espi_debug_pr_status(hc.pkt_status);

	LOG_INF("Poll reading FC Config until Target sets FC Ready");
	ret = wait_espi_chan_ready(&hc, cfgid, -1);
	if (ret) {
		goto app_exit;
	}

	LOG_INF("Issue GET_STATUS");
	ret = espi_hc_ctx_get_status(&hc);
	if (ret) {
		LOG_ERR("GET_STATUS failed (%d)", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}
	espi_debug_pr_status(hc.pkt_status);

	esd.buf = &hcvw2;
	esd.bufsz = sizeof(hcvw2);
	esd.nitems = 0u;
	ret = app_process_espi_status(&hc, hc.pkt_status, &esd);
	if (ret) {
		LOG_ERR("App process eSPI status: %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}

	k_sleep(K_MSEC(100));

	LOG_INF("Issue GET_STATUS");
	ret = espi_hc_ctx_get_status(&hc);
	if (ret) {
		LOG_ERR("GET_STATUS failed (%d)", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}
	espi_debug_pr_status(hc.pkt_status);

	esd.buf = &hcvw2;
	esd.bufsz = sizeof(hcvw2);
	esd.nitems = 0u;
	ret = app_process_espi_status(&hc, hc.pkt_status, &esd);
	if (ret) {
		LOG_ERR("App process eSPI status: %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}

	memset(&vwg, 0, sizeof(vwg));
	ret = espi_hc_get_vw_group_data(&hc, 5u, &vwg);
	if (ret) {
		LOG_ERR("Could not find VWire at Host Index 5h. (%d)", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}

	if ((vwg.valid & (BIT(0) | BIT(3))) == (BIT(0) | BIT(3))) {
		LOG_INF("TARGET_BOOT_LOAD_DONE/STATUS are valid");
		LOG_INF("TARGET_BOOT_LOAD_DONE = %lu", (vwg.levels & BIT(0)));
		LOG_INF("TARGET_BOOT_LOAD_DONE_STATUS = %lu", ((vwg.levels & BIT(3)) >> 3));
		if ((vwg.levels & (BIT(0) | BIT(3))) == (BIT(0) | BIT(3))) {
			LOG_INF("Both VWires set by Target as expected");
		} else {
			LOG_ERR("Target did not set TARGET_BOOT_LOAD_DONE/STATUS "
				"to expected values");
			spin_on((uint32_t)__LINE__, ret);
			goto app_exit;
		}
	} else {
		LOG_ERR("TARGET_BOOT_LOAD_DONE/STATUS VWire values are not valid!");
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}

	LOG_INF("Send default values of all Controller-to-Target VWires");
	ret = espi_hc_ctx_emu_put_mult_host_index(&hc, c2t_vw_host_idxs,
						  ARRAY_SIZE(c2t_vw_host_idxs),
						  ESPI_HC_EMU_PUT_VW_RST_VAL);
	if (ret) {
		LOG_ERR("C2T PUT_VW reset values failed (%d)", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}

	LOG_INF("Issue GET_STATUS");
	ret = espi_hc_ctx_get_status(&hc);
	if (ret) {
		LOG_ERR("GET_STATUS failed (%d)", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}
	espi_debug_pr_status(hc.pkt_status);

	esd.buf = &hcvw2;
	esd.bufsz = sizeof(hcvw2);
	esd.nitems = 0u;
	ret = app_process_espi_status(&hc, hc.pkt_status, &esd);
	if (ret) {
		LOG_ERR("App process eSPI status: %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}

	/* Host set Target's VCC_PWRGD input High */
	ret = espi_hc_emu_vcc_pwrgd(1);
	if (ret) {
		LOG_ERR("Set VCC_PWRGD error: %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}

	/* PUT_VWIRE SUS_WARN# = 1. Host index 0x41 bit 0 */
	LOG_INF("Host send C2T nSUS_WARN = 1");
	ret = espi_hc_set_ct_vwire(&hc, 0x41u, 0, 1);
	if (ret) {
		LOG_ERR("In HC CTX set SUS_WARN# = 1 error %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}
	ret = espi_hc_ctx_emu_put_host_index(&hc, 0x41u);
	if (ret) {
		LOG_ERR("PUT_VW Host Index 0x41 error %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}

	/* Send VW nSLP_S5=1. Host index 0x02 bit 2 */
	LOG_INF("Host send C2T nSLP_S5 = 1");
	ret = espi_hc_set_ct_vwire(&hc, 0x02u, 2, 1);
	if (ret) {
		LOG_ERR("In HC CTX set SLP_S5# = 1 error %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}
	ret = espi_hc_ctx_emu_put_host_index(&hc, 0x02u);
	if (ret) {
		LOG_ERR("PUT_VW Host Index 0x02 error %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}

	/* Send VW nSLP_S4=1. Host index 0x02 bit 1 */
	LOG_INF("Host send C2T nSLP_S4 = 1");
	ret = espi_hc_set_ct_vwire(&hc, 0x02u, 1, 1);
	if (ret) {
		LOG_ERR("In HC CTX set SLP_S4# = 1 error %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}
	ret = espi_hc_ctx_emu_put_host_index(&hc, 0x02u);
	if (ret) {
		LOG_ERR("PUT_VW Host Index 0x02 error %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}

	/* Send VW nSLP_S3=1, Host index 0x02 bit 0 */
	LOG_INF("Host send C2T nSLP_S3 = 1");
	ret = espi_hc_set_ct_vwire(&hc, 0x02u, 0, 1);
	if (ret) {
		LOG_ERR("In HC CTX set SLP_S3# = 1 error %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}
	ret = espi_hc_ctx_emu_put_host_index(&hc, 0x02u);
	if (ret) {
		LOG_ERR("PUT_VW Host Index 0x02 error %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}

	/* Send VW nSLP_A=1, Host index 0x41 bit 3 */
	LOG_INF("Host send C2T nSLP_A = 1");
	ret = espi_hc_set_ct_vwire(&hc, 0x41u, 3, 1);
	if (ret) {
		LOG_ERR("In HC CTX set SLP_A# = 1 error %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}
	ret = espi_hc_ctx_emu_put_host_index(&hc, 0x41u);
	if (ret) {
		LOG_ERR("PUT_VW Host Index 0x41 error %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}

	/* Send VW nSLP_LAN=1, Host index 0x42 bit 0 */
	LOG_INF("Host send C2T nSLP_LAN = 1");
	ret = espi_hc_set_ct_vwire(&hc, 0x42u, 0, 1);
	if (ret) {
		LOG_ERR("In HC CTX set SLP_LAN# = 1 error %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}
	ret = espi_hc_ctx_emu_put_host_index(&hc, 0x42u);
	if (ret) {
		LOG_ERR("PUT_VW Host Index 0x42 error %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}

	/* Send VW nSLP_WLAN=1, Host index 0x42 bit 1 */
	LOG_INF("Host send C2T nSLP_WLAN = 1");
	ret = espi_hc_set_ct_vwire(&hc, 0x42u, 1, 1);
	if (ret) {
		LOG_ERR("In HC CTX set SLP_WLAN# = 1 error %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}
	ret = espi_hc_ctx_emu_put_host_index(&hc, 0x42u);
	if (ret) {
		LOG_ERR("PUT_VW Host Index 0x42 error %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}

	/* Send nSUS_STAT=1 and nPLTRST=1
	 * nSUS_STAT Host index 0x03 bit 0
	 * nPLTRST Host index 0x03 bit 1
	 */
	LOG_INF("Host send C2T nSLP_WLAN = 1 and nPLTRST = 1");
	ret = espi_hc_set_ct_vwire(&hc, 0x03u, 0, 1);
	if (ret) {
		LOG_ERR("In HC CTX set SUS_STAT# = 1 error %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}
	ret = espi_hc_set_ct_vwire(&hc, 0x03u, 1, 1);
	if (ret) {
		LOG_ERR("In HC CTX set PLTRST# = 1 error %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}
	ret = espi_hc_ctx_emu_put_host_index(&hc, 0x03u);
	if (ret) {
		LOG_ERR("PUT_VW Host Index 0x03 error %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}

	k_sleep(K_MSEC(50));

	/* write 1-byte to I/O 0x62 currently mapped to EC ACPI_EC0 */
	io_addr_len = 0x10062u;
	io_data = 0x5Au;
	cmd_status = 0u;
	LOG_INF("Write to ACPI_EC0 I/O: 1 byte data = 0x%02x", io_data);
	ret = espi_hc_emu_put_iowr(&hc, io_addr_len, io_data, &cmd_status);
	if (ret) {
		LOG_ERR("eSPI EMU PUT_IOWR error %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}

	io_addr_len = 0x10080u;
	io_data = 0x69u;
	cmd_status = 0u;
	LOG_INF("8-bit Write I/O Port 0x%02x = 0x%02x", io_addr_len, io_data);
	ret = espi_hc_emu_put_iowr(&hc, io_addr_len, io_data, &cmd_status);
	if (ret) {
		LOG_ERR("eSPI EMU PUT_IOWR error %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}

	k_sleep(K_MSEC(50));


	io_addr_len = 0x10081u;
	io_data = 0x6Au;
	cmd_status = 0u;
	LOG_INF("8-bit Write I/O Port 0x%02x = 0x%02x", io_addr_len, io_data);
	ret = espi_hc_emu_put_iowr(&hc, io_addr_len, io_data, &cmd_status);
	if (ret) {
		LOG_ERR("eSPI EMU PUT_IOWR error %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}

	k_sleep(K_MSEC(50));

	io_addr_len = 0x10082u;
	io_data = 0x6Bu;
	cmd_status = 0u;
	LOG_INF("8-bit Write I/O Port 0x%02x = 0x%02x", io_addr_len, io_data);
	ret = espi_hc_emu_put_iowr(&hc, io_addr_len, io_data, &cmd_status);
	if (ret) {
		LOG_ERR("eSPI EMU PUT_IOWR error %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}

	k_sleep(K_MSEC(50));

	io_addr_len = 0x10083u;
	io_data = 0x6Cu;
	cmd_status = 0u;
	LOG_INF("8-bit Write I/O Port 0x%02x = 0x%02x", io_addr_len, io_data);
	ret = espi_hc_emu_put_iowr(&hc, io_addr_len, io_data, &cmd_status);
	if (ret) {
		LOG_ERR("eSPI EMU PUT_IOWR error %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}

	k_sleep(K_MSEC(50));

	io_addr_len = 0x20080u;
	io_data = 0x4321u;
	cmd_status = 0u;
	LOG_INF("16-bit Write I/O Port 0x%02x = 0x%04x", io_addr_len, io_data);
	ret = espi_hc_emu_put_iowr(&hc, io_addr_len, io_data, &cmd_status);
	if (ret) {
		LOG_ERR("eSPI EMU PUT_IOWR error %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}

	k_sleep(K_MSEC(50));

	io_addr_len = 0x20082u;
	io_data = 0x9876u;
	cmd_status = 0u;
	LOG_INF("16-bit Write I/O Port 0x%02x = 0x%04x", io_addr_len, io_data);
	ret = espi_hc_emu_put_iowr(&hc, io_addr_len, io_data, &cmd_status);
	if (ret) {
		LOG_ERR("eSPI EMU PUT_IOWR error %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}

	k_sleep(K_MSEC(50));

	io_addr_len = 0x20081u;
	io_data = 0xA987u;
	cmd_status = 0u;
	LOG_INF("16-bit Write I/O Port 0x%02x = 0x%04x", io_addr_len, io_data);
	ret = espi_hc_emu_put_iowr(&hc, io_addr_len, io_data, &cmd_status);
	if (ret) {
		LOG_ERR("eSPI EMU PUT_IOWR error %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}

	k_sleep(K_MSEC(50));

	io_addr_len = 0x40080u;
	io_data = 0x99887766u;
	cmd_status = 0u;
	LOG_INF("32-bit Write I/O Port 0x%02x = 0x%08x", io_addr_len, io_data);
	ret = espi_hc_emu_put_iowr(&hc, io_addr_len, io_data, &cmd_status);
	if (ret) {
		LOG_ERR("eSPI EMU PUT_IOWR error %d", ret);
		spin_on((uint32_t)__LINE__, ret);
		goto app_exit;
	}

	k_sleep(K_MSEC(50));

	app_flags |= APP_FLAG_GET_STATUS_LOOP;
	if (app_flags & APP_FLAG_GET_STATUS_LOOP) {
		LOG_INF("Application loop issuing eSPI GET_STATUS");
	}

	while (app_flags & APP_FLAG_GET_STATUS_LOOP) {
		ret = espi_hc_ctx_get_status(&hc);
		if (ret) {
			LOG_ERR("GET_STATUS failed (%d)", ret);
			spin_on((uint32_t)__LINE__, ret);
			goto app_exit;
		}
		if (hc.pkt_status & ESPI_STATUS_ALL_AVAIL) {
			esd.buf = &hcvw2;
			esd.bufsz = sizeof(hcvw2);
			esd.nitems = 0u;
			ret = app_process_espi_status(&hc, hc.pkt_status, &esd);
		}

		k_sleep(K_MSEC(200));
	}

	LOG_INF("Application Done");
	spin_val = 99u;
	while (spin_val) {
		k_sleep(K_MSEC(100));
	}

app_exit:
	LOG_INF("App Exit");
	log_panic();
	app_main_exit = 1;

	return 0;
}

static void spin_on(uint32_t id, int rval)
{
	spin_val = id;
	ret_val = rval;

	LOG_INF("spin id = %u", id);

	while (spin_val) {
		;
	}
}

int app_process_espi_status(struct espi_hc_context *hc, uint16_t espi_status,
			    struct espi_status_data *data)
{
	int rc = 0;
	uint8_t nvw_recv = 0;

	if (!hc || !data) {
		return -EINVAL;
	}

	LOG_INF("Process eSPI protocol status = 0x%04x", espi_status);
	if (espi_status & BIT(ESPI_STATUS_VW_AVAIL_POS)) {
		LOG_INF("VWires available: Read VW groups");

		struct espi_hc_vw *vws = (struct espi_hc_vw *)data->buf;
		uint32_t nvws = data->bufsz / sizeof(struct espi_hc_vw);

		rc = espi_hc_ctx_emu_get_vws(hc, vws, (uint8_t)(nvws & 0xffu), &nvw_recv);
		if (rc) {
			return rc;
		}
		data->nitems = nvw_recv;
		rc = espi_hc_ctx_emu_update_t2c_vw(hc, vws, nvw_recv);
		if (rc) {
			LOG_ERR("eSPI HC CTX update T2C error %d", rc);
			return rc;
		}
		espi_debug_print_vws(hc->t2c_vw, ESPI_MAX_TC_VW_GROUPS);
	}
	if (espi_status & BIT(ESPI_STATUS_PC_AVAIL_POS)) {
		LOG_INF("PC available");
		/* TODO Issue GET_PC */
	}
	if (espi_status & BIT(ESPI_STATUS_PC_NP_AVAIL_POS)) {
		LOG_INF("PC NP available");
		/* TODO Issue GET_NP */
	}
	if (espi_status & BIT(ESPI_STATUS_OOB_AVAIL_POS)) {
		LOG_INF("OOB available");
		/* TODO Issue GET_OOB */
	}
	if (espi_status & BIT(ESPI_STATUS_FC_AVAIL_POS)) {
		LOG_INF("FC available");
		/* TODO Issue GET_FC */
	}
	if (espi_status & BIT(ESPI_STATUS_FC_NP_AVAIL_POS)) {
		LOG_INF("FC NP available");
		/* TODO Issue GET_FC_NP */
	}

	return 0;
}

static int wait_espi_chan_ready(struct espi_hc_context *pch, uint16_t cfgid, int timeout_cnt)
{
	uint32_t *pcfg = NULL;
	int ret = 0;

	switch (cfgid) {
	case ESPI_GET_CONFIG_PC_CAP:
		pcfg = &pch->pc_cap_cfg;
		break;
	case ESPI_GET_CONFIG_VW_CAP:
		pcfg = &pch->vw_cap_cfg;
		break;
	case ESPI_GET_CONFIG_OOB_CAP:
		pcfg = &pch->oob_cap_cfg;
		break;
	case ESPI_GET_CONFIG_FC_CAP:
		pcfg = &pch->fc_cap_cfg;
		break;
	default:
		return -EINVAL;
	}

	do {
		ret = espi_hc_ctx_emu_get_config(&hc, cfgid);
		if (ret) {
			LOG_ERR("FAIL: GET_CONFIG %u err (%d)", cfgid, ret);
			return -EIO;
		}
		/* Ready is bit[1] for all channels */
		if (*pcfg & BIT(1)) {
			LOG_INF("Target set Channel Ready = 1");
			return 0;
		}
		k_sleep(K_MSEC(20));
		if (timeout_cnt == 0) {
			break;
		} else if (timeout_cnt > 0) {
			timeout_cnt--;
		}
	} while (1);

	LOG_ERR("TIMEOUT Target did not set Channel Ready to 1");
	return -ETIMEDOUT;
}
