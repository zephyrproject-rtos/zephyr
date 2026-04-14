/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SIWX91X_NWP_H
#define SIWX91X_NWP_H
#include <zephyr/device.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/net/wifi.h>

#include "protocol/wifi/inc/sl_wifi.h"

struct net_buf;

/* The NWP don't have same memory mapping. This offset has to be applied to the address in the
 * dma descriptor.
 */
#define SIWX91X_NWP_MEMORY_OFFSET_ADDRESS 0x00500000

/* Declare NWP shared structs with __packed with __aligned(4). Otherwise, compiler may access to the
 * register byte per byte which break the communication with the remote processor
 */
struct siwx91x_nwp_dma_desc {
	uint8_t *addr;
	uint16_t length;
} __packed __aligned(4);

/* TA Status register */
#define SIWX91X_NWP_WIFI_BUFFER_FULL BIT(0)
#define SIWX91X_NWP_BUFFER_EMPTY     BIT(1)
#define SIWX91X_NWP_RX_PKT_PENDING   BIT(3)
#define SIWX91X_NWP_BLE_BUFFER_FULL  BIT(4)
#define SIWX91X_NWP_ASSERT_INTR      BIT(7)

struct siwx91x_nwp_ta_regs {
	volatile uint32_t reserved0;
	volatile uint32_t status;
	volatile uint32_t reserved1[11];
	volatile struct siwx91x_nwp_dma_desc *dma_desc_tx;
	volatile uint32_t reserved2;
	volatile uint32_t bootloader_out;
	volatile uint32_t reserved3[7];
	volatile struct siwx91x_nwp_dma_desc *dma_desc_rx;
	volatile uint32_t reserved4[11];
	volatile uint32_t ta_int_set;
	volatile uint32_t ta_int_clr;
} __packed __aligned(4);

/*  M4 to TA interrupt bits (m4_int_*) */
#define SIWX91X_RX_BUFFER_VALID          BIT(1)
#define SIWX91X_TX_PKT_PENDING           BIT(2)
#define SIWX91X_PROGRAM_COMMON_FLASH     BIT(4)
#define SIWX91X_UPGRADE_M4_IMAGE         BIT(5)
#define SIWX91X_M4_WAITING_FOR_TA_FLASH  BIT(6)
#define SIWX91X_M4_WAITING_FOR_TA_DEINIT BIT(8)

/* M4 status register */
#define SIWX91X_M4_WAKEUP_TA             BIT(0)
#define SIWX91X_M4_IS_ACTIVE             BIT(1)
#define SIWX91X_TA_WAKEUP_M4             BIT(2)
#define SIWX91X_TA_IS_ACTIVE             BIT(3)

/* TA to M4 interrupt bits (ta_int) */
#define SIWX91X_RX_PKT_DONE              BIT(1)
#define SIWX91X_TX_PKT_DONE              BIT(2)
#define SIWX91X_TA_USING_FLASH           BIT(3)
#define SIWX91X_M4_IMAGE_UPGRADATION     BIT(4)
#define SIWX91X_TA_WRITING_ON_FLASH      BIT(5)
#define SIWX91X_SIDE_BAND_CRYPTO_DONE    BIT(6)
#define SIWX91X_NWP_DEINIT               BIT(7)
#define SIWX91X_TA_BUFFER_FULL_CLEAR     BIT(8)
#define SIWX91X_TURN_ON_XTAL_REQUEST     BIT(9)
#define SIWX91X_TURN_OFF_XTAL_REQUEST    BIT(10)
#define SIWX91X_M4_IS_USING_XTAL_REQUEST BIT(11)

struct siwx91x_nwp_m4_regs {
	volatile uint32_t reserved0[3];
	volatile uint32_t ctrl;
	volatile uint32_t reserved1[87];
	volatile uint32_t m4_int_set;
	volatile uint32_t m4_int_clr;
	volatile uint32_t status;
	volatile uint32_t ta_int_mask_set;
	volatile uint32_t ta_int_mask_clr;
	volatile uint32_t ta_int;
} __packed __aligned(4);

struct siwx91x_nwp_config {
	const struct pinctrl_dev_config *pcfg;
	void (*config_irq)(const struct device *dev);
	struct siwx91x_nwp_ta_regs *ta_regs;
	struct siwx91x_nwp_m4_regs *m4_regs;

	uint8_t antenna_selection;
	bool support_1p8v;
	bool enable_xtal_correction;
	bool qspi_80mhz_clk;
	uint32_t clock_frequency;

	struct net_buf_pool *rx_pool;
	struct net_buf_pool *tx_pool;
	k_thread_stack_t *thread_stack;
	size_t thread_stack_size;
	int thread_priority;
};

/* The wifi/bt drivers are expected to define the list of callback in a wider structrure and retirve
 * it using CONTAINER_OF():
 *
 *  struct siwx91x_wifi_context {
 *     struct device *dev;
 *     struct siwx91x_nwp_wifi_cb cb;
 *  } = {
 *     .dev = dev,
 *     .cb.on_rx = siwx91x_wifi_rx,
 *  };
 *
 *  void siwx91x_wifi_rx(struct siwx91x_nwp_wifi_cb *arg, struct net_buf *buf) {
 *     struct siwx91x_wifi_context *ctxt = CONTAINER_OF(arg, struct siwx91x_wifi_context, cb);
 *     ...
 *  }
 */
struct siwx91x_nwp_wifi_cb {
	void (*on_rx)(const struct siwx91x_nwp_wifi_cb *ctxt, struct net_buf *buf);
	void (*on_scan_results)(const struct siwx91x_nwp_wifi_cb *ctxt, struct net_buf *buf);
	void (*on_join)(const struct siwx91x_nwp_wifi_cb *ctxt, struct net_buf *buf);
	void (*on_sta_connect)(const struct siwx91x_nwp_wifi_cb *ctxt,
			       uint8_t remote_addr[WIFI_MAC_ADDR_LEN]);
	void (*on_sta_disconnect)(const struct siwx91x_nwp_wifi_cb *ctxt,
				  uint8_t remote_addr[WIFI_MAC_ADDR_LEN]);
	void (*on_sock_select)(const struct siwx91x_nwp_wifi_cb *ctxt, int select_slot,
			       uint32_t read_fds, uint32_t write_fds);
};

struct siwx91x_nwp_bt_cb {
	void (*on_rx)(const struct siwx91x_nwp_bt_cb *ctxt, uint8_t type,
		      void *payload, size_t len);
};

struct siwx91x_nwp_cmd_queue {
	int id;
	struct k_fifo tx_queue;
	/* Keep a pointer to the current sync frame. So we can match the reply */
	struct net_buf *tx_in_progress;
	/* Unlock the emiter of the current sync frame */
	struct k_sem tx_done;
	/* Because the synchronisation, is made by a lonely semaphore (tx_done), only one
	 * synchronous frame can be queued at once (while several asynchronous frame can the
	 * queued). We just use a simple mutex to avoid simultaneous queries.
	 */
	struct k_mutex sync_frame_in_queue;
};

struct siwx91x_nwp_data {
	uint8_t power_profile;
	char current_country_code[2];
	const struct siwx91x_nwp_bt_cb *bt;
	const struct siwx91x_nwp_wifi_cb *wifi;

	struct k_sem global_lock;
	struct k_sem firmware_ready;

	struct k_thread thread_id;
	struct k_sem refresh_queues_state;
	struct k_sem tx_data_complete;
	struct k_sem rx_data_complete;
	struct siwx91x_nwp_cmd_queue cmd_queues[3];
	struct siwx91x_nwp_dma_desc tx_desc[2];
	struct siwx91x_nwp_dma_desc rx_desc[2];
	struct net_buf *rx_buf_in_progress;
	struct net_buf *tx_buf_in_progress;
};

#define SIWX91X_MAX_PAYLOAD_SIZE (1600 + 16)

/* Any frame sent to the NWP has to start with this header */
struct siwx91x_frame_desc {
	uint16_t length_and_queue;
	uint16_t command;
	/* The content of the array below varies with frame types. It's worth to note bytes 8 and 9
	 * are often used for the frame status in the replies.
	 */
	uint8_t  reserved[12];
	uint8_t  data[];
} __packed __aligned(4);
BUILD_ASSERT(sizeof(struct siwx91x_frame_desc) == 16, "wrong frame description definition");

/* This value is arbitrary. More slots user allocates on the NWP, less memory is available for
 * network buffers and other purpose.
 */
#define SIWX91X_MAX_CONCURRENT_SELECT 10

void siwx91x_nwp_register_wifi(const struct device *dev, const struct siwx91x_nwp_wifi_cb *val);
void siwx91x_nwp_register_bt(const struct device *dev, const struct siwx91x_nwp_bt_cb *val);
int siwx91x_nwp_reset(const struct device *dev, uint8_t oper_mode, bool hidden_ssid,
		      uint8_t max_num_sta);
int siwx91x_nwp_apply_power_profile(const struct device *dev);

#endif
