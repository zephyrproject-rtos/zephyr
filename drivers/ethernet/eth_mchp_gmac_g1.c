/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/net/phy.h>
#include <zephyr/net/mii.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>
#include <zephyr/types.h>
#include <zephyr/sys/ring_buffer.h>

LOG_MODULE_REGISTER(eth_mchp_gmac_g1, CONFIG_ETHERNET_LOG_LEVEL);

#define DT_DRV_COMPAT microchip_gmac_g1_eth

#define GMAC_PHY_CONN_TYPE_MII  0
#define GMAC_PHY_CONN_TYPE_RMII 1

#if (DT_INST_ENUM_IDX(0, phy_connection_type) == GMAC_PHY_CONN_TYPE_MII)
#define GMAC_PHY_CONN_TYPE_MII_ENABLED
#endif

#if (DT_INST_ENUM_IDX(0, phy_connection_type) == GMAC_PHY_CONN_TYPE_RMII)
#define GMAC_PHY_CONN_TYPE_RMII_ENABLED
#endif

#define GMAC_MTU            NET_ETH_MTU
#define GMAC_FRAME_SIZE_MAX (GMAC_MTU + 18)

/* Memory alignment of the RX/TX Buffer Descriptor List */
#define GMAC_DESC_ALIGNMENT 4

/* Total number of queues supported by GMAC hardware module */
#define GMAC_QUEUE_NUM          DT_INST_PROP(0, num_queues)
#define GMAC_PRIORITY_QUEUE_NUM (GMAC_QUEUE_NUM - 1)

#if (GMAC_PRIORITY_QUEUE_NUM >= 1)
BUILD_ASSERT(ARRAY_SIZE(GMAC->GMAC_TBQBAPQ) + 1 == GMAC_QUEUE_NUM,
	     "GMAC_QUEUE_NUM doesn't match soc header");
#endif /* (GMAC_PRIORITY_QUEUE_NUM >= 1) */

#define GMAC_ACTIVE_QUEUE_NUM    (GMAC_QUEUE_NUM)
#define MAIN_QUEUE_RX_DESC_COUNT (CONFIG_ETH_MCHP_BUF_RX_COUNT + 1)
#define MAIN_QUEUE_TX_DESC_COUNT (CONFIG_NET_BUF_TX_COUNT + 1)

#define PRIORITY_QUEUE1_RX_DESC_COUNT 1
#define PRIORITY_QUEUE1_TX_DESC_COUNT 1

/* Receive buffer descriptor bit field definitions */
#define GMAC_RXW0_OWNERSHIP (0x1u)
#define GMAC_RXW0_WRAP      (0x2u)
#define GMAC_RXW0_ADDR      (0xFFFFFFFCu)

/* Receive frame length including FCS */
#define GMAC_RXW1_LEN               (0x1FFFu)
#define GMAC_RXW1_FCS_STATUS        (0x2000u)
#define GMAC_RXW1_SOF               (0x4000u)
#define GMAC_RXW1_EOF               (0x8000u)
#define GMAC_RXW1_CFI               (0x1u << 16)
#define GMAC_RXW1_VLANPRIORITY      (0x7u << 17)
#define GMAC_RXW1_PRIORITYDETECTED  (0x1u << 20)
#define GMAC_RXW1_VLANDETECTED      (0x1u << 21)
#define GMAC_RXW1_TYPEIDMATCH       (0x3u << 22)
#define GMAC_RXW1_TYPEIDFOUND       (0x1u << 24)
#define GMAC_RXW1_ADDRMATCH         (0x3u << 25)
#define GMAC_RXW1_ADDRFOUND         (0x1u << 27)
#define GMAC_RXW1_UNIHASHMATCH      (0x1u << 29)
#define GMAC_RXW1_MULTIHASHMATCH    (0x1u << 30)
#define GMAC_RXW1_BROADCASTDETECTED (0x1u << 31)

/* Transmit buffer descriptor bit field definitions */
#define GMAC_TXW1_LEN        (0x3FFFu << 0)
#define GMAC_TXW1_LASTBUFFER (0x1u << 15)
#define GMAC_TXW1_NOCRC      (0x1u << 16)
#define GMAC_TXW1_CHKSUMERR  (0x7u << 20)
#define GMAC_TXW1_LATECOLERR (0x1u << 26)
#define GMAC_TXW1_TRANSERR   (0x1u << 27)
#define GMAC_TXW1_RETRYEXC   (0x1u << 29)
#define GMAC_TXW1_WRAP       (0x40000000u)
#define GMAC_TXW1_USED       (0x80000000u)

/* Interrupt Status/Enable/Disable/Mask register bit field definitions */
#define GMAC_INT_RX_ERR_BITS (GMAC_IER_RXUBR_Msk | GMAC_IER_ROVR_Msk)
#define GMAC_INT_TX_ERR_BITS (GMAC_IER_TUR_Msk | GMAC_IER_RLEX_Msk | GMAC_IER_TFC_Msk)
#define GMAC_INT_EN_FLAGS                                                                          \
	(GMAC_IER_RCOMP_Msk | GMAC_INT_RX_ERR_BITS | GMAC_IER_TCOMP_Msk | GMAC_INT_TX_ERR_BITS |   \
	 GMAC_IER_HRESP_Msk)

#define GMAC_DMA_QUEUE_FLAGS (0)

enum queue_idx {
	GMAC_QUE_0, /* Main queue */
};

#define ETH_MCHP_SHIFT_1_BYTE 8
#define ETH_MCHP_SHIFT_2_BYTE 16
#define ETH_MCHP_SHIFT_3_BYTE 24

#define ETH_MCHP_RX_PKT_WAIT_TIMEOUT_MS 500

#define ETH_MCHP_64_BYTE_ALIGNED_SIZE 0x3Fu

#define GMAC_RX_PROCESS_LIMIT 32u

struct gmac_desc {
	uint32_t addr;
	uint32_t status;
};

struct gmac_desc_list {
	struct gmac_desc *buf_desc;
	uint16_t len;
	uint16_t head;
	uint16_t tail;
};

struct gmac_queue {
	struct gmac_desc_list rx_desc_list;
	struct gmac_desc_list tx_desc_list;
	struct k_sem tx_desc_sem;
	struct net_buf **rx_frag_list;
	struct ring_buf tx_frag_rb;
	volatile uint32_t err_rx_queue_frames_dropped;
	volatile uint32_t err_rx_queue_flushed_count;
	volatile uint32_t err_tx_queue_flushed_count;
	enum queue_idx que_idx;
};

struct gmac_dev_data {
	struct net_if *iface;
	uint32_t phy_connection_type;
	struct k_sem rx_int_sem;

	K_KERNEL_STACK_MEMBER(rx_thread_stack, CONFIG_ETH_MCHP_RX_THREAD_STACK_SIZE);
	struct k_thread rx_thread;
	struct gmac_queue *rx_queue;
	struct gmac_queue queue_list[GMAC_QUEUE_NUM];
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	struct net_stats_eth stats;
#endif /* CONFIG_NET_STATISTICS_ETHERNET */
};

struct gmac_dev_config {
	gmac_registers_t *const regs;
	uint32_t active_queues;
	const struct pinctrl_dev_config *pinctrl_cfg;
	void (*config_func)(void);
	const struct device *phy_dev;
	clock_control_subsys_t mclk_apb_sys;
	clock_control_subsys_t mclk_ahb_sys;
	const struct net_eth_mac_config mcfg;
};

struct gmac_frame_info {
	int16_t sof_index;
	int16_t eof_index;
	uint16_t num_descriptors;
};

#if (CONFIG_NET_BUF_DATA_SIZE * CONFIG_ETH_MCHP_BUF_RX_COUNT) < GMAC_FRAME_SIZE_MAX
#error network data fragment not large enough to hold a full frame
#endif

#if (CONFIG_NET_BUF_DATA_SIZE * (CONFIG_NET_BUF_RX_COUNT - CONFIG_ETH_MCHP_BUF_RX_COUNT)) <        \
	GMAC_FRAME_SIZE_MAX
#error network data fragment are not large enough to hold a full frame
#endif

#if CONFIG_NET_BUF_DATA_SIZE & ETH_MCHP_64_BYTE_ALIGNED_SIZE
#error message "CONFIG_NET_BUF_DATA_SIZE should be a multiple of 64 bytes \
		due to the granularity of RX DMA"
#endif

#if ((CONFIG_ETH_MCHP_BUF_RX_COUNT + 1) * GMAC_ACTIVE_QUEUE_NUM) > CONFIG_NET_BUF_RX_COUNT
#error Not enough RX buffers to allocate descriptors for each HW queue
#endif

BUILD_ASSERT(DT_INST_ENUM_IDX(0, phy_connection_type) <= 1, "Invalid PHY connection");

static struct gmac_desc rx_desc_que0[MAIN_QUEUE_RX_DESC_COUNT] __nocache
	__aligned(GMAC_DESC_ALIGNMENT);

static struct gmac_desc tx_desc_que0[MAIN_QUEUE_TX_DESC_COUNT] __nocache
	__aligned(GMAC_DESC_ALIGNMENT);

static struct net_buf *rx_frag_list_que0[MAIN_QUEUE_RX_DESC_COUNT];
static uint8_t tx_frag_rb_buf_que0[MAIN_QUEUE_TX_DESC_COUNT * sizeof(uintptr_t)];

#define MODULO_INC(val, max)                                                                       \
	{                                                                                          \
		val++;                                                                             \
		val = (val < max) ? val : 0;                                                       \
	}

#define MODULO_DEC(val, max)                                                                       \
	{                                                                                          \
		val--;                                                                             \
		val = (val == -1) ? (max - 1) : val;                                               \
	}

static void gmac_free_rx_bufs(struct net_buf **rx_frag_list, uint16_t len)
{
	for (int i = 0; i < len; i++) {
		if (rx_frag_list[i] != NULL) {
			net_buf_unref(rx_frag_list[i]);
			rx_frag_list[i] = NULL;
		}
	}
}

static int gmac_rx_descriptors_init(gmac_registers_t *gmac_regs, struct gmac_queue *queue)
{
	struct gmac_desc_list *rx_desc_list = &queue->rx_desc_list;
	struct net_buf **rx_frag_list = queue->rx_frag_list;
	struct net_buf *rx_buf;
	uint8_t *rx_buf_addr;

	__ASSERT_NO_MSG(rx_frag_list);
	rx_desc_list->tail = 0U;
	for (int i = 0; i < rx_desc_list->len; i++) {
		rx_buf = net_pkt_get_reserve_rx_data(CONFIG_NET_BUF_DATA_SIZE, K_NO_WAIT);
		if (rx_buf == NULL) {
			gmac_free_rx_bufs(rx_frag_list, i);
			LOG_ERR("Failed to reserve data net buffers");

			return -ENOBUFS;
		}

		rx_frag_list[i] = rx_buf;
		rx_buf_addr = rx_buf->data;
		__ASSERT(((uint32_t)rx_buf_addr & ~GMAC_RXW0_ADDR) == 0,
			 "Misaligned RX buffer address");
		__ASSERT(rx_buf->size == CONFIG_NET_BUF_DATA_SIZE,
			 "Incorrect length of RX data buffer");
		rx_desc_list->buf_desc[i].addr = (uint32_t)rx_buf_addr & GMAC_RXW0_ADDR;
		rx_desc_list->buf_desc[i].status = 0U;
	}

	rx_desc_list->buf_desc[rx_desc_list->len - 1U].addr |= GMAC_RXW0_WRAP;
	gmac_regs->GMAC_RBQB = (uint32_t)queue->rx_desc_list.buf_desc;

	return 0;
}

static void gmac_tx_descriptors_init(gmac_registers_t *gmac_regs, struct gmac_queue *queue)
{
	struct gmac_desc_list *tx_desc_list = &queue->tx_desc_list;

	tx_desc_list->head = 0U;
	tx_desc_list->tail = 0U;
	for (int i = 0; i < tx_desc_list->len; i++) {
		tx_desc_list->buf_desc[i].addr = 0U;
		tx_desc_list->buf_desc[i].status = GMAC_TXW1_USED;
	}

	tx_desc_list->buf_desc[tx_desc_list->len - 1U].status |= GMAC_TXW1_WRAP;
	ring_buf_reset(&queue->tx_frag_rb);
	gmac_regs->GMAC_TBQB = (uint32_t)queue->tx_desc_list.buf_desc;
}

static int gmac_queue_init(gmac_registers_t *gmac_regs, struct gmac_queue *queue)
{
	int result;

	__ASSERT_NO_MSG(queue->rx_desc_list.len > 0);
	__ASSERT_NO_MSG(queue->tx_desc_list.len > 0);
	__ASSERT(((uint32_t)queue->rx_desc_list.buf_desc & ~GMAC_RBQB_ADDR_Msk) == 0,
		 "RX descriptors have to be word aligned");
	__ASSERT(((uint32_t)queue->tx_desc_list.buf_desc & ~GMAC_TBQB_ADDR_Msk) == 0,
		 "TX descriptors have to be word aligned");

	result = gmac_rx_descriptors_init(gmac_regs, queue);
	if (result < 0) {
		return result;
	}

	gmac_tx_descriptors_init(gmac_regs, queue);
	k_sem_init(&queue->tx_desc_sem, queue->tx_desc_list.len - 1, queue->tx_desc_list.len - 1);

	gmac_regs->GMAC_DCFGR =
		/* Receive Buffer Size (defined in multiples of 64 bytes) */
		GMAC_DCFGR_DRBS(CONFIG_NET_BUF_DATA_SIZE >> 6) | GMAC_DCFGR_RXBMS(3) |
		GMAC_DCFGR_FBLDO_INCR4 | GMAC_DMA_QUEUE_FLAGS;
	gmac_regs->GMAC_IER = GMAC_INT_EN_FLAGS;

	queue->err_rx_queue_frames_dropped = 0U;
	queue->err_rx_queue_flushed_count = 0U;
	queue->err_tx_queue_flushed_count = 0U;

	LOG_INF("Queue %d activated", queue->que_idx);

	return 0;
}

static void gmac_mac_addr_set(gmac_registers_t *gmac, uint8_t index, uint8_t mac_addr[6])
{
	uint32_t top_addr = (mac_addr[5] << ETH_MCHP_SHIFT_1_BYTE) | (mac_addr[4]);
	uint32_t bottom_addr = (mac_addr[3] << ETH_MCHP_SHIFT_3_BYTE) |
			       (mac_addr[2] << ETH_MCHP_SHIFT_2_BYTE) |
			       (mac_addr[1] << ETH_MCHP_SHIFT_1_BYTE) | (mac_addr[0]);

	__ASSERT(index < 4, "index has to be in the range 0..3");
	gmac->SA[index].GMAC_SAB = GMAC_SAB_ADDR(bottom_addr);
	gmac->SA[index].GMAC_SAT = GMAC_SAT_ADDR(top_addr);
}

static void gmac_tx_completed(gmac_registers_t *gmac, struct gmac_queue *queue)
{
	struct gmac_desc_list *tx_desc_list = &queue->tx_desc_list;
	struct gmac_desc *tx_desc;
	struct net_buf *frag;
	uintptr_t ptr;

	__ASSERT(tx_desc_list->buf_desc[tx_desc_list->tail].status & GMAC_TXW1_USED,
		 "first buffer of a frame is not marked as own by GMAC");
	while (tx_desc_list->tail != tx_desc_list->head) {
		tx_desc = &tx_desc_list->buf_desc[tx_desc_list->tail];
		tx_desc->status &= ~(GMAC_TXW1_LEN | GMAC_TXW1_LASTBUFFER | GMAC_TXW1_NOCRC |
				     GMAC_TXW1_CHKSUMERR | GMAC_TXW1_LATECOLERR |
				     GMAC_TXW1_TRANSERR | GMAC_TXW1_RETRYEXC | GMAC_TXW1_USED);
		MODULO_INC(tx_desc_list->tail, tx_desc_list->len);
		k_sem_give(&queue->tx_desc_sem);
		ring_buf_get(&queue->tx_frag_rb, (uint8_t *)&ptr, sizeof(ptr));
		frag = UINT_TO_POINTER(ptr);
		net_pkt_frag_unref(frag);
		LOG_DBG("Dropping frag %p", frag);

		if (tx_desc->status & GMAC_TXW1_LASTBUFFER) {
			tx_desc->status &= ~GMAC_TXW1_LASTBUFFER;
			break;
		}
	}
}

static void gmac_tx_error_handler(gmac_registers_t *gmac, struct gmac_queue *queue)
{
	struct net_buf *frag;
	uintptr_t ptr;

	gmac->GMAC_NCR &= ~GMAC_NCR_TXEN_Msk;
	queue->err_tx_queue_flushed_count++;

	while (ring_buf_get(&queue->tx_frag_rb, (uint8_t *)&ptr, sizeof(ptr)) == sizeof(ptr)) {
		frag = UINT_TO_POINTER(ptr);
		net_pkt_frag_unref(frag);
	}

	/* Reinitialize TX descriptor list */
	k_sem_reset(&queue->tx_desc_sem);
	for (int i = 0; i < queue->tx_desc_list.len - 1; i++) {
		k_sem_give(&queue->tx_desc_sem);
	}

	gmac_tx_descriptors_init(gmac, queue);
	gmac->GMAC_NCR |= GMAC_NCR_TXEN_Msk;
}

static void gmac_rx_error_handler(gmac_registers_t *gmac, struct gmac_queue *queue)
{
	gmac->GMAC_NCR &= ~GMAC_NCR_RXEN_Msk;
	queue->err_rx_queue_flushed_count++;
	queue->rx_desc_list.tail = 0U;

	LOG_DBG("rx Err");

	for (int i = 0; i < queue->rx_desc_list.len; i++) {
		queue->rx_desc_list.buf_desc[i].status = 0U;
		queue->rx_desc_list.buf_desc[i].addr &= ~GMAC_RXW0_OWNERSHIP;
	}

	gmac->GMAC_RBQB = (uint32_t)queue->rx_desc_list.buf_desc;
	gmac->GMAC_NCR |= GMAC_NCR_RXEN_Msk;
}

static int gmac_set_phy_connection_type(gmac_registers_t *gmac)
{
#ifdef GMAC_PHY_CONN_TYPE_MII_ENABLED
	gmac->GMAC_UR = 0x1;
#elif defined(GMAC_PHY_CONN_TYPE_RMII_ENABLED)
	gmac->GMAC_UR = 0x0;
#endif /* GMAC_PHY_CONN_TYPE_MII_ENABLED */

	return 0;
}

static inline int gmac_get_mck_clock_divisor(uint32_t mck, uint32_t *mck_divisor)
{
	if (mck <= MHZ(20)) {
		*mck_divisor = GMAC_NCFGR_CLK_MCK8;
	} else if (mck <= MHZ(40)) {
		*mck_divisor = GMAC_NCFGR_CLK_MCK16;
	} else if (mck <= MHZ(80)) {
		*mck_divisor = GMAC_NCFGR_CLK_MCK32;
	} else if (mck <= MHZ(120)) {
		*mck_divisor = GMAC_NCFGR_CLK_MCK48;
	} else if (mck <= MHZ(160)) {
		*mck_divisor = GMAC_NCFGR_CLK_MCK64;
	} else if (mck <= MHZ(240)) {
		*mck_divisor = GMAC_NCFGR_CLK_MCK96;
	} else {
		LOG_ERR("No valid MDC clock");

		return -ENOTSUP;
	}

	LOG_INF("mck %d mck_divisor = 0x%x", mck, *mck_divisor);

	return 0;
}

static int gmac_init(const struct device *dev, gmac_registers_t *gmac)
{
	int retval;
	uint32_t clk_freq_hz = 0;
	uint32_t mck_divisor;
	const struct gmac_dev_config *const cfg = dev->config;

	gmac->GMAC_NCFGR |= GMAC_NCFGR_MTIHEN_Msk | GMAC_NCFGR_LFERD_Msk | GMAC_NCFGR_RFCS_Msk |
#ifdef CONFIG_NET_VLAN
			    GMAC_NCFGR_MAXFS_Msk |
#endif
			    GMAC_NCFGR_RXCOEN_Msk;

	gmac->GMAC_NCR = GMAC_NCR_CLRSTAT_Msk | GMAC_NCR_MPE_Msk;
	gmac->GMAC_IDR = UINT32_MAX;
	(void)gmac->GMAC_ISR;
	gmac->GMAC_HRB = UINT32_MAX;
	gmac->GMAC_HRT = UINT32_MAX;
	gmac->GMAC_RSR = GMAC_RSR_RESETVALUE;

	retval = clock_control_get_rate(DEVICE_DT_GET(DT_NODELABEL(clock)), cfg->mclk_apb_sys,
					&clk_freq_hz);
	if (retval < 0) {
		LOG_ERR("ETH_MCHP_GET_CLOCK_FREQ Failed");

		return retval;
	}

	retval = gmac_get_mck_clock_divisor(clk_freq_hz, &mck_divisor);
	if (retval < 0) {
		return retval;
	}

	gmac->GMAC_NCFGR &= ~(GMAC_NCFGR_CLK_MCK8 | GMAC_NCFGR_CLK_MCK16 | GMAC_NCFGR_CLK_MCK32 |
			      GMAC_NCFGR_CLK_MCK48 | GMAC_NCFGR_CLK_MCK64 | GMAC_NCFGR_CLK_MCK96);
	gmac->GMAC_NCFGR |= mck_divisor;

	return gmac_set_phy_connection_type(gmac);
}

static bool gmac_check_queue_for_data(struct gmac_desc_list *rx_desc_list)
{
	uint16_t tail = rx_desc_list->tail;
	struct gmac_desc *rx_desc = &rx_desc_list->buf_desc[tail];

	while ((rx_desc->addr & GMAC_RXW0_OWNERSHIP) == 0) {
		MODULO_INC(tail, rx_desc_list->len);
		rx_desc = &rx_desc_list->buf_desc[tail];
		if (tail == rx_desc_list->tail) {
			return false;
		}
	}

	rx_desc_list->tail = tail;

	return true;
}

static int gmac_find_valid_frame(struct gmac_desc_list *rx_desc_list,
				 struct gmac_frame_info *frame_info)
{
	uint16_t tail = rx_desc_list->tail;
	bool sof_found = false;
	struct gmac_desc *rx_desc = &rx_desc_list->buf_desc[tail];

	frame_info->sof_index = -1;
	frame_info->eof_index = -1;
	frame_info->num_descriptors = 0;
	while (((rx_desc->addr & GMAC_RXW0_OWNERSHIP) != 0) && (frame_info->eof_index == -1)) {
		if ((rx_desc->addr & GMAC_RXW0_ADDR) == 0) {
			rx_desc->addr &= ~GMAC_RXW0_OWNERSHIP;
			break;
		}

		if (rx_desc->status & GMAC_RXW1_SOF) {
			frame_info->sof_index = tail;
			sof_found = true;
		}

		if (rx_desc->status & GMAC_RXW1_EOF) {
			frame_info->eof_index = tail;
		}

		if (sof_found) {
			frame_info->num_descriptors++;
		}

		MODULO_INC(tail, rx_desc_list->len);
		rx_desc = &rx_desc_list->buf_desc[tail];
	}

	if ((frame_info->eof_index == -1) || (frame_info->sof_index == -1)) {
		return -EINVAL;
	}

	rx_desc_list->tail = frame_info->sof_index;

	return 0;
}

static struct net_pkt *gmac_extract_and_replace_buffers(struct gmac_queue *queue, int *err_no,
							struct gmac_frame_info *frame_info)
{
	struct gmac_desc_list *rx_desc_list = &queue->rx_desc_list;
	struct net_buf **rx_frag_list = queue->rx_frag_list;
	struct net_pkt *rx_pkt = NULL;
	bool frame_is_complete = false;
	struct net_buf *frag;
	struct net_buf *new_frag;
	struct net_buf *last_frag = NULL;
	uint8_t *frag_data;
	uint32_t frag_len;
	uint32_t frame_len = 0U;
	uint16_t tail = rx_desc_list->tail;
	struct gmac_desc *rx_desc = &rx_desc_list->buf_desc[tail];

	rx_pkt = net_pkt_rx_alloc(K_NO_WAIT);
	if (rx_pkt == NULL) {
		for (int i = rx_desc_list->tail; (frame_is_complete == false);) {
			rx_desc = &rx_desc_list->buf_desc[i];
			frame_is_complete = (bool)(rx_desc->status & GMAC_RXW1_EOF);
			rx_desc->status = 0U;
			rx_desc->addr &= ~GMAC_RXW0_OWNERSHIP;
			MODULO_INC(i, rx_desc_list->len);
			rx_desc = &rx_desc_list->buf_desc[i];
		}

		queue->err_rx_queue_frames_dropped++;
		*err_no = -ENOMEM;

		return NULL;
	}

	while (frame_is_complete == false) {
		frag = rx_frag_list[tail];
		frag_data = (uint8_t *)(rx_desc->addr & GMAC_RXW0_ADDR);
		__ASSERT(frag->data == frag_data, "RX descriptor and buffer list desynchronized");
		if (frame_info->eof_index == tail) {
			frame_is_complete = true;
			frag_len = (rx_desc->status & GMAC_RXW1_LEN) - frame_len;
		} else {
			frag_len = CONFIG_NET_BUF_DATA_SIZE;
		}

		frame_len += frag_len;
		new_frag = net_pkt_get_frag(rx_pkt, CONFIG_NET_BUF_DATA_SIZE, K_NO_WAIT);
		if (new_frag == NULL) {
			bool eof_flag = false;

			for (int i = rx_desc_list->tail; (eof_flag == false);) {
				rx_desc = &rx_desc_list->buf_desc[i];
				eof_flag = (bool)(rx_desc->status & GMAC_RXW1_EOF);
				rx_desc->status = 0U;
				rx_desc->addr &= ~GMAC_RXW0_OWNERSHIP;
				MODULO_INC(i, rx_desc_list->len);
				rx_desc = &rx_desc_list->buf_desc[i];
			}

			queue->err_rx_queue_frames_dropped++;
			net_pkt_unref(rx_pkt);
			*err_no = -ENOMEM;

			return NULL;
		}

		net_buf_add(frag, frag_len);
		if (!last_frag) {
			net_pkt_frag_insert(rx_pkt, frag);
		} else {
			net_buf_frag_insert(last_frag, frag);
		}

		last_frag = frag;
		frag = new_frag;
		rx_frag_list[tail] = frag;
		rx_desc->status = 0U;
		rx_desc->addr = ((uint32_t)frag->data & GMAC_RXW0_ADDR) |
				(rx_desc->addr & GMAC_RXW0_WRAP);

		MODULO_INC(tail, rx_desc_list->len);
		rx_desc = &rx_desc_list->buf_desc[tail];
	}

	rx_desc_list->tail = tail;
	LOG_DBG("Frame complete: rx=%p, tail=%d", rx_pkt, tail);
	__ASSERT_NO_MSG(frame_is_complete);

	return rx_pkt;
}

static struct net_pkt *gmac_pkt_get(gmac_registers_t *gmac_regs, struct gmac_queue *queue,
				    int *err_no)
{
	struct gmac_desc_list *rx_desc_list = &queue->rx_desc_list;
	struct net_pkt *rx_pkt = NULL;
	struct gmac_frame_info frame_info;

	/* if the packet not available in the first descriptor */
	if (gmac_check_queue_for_data(rx_desc_list) == false) {
		*err_no = -ENODATA;

		return NULL;
	}

	if (gmac_find_valid_frame(rx_desc_list, &frame_info) != 0) {
		gmac_rx_error_handler(gmac_regs, queue);
		*err_no = -EINVAL;

		return NULL;
	}

	rx_pkt = gmac_extract_and_replace_buffers(queue, err_no, &frame_info);

	return rx_pkt;
}

static void gmac_rx(gmac_registers_t *gmac_regs, struct gmac_queue *queue)
{
	int err_no = 0;
	struct gmac_dev_data *dev_data =
		CONTAINER_OF(queue, struct gmac_dev_data, queue_list[queue->que_idx]);
	struct net_pkt *rx_pkt = NULL;
	bool bna_flag = (gmac_regs->GMAC_RSR & GMAC_RSR_BNA_Msk) ? true : false;

	while (1) {
		rx_pkt = gmac_pkt_get(gmac_regs, queue, &err_no);
		if (rx_pkt == NULL) {
			if (err_no == -ENODATA) {
				break;
			}

			if (err_no == -ENOMEM) {
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
				dev_data->stats.error_details.rx_buf_alloc_failed++;
#endif /* CONFIG_NET_STATISTICS_ETHERNET */
				break;
			}

			if (err_no == -EINVAL) {
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
				dev_data->stats.error_details.rx_frame_errors++;
#endif /* CONFIG_NET_STATISTICS_ETHERNET */
				continue;
			}
		}

		if (net_recv_data(dev_data->iface, rx_pkt) < 0) {
			net_pkt_unref(rx_pkt);
		}
	}

	if (bna_flag) {
		gmac_regs->GMAC_RSR = GMAC_RSR_BNA_Msk;
	}
}

static void gmac_rx_thread(void *arg1, void *unused1, void *unused2)
{
	int res;
	struct device *dev = (struct device *)arg1;
	struct gmac_dev_data *dev_data = dev->data;
	const struct gmac_dev_config *const cfg = dev->config;
	gmac_registers_t *gmac_regs = cfg->regs;

	while (1) {
		res = k_sem_take(&dev_data->rx_int_sem, K_MSEC(ETH_MCHP_RX_PKT_WAIT_TIMEOUT_MS));
		if (res == 0) {
			struct gmac_queue *queue = dev_data->rx_queue;

			gmac_rx(gmac_regs, queue);
		}
	}
}

static int eth_mchp_send(const struct device *dev, struct net_pkt *pkt)
{
	const struct gmac_dev_config *const cfg = dev->config;
	struct gmac_dev_data *const dev_data = dev->data;
	gmac_registers_t *const gmac_regs = cfg->regs;
	struct gmac_queue *queue;
	struct gmac_desc_list *tx_desc_list;
	struct gmac_desc *tx_desc;
	struct gmac_desc *tx_first_desc;
	struct net_buf *frag;
	uint8_t *frag_data;
	uint16_t frag_len = 0;
	uint32_t err_tx_queue_flushed_count_at_entry;
	uint8_t pkt_buff_cnt = 0;
	uint8_t free_desc_cnt = 0;
	int8_t head_desc_index = 0;

	queue = &dev_data->queue_list[0];
	err_tx_queue_flushed_count_at_entry = queue->err_tx_queue_flushed_count;
	tx_desc_list = &queue->tx_desc_list;
	tx_first_desc = &tx_desc_list->buf_desc[tx_desc_list->head];
	frag = pkt->frags;

	free_desc_cnt = k_sem_count_get(&queue->tx_desc_sem);
	while (frag != NULL) {
		frag = frag->frags;
		pkt_buff_cnt++;
	}

	if (free_desc_cnt < pkt_buff_cnt) {
		return -EIO;
	}

	frag = pkt->frags;

	while (frag != NULL) {
		uintptr_t ptr = POINTER_TO_UINT(frag);
		unsigned int irq_lock_key;

		frag_data = frag->data;
		frag_len = frag->len;
		k_sem_take(&queue->tx_desc_sem, K_FOREVER);
		if (queue->err_tx_queue_flushed_count != err_tx_queue_flushed_count_at_entry) {
			return -EIO;
		}

		tx_desc = &tx_desc_list->buf_desc[tx_desc_list->head];
		tx_desc->addr = (uint32_t)frag_data;
		tx_desc->status |= GMAC_TXW1_USED;

		if (frag->frags == NULL) {
			tx_desc->status |= GMAC_TXW1_LASTBUFFER;
		}

		tx_desc->status &= ~GMAC_TXW1_LEN;
		tx_desc->status |= (frag_len & GMAC_TXW1_LEN);
		irq_lock_key = irq_lock();
		MODULO_INC(tx_desc_list->head, tx_desc_list->len);
		irq_unlock(irq_lock_key);
		__ASSERT(tx_desc_list->head != tx_desc_list->tail, "tx_desc_list overflow");
		ring_buf_put(&queue->tx_frag_rb, (uint8_t *)&ptr, sizeof(ptr));
		net_pkt_frag_ref(frag);
		frag = frag->frags;
	}

	if (queue->err_tx_queue_flushed_count != err_tx_queue_flushed_count_at_entry) {
		gmac_regs->GMAC_NCR |= GMAC_NCR_TSTART_Msk;

		return -EIO;
	}

	head_desc_index = tx_desc_list->head;
	MODULO_DEC(head_desc_index, tx_desc_list->len);
	frag = pkt->frags;

	while (pkt_buff_cnt) {
		tx_desc_list->buf_desc[head_desc_index].status &= ~GMAC_TXW1_USED;
		MODULO_DEC(head_desc_index, tx_desc_list->len);
		pkt_buff_cnt--;
	}

	gmac_regs->GMAC_NCR |= GMAC_NCR_TSTART_Msk;

	return 0;
}

static void eth_mchp_queue0_isr(const struct device *dev)
{
	const struct gmac_dev_config *const cfg = dev->config;
	struct gmac_dev_data *const dev_data = dev->data;
	gmac_registers_t *const gmac_regs = cfg->regs;
	struct gmac_queue *queue = &dev_data->queue_list[0];
	struct gmac_desc_list *rx_desc_list = &queue->rx_desc_list;
	struct gmac_desc_list *tx_desc_list = &queue->tx_desc_list;
	struct gmac_desc *tail_desc;
	uint32_t isr = gmac_regs->GMAC_ISR;

	LOG_DBG("GMAC_ISR=0x%08x", isr);

	if (isr & GMAC_ISR_RCOMP_Msk) {
		tail_desc = &rx_desc_list->buf_desc[rx_desc_list->tail];
		LOG_DBG("rx.w1=0x%08x, tail=%d", tail_desc->status, rx_desc_list->tail);
		dev_data->rx_queue = queue;
		k_sem_give(&dev_data->rx_int_sem);
	}

	if ((isr & GMAC_INT_RX_ERR_BITS) != 0) {
		queue->err_rx_queue_flushed_count++;
	}

	if (isr & GMAC_ISR_TCOMP_Msk) {
		tail_desc = &tx_desc_list->buf_desc[tx_desc_list->tail];
		LOG_DBG("tx.w1=0x%08x, tail=%d", tail_desc->status, tx_desc_list->tail);
		gmac_tx_completed(gmac_regs, queue);
	}

	if ((isr & GMAC_INT_TX_ERR_BITS) != 0) {
		gmac_tx_error_handler(gmac_regs, queue);
	}

	if ((isr & GMAC_IER_HRESP_Msk) != 0) {
		LOG_DBG("IER HRESP");
	}
}

/* Declare pin-ctrl __pinctrl_dev_config__device_dts_ord_xx before
 * PINCTRL_DT_INST_DEV_CONFIG_GET()
 */
PINCTRL_DT_INST_DEFINE(0);

static int eth_mchp_initialize(const struct device *dev)
{
	const struct gmac_dev_config *const cfg = dev->config;
	struct gmac_dev_data *const dev_data = dev->data;
	gmac_registers_t *const gmac_regs = cfg->regs;
	int retval;

	cfg->config_func();

	retval = clock_control_on(DEVICE_DT_GET(DT_NODELABEL(clock)), cfg->mclk_apb_sys);
	if ((retval != 0) && (retval != -EALREADY)) {
		LOG_ERR("Failed to enable the MCLK APB for Ethernet: %d", retval);

		return retval;
	}

	retval = clock_control_on(DEVICE_DT_GET(DT_NODELABEL(clock)), cfg->mclk_ahb_sys);
	if ((retval != 0) && (retval != -EALREADY)) {
		LOG_ERR("Failed to enable the MCLK AHB for Ethernet: %d", retval);

		return retval;
	}

	retval = pinctrl_apply_state(cfg->pinctrl_cfg, PINCTRL_STATE_DEFAULT);
	if (retval != 0) {
		LOG_ERR("pinctrl_apply_state() Failed for Ethernet driver: %d", retval);

		return retval;
	}

	dev_data->queue_list[0].que_idx = GMAC_QUE_0;
	dev_data->queue_list[0].rx_desc_list.buf_desc = rx_desc_que0;
	dev_data->queue_list[0].rx_desc_list.len = ARRAY_SIZE(rx_desc_que0);
	dev_data->queue_list[0].tx_desc_list.buf_desc = tx_desc_que0;
	dev_data->queue_list[0].tx_desc_list.len = ARRAY_SIZE(tx_desc_que0);
	dev_data->queue_list[0].rx_frag_list = rx_frag_list_que0;
	dev_data->queue_list[0].tx_frag_rb.buffer = tx_frag_rb_buf_que0;
	dev_data->queue_list[0].tx_frag_rb.size = sizeof(tx_frag_rb_buf_que0);

	retval = gmac_init(dev, gmac_regs);
	if (retval < 0) {
		LOG_ERR("Unable to initialize ETH driver");

		return retval;
	}

	return retval;
}

static void eth_mchp_phy_link_state_changed(const struct device *pdev, struct phy_link_state *state,
					    void *user_data)
{
	const struct device *dev = (const struct device *)user_data;
	struct gmac_dev_data *const dev_data = dev->data;
	const struct gmac_dev_config *const cfg = dev->config;
	gmac_registers_t *const gmac_regs = cfg->regs;

	net_eth_carrier_set(dev_data->iface, state->is_up);
	if (state->is_up == true) {
		uint32_t val = gmac_regs->GMAC_NCFGR;

		val &= ~(GMAC_NCFGR_FD_Msk | GMAC_NCFGR_SPD_Msk);
		val |= PHY_LINK_IS_FULL_DUPLEX(state->speed) ? GMAC_NCFGR_FD_Msk : 0;
		val |= PHY_LINK_IS_SPEED_100M(state->speed) ? GMAC_NCFGR_SPD_Msk : 0;
		gmac_regs->GMAC_NCFGR = val;
		gmac_regs->GMAC_NCR |= (GMAC_NCR_RXEN_Msk | GMAC_NCR_TXEN_Msk);
	}
}

static const struct device *eth_mchp_get_phy(const struct device *dev,
					     struct net_if *iface __unused)
{
	const struct gmac_dev_config *const cfg = dev->config;

	return cfg->phy_dev;
}

static void eth_mchp_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct gmac_dev_data *const dev_data = dev->data;
	const struct gmac_dev_config *const cfg = dev->config;
	gmac_registers_t *const gmac_regs = cfg->regs;
	int result;
	uint8_t mac_addr[6];

	dev_data->iface = iface;
	ethernet_init(iface);

	result = net_eth_mac_load(&cfg->mcfg, mac_addr);
	if (result == -ENODATA) {
		LOG_DBG("No MAC address configured");
	} else if (result < 0) {
		LOG_ERR("Failed to load MAC (%d)", result);

		return;
	}

	if (result != -ENODATA) {
		gmac_mac_addr_set(gmac_regs, 0, mac_addr);

		LOG_INF("MAC: %02x:%02x:%02x:%02x:%02x:%02x", mac_addr[0], mac_addr[1], mac_addr[2],
			mac_addr[3], mac_addr[4], mac_addr[5]);

		net_if_set_link_addr(iface, mac_addr, sizeof(mac_addr), NET_LINK_ETHERNET);
	}

	for (int i = GMAC_QUE_0; i < GMAC_QUEUE_NUM; i++) {
		result = gmac_queue_init(gmac_regs, &dev_data->queue_list[i]);
		if (result < 0) {
			LOG_ERR("Unable to initialize ETH queue%d", i);

			return;
		}
	}

	if (device_is_ready(cfg->phy_dev)) {
		net_if_carrier_off(iface);
		phy_link_callback_set(cfg->phy_dev, &eth_mchp_phy_link_state_changed, (void *)dev);
	} else {
		LOG_ERR("PHY device not ready");
	}

	k_sem_init(&dev_data->rx_int_sem, 0, K_SEM_MAX_LIMIT);
	k_thread_create(&dev_data->rx_thread, dev_data->rx_thread_stack,
			K_KERNEL_STACK_SIZEOF(dev_data->rx_thread_stack), gmac_rx_thread,
			(void *)dev, NULL, NULL,
			IS_ENABLED(CONFIG_NET_TC_THREAD_PREEMPTIVE)
				? K_PRIO_PREEMPT(CONFIG_ETH_MCHP_RX_THREAD_PRIORITY)
				: K_PRIO_COOP(CONFIG_ETH_MCHP_RX_THREAD_PRIORITY),
			0, K_NO_WAIT);
	k_thread_name_set(&dev_data->rx_thread, "mchp_eth_rx");
}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
static struct net_stats_eth *eth_mchp_get_stats(const struct device *dev,
						struct net_if *iface __unused)
{
	struct gmac_dev_data *dev_data = dev->data;
	const struct gmac_dev_config *const cfg = dev->config;
	gmac_registers_t *const gmac_regs = cfg->regs;
	struct net_stats_eth *eth_stats = &dev_data->stats;

	uint32_t err_ae = gmac_regs->GMAC_AE;    /* Alignment Error */
	uint32_t err_ofr = gmac_regs->GMAC_OFR;  /* Over Length Frames */
	uint32_t err_fcs = gmac_regs->GMAC_FCSE; /* FCS Error */
	uint32_t err_scf = gmac_regs->GMAC_SCF;  /* Single Collision */
	uint32_t err_mcf = gmac_regs->GMAC_MCF;  /* Multiple Collision */
	uint32_t err_ecf = gmac_regs->GMAC_EC;   /* Excess Collision */

	eth_stats->collisions += err_scf + err_mcf + err_ecf;
	eth_stats->csum.rx_csum_offload_errors += gmac_regs->GMAC_FCSE;
	eth_stats->csum.rx_csum_offload_good = 0;
	eth_stats->error_details.rx_align_errors += err_ae;
	eth_stats->error_details.rx_long_length_errors += err_ofr;
	eth_stats->error_details.rx_crc_errors += err_fcs;
	eth_stats->error_details.rx_length_errors += err_ofr;
	eth_stats->errors.rx += gmac_regs->GMAC_UCE + gmac_regs->GMAC_TCE + gmac_regs->GMAC_IHCE +
				gmac_regs->GMAC_ROE + gmac_regs->GMAC_RRE + err_ae +
				gmac_regs->GMAC_RSE + gmac_regs->GMAC_LFFE + err_fcs +
				gmac_regs->GMAC_JR + err_ofr + gmac_regs->GMAC_UFR;
	eth_stats->errors.tx +=
		err_scf + err_mcf + err_ecf + gmac_regs->GMAC_TUR + gmac_regs->GMAC_CSE;
	eth_stats->multicast.rx += gmac_regs->GMAC_MFR;
	eth_stats->multicast.tx += gmac_regs->GMAC_MFT;
	eth_stats->tx_dropped = 0;
	eth_stats->tx_restart_queue = 0;
	eth_stats->tx_timeout_count = 0;
	eth_stats->unknown_protocol = 0;

#ifdef CONFIG_NET_STATISTICS_ETHERNET_VENDOR
	eth_stats->vendor.key = NULL;
	eth_stats->vendor.value = 0;
#endif /* CONFIG_NET_STATISTICS_ETHERNET_VENDOR */

	return eth_stats;
}
#endif /* CONFIG_NET_STATISTICS_ETHERNET */

static enum ethernet_hw_caps eth_mchp_get_capabilities(const struct device *dev __unused,
						       struct net_if *iface __unused)
{
	enum ethernet_hw_caps hw_caps = ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE;

#if defined(CONFIG_NET_VLAN)
	hw_caps |= ETHERNET_HW_VLAN;
#endif /* CONFIG_NET_VLAN */
	hw_caps |= ETHERNET_HW_TX_CHKSUM_OFFLOAD | ETHERNET_HW_RX_CHKSUM_OFFLOAD;

	return hw_caps;
}

static int eth_mchp_set_config(const struct device *dev, struct net_if *iface __unused,
			       enum ethernet_config_type type, const struct ethernet_config *config)
{
	int result = 0;
	const struct gmac_dev_config *const cfg = dev->config;
	gmac_registers_t *const gmac_regs = cfg->regs;
	uint8_t mac_addr[6];

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(mac_addr, config->mac_address.addr, sizeof(mac_addr));
		gmac_mac_addr_set(gmac_regs, 0, mac_addr);
		LOG_INF("%s MAC set to %02x:%02x:%02x:%02x:%02x:%02x", dev->name, mac_addr[0],
			mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
		break;

	default:
		return -ENOTSUP;
	}

	return result;
}

static int eth_mchp_get_config(const struct device *dev, struct net_if *iface __unused,
			       enum ethernet_config_type type, struct ethernet_config *config)
{
	const struct gmac_dev_config *const cfg = dev->config;
	gmac_registers_t *const gmac_regs = cfg->regs;
	uint32_t val;
	uint32_t retval = 0;
	uint32_t chksum_support = ETHERNET_CHECKSUM_SUPPORT_IPV4_HEADER |
				  ETHERNET_CHECKSUM_SUPPORT_TCP | ETHERNET_CHECKSUM_SUPPORT_UDP;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_RX_CHECKSUM_SUPPORT:
		val = gmac_regs->GMAC_NCFGR;
		config->chksum_support = (val & GMAC_NCFGR_RXCOEN_Msk) ? chksum_support : 0;
		break;

	case ETHERNET_CONFIG_TYPE_TX_CHECKSUM_SUPPORT:
		val = gmac_regs->GMAC_DCFGR;
		config->chksum_support = (val & GMAC_DCFGR_TXCOEN_Msk) ? chksum_support : 0;
		break;

	default:
		return -ENOTSUP;
	}

	return retval;
}

static const struct ethernet_api eth_mchp_api = {
	.iface_api.init = eth_mchp_iface_init,
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	.get_stats = eth_mchp_get_stats,
#endif /* CONFIG_NET_STATISTICS_ETHERNET */
	.get_capabilities = eth_mchp_get_capabilities,
	.set_config = eth_mchp_set_config,
	.get_config = eth_mchp_get_config,
	.get_phy = eth_mchp_get_phy,
	.send = eth_mchp_send,
};

static void eth_irq_config(void)
{
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(0, gmac, irq), DT_INST_IRQ_BY_NAME(0, gmac, priority),
		    eth_mchp_queue0_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_NAME(0, gmac, irq));
}

static const struct gmac_dev_config eth_config = {
	.regs = (gmac_registers_t *)DT_INST_REG_ADDR(0),
	.active_queues = DT_INST_PROP(0, num_queues),
	.pinctrl_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.config_func = eth_irq_config,
	.mcfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(0),
	.mclk_apb_sys = (void *)DT_INST_CLOCKS_CELL_BY_NAME(0, mclk_apb, subsystem),
	.mclk_ahb_sys = (void *)DT_INST_CLOCKS_CELL_BY_NAME(0, mclk_ahb, subsystem),
	.phy_dev = DEVICE_DT_GET(DT_INST_PHANDLE(0, phy_handle))};

static struct gmac_dev_data eth_data;

ETH_NET_DEVICE_DT_INST_DEFINE(0, eth_mchp_initialize, NULL, &eth_data, &eth_config,
			      CONFIG_ETH_INIT_PRIORITY, &eth_mchp_api, GMAC_MTU);
