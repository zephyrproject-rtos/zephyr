/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ieee802154_qorvo_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dw1000, LOG_LEVEL_INF);

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/debug/stack.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>

#include <zephyr/sys/byteorder.h>
#include <string.h>
#include <zephyr/random/random.h>
#include <zephyr/debug/stack.h>
#include <math.h>

#include <zephyr/drivers/gpio.h>

#include <zephyr/net/ieee802154_radio.h>
#include "ieee802154_dw1000_regs.h"


#define DT_DRV_COMPAT decawave_dw1000

#define DW1000_TX_ANT_DLY		16450
#define DW1000_RX_ANT_DLY		16450

#define DWT_WORK_QUEUE_STACK_SIZE	512

static struct k_work_q dwt_work_queue;
static K_KERNEL_STACK_DEFINE(dwt_work_queue_stack,
			     DWT_WORK_QUEUE_STACK_SIZE);

static const struct dwt_hi_cfg dw1000_0_config = {
	.bus = SPI_DT_SPEC_INST_GET(0, SPI_WORD_SET(8), 0),
	.irq_gpio = GPIO_DT_SPEC_INST_GET(0, int_gpios),
	.rst_gpio = GPIO_DT_SPEC_INST_GET(0, reset_gpios),
};

static void dwt_gpio_callback(const struct device *dev,
			      struct gpio_callback *cb, uint32_t pins)
{
	struct dwt_context *ctx = CONTAINER_OF(cb, struct dwt_context, gpio_cb);

	LOG_DBG("IRQ callback triggered %p", ctx);
	k_work_submit_to_queue(&dwt_work_queue, &ctx->irq_cb_work);
}

static int dwt_initialise_dev(const struct device *dev)
{
	struct dwt_context *ctx = dev->data;
	uint32_t otp_val = 0;
	uint8_t xtal_trim;

	dwt_set_sysclks_xti(dev, false);
	ctx->sleep_mode = 0;

	/* Disable PMSC control of analog RF subsystem */
	dwt_reg_write_u16(dev, DWT_PMSC_ID, DWT_PMSC_CTRL1_OFFSET,
			  DWT_PMSC_CTRL1_PKTSEQ_DISABLE);

	/* Clear all status flags */
	dwt_reg_write_u32(dev, DWT_SYS_STATUS_ID, 0, DWT_SYS_STATUS_MASK_32);

	/*
	 * Apply soft reset,
	 * see SOFTRESET field description in DW1000 User Manual.
	 */
	dwt_reg_write_u8(dev, DWT_PMSC_ID, DWT_PMSC_CTRL0_SOFTRESET_OFFSET,
			 DWT_PMSC_CTRL0_RESET_ALL);
	k_sleep(K_MSEC(1));
	dwt_reg_write_u8(dev, DWT_PMSC_ID, DWT_PMSC_CTRL0_SOFTRESET_OFFSET,
			 DWT_PMSC_CTRL0_RESET_CLEAR);

	dwt_set_sysclks_xti(dev, false);

	/*
	 * This bit (a.k.a PLLLDT) should be set to ensure reliable
	 * operation of the CPLOCK bit.
	 */
	dwt_reg_write_u8(dev, DWT_EXT_SYNC_ID, DWT_EC_CTRL_OFFSET,
			 DWT_EC_CTRL_PLLLCK);

	/* Kick LDO if there is a value programmed. */
	otp_val = dwt_otpmem_read(dev, DWT_OTP_LDOTUNE_ADDR);
	if ((otp_val & 0xFF) != 0) {
		dwt_reg_write_u8(dev, DWT_OTP_IF_ID, DWT_OTP_SF,
				 DWT_OTP_SF_LDO_KICK);
		ctx->sleep_mode |= DWT_AON_WCFG_ONW_LLDO;
		LOG_INF("Load LDOTUNE_CAL parameter");
	}

	otp_val = dwt_otpmem_read(dev, DWT_OTP_XTRIM_ADDR);
	xtal_trim = otp_val & DWT_FS_XTALT_MASK;
	LOG_INF("OTP Revision 0x%02x, XTAL Trim 0x%02x",
		(uint8_t)(otp_val >> 8), xtal_trim);

	LOG_DBG("CHIP ID 0x%08x", dwt_otpmem_read(dev, DWT_OTP_PARTID_ADDR));
	LOG_DBG("LOT ID 0x%08x", dwt_otpmem_read(dev, DWT_OTP_LOTID_ADDR));
	LOG_DBG("Vbat 0x%02x", dwt_otpmem_read(dev, DWT_OTP_VBAT_ADDR));
	LOG_DBG("Vtemp 0x%02x", dwt_otpmem_read(dev, DWT_OTP_VTEMP_ADDR));

	if (xtal_trim == 0) {
		/* Set to default */
		xtal_trim = DWT_FS_XTALT_MIDRANGE;
	}

	/* For FS_XTALT bits 7:5 must always be set to binary “011” */
	xtal_trim |= BIT(6) | BIT(5);
	dwt_reg_write_u8(dev, DWT_FS_CTRL_ID, DWT_FS_XTALT_OFFSET, xtal_trim);

	/* Load LDE microcode into RAM, see 2.5.5.10 LDELOAD */
	dwt_set_sysclks_xti(dev, true);
	dwt_reg_write_u16(dev, DWT_OTP_IF_ID, DWT_OTP_CTRL,
			  DWT_OTP_CTRL_LDELOAD);
	k_sleep(K_MSEC(1));
	dwt_set_sysclks_xti(dev, false);
	ctx->sleep_mode |= DWT_AON_WCFG_ONW_LLDE;

	dwt_set_sysclks_auto(dev);

	if (!(dwt_reg_read_u8(dev, DWT_SYS_STATUS_ID, 0) &
	     DWT_SYS_STATUS_CPLOCK)) {
		LOG_WRN("PLL has not locked");
		return -EIO;
	}

	dwt_set_spi_fast(dev);

	/* Setup antenna delay values */
	dwt_reg_write_u16(dev, DWT_LDE_IF_ID, DWT_LDE_RXANTD_OFFSET,
			  DW1000_RX_ANT_DLY);
	dwt_reg_write_u16(dev, DWT_TX_ANTD_ID, DWT_TX_ANTD_OFFSET,
			  DW1000_TX_ANT_DLY);

	/* Clear AON_CFG1 register */
	dwt_reg_write_u8(dev, DWT_AON_ID, DWT_AON_CFG1_OFFSET, 0);
	/*
	 * Configure sleep mode:
	 *  - On wake-up load configurations from the AON memory
	 *  - preserve sleep mode configuration
	 *  - On Wake-up load the LDE microcode
	 *  - When available, on wake-up load the LDO tune value
	 */
	ctx->sleep_mode |= DWT_AON_WCFG_ONW_LDC |
			   DWT_AON_WCFG_PRES_SLEEP;
	dwt_reg_write_u16(dev, DWT_AON_ID, DWT_AON_WCFG_OFFSET,
			  ctx->sleep_mode);
	LOG_DBG("sleep mode 0x%04x", ctx->sleep_mode);
	/* Enable sleep and wake using SPI CSn */
	dwt_reg_write_u8(dev, DWT_AON_ID, DWT_AON_CFG0_OFFSET,
			 DWT_AON_CFG0_WAKE_SPI | DWT_AON_CFG0_SLEEP_EN);

	return 0;
}

static int dw1000_init(const struct device *dev)
{
	struct dwt_context *ctx = dev->data;
	const struct dwt_hi_cfg *hi_cfg = dev->config;

	LOG_INF("Initialize DW1000 Transceiver");
	k_sem_init(&ctx->phy_sem, 0, 1);

	/* slow SPI config */
	memcpy(&ctx->spi_cfg_slow, &hi_cfg->bus.config, sizeof(ctx->spi_cfg_slow));
	ctx->spi_cfg_slow.frequency = DWT_SPI_SLOW_FREQ;

	if (!spi_is_ready_dt(&hi_cfg->bus)) {
		LOG_ERR("SPI device not ready");
		return -ENODEV;
	}

	dwt_set_spi_slow(dev, DWT_SPI_SLOW_FREQ);

	/* Initialize IRQ GPIO */
	if (!gpio_is_ready_dt(&hi_cfg->irq_gpio)) {
		LOG_ERR("IRQ GPIO device not ready");
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&hi_cfg->irq_gpio, GPIO_INPUT)) {
		LOG_ERR("Unable to configure GPIO pin %u", hi_cfg->irq_gpio.pin);
		return -EINVAL;
	}

	gpio_init_callback(&(ctx->gpio_cb), dwt_gpio_callback,
			   BIT(hi_cfg->irq_gpio.pin));

	if (gpio_add_callback(hi_cfg->irq_gpio.port, &(ctx->gpio_cb))) {
		LOG_ERR("Failed to add IRQ callback");
		return -EINVAL;
	}

	/* Initialize RESET GPIO */
	if (!gpio_is_ready_dt(&hi_cfg->rst_gpio)) {
		LOG_ERR("Reset GPIO device not ready");
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&hi_cfg->rst_gpio, GPIO_INPUT)) {
		LOG_ERR("Unable to configure GPIO pin %u", hi_cfg->rst_gpio.pin);
		return -EINVAL;
	}

	LOG_INF("GPIO and SPI configured");

	dwt_hw_reset(dev);

	if (dwt_reg_read_u32(dev, DWT_DEV_ID_ID, 0) != DWT_DEVICE_ID) {
		LOG_ERR("Failed to read device ID %p", dev);
		return -ENODEV;
	}

	if (dwt_initialise_dev(dev)) {
		LOG_ERR("Failed to initialize DW1000");
		return -EIO;
	}

	if (dwt_configure_rf_phy(dev)) {
		LOG_ERR("Failed to configure RF PHY");
		return -EIO;
	}

	/* Allow Beacon, Data, Acknowledgement, MAC command */
	dwt_set_frame_filter(dev, true, DWT_SYS_CFG_FFAB | DWT_SYS_CFG_FFAD |
			     DWT_SYS_CFG_FFAA | DWT_SYS_CFG_FFAM);

	/*
	 * Enable system events:
	 *  - transmit frame sent,
	 *  - receiver FCS good,
	 *  - receiver PHY header error,
	 *  - receiver FCS error,
	 *  - receiver Reed Solomon Frame Sync Loss,
	 *  - receive Frame Wait Timeout,
	 *  - preamble detection timeout,
	 *  - receive SFD timeout
	 */
	dwt_reg_write_u32(dev, DWT_SYS_MASK_ID, 0,
			  DWT_SYS_MASK_MTXFRS |
			  DWT_SYS_MASK_MRXFCG |
			  DWT_SYS_MASK_MRXPHE |
			  DWT_SYS_MASK_MRXFCE |
			  DWT_SYS_MASK_MRXRFSL |
			  DWT_SYS_MASK_MRXRFTO |
			  DWT_SYS_MASK_MRXPTO |
			  DWT_SYS_MASK_MRXSFDTO);

	/* Initialize IRQ event work queue */
	ctx->dev = dev;

	k_work_queue_start(&dwt_work_queue, dwt_work_queue_stack,
			   K_KERNEL_STACK_SIZEOF(dwt_work_queue_stack),
			   CONFIG_SYSTEM_WORKQUEUE_PRIORITY, NULL);

	k_work_init(&ctx->irq_cb_work, dwt_irq_work_handler);

	dwt_setup_int(dev, true);

	LOG_INF("DW1000 device initialized and configured");

	return 0;
}


#define DWT_PSDU_LENGTH		(127 - DWT_FCS_LENGTH)

#if defined(CONFIG_IEEE802154_RAW_MODE)
DEVICE_DT_INST_DEFINE(0, dw1000_init, NULL,
		    &dwt_0_context, &dw1000_0_config,
		    POST_KERNEL, CONFIG_IEEE802154_DW1000_INIT_PRIO,
		    &dwt_radio_api);
#else
NET_DEVICE_DT_INST_DEFINE(0,
		dw1000_init,
		NULL,
		&dwt_0_context,
		&dw1000_0_config,
		CONFIG_IEEE802154_DW1000_INIT_PRIO,
		&dwt_radio_api,
		IEEE802154_L2,
		NET_L2_GET_CTX_TYPE(IEEE802154_L2),
		DWT_PSDU_LENGTH);
#endif
