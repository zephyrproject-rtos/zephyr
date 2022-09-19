/* ieee802154_rf2xx.h - IEEE 802.15.4 Driver definition for ATMEL RF2XX */

/*
 * Copyright (c) 2019-2020 Gerson Fernando Budke
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_IEEE802154_IEEE802154_RF2XX_H_
#define ZEPHYR_DRIVERS_IEEE802154_IEEE802154_RF2XX_H_

/* Runtime context structure
 ***************************
 */
enum rf2xx_trx_state_cmd_t {
	RF2XX_TRX_PHY_STATE_CMD_NOP             = 0x00,
	RF2XX_TRX_PHY_STATE_CMD_TX_START        = 0x02,
	RF2XX_TRX_PHY_STATE_CMD_FORCE_TRX_OFF   = 0x03,
	RF2XX_TRX_PHY_STATE_CMD_FORCE_PLL_ON    = 0x04,
	RF2XX_TRX_PHY_STATE_CMD_RX_ON           = 0x06,
	RF2XX_TRX_PHY_STATE_CMD_TRX_OFF         = 0x08,
	RF2XX_TRX_PHY_STATE_CMD_PLL_ON          = 0x09,
	RF2XX_TRX_PHY_STATE_CMD_PREP_DEEP_SLEEP = 0x10,
	RF2XX_TRX_PHY_STATE_CMD_RX_AACK_ON      = 0x16,
	RF2XX_TRX_PHY_STATE_CMD_TX_ARET_ON      = 0x19,
	/* Implemented by Software */
	RF2XX_TRX_PHY_STATE_CMD_SLEEP           = 0x0f,
	RF2XX_TRX_PHY_STATE_CMD_DEEP_SLEEP      = 0x20,
};

enum rf2xx_trx_state_status_t {
	RF2XX_TRX_PHY_STATUS_P_ON               = 0x00,
	RF2XX_TRX_PHY_STATUS_BUSY_RX            = 0x01,
	RF2XX_TRX_PHY_STATUS_BUSY_TX            = 0x02,
	RF2XX_TRX_PHY_STATUS_RX_ON              = 0x06,
	RF2XX_TRX_PHY_STATUS_TRX_OFF            = 0x08,
	RF2XX_TRX_PHY_STATUS_PLL_ON             = 0x09,
	RF2XX_TRX_PHY_STATUS_SLEEP              = 0x0f,
	RF2XX_TRX_PHY_STATUS_BUSY_RX_AACK       = 0x11,
	RF2XX_TRX_PHY_STATUS_BUSY_TX_ARET       = 0x12,
	RF2XX_TRX_PHY_STATUS_RX_AACK_ON         = 0x16,
	RF2XX_TRX_PHY_STATUS_TX_ARET_ON         = 0x19,
	RF2XX_TRX_PHY_STATUS_RX_ON_NOCLK        = 0x1c,
	RF2XX_TRX_PHY_STATUS_RX_AACK_ON_NOCLK   = 0x1d,
	RF2XX_TRX_PHY_STATUS_BUSY_RX_AACK_NOCLK = 0x1e,
	RF2XX_TRX_PHY_STATUS_STATE_TRANSITION   = 0x1f,
	RF2XX_TRX_PHY_STATUS_MASK               = 0x1f
};

/**
 *	TRAC STATE			RX_AACK	TX_ARET
 *	SUCCESS				   X	   X
 *	SUCCESS_DATA_PENDING			   X
 *	SUCCESS_WAIT_FOR_ACK		   X
 *	CHANNEL_ACCESS_FAILED			   X
 *	NO_ACK					   X
 *	INVALID				   X	   X
 */
enum rf2xx_trx_state_trac_t {
	RF2XX_TRX_PHY_STATE_TRAC_SUCCESS                = 0x00,
	RF2XX_TRX_PHY_STATE_TRAC_SUCCESS_DATA_PENDING   = 0x01,
	RF2XX_TRX_PHY_STATE_TRAC_SUCCESS_WAIT_FOR_ACK   = 0x02,
	RF2XX_TRX_PHY_STATE_TRAC_CHANNEL_ACCESS_FAILED  = 0x03,
	RF2XX_TRX_PHY_STATE_TRAC_NO_ACK                 = 0x05,
	RF2XX_TRX_PHY_STATE_TRAC_INVALID                = 0x07,
};

enum rf2xx_trx_model_t {
	RF2XX_TRX_MODEL_INV     = 0x00,
	RF2XX_TRX_MODEL_230     = 0x02,
	RF2XX_TRX_MODEL_231     = 0x03,
	RF2XX_TRX_MODEL_212     = 0x07,
	RF2XX_TRX_MODEL_232     = 0x0A,
	RF2XX_TRX_MODEL_233     = 0x0B,
};

enum rf2xx_trx_channel_page_t {
	RF2XX_TRX_CC_PAGE_0     = 0x00,
	RF2XX_TRX_CC_PAGE_2     = 0x02,
	RF2XX_TRX_CC_PAGE_5     = 0x05,
};

struct rf2xx_config {
	struct gpio_dt_spec irq_gpio;
	struct gpio_dt_spec reset_gpio;
	struct gpio_dt_spec slptr_gpio;
	struct gpio_dt_spec dig2_gpio;
	struct gpio_dt_spec clkm_gpio;

	struct spi_dt_spec spi;

	uint8_t inst;
	uint8_t has_mac;

	uint8_t const *tx_pwr_table;
	uint8_t tx_pwr_table_size;
	int8_t tx_pwr_min[2];
	int8_t tx_pwr_max[2];
};

struct rf2xx_context {
	struct net_if *iface;

	const struct device *dev;

	struct gpio_callback irq_cb;

	struct k_thread trx_thread;
	K_KERNEL_STACK_MEMBER(trx_stack,
			      CONFIG_IEEE802154_RF2XX_RX_STACK_SIZE);
	struct k_sem trx_isr_lock;
	struct k_sem trx_tx_sync;

	enum rf2xx_trx_model_t trx_model;
	enum rf2xx_trx_channel_page_t cc_page;
	enum rf2xx_trx_state_trac_t trx_trac;

	enum ieee802154_tx_mode tx_mode;
	uint8_t mac_addr[8];
	uint8_t pkt_lqi;
	uint8_t pkt_ed;
	int8_t trx_rssi_base;
	uint8_t trx_version;
	uint8_t rx_phr;
};

#endif /* ZEPHYR_DRIVERS_IEEE802154_IEEE802154_RF2XX_H_ */
