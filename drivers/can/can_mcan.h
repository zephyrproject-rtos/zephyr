/*
 * Copyright (c) 2020 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_DRIVERS_CAN_MCAN_H_
#define ZEPHYR_DRIVERS_CAN_MCAN_H_

#define NUM_STD_FILTER_ELEMENTS DT_PROP(DT_PATH(soc, can), std_filter_elements)
#define NUM_EXT_FILTER_ELEMENTS DT_PROP(DT_PATH(soc, can), ext_filter_elements)
#define NUM_RX_FIFO0_ELEMENTS   DT_PROP(DT_PATH(soc, can), rx_fifo0_elements)
#define NUM_RX_FIFO1_ELEMENTS   DT_PROP(DT_PATH(soc, can), rx_fifo0_elements)
#define NUM_RX_BUF_ELEMENTS     DT_PROP(DT_PATH(soc, can), rx_buffer_elements)
#define NUM_TX_EVENT_FIFO_ELEMENTS \
			DT_PROP(DT_PATH(soc, can), tx_event_fifo_elements)
#define NUM_TX_BUF_ELEMENTS     DT_PROP(DT_PATH(soc, can), tx_buffer_elements)


#ifdef CONFIG_CAN_STM32FD
#define NUM_STD_FILTER_DATA CONFIG_CAN_MAX_STD_ID_FILTER
#define NUM_EXT_FILTER_DATA CONFIG_CAN_MAX_EXT_ID_FILTER
#else
#define NUM_STD_FILTER_DATA NUM_STD_FILTER_ELEMENTS
#define NUM_EXT_FILTER_DATA NUM_EXT_FILTER_ELEMENTS
#endif

struct can_mcan_rx_fifo_hdr {
	union {
		struct {
			volatile u32_t ext_id : 29; /* Extended Identifier */
			volatile u32_t rtr    :  1; /* Retmote Transmission Request*/
			volatile u32_t xtd    :  1; /* Extended identifier */
			volatile u32_t esi    :  1; /* Error state indicator */
		};
		struct {
			volatile u32_t pad1   : 18;
			volatile u32_t std_id : 11; /* Standard Identifier */
			volatile u32_t pad2   : 3;
		};
	};

	volatile u32_t rxts : 16; /* Rx timestamp */
	volatile u32_t dlc  :  4; /* Data Length Code */
	volatile u32_t brs  :  1; /* Bit Rate Switch */
	volatile u32_t fdf  :  1; /* FD Format */
	volatile u32_t res  :  2; /* Reserved */
	volatile u32_t fidx :  7; /* Filter Index */
	volatile u32_t anmf :  1; /* Accepted non-matching frame */
} __packed;

struct can_mcan_rx_fifo {
	struct can_mcan_rx_fifo_hdr hdr;
	union {
		volatile u8_t  data[64];
		volatile u32_t data_32[16];
	};
} __packed;

struct can_mcan_mm {
	volatile u8_t idx : 5;
	volatile u8_t cnt : 3;
} __packed;

struct can_mcan_tx_fifo_hdr {
	union {
		struct {
			volatile u32_t ext_id : 29; /* Identifier */
			volatile u32_t rtr    :  1; /* Retmote Transmission Request*/
			volatile u32_t xtd    :  1; /* Extended identifier */
			volatile u32_t esi    :  1; /* Error state indicator */
		};
		struct {
			volatile u32_t pad1   : 18;
			volatile u32_t std_id : 11; /* Identifier */
			volatile u32_t pad2   :  3;
		};
	};
	volatile u16_t res1;      /* Reserved */
	volatile u8_t dlc  :  4; /* Data Length Code */
	volatile u8_t brs  :  1; /* Bit Rate Switch */
	volatile u8_t fdf  :  1; /* FD Format */
	volatile u8_t res2 :  1; /* Reserved */
	volatile u8_t efc  :  1; /* Event FIFO control (Store Tx events) */
	struct can_mcan_mm mm; /* Message marker */
} __packed;

struct can_mcan_tx_fifo {
	struct can_mcan_tx_fifo_hdr hdr;
	union {
		volatile u8_t  data[64];
		volatile u32_t data_32[16];
	};
} __packed;

#define can_mcan_TE_TX    0x1 /* TX event */
#define can_mcan_TE_TXC   0x2 /* TX event in spite of cancellation */

struct can_mcan_tx_event_fifo {
	volatile u32_t id   : 29; /* Identifier */
	volatile u32_t rtr  :  1; /* Retmote Transmission Request*/
	volatile u32_t xtd  :  1; /* Extended identifier */
	volatile u32_t esi  :  1; /* Error state indicator */

	volatile u16_t txts;      /* TX Timestamp */
	volatile u8_t dlc   :  4; /* Data Length Code */
	volatile u8_t brs   :  1; /* Bit Rate Switch */
	volatile u8_t fdf   :  1; /* FD Format */
	volatile u8_t et    :  2; /* Event type */
	struct can_mcan_mm mm; /* Message marker */
} __packed;

#define can_mcan_FCE_DISABLE    0x0
#define can_mcan_FCE_FIFO0      0x1
#define can_mcan_FCE_FIFO1      0x2
#define can_mcan_FCE_REJECT     0x3
#define can_mcan_FCE_PRIO       0x4
#define can_mcan_FCE_PRIO_FIFO0 0x5
#define can_mcan_FCE_PRIO_FIFO1 0x7

#define can_mcan_SFT_RANGE       0x0
#define can_mcan_SFT_DUAL        0x1
#define can_mcan_SFT_MASKED      0x2
#define can_mcan_SFT_DISABLED    0x3

struct can_mcan_std_filter {
	volatile u32_t id2  : 11; /* ID2 for dual or range, mask otherwise */
	volatile u32_t res  :  5;
	volatile u32_t id1  : 11;
	volatile u32_t sfce :  3; /* Filter config */
	volatile u32_t sft  :  2; /* Filter type */
} __packed;

#define can_mcan_EFT_RANGE_XIDAM 0x0
#define can_mcan_EFT_DUAL        0x1
#define can_mcan_EFT_MASKED      0x2
#define can_mcan_EFT_RANGE       0x3

struct can_mcan_ext_filter {
	volatile u32_t id1  : 29;
	volatile u32_t efce :  3; /* Filter config */
	volatile u32_t id2  : 29; /* ID2 for dual or range, mask otherwise */
	volatile u32_t res  :  1;
	volatile u32_t eft   : 2; /* Filter type */
} __packed;

struct can_mcan_msg_sram {
	volatile struct can_mcan_std_filter std_filt[NUM_STD_FILTER_ELEMENTS];
	volatile struct can_mcan_ext_filter ext_filt[NUM_EXT_FILTER_ELEMENTS];
	volatile struct can_mcan_rx_fifo rx_fifo0[NUM_RX_FIFO0_ELEMENTS];
	volatile struct can_mcan_rx_fifo rx_fifo1[NUM_RX_FIFO1_ELEMENTS];
	volatile struct can_mcan_rx_fifo rx_buffer[NUM_RX_BUF_ELEMENTS];
	volatile struct can_mcan_tx_event_fifo tx_event_fifo[NUM_TX_BUF_ELEMENTS];
	volatile struct can_mcan_tx_fifo tx_fifo[NUM_TX_BUF_ELEMENTS];
} __packed;

struct can_mcan_data {
	struct k_mutex inst_mutex;
	struct k_sem tx_sem;
	struct k_mutex tx_mtx;
	struct k_sem tx_fin_sem[NUM_TX_BUF_ELEMENTS];
	can_tx_callback_t tx_fin_cb[NUM_TX_BUF_ELEMENTS];
	void *tx_fin_cb_arg[NUM_TX_BUF_ELEMENTS];
	can_rx_callback_t rx_cb_std[NUM_STD_FILTER_DATA];
	can_rx_callback_t rx_cb_ext[NUM_EXT_FILTER_DATA];
	void *cb_arg_std[NUM_STD_FILTER_DATA];
	void *cb_arg_ext[NUM_EXT_FILTER_DATA];
	can_state_change_isr_t state_change_isr;
	u32_t std_filt_rtr;
	u32_t std_filt_rtr_mask;
	u8_t ext_filt_rtr;
	u8_t ext_filt_rtr_mask;
	struct can_mcan_mm mm;
} __aligned(4);

struct can_mcan_config {
	struct can_mcan_reg *can;   /*!< CAN Registers*/
	u32_t bus_speed;
	u32_t bus_speed_data;
	u16_t sjw;
	u16_t sample_point;
	u16_t prop_ts1;
	u16_t ts2;
#ifdef CONFIG_CAN_FD_MODE
	u16_t sample_point_data;
	u8_t sjw_data;
	u8_t prop_ts1_data;
	u8_t ts2_data;
#endif
};

struct can_mcan_reg;

int can_mcan_runtime_configure(const struct can_mcan_config *cfg,
			       enum can_mode mode, struct can_timing *timing,
			       struct can_timing *timing_data);

int can_mcan_init(struct device *dev, const struct can_mcan_config *cfg,
		  struct can_mcan_msg_sram *msg_ram,
		  struct can_mcan_data *data);

void can_mcan_line_0_isr(const struct can_mcan_config *cfg,
			 struct can_mcan_msg_sram *msg_ram,
			 struct can_mcan_data *data);

void can_mcan_line_1_isr(const struct can_mcan_config *cfg,
			 struct can_mcan_msg_sram *msg_ram,
			 struct can_mcan_data *data);

int can_mcan_recover(struct can_mcan_reg *can, k_timeout_t timeout);

int can_mcan_send(const struct can_mcan_config *cfg, struct can_mcan_data *data,
		  struct can_mcan_msg_sram *msg_ram,
		  const struct zcan_frame *frame,
		  k_timeout_t timeout, can_tx_callback_t callback,
		  void *callback_arg);

int can_mcan_attach_isr(struct can_mcan_data *data,
			struct can_mcan_msg_sram *msg_ram,
			can_rx_callback_t isr, void *cb_arg,
			const struct zcan_filter *filter);

void can_mcan_detach(struct can_mcan_data *data,
		     struct can_mcan_msg_sram *msg_ram, int filter_nr);

enum can_state can_mcan_get_state(const struct can_mcan_config *cfg,
				  struct can_bus_err_cnt *err_cnt);

#endif /* ZEPHYR_DRIVERS_CAN_MCAN_H_ */
