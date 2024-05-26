/* LAN9250 Stand-alone Ethernet Controller with SPI
 *
 * Copyright (c) 2024 Mario Paja
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT microchip_lan9250

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>
#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet.h>
#include <ethernet/eth_stats.h>

#include "eth_lan9250_priv.h"

LOG_MODULE_REGISTER(eth_lan9250, CONFIG_ETHERNET_LOG_LEVEL);

static int lan9250_write_sys_reg(const struct device *dev, uint16_t address, uint32_t data)
{
	const struct lan9250_config *config = dev->config;
	uint8_t cmd[1 + 2 + 4] = {
		LAN9250_SPI_INSTR_WRITE, (address >> 8) & 0xFF, (address & 0xFF),   (data & 0xFF),
		(data >> 8 & 0xFF),      (data >> 16 & 0xFF),   (data >> 24 & 0xFF)};
	struct spi_buf tx_buf = {
		.buf = &cmd,
		.len = ARRAY_SIZE(cmd),
	};
	struct spi_buf_set tx_bufs = {.buffers = &tx_buf, .count = 1};

	return spi_write_dt(&config->spi, &tx_bufs);
}

static int lan9250_read_sys_reg(const struct device *dev, uint16_t address, uint32_t *value)
{
	const struct lan9250_config *config = dev->config;
	uint32_t data;
	uint8_t cmd[3] = {LAN9250_SPI_INSTR_READ, (address >> 8) & 0xFF, (address & 0xFF)};
	uint8_t tmp[7];

	int ret = 0;

	struct spi_buf tx_buf = {
		.buf = &cmd,
		.len = ARRAY_SIZE(cmd),
	};
	struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	const struct spi_buf rx_buf = {
		.buf = tmp,
		.len = ARRAY_SIZE(tmp),
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1,
	};

	ret |= spi_transceive_dt(&config->spi, &tx, &rx);

	data = tmp[3];
	data |= tmp[4] << 8;
	data |= tmp[5] << 16;
	data |= tmp[6] << 24;

	*value = data;

	return ret;
}

static int lan9250_wait_ready(const struct device *dev, uint16_t address, uint32_t mask,
			      uint32_t expected, uint32_t m_second)
{
	uint32_t tmp;
	int wait_time = 0;

	while (true) {
		lan9250_read_sys_reg(dev, address, &tmp);
		wait_time++;
		k_msleep(1);
		if ((tmp & mask) == expected) {
			return 0;
		} else if (wait_time == m_second) {
			LOG_ERR("NOT READY");
			return -EIO;
		}
	}
}

static int lan9250_read_mac_reg(const struct device *dev, uint8_t address, uint32_t *value)
{
	uint32_t tmp;

	/* Wait for MAC to be ready and send writing register command and data */
	lan9250_wait_ready(dev, LAN9250_MAC_CSR_CMD, LAN9250_MAC_CSR_CMD_BUSY, 0,
			   LAN9250_MAC_TIMEOUT);

	lan9250_write_sys_reg(dev, LAN9250_MAC_CSR_CMD,
			      address | LAN9250_MAC_CSR_CMD_BUSY | LAN9250_MAC_CSR_CMD_READ);

	/* Wait for MAC to be ready and send writing register command and data */
	lan9250_wait_ready(dev, LAN9250_MAC_CSR_CMD, LAN9250_MAC_CSR_CMD_BUSY, 0,
			   LAN9250_MAC_TIMEOUT);

	lan9250_read_sys_reg(dev, LAN9250_MAC_CSR_DATA, &tmp);

	*value = tmp;
	return 0;
}

static int lan9250_write_mac_reg(const struct device *dev, uint8_t address, uint32_t data)
{
	int ret = 0;

	/* Wait for MAC to be ready and send writing register command and data */
	lan9250_wait_ready(dev, LAN9250_MAC_CSR_CMD, LAN9250_MAC_CSR_CMD_BUSY, 0,
			   LAN9250_MAC_TIMEOUT);

	lan9250_write_sys_reg(dev, LAN9250_MAC_CSR_DATA, data);

	lan9250_write_sys_reg(dev, LAN9250_MAC_CSR_CMD, address | LAN9250_MAC_CSR_CMD_BUSY);

	/* Wait until writing MAC is done */
	lan9250_wait_ready(dev, LAN9250_MAC_CSR_CMD, LAN9250_MAC_CSR_CMD_BUSY, 0,
			   LAN9250_MAC_TIMEOUT);

	return ret;
}

static int lan9250_wait_mac_ready(const struct device *dev, uint8_t address, uint32_t mask,
				  uint32_t expected, uint32_t m_second)
{
	uint32_t tmp;
	int wait_time = 0;

	while (true) {
		lan9250_read_mac_reg(dev, address, &tmp);
		wait_time++;
		k_msleep(1);
		if ((tmp & mask) == expected) {
			return 0;
		} else if (wait_time == m_second) {
			return -EIO;
		}
	}
}

static int lan9250_read_phy_reg(const struct device *dev, uint8_t address, uint16_t *value)
{
	uint32_t tmp;

	/* Wait PHY to be ready and send reading register command */
	lan9250_wait_mac_ready(dev, LAN9250_HMAC_MII_ACC, LAN9250_HMAC_MII_ACC_MIIBZY, 0,
			       LAN9250_PHY_TIMEOUT);

	lan9250_write_mac_reg(dev, LAN9250_HMAC_MII_ACC,
			      LAN9250_HMAC_MII_ACC_PHY_ADDR_DEFAULT |
				      (address << HMACMIIAR_MIIRX_S));

	/* Wait PHY to be ready and send reading register command */
	lan9250_wait_mac_ready(dev, LAN9250_HMAC_MII_ACC, LAN9250_HMAC_MII_ACC_MIIBZY, 0,
			       LAN9250_PHY_TIMEOUT);

	lan9250_read_mac_reg(dev, LAN9250_HMAC_MII_DATA, &tmp);

	*value = tmp;

	return 0;
}

static int lan9250_write_phy_reg(const struct device *dev, uint8_t address, uint16_t data)
{
	/* Wait PHY to be ready and send reading register command */
	lan9250_wait_mac_ready(dev, LAN9250_HMAC_MII_ACC, LAN9250_HMAC_MII_ACC_MIIBZY, 0,
			       LAN9250_PHY_TIMEOUT);

	lan9250_write_mac_reg(dev, LAN9250_HMAC_MII_DATA, data);
	lan9250_write_mac_reg(dev, LAN9250_HMAC_MII_ACC,
			      (1 << 11) | (address << HMACMIIAR_MIIRX_S) |
				      LAN9250_HMAC_MII_ACC_MIIW_R);

	/* Wait PHY to be ready and send reading register command */
	lan9250_wait_mac_ready(dev, LAN9250_HMAC_MII_ACC, LAN9250_HMAC_MII_ACC_MIIBZY, 0,
			       LAN9250_PHY_TIMEOUT);

	return 0;
}

static int lan9250_set_macaddr(const struct device *dev)
{
	struct lan9250_runtime *ctx = dev->data;
	int ret = 0;

	ret |= lan9250_write_mac_reg(dev, LAN9250_HMAC_ADDRL,
				     ctx->mac_address[0] | (ctx->mac_address[1] << 8) |
					     (ctx->mac_address[2] << 16) |
					     (ctx->mac_address[3] << 24));

	ret |= lan9250_write_mac_reg(dev, LAN9250_HMAC_ADDRH,
				     ctx->mac_address[4] | (ctx->mac_address[5] << 8));

	return ret;
}

static int lan9250_spi_check(const struct device *dev)
{
	uint32_t tmp;
	int ret = 0;
	uint8_t retries = LAN9250_DEFAULT_NUMOF_RETRIES;

	do {
		ret |= lan9250_read_sys_reg(dev, LAN9250_BYTE_TEST, &tmp);
		k_busy_wait(USEC_PER_MSEC * 1U);
	} while (tmp != LAN9250_BYTE_TEST_DEFAULT && retries--);

	if (tmp != LAN9250_BYTE_TEST_DEFAULT) {
		LOG_ERR("Timeout, failed to establish SPI connection");
		return -EIO;
	}

	LOG_DBG("SPI OK");

	return ret;
}

static int lan9250_hw_cfg_check(const struct device *dev)
{
	uint32_t tmp;
	int ret = 0;

	do {
		ret |= lan9250_read_sys_reg(dev, LAN9250_HW_CFG, &tmp);
		k_busy_wait(USEC_PER_MSEC * 1U);
	} while ((tmp & LAN9250_HW_CFG_DEVICE_READY) == 0);

	LOG_DBG("LAN9250_HW_CFG OK");

	return ret;
}

static int lan9250_sw_reset(const struct device *dev)
{
	/* Wait until LAN9250 SPI bus is ready */

	lan9250_wait_ready(dev, LAN9250_BYTE_TEST, BOTR_MASK, LAN9250_BYTE_TEST_DEFAULT,
			   LAN9250_RESET_TIMEOUT);

	lan9250_write_sys_reg(dev, LAN9250_RESET_CTL,
			      LAN9250_RESET_CTL_HMAC_RST | LAN9250_RESET_CTL_PHY_RST |
				      LAN9250_RESET_CTL_DIGITAL_RST);

	lan9250_wait_ready(dev, LAN9250_BYTE_TEST, BOTR_MASK, LAN9250_BYTE_TEST_DEFAULT,
			   LAN9250_RESET_TIMEOUT);

	return 0;
}

static int lan9250_reset(const struct device *dev)
{
	lan9250_hw_cfg_check(dev);
	lan9250_sw_reset(dev);

	/* Configure TX FIFO size mode to be 8:
	 *
	 *   - TX data FIFO size:   7680
	 *   - RX data FIFO size:   7680
	 *   - TX status FIFO size: 512
	 *   - RX status FIFO size: 512
	 */
	lan9250_write_sys_reg(dev, LAN9250_HW_CFG,
			      LAN9250_HW_CFG_MBO | LAN9250_HW_CFG_TX_FIF_SZ_8KB);

	/* Configure MAC automatic flow control:
	 *
	 *   - Automatic flow control high level: 110
	 *   - Automatic flow control low level: 55
	 *   - Backpressure duration: 4
	 *   - Flow control on any frame
	 */

	lan9250_write_sys_reg(dev, LAN9250_AFC_CFG,
			      (110 << HMAFCCFGR_AFCHL_S) | (55 << LAN9250_AFC_CFG_FCMULT) |
				      (4 << LAN9250_AFC_CFG_FCBRD) | LAN9250_AFC_CFG_FCANY);

	/* Configure host MAC flow control:
	 *
	 *   - Pause time: 15
	 *   - Flow control
	 */

	lan9250_write_mac_reg(dev, LAN9250_HMAC_FLOW,
			      (0xf << HMACFCR_PT_S) | LAN9250_HMAC_FLOW_FCEN);

	/* Configure interrupt:
	 *
	 *   - Interrupt De-assertion interval: 10
	 *   - Interrupt output to pin
	 *   - Interrupt pin active output low
	 *   - Interrupt pin push-pull driver
	 */

	lan9250_write_sys_reg(dev, LAN9250_IRQ_CFG,
			      LAN9250_IRQ_CFG_INT_DEAS_100US | LAN9250_IRQ_CFG_IRQ_EN |
				      LAN9250_IRQ_CFG_IRQ_TYPE_PP);

	/* Configure interrupt trigger source, please refer to macro
	 * LAN9250_INT_SOURCE.
	 */
	lan9250_write_sys_reg(dev, LAN9250_INT_EN,
			      LAN9250_INT_EN_PHY_INT_EN | LAN9250_INT_EN_TDFA_EN |
				      LAN9250_INT_EN_RSFL_EN);

	/* Disable TX data FIFO available interrupt */
	lan9250_write_sys_reg(dev, LAN9250_FIFO_INT,
			      LAN9250_FIFO_INT_TX_DATA_AVAILABLE_LEVEL |
				      LAN9250_FIFO_INT_TX_STATUS_LEVEL);

	/* Configure RX:
	 *
	 *   - RX DMA counter: Ethernet maximum packet size
	 *   - RX data offset: 4, so that need read dummy before reading data
	 */
	lan9250_write_sys_reg(dev, LAN9250_RX_CFG, (NET_ETH_MAX_FRAME_SIZE << 16) | (4 << 8));

	/* Configure remote power management:
	 *
	 *   - Auto wakeup
	 *   - Disable 1588 clock
	 *   - Disable 1588 timestamp unit clock
	 *   - Energy-detect
	 *   - Wake on
	 *   - PME pin push-pull driver
	 *   - Clear wakeon
	 *   - PME active high
	 *   - PME pin
	 */
	lan9250_write_sys_reg(dev, LAN9250_PMT_CTRL,
			      LAN9250_PMT_CTRL_PM_WAKE | LAN9250_PMT_CTRL_1588_DIS |
				      LAN9250_PMT_CTRL_1588_TSU_DIS | LAN9250_PMT_CTRL_ED_EN |
				      LAN9250_PMT_CTRL_WOL_EN | LAN9250_PMT_CTRL_PME_TYPE |
				      LAN9250_PMT_CTRL_WOL_STS | LAN9250_PMT_CTRL_PME_POL |
				      LAN9250_PMT_CTRL_PME_EN);

	/* Configure PHY basic control:
	 *
	 *   - Auto-Negotiation for 10/100 Mbits and Half/Full Duplex
	 */

	lan9250_write_phy_reg(dev, LAN9250_PHY_BASIC_CONTROL, LAN9250_PHY_BASIC_CONTROL_PHY_AN);

	/* Configure PHY auto-negotiation advertisement capability:
	 *
	 *   - Asymmetric pause
	 *   - Symmetric pause
	 *   - 100Base-X half duplex or full duplex
	 *   - 10Base-X half duplex or full duplex
	 *   - Select IEEE802.3
	 */
	lan9250_write_phy_reg(
		dev, LAN9250_PHY_AN_ADV,
		LAN9250_PHY_AN_ADV_ASYM_PAUSE | LAN9250_PHY_AN_ADV_SYM_PAUSE |
			/*LAN9250_PHY_AN_ADV_100BTX_HD |*/ LAN9250_PHY_AN_ADV_100BTX_FD |
			/*LAN9250_PHY_AN_ADV_10BT_HD |*/ LAN9250_PHY_AN_ADV_10BT_FD |
			LAN9250_PHY_AN_ADV_SELECTOR_DEFAULT);

	/* Configure PHY special mode:
	 *
	 *   - PHY mode = 111b, enable all capable and auto-nagotiation
	 *   - PHY address = 1, default value is fixed to 1 by manufacturer
	 */

	lan9250_write_phy_reg(dev, LAN9250_PHY_SPECIAL_MODES, LAN9250_PHY_SPECIAL_MODES_MODE | 1);

	/* Configure PHY special control or status indication:
	 *
	 *   - Port auto-MDIX determined by bits 14 and 13
	 *   - Auto-MDIX
	 *   - Disable SQE tests
	 */

	lan9250_write_phy_reg(dev, LAN9250_PHY_SPECIAL_CONTROL_STAT_IND,
			      LAN9250_PHY_SPECIAL_CONTROL_STAT_IND_AMDIXCTRL |
				      LAN9250_PHY_SPECIAL_CONTROL_STAT_IND_AMDIXEN |
				      LAN9250_PHY_SPECIAL_CONTROL_STAT_IND_SQEOFF);

	/* Configure PHY interrupt source:
	 *
	 *   - Link up
	 *   - Link down
	 */
	lan9250_write_phy_reg(dev, LAN9250_PHY_INTERRUPT_MASK,
			      LAN9250_PHY_INTERRUPT_SOURCE_LINK_UP |
				      LAN9250_PHY_INTERRUPT_SOURCE_LINK_DOWN);

	/* Configure special control or status:
	 *
	 *   - Fixed to write 0000010b to reserved filed
	 */

	lan9250_write_phy_reg(dev, LAN9250_PHY_SPECIAL_CONTROL_STATUS,
			      LAN9250_PHY_MODE_CONTROL_STATUS_ALTINT);

	/* Clear interrupt status */
	lan9250_write_sys_reg(dev, LAN9250_INT_STS, 0xFFFFFFFF);

	/* Configure HMAC control:
	 *
	 *   - Automatically strip the pad field on incoming packets
	 *   - TX enable
	 *   - RX enable
	 *   - Full duplex
	 */
	lan9250_write_mac_reg(dev, LAN9250_HMAC_CR,
			      LAN9250_HMAC_CR_PADSTR | LAN9250_HMAC_CR_TXEN | LAN9250_HMAC_CR_RXEN |
				      LAN9250_HMAC_CR_FDPX);

	/** Configure TX:
	 *
	 *   - TX enable
	 */
	lan9250_write_sys_reg(dev, LAN9250_TX_CFG, LAN9250_TX_CFG_TX_ON);

	return 0;
}

static int lan9250_write_buf(const struct device *dev, uint8_t *data_buffer, uint16_t buf_len)
{
	const struct lan9250_config *config = dev->config;
	int ret = 0;
	uint8_t cmd[3] = {LAN9250_SPI_INSTR_WRITE, (LAN9250_TX_DATA_FIFO >> 8) & 0xFF,
			  (LAN9250_TX_DATA_FIFO & 0xFF)};

	struct spi_buf tx_buf[2];

	tx_buf[0].buf = &cmd;
	tx_buf[0].len = ARRAY_SIZE(cmd);

	tx_buf[1].buf = data_buffer;
	tx_buf[1].len = buf_len;

	struct spi_buf_set tx = {.buffers = tx_buf, .count = 2};

	ret |= spi_transceive_dt(&config->spi, &tx, NULL);

	return ret;
}

static int lan9250_read_buf(const struct device *dev, uint8_t *data_buffer, uint16_t buf_len)
{
	const struct lan9250_config *config = dev->config;
	uint8_t cmd[3] = {LAN9250_SPI_INSTR_READ, (LAN9250_RX_DATA_FIFO >> 8) & 0xFF,
			  (LAN9250_RX_DATA_FIFO & 0xFF)};

	int ret = 0;

	struct spi_buf tx_buf = {
		.buf = &cmd,
		.len = ARRAY_SIZE(cmd),
	};
	struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	struct spi_buf rx_buf[2];

	rx_buf[0].buf = NULL;
	rx_buf[0].len = 3;

	rx_buf[1].buf = data_buffer;
	rx_buf[1].len = buf_len;

	const struct spi_buf_set rx = {.buffers = rx_buf, .count = 2};

	ret |= spi_transceive_dt(&config->spi, &tx, &rx);

	return ret;
}

static int lan9250_rx(const struct device *dev)
{
	const struct lan9250_config *config = dev->config;
	struct lan9250_runtime *ctx = dev->data;
	uint8_t pktcnt;
	uint32_t tmp;
	uint16_t pkt_len;
	uint16_t lengthfr;

	/* Check valid packet count in RX FIFO */
	lan9250_read_sys_reg(dev, LAN9250_RX_FIFO_INF, &tmp);
	pktcnt = (tmp & LAN9250_RX_FIFO_INF_RXSUSED) >> 16;

	if (!pktcnt) {
		return 0;
	}

	k_sem_take(&ctx->tx_rx_sem, K_FOREVER);

	while (pktcnt--) {

		/* Get the length of the packet */
		lan9250_read_sys_reg(dev, LAN9250_RX_STATUS_FIFO, &tmp);

		pkt_len = (tmp & LAN9250_RX_STS_PACKET_LEN) >> 16;
		pkt_len -= 4;
		lengthfr = pkt_len;

		struct net_buf *pkt_buf;
		struct net_pkt *pkt;

		/* Get the frame from the buffer */
		pkt = net_pkt_rx_alloc_with_buffer(ctx->iface, pkt_len, AF_UNSPEC, 0,
						   K_MSEC(config->timeout));
		if (!pkt) {
			LOG_ERR("%s: Could not allocate rx buffer", dev->name);
			eth_stats_update_errors_rx(ctx->iface);
			return 0;
		}

		pkt_buf = pkt->buffer;

		lan9250_read_sys_reg(dev, LAN9250_RX_DATA_FIFO, &tmp);

		do {

			size_t frag_len;
			uint8_t *data_ptr;
			size_t spi_frame_len;

			data_ptr = pkt_buf->data;

			/* Review the space available for the new frag */
			frag_len = net_buf_tailroom(pkt_buf);

			if (pkt_len > frag_len) {
				spi_frame_len = frag_len;
			} else {
				spi_frame_len = pkt_len;
			}

			/* Review the space available for the new frag */
			lan9250_read_buf(dev, data_ptr, spi_frame_len);
			net_buf_add(pkt_buf, pkt_len);

			pkt_len -= spi_frame_len;
			pkt_buf = pkt_buf->frags;

		} while (pkt_len > 0);

		uint8_t dummy[4];

		lan9250_read_buf(dev, dummy, 4);

		net_pkt_set_iface(pkt, ctx->iface);

		/* Feed buffer frame to IP stack */
		if (net_recv_data(net_pkt_iface(pkt), pkt) < 0) {
			net_pkt_unref(pkt);
		}
	}

	k_sem_give(&ctx->tx_rx_sem);

	return 0;
}

static int lan9250_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct lan9250_runtime *ctx = dev->data;
	size_t len = net_pkt_get_len(pkt);

	LOG_DBG("%s: pkt %p (len %u)", dev->name, pkt, len);

	uint32_t regval;
	uint16_t free_size;
	uint8_t status_size;

	lan9250_read_sys_reg(dev, LAN9250_TX_FIFO_INF, &regval);

	status_size = (regval & LAN9250_TX_FIFO_INF_TXSUSED) >> 16;
	free_size = regval & LAN9250_TX_FIFO_INF_TXFREE;

	LOG_DBG("status_size:%d, free_size:%d", status_size, free_size);

	/* Clear TX status FIFO if it is no empty by reading data */

	uint32_t tmp;

	for (int i = 0; i < status_size; i++) {
		lan9250_read_sys_reg(dev, LAN9250_TX_STATUS_FIFO, &tmp);
	}

	k_sem_take(&ctx->tx_rx_sem, K_FOREVER);

	/* TX command 'A' */
	lan9250_write_sys_reg(dev, LAN9250_TX_DATA_FIFO,
			      LAN9250_TX_CMD_A_INT_ON_COMP | LAN9250_TX_CMD_A_BUFFER_ALIGN_4B |
				      LAN9250_TX_CMD_A_START_OFFSET_0B |
				      LAN9250_TX_CMD_A_FIRST_SEG | LAN9250_TX_CMD_A_LAST_SEG | len);

	/* TX command 'B' */
	lan9250_write_sys_reg(dev, LAN9250_TX_DATA_FIFO, LAN9250_TX_CMD_B_PACKET_TAG | len);

	if (net_pkt_read(pkt, ctx->buf, len)) {
		return -EIO;
	}

	lan9250_write_buf(dev, ctx->buf, LAN9250_ALIGN(len));

	k_sem_give(&ctx->tx_rx_sem);

	return 0;
}

static void lan9250_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct lan9250_runtime *context = CONTAINER_OF(cb, struct lan9250_runtime, gpio_cb);

	k_sem_give(&context->int_sem);
}

static void lan9250_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct lan9250_runtime *context = p1;
	uint32_t isr;
	uint16_t tmp;

	while (true) {
		k_sem_take(&context->int_sem, K_FOREVER);
		lan9250_read_sys_reg(context->dev, LAN9250_INT_STS, &isr);

		if ((isr & LAN9250_INT_STS_PHY_INT) != 0) {
			/* Read PHY interrupt source register */
			lan9250_read_phy_reg(context->dev, LAN9250_PHY_INTERRUPT_SOURCE, &tmp);

			if (tmp & LAN9250_PHY_INTERRUPT_SOURCE_LINK_UP) {
				LOG_DBG("LINK UP");
				net_eth_carrier_on(context->iface);
			} else if (tmp & LAN9250_PHY_INTERRUPT_SOURCE_LINK_DOWN) {
				LOG_DBG("LINK DOWN");
				net_eth_carrier_off(context->iface);
			}
		}

		if ((isr & LAN9250_INT_STS_RSFL) != 0) {

			lan9250_rx(context->dev);
		}

		lan9250_write_sys_reg(context->dev, LAN9250_INT_STS, isr);
	}
}

static enum ethernet_hw_caps lan9250_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE_T | ETHERNET_LINK_100BASE_T;
}

static void lan9250_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct lan9250_runtime *context = dev->data;

	net_if_set_link_addr(iface, context->mac_address, sizeof(context->mac_address),
			     NET_LINK_ETHERNET);
	context->iface = iface;
	ethernet_init(iface);

	net_if_carrier_off(iface);
}

static const struct ethernet_api api_funcs = {
	.iface_api.init = lan9250_iface_init,
	.get_capabilities = lan9250_get_capabilities,
	.send = lan9250_tx,
};

static int lan9250_init(const struct device *dev)
{
	const struct lan9250_config *config = dev->config;
	struct lan9250_runtime *context = dev->data;

	context->dev = dev;

	/* SPI config */
	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI master port %s not ready", config->spi.bus->name);
		return -EINVAL;
	}

	/* Initialize GPIO */
	if (!gpio_is_ready_dt(&config->interrupt)) {
		LOG_ERR("GPIO port %s not ready", config->interrupt.port->name);
		return -EINVAL;
	}

	if (gpio_pin_configure_dt(&config->interrupt, GPIO_INPUT)) {
		LOG_ERR("Unable to configure GPIO pin %u", config->interrupt.pin);
		return -EINVAL;
	}

	gpio_init_callback(&(context->gpio_cb), lan9250_gpio_callback, BIT(config->interrupt.pin));

	if (gpio_add_callback(config->interrupt.port, &(context->gpio_cb))) {
		return -EINVAL;
	}

	gpio_pin_interrupt_configure_dt(&config->interrupt, GPIO_INT_EDGE_TO_ACTIVE);

	lan9250_spi_check(dev);
	lan9250_reset(dev);

	lan9250_set_macaddr(dev);

	k_thread_create(&context->thread, context->thread_stack,
			CONFIG_ETH_LAN9250_RX_THREAD_STACK_SIZE, lan9250_thread, context, NULL,
			NULL, K_PRIO_COOP(CONFIG_ETH_LAN9250_RX_THREAD_PRIO), 0, K_NO_WAIT);

	LOG_INF("LAN9250 Initialized");

	return 0;
}

static struct lan9250_runtime lan9250_0_runtime = {
	.mac_address = DT_INST_PROP(0, local_mac_address),
	.tx_rx_sem = Z_SEM_INITIALIZER(lan9250_0_runtime.tx_rx_sem, 1, UINT_MAX),
	.int_sem = Z_SEM_INITIALIZER(lan9250_0_runtime.int_sem, 0, UINT_MAX),
};

static const struct lan9250_config lan9250_0_config = {
	.spi = SPI_DT_SPEC_INST_GET(0, SPI_WORD_SET(8), 0),
	.interrupt = GPIO_DT_SPEC_INST_GET(0, int_gpios),
	.timeout = CONFIG_ETH_LAN9250_TIMEOUT,
};

ETH_NET_DEVICE_DT_INST_DEFINE(0, lan9250_init, NULL, &lan9250_0_runtime, &lan9250_0_config,
			      CONFIG_ETH_INIT_PRIORITY, &api_funcs, NET_ETH_MTU);
