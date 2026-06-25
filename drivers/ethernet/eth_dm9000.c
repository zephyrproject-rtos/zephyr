/*
 * Copyright (c) 2026 Fuyu Fei
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT davicom_dm9000

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/phy.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <ethernet/eth_stats.h>
#include <zephyr/net/net_stats.h>

LOG_MODULE_REGISTER(eth_dm9000, CONFIG_ETHERNET_LOG_LEVEL);

#define DM9000_CRC_SIZE		4U
#define DM9000_MIN_FRAME_SIZE	64U
#define DM9000_MAX_RX_SIZE	(NET_ETH_MAX_FRAME_SIZE + DM9000_CRC_SIZE)
#define DM9000_ID		0x0a469000U

#define DM9000_NCR		0x00
#define DM9000_NSR		0x01
#define DM9000_TCR		0x02
#define DM9000_RCR		0x05
#define DM9000_BPTR		0x08
#define DM9000_FCTR		0x09
#define DM9000_FCR		0x0a
#define DM9000_EPCR		0x0b
#define DM9000_EPAR		0x0c
#define DM9000_EPDRL		0x0d
#define DM9000_PAR		0x10
#define DM9000_MAR		0x16
#define DM9000_GPCR		0x1e
#define DM9000_GPR		0x1f
#define DM9000_VIDL		0x28
#define DM9000_PIDL		0x2a
#define DM9000_TCR2		0x2d
#define DM9000_OTCR		0x2e
#define DM9000_SMCR		0x2f
#define DM9000_TCSCR		0x31
#define DM9000_RCSCSR		0x32
#define DM9000_MRCMDX		0xf0
#define DM9000_MRCMD		0xf2
#define DM9000_MWCMD		0xf8
#define DM9000_TXPLL		0xfc
#define DM9000_ISR		0xfe
#define DM9000_IMR		0xff

#define DM9000_NCR_RST		BIT(0)
#define DM9000_NCR_MAC_LBK	BIT(3)
#define DM9000_TCR_TXREQ	BIT(0)
#define DM9000_RCR_DIS_LONG	BIT(5)
#define DM9000_RCR_DIS_CRC	BIT(4)
#define DM9000_RCR_PRMSC	BIT(1)
#define DM9000_RCR_RXEN		BIT(0)
#define DM9000_RSR_RF		BIT(7)
#define DM9000_RSR_MF		BIT(6)
#define DM9000_RSR_LCS		BIT(5)
#define DM9000_RSR_RWTO		BIT(4)
#define DM9000_RSR_PLE		BIT(3)
#define DM9000_RSR_AE		BIT(2)
#define DM9000_RSR_CE		BIT(1)
#define DM9000_RSR_FOE		BIT(0)
#define DM9000_RSR_ERRORS	(DM9000_RSR_RF | DM9000_RSR_LCS | DM9000_RSR_RWTO | \
				 DM9000_RSR_PLE | DM9000_RSR_AE | DM9000_RSR_CE | \
				 DM9000_RSR_FOE)
#define DM9000_EPCR_EPOS	BIT(3)
#define DM9000_EPCR_ERPRR	BIT(2)
#define DM9000_EPCR_ERPRW	BIT(1)
#define DM9000_EPCR_ERRE	BIT(0)
#define DM9000_GPR_PHY_ON	0x00
#define DM9000_GPR_PHY_OFF	0x01
#define DM9000_GPCR_GEP_CNTL	BIT(0)
#define DM9000_BPTR_DEFAULT	0x37
#define DM9000_FCTR_DEFAULT	0x38
#define DM9000_FCR_DEFAULT	0x28
#define DM9000_MAR_7_BCAST_EN	0x80
#define DM9000_OTCR_100MHZ	BIT(7)
#define DM9000_TCR2_LED_MODE	0x80
/* TX checksum generation: enable IPv4 (BIT0), TCP (BIT1) and UDP (BIT2). */
#define DM9000_TCSCR_ALL	0x07
/* RX checksum: enable checking (RCSEN, BIT0) and let the controller discard
 * frames that fail the checksum in hardware (DCSE, BIT1) so only verified
 * frames reach the host and the stack can skip the software checksum.
 */
#define DM9000_RCSCSR_ENABLE	0x03
/* NSR (Network Status Register): write-1-to-clear the wake-event and the two
 * transmit-complete flags (WAKEST | TX2END | TX1END) left over from reset.
 */
#define DM9000_NSR_CLEAR	0x2c
#define DM9000_ISR_PR		BIT(0)
/* ISR (Interrupt Status Register): write-1-to-clear all status flags. */
#define DM9000_ISR_CLEAR_ALL	0x3f
#define DM9000_IMR_PAR		BIT(7)
#define DM9000_IMR_PRI		BIT(0)
#define DM9000_PKT_RDY		0x01
#if defined(CONFIG_ETH_DM9000_HW_CHECKSUM)
/* With RX checksum offload enabled (RCSCSR RCSEN) the controller overwrites
 * bits [7:2] of the packet's first status byte with the checksum status
 * (IPP/TCPP/UDPP present and the IP/TCP/UDP fail bits); only bits [1:0] carry
 * the packet-ready indicator. Mask the status bits before validating
 * readiness. DCSE is also set, so frames whose IP/TCP/UDP checksum fails are
 * discarded in hardware and never reach the host.
 */
#define DM9000_PKT_RDY_MASK	0x03
#else
/* No checksum offload: the first status byte carries only the ready bits. */
#define DM9000_PKT_RDY_MASK	0xff
#endif
/* Reset pulse settle time, matching the DM9000 reference BSP (microseconds). */
#define DM9000_HW_RESET_DELAY_US	100
/* MII access settle time before polling EPCR, matching the reference BSP. */
#define DM9000_EPCR_PREWAIT_US		200
/* Poll interval while waiting for the two-packet TX FIFO to drain. */
#define DM9000_TX_DRAIN_POLL_US		5
/* Optional reset-gpios pulse: assert hold and post-deassert recovery. */
#define DM9000_RESET_ASSERT_MS		1
#define DM9000_RESET_RECOVERY_MS	20

#define DM9000_EPCR_POLL_TIMEOUT	K_MSEC(10)
#define DM9000_EPCR_POLL_INTERVAL	K_USEC(100)

struct eth_dm9000_config {
	DEVICE_MMIO_NAMED_ROM(addr);
	DEVICE_MMIO_NAMED_ROM(data);
	struct net_eth_mac_config mac_cfg;
	struct gpio_dt_spec gpio_int;
	struct gpio_dt_spec gpio_reset;
	const struct device *phy_dev;
	uint32_t host_clock_freq;
};

struct eth_dm9000_data {
	DEVICE_MMIO_NAMED_RAM(addr);
	DEVICE_MMIO_NAMED_RAM(data);
	K_KERNEL_STACK_MEMBER(rx_thread_stack, CONFIG_ETH_DM9000_RX_THREAD_STACK_SIZE);
	struct gpio_callback gpio_cb;
	struct k_thread rx_thread;
	struct k_mutex lock;
	struct k_sem int_event;
	struct net_if *iface;
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	struct net_stats_eth stats;
#endif
	uint8_t mac_addr[6];
	uint8_t tx_count;
};

/* The integrated MII PHY is reached through the controller's EPCR/EPAR/EPDR
 * registers rather than a dedicated MDIO pin pair. A child "davicom,dm9000-mdio"
 * node binds this controller as an MDIO bus so the generic phy_mii driver can
 * manage the PHY; that controller forwards every transfer to the parent MAC.
 */
struct eth_dm9000_mdio_config {
	const struct device *mac;
};

#define DEV_CFG(dev)  ((const struct eth_dm9000_config *)(dev)->config)
#define DEV_DATA(dev) ((struct eth_dm9000_data *)(dev)->data)

static inline volatile uint16_t *dm9000_addr_ptr(const struct device *dev)
{
	return (volatile uint16_t *)(DEVICE_MMIO_NAMED_GET(dev, addr));
}

static inline volatile uint16_t *dm9000_data_ptr(const struct device *dev)
{
	return (volatile uint16_t *)(DEVICE_MMIO_NAMED_GET(dev, data));
}

/*
 * 16-bit accessors for the index and data ports.
 *
 * The DM9000 selects its 8-, 16- or 32-bit host interface width by pin
 * strapping sampled at reset; this driver implements the 16-bit interface
 * only and expects the bus to be wired and configured accordingly.
 *
 * With CONFIG_ETH_DM9000_DIRECT_IO the ports are accessed through a plain
 * volatile dereference, which omits the data memory barrier that
 * sys_read16()/sys_write16() emit on some architectures (for example Arm).
 * Dropping that barrier measurably speeds up the per-frame FIFO copy loops
 * and is safe when the controller sits on a strongly ordered, non-speculative
 * device-memory region (the normal case for an external parallel bus such as
 * the STM32 FMC).  Otherwise the portable sys_read16()/sys_write16() helpers
 * are used so the driver stays decoupled from any particular bus mapping.
 */
static inline uint16_t dm9000_io_read16(volatile uint16_t *port)
{
#ifdef CONFIG_ETH_DM9000_DIRECT_IO
	return *port;
#else
	return sys_read16((mem_addr_t)port);
#endif
}

static inline void dm9000_io_write16(volatile uint16_t *port, uint16_t val)
{
#ifdef CONFIG_ETH_DM9000_DIRECT_IO
	*port = val;
#else
	sys_write16(val, (mem_addr_t)port);
#endif
}

static inline uint8_t dm9000_read_reg(const struct device *dev, uint8_t reg)
{
	dm9000_io_write16(dm9000_addr_ptr(dev), reg);
	return (uint8_t)dm9000_io_read16(dm9000_data_ptr(dev));
}

static inline void dm9000_write_reg(const struct device *dev, uint8_t reg, uint8_t val)
{
	dm9000_io_write16(dm9000_addr_ptr(dev), reg);
	dm9000_io_write16(dm9000_data_ptr(dev), val);
}

static int dm9000_write_regs(const struct device *dev, uint8_t reg, const uint8_t *data,
			     size_t len)
{
	for (size_t i = 0; i < len; i++) {
		dm9000_write_reg(dev, (uint8_t)(reg + i), data[i]);
	}

	return 0;
}

static void dm9000_read_data(const struct device *dev, uint8_t *data, uint16_t len)
{
	volatile uint16_t *port = dm9000_data_ptr(dev);
	uint16_t *dst = (uint16_t *)data;
	uint16_t words = len / 2U;

	while (words >= 4U) {
		*dst++ = dm9000_io_read16(port);
		*dst++ = dm9000_io_read16(port);
		*dst++ = dm9000_io_read16(port);
		*dst++ = dm9000_io_read16(port);
		words -= 4U;
	}

	while (words-- != 0U) {
		*dst++ = dm9000_io_read16(port);
	}

	if (len & 1U) {
		data[len - 1U] = (uint8_t)dm9000_io_read16(port);
	}
}

static void dm9000_write_pkt(const struct device *dev, struct net_pkt *pkt, uint16_t len)
{
	volatile uint16_t *port = dm9000_data_ptr(dev);
	struct net_buf *frag = pkt->buffer;
	uint16_t remaining = len;
	uint8_t low = 0U;
	bool has_low = false;

	dm9000_io_write16(dm9000_addr_ptr(dev), DM9000_MWCMD);

	while (remaining != 0U) {
		const uint8_t *data;
		uint16_t frag_len;
		uint16_t pos = 0U;

		__ASSERT_NO_MSG(frag != NULL);

		data = frag->data;
		frag_len = MIN(remaining, (uint16_t)frag->len);

		if (has_low && frag_len != 0U) {
			dm9000_io_write16(port, low | ((uint16_t)data[0] << 8));
			pos = 1U;
			has_low = false;
		}

		while ((uint16_t)(frag_len - pos) >= 2U) {
			dm9000_io_write16(port, UNALIGNED_GET((uint16_t *)(data + pos)));
			pos += 2U;
		}

		if (pos < frag_len) {
			low = data[pos];
			has_low = true;
		}

		remaining -= frag_len;
		frag = frag->frags;
	}

	if (has_low) {
		dm9000_io_write16(port, low);
	}
}

static void dm9000_discard_data(const struct device *dev, uint16_t len)
{
	volatile uint16_t *port = dm9000_data_ptr(dev);
	uint16_t words = len / 2U;

	while (words-- != 0U) {
		(void)dm9000_io_read16(port);
	}

	if (len & 1U) {
		(void)dm9000_io_read16(port);
	}
}

static int dm9000_epcr_poll(const struct device *dev, struct k_mutex *lock, k_timeout_t timeout)
{
	/* MII register access takes ~100 µs.  The ERRE busy bit does not
	 * assert until the DM9000 starts the internal operation — reading
	 * EPCR immediately after the command write sees ERRE=0 and returns
	 * "done" before the transfer has begun.  Pre-wait 200 µs (matching
	 * the reference BSP DM9K_Delay(100)) before entering the poll loop.
	 *
	 * The MII transfer runs autonomously inside the DM9000 once the
	 * command is written, leaving the host bus idle for the duration.
	 * Drop the device lock across every settle/poll wait (mirroring the
	 * TX-drain busy-wait in dm9000_tx) so RX/TX can use the bus meanwhile;
	 * the lock is held on entry and re-acquired before returning. Each
	 * MII register lives at a different index than the RX/TX FIFO ports
	 * and every port access re-selects its index under the lock, so an
	 * interleaved frame transfer cannot disturb the in-flight transfer.
	 */
	k_mutex_unlock(lock);
	k_busy_wait(DM9000_EPCR_PREWAIT_US);
	k_mutex_lock(lock, K_FOREVER);

	k_timepoint_t timepoint = sys_timepoint_calc(timeout);

	while ((dm9000_read_reg(dev, DM9000_EPCR) & DM9000_EPCR_ERRE) != 0U) {
		if (sys_timepoint_expired(timepoint)) {
			return -ETIMEDOUT;
		}
		k_mutex_unlock(lock);
		k_sleep(DM9000_EPCR_POLL_INTERVAL);
		k_mutex_lock(lock, K_FOREVER);
	}

	return 0;
}

/* Single MII (clause-22) register access through EPCR/EPAR/EPDR. The PHY
 * address occupies EPAR bits [7:6]; the integrated PHY answers at address 1.
 * The caller holds the device lock; dm9000_epcr_poll() drops it across the
 * hardware settle/poll waits and re-acquires it before returning.
 */
static int dm9000_mii_read_locked(const struct device *dev, uint8_t prtad, uint8_t regad,
				  uint16_t *data)
{
	struct eth_dm9000_data *dev_data = dev->data;

	dm9000_write_reg(dev, DM9000_EPAR, (uint8_t)((prtad << 6) | (regad & 0x1fU)));
	dm9000_write_reg(dev, DM9000_EPCR, DM9000_EPCR_ERPRR | DM9000_EPCR_EPOS);

	int ret = dm9000_epcr_poll(dev, &dev_data->lock, DM9000_EPCR_POLL_TIMEOUT);

	dm9000_write_reg(dev, DM9000_EPCR, 0);
	if (ret < 0) {
		return ret;
	}

	*data = dm9000_read_reg(dev, DM9000_EPDRL) |
		((uint16_t)dm9000_read_reg(dev, DM9000_EPDRL + 1U) << 8);

	return 0;
}

static int dm9000_mii_write_locked(const struct device *dev, uint8_t prtad, uint8_t regad,
				   uint16_t data)
{
	struct eth_dm9000_data *dev_data = dev->data;

	dm9000_write_reg(dev, DM9000_EPAR, (uint8_t)((prtad << 6) | (regad & 0x1fU)));
	dm9000_write_reg(dev, DM9000_EPDRL, (uint8_t)data);
	dm9000_write_reg(dev, DM9000_EPDRL + 1U, (uint8_t)(data >> 8));
	dm9000_write_reg(dev, DM9000_EPCR, DM9000_EPCR_ERPRW | DM9000_EPCR_EPOS);

	int ret = dm9000_epcr_poll(dev, &dev_data->lock, DM9000_EPCR_POLL_TIMEOUT);

	dm9000_write_reg(dev, DM9000_EPCR, 0);

	return ret;
}

static int dm9000_check_id(const struct device *dev)
{
	uint32_t id;

	id = ((uint32_t)dm9000_read_reg(dev, DM9000_VIDL + 1U) << 24) |
	     ((uint32_t)dm9000_read_reg(dev, DM9000_VIDL) << 16) |
	     ((uint32_t)dm9000_read_reg(dev, DM9000_PIDL + 1U) << 8) |
	     dm9000_read_reg(dev, DM9000_PIDL);

	if (id != DM9000_ID) {
		LOG_ERR("%s: Found ID: %08x, expected ID: %08x", dev->name, id, DM9000_ID);
		return -EIO;
	}

	LOG_INF("%s: Found ID: %08x", dev->name, id);

	return 0;
}

static int dm9000_hw_reset(const struct device *dev)
{
	const struct eth_dm9000_config *config = dev->config;

	dm9000_write_reg(dev, DM9000_NCR, DM9000_NCR_RST | DM9000_NCR_MAC_LBK);
	k_busy_wait(DM9000_HW_RESET_DELAY_US);
	dm9000_write_reg(dev, DM9000_NCR, 0);
	dm9000_write_reg(dev, DM9000_NCR, DM9000_NCR_RST | DM9000_NCR_MAC_LBK);
	k_busy_wait(DM9000_HW_RESET_DELAY_US);
	dm9000_write_reg(dev, DM9000_NCR, 0);

	dm9000_write_reg(dev, DM9000_IMR, DM9000_IMR_PAR);
	dm9000_write_reg(dev, DM9000_GPCR, DM9000_GPCR_GEP_CNTL);
	dm9000_write_reg(dev, DM9000_GPR, DM9000_GPR_PHY_ON);
	dm9000_write_reg(dev, DM9000_TCR2, DM9000_TCR2_LED_MODE);
	dm9000_write_reg(dev, DM9000_NSR, DM9000_NSR_CLEAR);
	dm9000_write_reg(dev, DM9000_TCR, 0);
	dm9000_write_reg(dev, DM9000_BPTR, DM9000_BPTR_DEFAULT);
	dm9000_write_reg(dev, DM9000_FCTR, DM9000_FCTR_DEFAULT);
	dm9000_write_reg(dev, DM9000_FCR, DM9000_FCR_DEFAULT);
	/* Select the host-bus timing: the 100 MHz mode bit is set when the host
	 * clock runs at 100 MHz or faster, and cleared for slower buses.
	 */
	dm9000_write_reg(dev, DM9000_OTCR,
			 (config->host_clock_freq >= MHZ(100)) ? DM9000_OTCR_100MHZ : 0);
	dm9000_write_reg(dev, DM9000_SMCR, 0);
#if defined(CONFIG_ETH_DM9000_HW_CHECKSUM)
	/* Offload IPv4/TCP/UDP checksums to the controller: generate on TX and
	 * verify-and-discard on RX. The stack then skips the software checksum
	 * (advertised via dm9000_get_capabilities/dm9000_get_config).
	 */
	dm9000_write_reg(dev, DM9000_TCSCR, DM9000_TCSCR_ALL);
	dm9000_write_reg(dev, DM9000_RCSCSR, DM9000_RCSCSR_ENABLE);
#endif
	dm9000_write_reg(dev, DM9000_ISR, DM9000_ISR_CLEAR_ALL);

	for (uint8_t i = 0; i < 8U; i++) {
		dm9000_write_reg(dev, DM9000_MAR + i, 0);
	}
	dm9000_write_reg(dev, DM9000_MAR + 7U, DM9000_MAR_7_BCAST_EN);

	return 0;
}

static int dm9000_hw_start_locked(const struct device *dev)
{
	struct eth_dm9000_data *data = dev->data;
	/* Link is managed by the generic phy_mii driver over the MDIO child;
	 * the LNKCHG interrupt is left masked here.
	 */
	const uint8_t imr = DM9000_IMR_PAR | DM9000_IMR_PRI;
	const uint8_t rcr = DM9000_RCR_RXEN | DM9000_RCR_DIS_LONG | DM9000_RCR_DIS_CRC;
	int ret;

	ret = dm9000_check_id(dev);
	if (ret < 0) {
		return ret;
	}

	data->tx_count = 0U;
	dm9000_write_regs(dev, DM9000_PAR, data->mac_addr, sizeof(data->mac_addr));

	/* The integrated PHY stays powered (GPR set at reset); phy_mii drives
	 * its BMCR/ANAR and auto-negotiation through the MDIO controller.
	 * Unmask interrupts before enabling the receiver so the first RX
	 * interrupt cannot be missed (see the equivalent dm9051 fix).
	 */
	dm9000_write_reg(dev, DM9000_IMR, imr);
	dm9000_write_reg(dev, DM9000_RCR, rcr);

	return 0;
}

static int dm9000_hw_start(const struct device *dev, struct net_if *iface __unused)
{
	struct eth_dm9000_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = dm9000_hw_start_locked(dev);
	k_mutex_unlock(&data->lock);

	return ret;
}

static int dm9000_hw_stop(const struct device *dev, struct net_if *iface __unused)
{
	struct eth_dm9000_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
	dm9000_write_reg(dev, DM9000_IMR, DM9000_IMR_PAR);
	dm9000_write_reg(dev, DM9000_RCR, 0);
	dm9000_write_reg(dev, DM9000_GPR, DM9000_GPR_PHY_OFF);
	k_mutex_unlock(&data->lock);

	return 0;
}


static struct net_pkt *dm9000_recv_pkt(const struct device *dev)
{
	struct eth_dm9000_data *data = dev->data;
	struct net_pkt *pkt;
	struct net_buf *frag;
	volatile uint16_t *addr = dm9000_addr_ptr(dev);
	volatile uint16_t *data_port = dm9000_data_ptr(dev);
	uint16_t rx_hdr;
	uint16_t rx_len;
	uint16_t frame_len;
	uint16_t remaining;
	uint8_t rx_ready;
	uint8_t rx_status;

	dm9000_io_write16(addr, DM9000_MRCMDX);
	(void)dm9000_io_read16(data_port);
	rx_ready = (uint8_t)dm9000_io_read16(data_port) & DM9000_PKT_RDY_MASK;

	if ((rx_ready & ~DM9000_PKT_RDY) != 0U) {
		LOG_DBG("%s: RX ready status %02x", dev->name, rx_ready);
		dm9000_hw_start_locked(dev);
		return NULL;
	}

	if ((rx_ready & DM9000_PKT_RDY) == 0U) {
		LOG_DBG("%s: RX interrupt without packet ready", dev->name);
		return NULL;
	}

	dm9000_io_write16(addr, DM9000_MRCMD);
	rx_hdr = dm9000_io_read16(data_port);
	rx_ready = (uint8_t)rx_hdr & DM9000_PKT_RDY_MASK;
	rx_status = (uint8_t)(rx_hdr >> 8);
	rx_len = dm9000_io_read16(data_port);

	if (rx_ready != DM9000_PKT_RDY ||
	    (rx_status & DM9000_RSR_ERRORS) != 0U ||
	    !IN_RANGE(rx_len, DM9000_MIN_FRAME_SIZE, DM9000_MAX_RX_SIZE)) {
		LOG_DBG("%s: RX ready %02x status %02x length %u",
			dev->name, rx_ready, rx_status, rx_len);
		dm9000_hw_start_locked(dev);
		return NULL;
	}

	frame_len = rx_len - DM9000_CRC_SIZE;
	pkt = net_pkt_rx_alloc_with_buffer(data->iface, frame_len,
					   NET_AF_UNSPEC, 0,
					   K_MSEC(CONFIG_ETH_DM9000_TIMEOUT));
	if (!pkt) {
		/* The FIFO must be drained even when the network pool is full. */
		dm9000_discard_data(dev, rx_len);
		eth_stats_update_errors_rx(data->iface);
		return NULL;
	}

	/* Copy directly from the 16-bit FIFO into net_pkt fragments. Keep all
	 * non-final transfers even-sized so a word read cannot consume a byte
	 * belonging to the next fragment.
	 */
	remaining = frame_len;
	frag = pkt->buffer;
	while (remaining != 0U) {
		uint16_t chunk = MIN(remaining, (uint16_t)net_buf_tailroom(frag));

		if (chunk < remaining) {
			chunk &= ~1U;
		}

		__ASSERT_NO_MSG(chunk != 0U);

		dm9000_read_data(dev, frag->data, chunk);
		net_buf_add(frag, chunk);
		remaining -= chunk;
		frag = frag->frags;
	}

	/* CRC is not delivered to the stack. Two 16-bit reads consume it;
	 * for an odd frame, the final data read already consumed one CRC byte
	 * and the second read below consumes the FIFO alignment byte.
	 */
	(void)dm9000_io_read16(data_port);
	(void)dm9000_io_read16(data_port);

	return pkt;
}

/* Drain pending RX packets. Hold the device lock only while touching the
 * DM9000 bus. In particular, hand packets to the network stack unlocked so
 * TCP ACK transmission can interleave with a long RX burst.
 */
static int dm9000_rx(const struct device *dev)
{
	struct eth_dm9000_data *data = dev->data;
	struct net_pkt *pkt;
	int ret = 0;

	while (true) {
		k_mutex_lock(&data->lock, K_FOREVER);
		pkt = dm9000_recv_pkt(dev);
		k_mutex_unlock(&data->lock);
		if (!pkt) {
			break;
		}

		/* The Ethernet L2 accounts for accepted RX byte/packet/broadcast/
		 * multicast statistics; the driver only reports frames it had to
		 * drop itself (which never reach L2).
		 */
		ret = net_recv_data(data->iface, pkt);
		if (ret < 0) {
			net_pkt_unref(pkt);
			break;
		}
	}

	return ret;
}

static void dm9000_rx_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = p1;
	struct eth_dm9000_data *data = dev->data;
	uint8_t isr;

	while (true) {
		k_sem_take(&data->int_event, K_FOREVER);

		k_mutex_lock(&data->lock, K_FOREVER);

		/* Mask PR interrupt while we drain packets */
		dm9000_write_reg(dev, DM9000_IMR, DM9000_IMR_PAR);
		isr = dm9000_read_reg(dev, DM9000_ISR);
		dm9000_write_reg(dev, DM9000_ISR, isr);

		k_mutex_unlock(&data->lock);

		if ((isr & DM9000_ISR_PR) != 0U) {
			dm9000_rx(dev);

			/* Re-check ISR after RX drain, before re-enabling IMR.
			 * DM9000 INT is level-sensitive but the GPIO is
			 * edge-triggered; a packet arriving while PR is masked
			 * won't produce a new edge and will be lost.
			 */
			k_mutex_lock(&data->lock, K_FOREVER);
			isr = dm9000_read_reg(dev, DM9000_ISR);
			dm9000_write_reg(dev, DM9000_ISR, isr);
			k_mutex_unlock(&data->lock);

			if ((isr & DM9000_ISR_PR) != 0U) {
				dm9000_rx(dev);
			}
		}

		/* Re-enable PR interrupt */
		k_mutex_lock(&data->lock, K_FOREVER);
		dm9000_write_reg(dev, DM9000_IMR, DM9000_IMR_PAR | DM9000_IMR_PRI);
		k_mutex_unlock(&data->lock);
	}
}

/* phy_mii reports link up/down through this callback; mirror it onto the
 * interface carrier. The DM9000 MAC follows the PHY speed/duplex internally,
 * so there is no MAC speed register to program here.
 */
static void dm9000_phy_link_cb(const struct device *phy __unused,
			       struct phy_link_state *state, void *user_data)
{
	const struct device *dev = user_data;
	struct eth_dm9000_data *data = dev->data;

	net_eth_carrier_set(data->iface, state->is_up);
}

static const struct device *dm9000_get_phy(const struct device *dev,
					   struct net_if *iface __unused)
{
	const struct eth_dm9000_config *config = dev->config;

	return config->phy_dev;
}

static void dm9000_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	const struct eth_dm9000_config *config = dev->config;
	struct eth_dm9000_data *data = dev->data;

	net_if_set_link_addr(iface, data->mac_addr, sizeof(data->mac_addr), NET_LINK_ETHERNET);
	data->iface = iface;

	ethernet_init(iface);
	net_if_carrier_off(iface);

	if (device_is_ready(config->phy_dev)) {
		phy_link_callback_set(config->phy_dev, dm9000_phy_link_cb, (void *)dev);
	} else {
		LOG_ERR("%s: PHY device not ready", dev->name);
	}

	k_thread_create(&data->rx_thread, data->rx_thread_stack,
			CONFIG_ETH_DM9000_RX_THREAD_STACK_SIZE,
			dm9000_rx_thread, (void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_ETH_DM9000_RX_THREAD_PRIORITY),
			0, K_NO_WAIT);
	k_thread_name_set(&data->rx_thread, "eth_dm9000");
}

static enum ethernet_hw_caps dm9000_get_capabilities(const struct device *dev __unused,
						     struct net_if *iface __unused)
{
	return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE
#if defined(CONFIG_ETH_DM9000_HW_CHECKSUM)
		| ETHERNET_HW_TX_CHKSUM_OFFLOAD | ETHERNET_HW_RX_CHKSUM_OFFLOAD
#endif
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
		| ETHERNET_PROMISC_MODE
#endif
		;
}

#if defined(CONFIG_ETH_DM9000_HW_CHECKSUM)
/* The controller offloads only the IPv4 header, TCP and UDP checksums (TCSCR/
 * RCSCSR). It does not touch ICMP or IPv6, so narrow the offered support so the
 * stack keeps computing those in software (otherwise, e.g., transmitted ICMP
 * echo replies would go out with an empty checksum).
 */
static int dm9000_get_config(const struct device *dev __unused, struct net_if *iface __unused,
			     enum ethernet_config_type type, struct ethernet_config *config)
{
	switch (type) {
	case ETHERNET_CONFIG_TYPE_RX_CHECKSUM_SUPPORT:
	case ETHERNET_CONFIG_TYPE_TX_CHECKSUM_SUPPORT:
		config->chksum_support = ETHERNET_CHECKSUM_SUPPORT_IPV4_HEADER |
					 ETHERNET_CHECKSUM_SUPPORT_TCP |
					 ETHERNET_CHECKSUM_SUPPORT_UDP;
		return 0;
	default:
		return -ENOTSUP;
	}
}
#endif /* CONFIG_ETH_DM9000_HW_CHECKSUM */

static int dm9000_set_config(const struct device *dev, struct net_if *iface __unused,
			     enum ethernet_config_type type,
			     const struct ethernet_config *config)
{
	struct eth_dm9000_data *data = dev->data;
	uint8_t rcr;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(data->mac_addr, config->mac_address.addr, sizeof(data->mac_addr));

		k_mutex_lock(&data->lock, K_FOREVER);
		dm9000_write_regs(dev, DM9000_PAR, data->mac_addr, sizeof(data->mac_addr));
		k_mutex_unlock(&data->lock);

		return 0;
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		if (!IS_ENABLED(CONFIG_NET_PROMISCUOUS_MODE)) {
			return -ENOTSUP;
		}

		k_mutex_lock(&data->lock, K_FOREVER);
		rcr = dm9000_read_reg(dev, DM9000_RCR);
		if (config->promisc_mode == ((rcr & DM9000_RCR_PRMSC) != 0U)) {
			k_mutex_unlock(&data->lock);
			return -EALREADY;
		}

		rcr = (rcr & ~DM9000_RCR_PRMSC) | (config->promisc_mode ? DM9000_RCR_PRMSC : 0);
		dm9000_write_reg(dev, DM9000_RCR, rcr);
		k_mutex_unlock(&data->lock);

		return 0;
	default:
		return -ENOTSUP;
	}
}

static int dm9000_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct eth_dm9000_data *data = dev->data;
	uint16_t len = (uint16_t)net_pkt_get_len(pkt);

	if (len > NET_ETH_MAX_FRAME_SIZE) {
		return -EMSGSIZE;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* DM9000 holds up to two TX packets in its 3 KB FIFO.  Write and
	 * trigger each packet immediately; only block when both slots are
	 * occupied so the pair can drain.  This lets consecutive packets
	 * go out back-to-back with no inter-packet gap, cutting the per-
	 * packet idle overhead in half compared to waiting before every
	 * frame.  Release the lock during the busy-wait so the RX thread
	 * can process incoming ACKs and avoid TCP throughput starvation.
	 */
	if (data->tx_count >= 2U) {
		while ((dm9000_read_reg(dev, DM9000_TCR) & DM9000_TCR_TXREQ) != 0U) {
			k_mutex_unlock(&data->lock);
			k_busy_wait(DM9000_TX_DRAIN_POLL_US);
			k_mutex_lock(&data->lock, K_FOREVER);
		}
		data->tx_count = 0U;
	}

	dm9000_write_reg(dev, DM9000_TXPLL, (uint8_t)len);
	dm9000_write_reg(dev, DM9000_TXPLL + 1U, (uint8_t)(len >> 8));
	dm9000_write_pkt(dev, pkt, len);
	dm9000_write_reg(dev, DM9000_TCR, DM9000_TCR_TXREQ);
	data->tx_count++;

	k_mutex_unlock(&data->lock);

	/* TX byte/packet/broadcast/multicast and send-error statistics are
	 * accounted by the Ethernet L2 around the send call.
	 */
	return 0;
}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
static struct net_stats_eth *dm9000_get_stats(const struct device *dev,
						 struct net_if *iface __unused)
{
	struct eth_dm9000_data *data = dev->data;

	return &data->stats;
}
#endif

static const struct ethernet_api dm9000_api = {
	.iface_api.init = dm9000_iface_init,
	.get_capabilities = dm9000_get_capabilities,
#if defined(CONFIG_ETH_DM9000_HW_CHECKSUM)
	.get_config = dm9000_get_config,
#endif
	.set_config = dm9000_set_config,
	.start = dm9000_hw_start,
	.stop = dm9000_hw_stop,
	.get_phy = dm9000_get_phy,
	.send = dm9000_tx,
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	.get_stats = dm9000_get_stats,
#endif
};

static void dm9000_gpio_callback(const struct device *dev, struct gpio_callback *cb,
				 uint32_t pins)
{
	struct eth_dm9000_data *data = CONTAINER_OF(cb, struct eth_dm9000_data, gpio_cb);

	k_sem_give(&data->int_event);
}

static int dm9000_init(const struct device *dev)
{
	const struct eth_dm9000_config *config = dev->config;
	struct eth_dm9000_data *data = dev->data;
	int ret;

	DEVICE_MMIO_NAMED_MAP(dev, addr, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, data, K_MEM_CACHE_NONE);

	k_sem_init(&data->int_event, 0, UINT_MAX);
	k_mutex_init(&data->lock);

	/* Optional hardware reset: drive the RESET pin to its inactive level,
	 * pulse it active, then wait for the controller to come back before any
	 * register access. Boards that tie the DM9000 reset to the SoC reset
	 * line leave the reset-gpios property unset.
	 */
	if (config->gpio_reset.port != NULL) {
		if (!gpio_is_ready_dt(&config->gpio_reset)) {
			LOG_ERR("%s: reset GPIO port %s is not ready", dev->name,
				config->gpio_reset.port->name);
			return -EINVAL;
		}

		ret = gpio_pin_configure_dt(&config->gpio_reset, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			return ret;
		}

		gpio_pin_set_dt(&config->gpio_reset, 1);
		k_msleep(DM9000_RESET_ASSERT_MS);
		gpio_pin_set_dt(&config->gpio_reset, 0);
		k_msleep(DM9000_RESET_RECOVERY_MS);
	}

	if (!gpio_is_ready_dt(&config->gpio_int)) {
		LOG_ERR("%s: GPIO port %s is not ready", dev->name, config->gpio_int.port->name);
		return -EINVAL;
	}

	ret = gpio_pin_configure_dt(&config->gpio_int, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, dm9000_gpio_callback, BIT(config->gpio_int.pin));

	ret = gpio_add_callback(config->gpio_int.port, &data->gpio_cb);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	(void)net_eth_mac_load(&config->mac_cfg, data->mac_addr);

	ret = dm9000_check_id(dev);
	if (ret < 0) {
		return ret;
	}

	ret = dm9000_hw_reset(dev);
	if (ret < 0) {
		return ret;
	}

	LOG_INF("%s: Device initialized", dev->name);

	return 0;
}

/* MDIO controller wrapping the integrated MII access. The bus serialization
 * reuses the MAC's device lock so PHY register access cannot interleave with a
 * half-finished register access on the data path. The MAC maps the MMIO and
 * powers the PHY in its own init; guard against being called before that.
 */
static int dm9000_mdio_read(const struct device *mdio_dev, uint8_t prtad, uint8_t regad,
			    uint16_t *data)
{
	const struct eth_dm9000_mdio_config *cfg = mdio_dev->config;
	const struct device *mac = cfg->mac;
	struct eth_dm9000_data *mac_data = mac->data;
	int ret;

	if (!device_is_ready(mac)) {
		return -EIO;
	}

	k_mutex_lock(&mac_data->lock, K_FOREVER);
	ret = dm9000_mii_read_locked(mac, prtad, regad, data);
	k_mutex_unlock(&mac_data->lock);

	return ret;
}

static int dm9000_mdio_write(const struct device *mdio_dev, uint8_t prtad, uint8_t regad,
			     uint16_t data)
{
	const struct eth_dm9000_mdio_config *cfg = mdio_dev->config;
	const struct device *mac = cfg->mac;
	struct eth_dm9000_data *mac_data = mac->data;
	int ret;

	if (!device_is_ready(mac)) {
		return -EIO;
	}

	k_mutex_lock(&mac_data->lock, K_FOREVER);
	ret = dm9000_mii_write_locked(mac, prtad, regad, data);
	k_mutex_unlock(&mac_data->lock);

	return ret;
}

static int dm9000_mdio_init(const struct device *dev __unused)
{
	return 0;
}

static DEVICE_API(mdio, dm9000_mdio_api) = {
	.read = dm9000_mdio_read,
	.write = dm9000_mdio_write,
};

#define ETH_DM9000_INIT(inst)									\
	static struct eth_dm9000_data eth_dm9000_data_##inst;					\
												\
	static const struct eth_dm9000_config eth_dm9000_config_##inst = {			\
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(addr, DT_DRV_INST(inst)),			\
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(data, DT_DRV_INST(inst)),			\
		.mac_cfg = NET_ETH_MAC_DT_INST_CONFIG_INIT(inst),				\
		.gpio_int = GPIO_DT_SPEC_INST_GET(inst, int_gpios),				\
		.gpio_reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),			\
		.phy_dev = DEVICE_DT_GET(DT_INST_PHANDLE(inst, phy_handle)),			\
		.host_clock_freq = DT_INST_PROP(inst, host_clock_frequency),			\
	};											\
												\
	ETH_NET_DEVICE_DT_INST_DEFINE(inst, dm9000_init, NULL, &eth_dm9000_data_##inst,	\
				      &eth_dm9000_config_##inst, CONFIG_ETH_INIT_PRIORITY,	\
				      &dm9000_api, NET_ETH_MTU);				\
												\
	static const struct eth_dm9000_mdio_config eth_dm9000_mdio_config_##inst = {		\
		.mac = DEVICE_DT_INST_GET(inst),						\
	};											\
												\
	DEVICE_DT_DEFINE(DT_INST_CHILD(inst, mdio), dm9000_mdio_init, NULL, NULL,		\
			 &eth_dm9000_mdio_config_##inst, POST_KERNEL,				\
			 CONFIG_MDIO_INIT_PRIORITY, &dm9000_mdio_api);

DT_INST_FOREACH_STATUS_OKAY(ETH_DM9000_INIT)
