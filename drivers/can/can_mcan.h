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
			volatile uint32_t ext_id : 29; /* Extended Identifier */
			volatile uint32_t rtr    :  1; /* Remote Transmission Request*/
			volatile uint32_t xtd    :  1; /* Extended identifier */
			volatile uint32_t esi    :  1; /* Error state indicator */
		};
		struct {
			volatile uint32_t pad1   : 18;
			volatile uint32_t std_id : 11; /* Standard Identifier */
			volatile uint32_t pad2   : 3;
		};
	};

	volatile uint32_t rxts : 16; /* Rx timestamp */
	volatile uint32_t dlc  :  4; /* Data Length Code */
	volatile uint32_t brs  :  1; /* Bit Rate Switch */
	volatile uint32_t fdf  :  1; /* FD Format */
	volatile uint32_t res  :  2; /* Reserved */
	volatile uint32_t fidx :  7; /* Filter Index */
	volatile uint32_t anmf :  1; /* Accepted non-matching frame */
} __packed;

struct can_mcan_rx_fifo {
	struct can_mcan_rx_fifo_hdr hdr;
	union {
		volatile uint8_t  data[64];
		volatile uint32_t data_32[16];
	};
} __packed;

struct can_mcan_mm {
	volatile uint8_t idx : 5;
	volatile uint8_t cnt : 3;
} __packed;

struct can_mcan_tx_buffer_hdr {
	union {
		struct {
			volatile uint32_t ext_id : 29; /* Identifier */
			volatile uint32_t rtr    :  1; /* Retmote Transmission Request*/
			volatile uint32_t xtd    :  1; /* Extended identifier */
			volatile uint32_t esi    :  1; /* Error state indicator */
		};
		struct {
			volatile uint32_t pad1   : 18;
			volatile uint32_t std_id : 11; /* Identifier */
			volatile uint32_t pad2   :  3;
		};
	};
	volatile uint16_t res1;      /* Reserved */
	volatile uint8_t dlc  :  4; /* Data Length Code */
	volatile uint8_t brs  :  1; /* Bit Rate Switch */
	volatile uint8_t fdf  :  1; /* FD Format */
	volatile uint8_t res2 :  1; /* Reserved */
	volatile uint8_t efc  :  1; /* Event FIFO control (Store Tx events) */
	struct can_mcan_mm mm; /* Message marker */
} __packed;

struct can_mcan_tx_buffer {
	struct can_mcan_tx_buffer_hdr hdr;
	union {
		volatile uint8_t  data[64];
		volatile uint32_t data_32[16];
	};
} __packed;

#define CAN_MCAN_TE_TX    0x1 /* TX event */
#define CAN_MCAN_TE_TXC   0x2 /* TX event in spite of cancellation */

struct can_mcan_tx_event_fifo {
	volatile uint32_t id   : 29; /* Identifier */
	volatile uint32_t rtr  :  1; /* Retmote Transmission Request*/
	volatile uint32_t xtd  :  1; /* Extended identifier */
	volatile uint32_t esi  :  1; /* Error state indicator */

	volatile uint16_t txts;      /* TX Timestamp */
	volatile uint8_t dlc   :  4; /* Data Length Code */
	volatile uint8_t brs   :  1; /* Bit Rate Switch */
	volatile uint8_t fdf   :  1; /* FD Format */
	volatile uint8_t et    :  2; /* Event type */
	struct can_mcan_mm mm; /* Message marker */
} __packed;

#define CAN_MCAN_FCE_DISABLE    0x0
#define CAN_MCAN_FCE_FIFO0      0x1
#define CAN_MCAN_FCE_FIFO1      0x2
#define CAN_MCAN_FCE_REJECT     0x3
#define CAN_MCAN_FCE_PRIO       0x4
#define CAN_MCAN_FCE_PRIO_FIFO0 0x5
#define CAN_MCAN_FCE_PRIO_FIFO1 0x7

#define CAN_MCAN_SFT_RANGE       0x0
#define CAN_MCAN_SFT_DUAL        0x1
#define CAN_MCAN_SFT_MASKED      0x2
#define CAN_MCAN_SFT_DISABLED    0x3

struct can_mcan_std_filter {
	volatile uint32_t id2  : 11; /* ID2 for dual or range, mask otherwise */
	volatile uint32_t res  :  5;
	volatile uint32_t id1  : 11;
	volatile uint32_t sfce :  3; /* Filter config */
	volatile uint32_t sft  :  2; /* Filter type */
} __packed;

#define CAN_MCAN_EFT_RANGE_XIDAM 0x0
#define CAN_MCAN_EFT_DUAL        0x1
#define CAN_MCAN_EFT_MASKED      0x2
#define CAN_MCAN_EFT_RANGE       0x3

struct can_mcan_ext_filter {
	volatile uint32_t id1  : 29;
	volatile uint32_t efce :  3; /* Filter config */
	volatile uint32_t id2  : 29; /* ID2 for dual or range, mask otherwise */
	volatile uint32_t res  :  1;
	volatile uint32_t eft   : 2; /* Filter type */
} __packed;

struct can_mcan_msg_sram {
	volatile struct can_mcan_std_filter std_filt[NUM_STD_FILTER_ELEMENTS];
	volatile struct can_mcan_ext_filter ext_filt[NUM_EXT_FILTER_ELEMENTS];
	volatile struct can_mcan_rx_fifo rx_fifo0[NUM_RX_FIFO0_ELEMENTS];
	volatile struct can_mcan_rx_fifo rx_fifo1[NUM_RX_FIFO1_ELEMENTS];
	volatile struct can_mcan_rx_fifo rx_buffer[NUM_RX_BUF_ELEMENTS];
	volatile struct can_mcan_tx_event_fifo tx_event_fifo[NUM_TX_BUF_ELEMENTS];
	volatile struct can_mcan_tx_buffer tx_buffer[NUM_TX_BUF_ELEMENTS];
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
	uint32_t std_filt_rtr;
	uint32_t std_filt_rtr_mask;
	uint8_t ext_filt_rtr;
	uint8_t ext_filt_rtr_mask;
	struct can_mcan_mm mm;
} __aligned(4);

struct can_mcan_config {
	struct can_mcan_reg *can;   /*!< CAN Registers*/
	uint32_t bus_speed;
	uint32_t bus_speed_data;
	uint16_t sjw;
	uint16_t sample_point;
	uint16_t prop_ts1;
	uint16_t ts2;
#ifdef CONFIG_CAN_FD_MODE
	uint16_t sample_point_data;
	uint8_t sjw_data;
	uint8_t prop_ts1_data;
	uint8_t ts2_data;
	uint8_t tx_delay_comp_offset;
#endif
};

struct can_mcan_reg;

int can_mcan_set_mode(const struct can_mcan_config *cfg, enum can_mode mode);

int can_mcan_set_timing(const struct can_mcan_config *cfg,
			const struct can_timing *timing,
			const struct can_timing *timing_data);

int can_mcan_init(const struct device *dev, const struct can_mcan_config *cfg,
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
		  void *user_data);

int can_mcan_attach_isr(struct can_mcan_data *data,
			struct can_mcan_msg_sram *msg_ram,
			can_rx_callback_t isr, void *cb_arg,
			const struct zcan_filter *filter);

void can_mcan_detach(struct can_mcan_data *data,
		     struct can_mcan_msg_sram *msg_ram, int filter_nr);

enum can_state can_mcan_get_state(const struct can_mcan_config *cfg,
				  struct can_bus_err_cnt *err_cnt);

#endif /* ZEPHYR_DRIVERS_CAN_MCAN_H_ */
